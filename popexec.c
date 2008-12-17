#include "owl.h"
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#include <sys/wait.h>

static const char fileIdent[] = "$Id$";

/* starts up popexec in a new viewwin */
owl_popexec *owl_popexec_new(char *command)
{
  owl_popexec *pe;
  owl_popwin *pw;
  owl_viewwin *v;
  int pipefds[2], child_write_fd, parent_read_fd;
  int pid;

  pe = owl_malloc(sizeof(owl_popexec));
  if (!pe) return NULL;
  pe->winactive=0;
  pe->pid=0;
  pe->refcount=0;

  pw=owl_global_get_popwin(&g);
  pe->vwin=v=owl_global_get_viewwin(&g);

  owl_popwin_up(pw);
  owl_viewwin_init_text(v, owl_popwin_get_curswin(pw),
			owl_popwin_get_lines(pw), owl_popwin_get_cols(pw),
			"");
  owl_popwin_refresh(pw);
  owl_viewwin_redisplay(v, 0);
  owl_global_set_needrefresh(&g);
  owl_viewwin_set_onclose_hook(v, owl_popexec_viewwin_onclose, pe);
  pe->refcount++;

  if (0 != pipe(pipefds)) {
    owl_function_error("owl_function_popless_exec: pipe failed\n");
    return NULL;
  }
  parent_read_fd = pipefds[0];
  child_write_fd = pipefds[1];
  pid = fork();
  if (pid == -1) {
    close(pipefds[0]);
    close(pipefds[1]);
    owl_function_error("owl_function_popless_exec: fork failed\n");
    return NULL;
  } else if (pid != 0) {
    close(child_write_fd);
    /* still in owl */
    pe->pid=pid;
    pe->winactive=1;
    pe->dispatch.fd = parent_read_fd;
    pe->dispatch.cfunc = owl_popexec_inputhandler;
    pe->dispatch.destroy = owl_popexec_free_dispatch;
    pe->dispatch.data = pe;
    owl_select_add_dispatch(&pe->dispatch);
    pe->refcount++;
  } else {
    /* in the child process */
    char *argv[4];
    int i;
    int fdlimit = sysconf(_SC_OPEN_MAX);

    for (i=0; i<fdlimit; i++) {
      if (i!=child_write_fd) close(i);
    }
    dup2(child_write_fd, 1 /*stdout*/);
    dup2(child_write_fd, 2 /*stderr*/);
    close(child_write_fd);

    argv[0] = "sh";
    argv[1] = "-c";
    argv[2] = command;
    argv[3] = 0;
    execv("/bin/sh", argv);
    _exit(127);
  }

  return pe;
}

void owl_popexec_inputhandler(owl_dispatch *d)
{
  owl_popexec *pe = d->data;
  int navail, bread, rv_navail;
  char *buf;
  int status;

  if (!pe) return;

  /* If pe->winactive is 0 then the vwin has closed. 
   * If pe->pid is 0 then the child has already been reaped.
   * if d->fd is -1 then the fd has been closed out.
   * Under these cases we want to get to a state where:
   *   - data read until end if child running
   *   - child reaped
   *   - fd closed
   *   - callback removed
   */

  /* the viewwin has closed */
  if (!pe->pid && !pe->winactive) {
    owl_select_remove_dispatch(d->fd);
    return;
  }

  if (0 != (rv_navail = ioctl(d->fd, FIONREAD, (void*)&navail))) {
    owl_function_debugmsg("ioctl error");
  }

  /* check to see if the child has ended gracefully and no more data is
   * ready to be read... */
  if (navail==0 && pe->pid>0 && waitpid(pe->pid, &status, WNOHANG) > 0) {
    owl_function_debugmsg("waitpid got child status: <%d>\n", status);
    pe->pid = 0;
    if (pe->winactive) { 
      owl_viewwin_append_text(pe->vwin, "\n");
      owl_viewwin_redisplay(pe->vwin, 1);
    }
    owl_select_remove_dispatch(d->fd);
    return;
  }

  if (d->fd<0 || !pe->pid || !pe->winactive || rv_navail) {
    owl_function_error("popexec should not have reached this point");
    return;
  }

  if (navail<=0) return;
  if (navail>1024) { navail = 1024; }
  buf = owl_malloc(navail+1);
  owl_function_debugmsg("about to read %d", navail);
  bread = read(d->fd, buf, navail);
  if (bread<0) {
    perror("read");
    owl_function_debugmsg("read error");
  }
  if (buf[navail-1] != '\0') {
    buf[navail] = '\0';
  }
  owl_function_debugmsg("got data:  <%s>", buf);
  if (pe->winactive) {
    owl_viewwin_append_text(pe->vwin, buf);
    owl_viewwin_redisplay(pe->vwin, 1);
  }
  owl_free(buf);
  
}

void owl_popexec_free_dispatch(owl_dispatch *d)
{
  owl_popexec *pe = d->data;
  close(d->fd);
  owl_popexec_unref(pe);
}

void owl_popexec_viewwin_onclose(owl_viewwin *vwin, void *data)
{
  owl_popexec *pe = (owl_popexec*)data;
  int status, rv;

  pe->winactive = 0;
  if (pe->dispatch.fd>0) {
    owl_select_remove_dispatch(pe->dispatch.fd);
  }
  if (pe->pid) {
    /* TODO: we should handle the case where SIGTERM isn't good enough */
    rv = kill(pe->pid, SIGTERM);
    owl_function_debugmsg("kill of pid %d returned %d", pe->pid, rv);
    rv = waitpid(pe->pid, &status, 0);
    owl_function_debugmsg("waidpid returned %d, status %d", rv, status);
    pe->pid = 0;
  }
  owl_function_debugmsg("unref of %p from onclose", pe);
  owl_popexec_unref(pe);
}

void owl_popexec_unref(owl_popexec *pe)
{
  owl_function_debugmsg("unref of %p was %d", pe, pe->refcount);
  pe->refcount--;
  if (pe->refcount<=0) {
    owl_function_debugmsg("doing free of %p", pe);
    owl_free(pe);
  }
}
