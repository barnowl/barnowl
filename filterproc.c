#include <signal.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <poll.h>
#include <fcntl.h>
#include <string.h>

#include <glib.h>

int send_receive(int rfd, int wfd, const char *out, char **in)
{
  GString *str = g_string_new("");
  char buf[1024];
  nfds_t nfds;
  int err = 0;
  struct pollfd fds[2];
  struct sigaction sig = {.sa_handler = SIG_IGN}, old;

  fcntl(rfd, F_SETFL, O_NONBLOCK | fcntl(rfd, F_GETFL));
  fcntl(wfd, F_SETFL, O_NONBLOCK | fcntl(wfd, F_GETFL));

  fds[0].fd = rfd;
  fds[0].events = POLLIN;
  fds[1].fd = wfd;
  fds[1].events = POLLOUT;

  sigaction(SIGPIPE, &sig, &old);
  
  while(1) {
    if(out && *out) {
      nfds = 2;
    } else {
      nfds = 1;
    }
    err = poll(fds, nfds, -1);
    if(err < 0) {
      break;
    }
    if(out && *out) {
      if(fds[1].revents & POLLOUT) {
        err = write(wfd, out, strlen(out));
        if(err > 0) {
          out += err;
        }
        if(err < 0) {
          out = NULL;
        }
      }
      if(!out || !*out || fds[1].revents & (POLLERR | POLLHUP)) {
        close(wfd);
        out = NULL;
      }
    }
    if(fds[0].revents & POLLIN) {
      err = read(rfd, buf, sizeof(buf));
      if(err <= 0) {
        break;
      }
      g_string_append_len(str, buf, err);
    } else if(fds[0].revents & (POLLHUP | POLLERR)) {
      err = 0;
      break;
    }
  }

  *in = g_string_free(str, err < 0);
  sigaction(SIGPIPE, &old, NULL);
  return err;
}

int call_filter(const char *prog, const char *const *argv, const char *in, char **out, int *status)
{
  int err = 0;
  pid_t pid;
  int rfd[2];
  int wfd[2];

  if((err = pipe(rfd))) goto out;
  if((err = pipe(wfd))) goto out_close_rfd;

  pid = fork();
  if(pid < 0) {
    err = pid;
    goto out_close_all;
  }
  if(pid) {
    /* parent */
    close(rfd[1]);
    close(wfd[0]);
    err = send_receive(rfd[0], wfd[1], in, out);
    if(err == 0) {
      waitpid(pid, status, 0);
    }
  } else {
    /* child */
    close(rfd[0]);
    close(wfd[1]);
    dup2(rfd[1], 1);
    dup2(wfd[0], 0);
    close(rfd[1]);
    close(wfd[0]);

    if(execvp(prog, (char *const *)argv)) {
      _exit(-1);
    }
  }

 out_close_all:
  close(wfd[0]);
  close(wfd[1]);
 out_close_rfd:
  close(rfd[0]);
  close(rfd[1]);
 out:
  return err;
}
