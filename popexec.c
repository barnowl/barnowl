#include "owl.h"
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#include <sys/wait.h>

/* starts up popexec in a new viewwin */
owl_popexec *owl_popexec_new(const char *command)
{
  owl_popexec *pe;
  owl_popwin *pw;
  owl_viewwin *v;
  int pipefds[2], child_write_fd, parent_read_fd;
  pid_t pid;
  GIOChannel *channel;

  if (owl_global_get_popwin(&g) || owl_global_get_viewwin(&g)) {
    owl_function_error("Popwin already in use.");
    return NULL;
  }

  pe = g_new(owl_popexec, 1);
  pe->winactive=0;
  pe->pid=0;
  pe->refcount=0;

  pw = owl_popwin_new();
  owl_global_set_popwin(&g, pw);
  owl_popwin_up(pw);
  pe->vwin = v = owl_viewwin_new_text(owl_popwin_get_content(pw), "");
  owl_global_set_viewwin(&g, v);
  owl_viewwin_set_onclose_hook(v, owl_popexec_viewwin_onclose, pe);

  owl_global_push_context(&g, OWL_CTX_POPLESS, v, "popless", NULL);
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
    channel = g_io_channel_unix_new(parent_read_fd);
    g_io_channel_set_close_on_unref(channel, TRUE);
    pe->io_watch = g_io_add_watch_full(channel, G_PRIORITY_DEFAULT,
				       G_IO_IN | G_IO_ERR | G_IO_HUP,
				       owl_popexec_inputhandler, pe,
				       (GDestroyNotify)owl_popexec_unref);
    g_io_channel_unref(channel);
    pe->refcount++;
  } else {
    /* in the child process */
    int i;
    int fdlimit = sysconf(_SC_OPEN_MAX);

    for (i=0; i<fdlimit; i++) {
      if (i!=child_write_fd) close(i);
    }
    dup2(child_write_fd, 1 /*stdout*/);
    dup2(child_write_fd, 2 /*stderr*/);
    close(child_write_fd);

    execl("/bin/sh", "sh", "-c", command, (const char *)NULL);
    _exit(127);
  }

  return pe;
}

gboolean owl_popexec_inputhandler(GIOChannel *source, GIOCondition condition, void *data)
{
  owl_popexec *pe = data;
  int navail, bread, rv_navail;
  char *buf;
  int status;
  int fd = g_io_channel_unix_get_fd(source);

  /* TODO: Reading from GIOChannel may be more convenient. */

  if (!pe) return FALSE;

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
    pe->io_watch = 0;
    return FALSE;
  }

  if (0 != (rv_navail = ioctl(fd, FIONREAD, &navail))) {
    owl_function_debugmsg("ioctl error");
  }

  /* check to see if the child has ended gracefully and no more data is
   * ready to be read... */
  if (navail==0 && pe->pid>0 && waitpid(pe->pid, &status, WNOHANG) > 0) {
    owl_function_debugmsg("waitpid got child status: <%d>\n", status);
    pe->pid = 0;
    if (pe->winactive) {
      owl_viewwin_append_text(pe->vwin, "\n");
    }
    pe->io_watch = 0;
    return FALSE;
  }

  if (fd<0 || !pe->pid || !pe->winactive || rv_navail) {
    owl_function_error("popexec should not have reached this point");
    return FALSE;
  }

  if (navail<=0) return TRUE;
  if (navail>1024) { navail = 1024; }
  buf = g_new(char, navail+1);
  owl_function_debugmsg("about to read %d", navail);
  bread = read(fd, buf, navail);
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
  }
  g_free(buf);
  return TRUE;
}

void owl_popexec_viewwin_onclose(owl_viewwin *vwin, void *data)
{
  owl_popexec *pe = data;
  int status, rv;

  pe->winactive = 0;
  if (pe->io_watch) {
    g_source_remove(pe->io_watch);
    pe->io_watch = 0;
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
    g_free(pe);
  }
}
