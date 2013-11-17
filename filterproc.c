#include "owl.h"
#include <sys/wait.h>
#include <poll.h>

/* Even in case of error, send_receive is responsible for closing wfd
 * (to EOF the child) and rfd (for consistency). */
static int send_receive(int rfd, int wfd, const char *out, char **in)
{
  GString *str = g_string_new("");
  char buf[1024];
  nfds_t nfds;
  int err = 0;
  struct pollfd fds[2];

  fcntl(rfd, F_SETFL, O_NONBLOCK | fcntl(rfd, F_GETFL));
  fcntl(wfd, F_SETFL, O_NONBLOCK | fcntl(wfd, F_GETFL));

  fds[0].fd = rfd;
  fds[0].events = POLLIN;
  fds[1].fd = wfd;
  fds[1].events = POLLOUT;

  if(!out || !*out) {
    /* Nothing to write. Close our end so the child doesn't hang waiting. */
    close(wfd); wfd = -1;
    out = NULL;
  }

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
        close(wfd); wfd = -1;
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

  if (wfd >= 0) close(wfd);
  close(rfd);
  *in = g_string_free(str, err < 0);
  return err;
}

int call_filter(const char *const *argv, const char *in, char **out, int *status)
{
  int err;
  GPid child_pid;
  int child_stdin, child_stdout;

  if (!g_spawn_async_with_pipes(NULL, (char**)argv, NULL,
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL,
                                &child_pid, &child_stdin, &child_stdout, NULL,
                                NULL)) {
    *out = NULL;
    return 1;
  }

  err = send_receive(child_stdout, child_stdin, in, out);
  if (err == 0) {
    waitpid(child_pid, status, 0);
  }
  return err;
}
