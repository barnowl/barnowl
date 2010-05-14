/*  Copyright (c) 2006-2010 The BarnOwl Developers. All rights reserved.
 *  Copyright (c) 2004 James Kretchmar. All rights reserved.
 *
 *  This program is free software. You can redistribute it and/or
 *  modify under the terms of the Sleepycat License. See the COPYING
 *  file included with the distribution for more information.
 */

#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/stat.h>
#include <locale.h>
#include "owl.h"


#if OWL_STDERR_REDIR
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
int stderr_replace(void);
#endif

#define STDIN 0

owl_global g;

typedef struct _owl_options {
  bool load_initial_subs;
  char *configfile;
  char *tty;
  char *confdir;
  bool debug;
  bool rm_debug;
} owl_options;

void usage(void)
{
  fprintf(stderr, "Barnowl version %s\n", OWL_VERSION_STRING);
  fprintf(stderr, "Usage: barnowl [-n] [-d] [-D] [-v] [-h] [-c <configfile>] [-s <confdir>] [-t <ttyname>]\n");
  fprintf(stderr, "  -n,--no-subs        don't load zephyr subscriptions\n");
  fprintf(stderr, "  -d,--debug          enable debugging\n");
  fprintf(stderr, "  -D,--remove-debug   enable debugging and delete previous debug file\n");
  fprintf(stderr, "  -v,--version        print the Barnowl version number and exit\n");
  fprintf(stderr, "  -h,--help           print this help message\n");
  fprintf(stderr, "  -c,--config-file    specify an alternate config file\n");
  fprintf(stderr, "  -s,--config-dir     specify an alternate config dir (default ~/.owl)\n");
  fprintf(stderr, "  -t,--tty            set the tty name\n");
}

/* TODO: free owl_options after init is done? */
void owl_parse_options(int argc, char *argv[], owl_options *opts) {
  static const struct option long_options[] = {
    { "no-subs",         0, 0, 'n' },
    { "config-file",     1, 0, 'c' },
    { "config-dir",      1, 0, 's' },
    { "tty",             1, 0, 't' },
    { "debug",           0, 0, 'd' },
    { "remove-debug",    0, 0, 'D' },
    { "version",         0, 0, 'v' },
    { "help",            0, 0, 'h' },
    { NULL, 0, NULL, 0}
  };
  char c;

  while((c = getopt_long(argc, argv, "nc:t:s:dDvh",
                         long_options, NULL)) != -1) {
    switch(c) {
    case 'n':
      opts->load_initial_subs = 0;
      break;
    case 'c':
      opts->configfile = owl_strdup(optarg);
      break;
    case 's':
      opts->confdir = owl_strdup(optarg);
      break;
    case 't':
      opts->tty = owl_strdup(optarg);
      break;
    case 'D':
      opts->rm_debug = 1;
      /* fallthrough */
    case 'd':
      opts->debug = 1;
      break;
    case 'v':
      printf("This is barnowl version %s\n", OWL_VERSION_STRING);
      exit(0);
    case 'h':
    default:
      usage();
      exit(1);
    }
  }
}

void owl_start_color(void) {
  start_color();
#ifdef HAVE_USE_DEFAULT_COLORS
  use_default_colors();
#endif

  /* define simple color pairs */
  if (has_colors() && COLOR_PAIRS>=8) {
    int bg = COLOR_BLACK;
#ifdef HAVE_USE_DEFAULT_COLORS
    bg = -1;
#endif
    init_pair(OWL_COLOR_BLACK,   COLOR_BLACK,   bg);
    init_pair(OWL_COLOR_RED,     COLOR_RED,     bg);
    init_pair(OWL_COLOR_GREEN,   COLOR_GREEN,   bg);
    init_pair(OWL_COLOR_YELLOW,  COLOR_YELLOW,  bg);
    init_pair(OWL_COLOR_BLUE,    COLOR_BLUE,    bg);
    init_pair(OWL_COLOR_MAGENTA, COLOR_MAGENTA, bg);
    init_pair(OWL_COLOR_CYAN,    COLOR_CYAN,    bg);
    init_pair(OWL_COLOR_WHITE,   COLOR_WHITE,   bg);
  }
}

void owl_start_curses(void) {
  struct termios tio;
  /* save initial terminal settings */
  tcgetattr(0, owl_global_get_startup_tio(&g));

  tcgetattr(0, &tio);
  tio.c_iflag &= ~(ISTRIP|IEXTEN);
  tio.c_cc[VQUIT] = 0;
  tio.c_cc[VSUSP] = 0;
  tcsetattr(0, TCSAFLUSH, &tio);

  /* screen init */
  initscr();
  cbreak();
  noecho();

  owl_start_color();
}

static void owl_setup_default_filters(void)
{
  int i;
  static const struct {
    const char *name;
    const char *desc;
  } filters[] = {
    { "personal",
      "isprivate ^true$ and ( not type ^zephyr$ or ( class ^message  ) )" },
    { "trash",
      "class ^mail$ or opcode ^ping$ or type ^admin$ or ( not login ^none$ )" },
    { "wordwrap", "not ( type ^admin$ or type ^zephyr$ )" },
    { "ping", "opcode ^ping$" },
    { "auto", "opcode ^auto$" },
    { "login", "not login ^none$" },
    { "reply-lockout", "class ^noc or class ^mail$" },
    { "out", "direction ^out$" },
    { "aim", "type ^aim$" },
    { "zephyr", "type ^zephyr$" },
    { "none", "false" },
    { "all", "true" },
    { NULL, NULL }
  };

  owl_function_debugmsg("startup: creating default filters");

  for (i = 0; filters[i].name != NULL; i++)
    owl_global_add_filter(&g, owl_filter_new_fromstring(filters[i].name,
                                                        filters[i].desc));
}

/*
 * Process a new message passed to us on the message queue from some
 * protocol. This includes adding it to the message list, updating the
 * view and scrolling if appropriate, logging it, and so on.
 *
 * Either a pointer is kept to the message internally, or it is freed
 * if unneeded. The caller no longer ``owns'' the message's memory.
 *
 * Returns 1 if the message was added to the message list, and 0 if it
 * was ignored due to user settings or otherwise.
 */
int owl_process_message(owl_message *m) {
  const owl_filter *f;
  /* if this message it on the puntlist, nuke it and continue */
  if (owl_global_message_is_puntable(&g, m)) {
    owl_message_delete(m);
    return 0;
  }

  /*  login or logout that should be ignored? */
  if (owl_global_is_ignorelogins(&g)
      && owl_message_is_loginout(m)) {
    owl_message_delete(m);
    return 0;
  }

  if (!owl_global_is_displayoutgoing(&g)
      && owl_message_is_direction_out(m)) {
    owl_message_delete(m);
    return 0;
  }

  /* add it to the global list */
  owl_messagelist_append_element(owl_global_get_msglist(&g), m);
  /* add it to any necessary views; right now there's only the current view */
  owl_view_consider_message(owl_global_get_current_view(&g), m);

  if(owl_message_is_direction_in(m)) {
    /* let perl know about it*/
    owl_perlconfig_getmsg(m, NULL);

    /* do we need to autoreply? */
    if (owl_global_is_zaway(&g) && !owl_message_get_attribute_value(m, "isauto")) {
      if (owl_message_is_type_zephyr(m)) {
        owl_zephyr_zaway(m);
      } else if (owl_message_is_type_aim(m)) {
        if (owl_message_is_private(m)) {
          owl_function_send_aimawymsg(owl_message_get_sender(m), owl_global_get_zaway_msg(&g));
        }
      }
    }

    /* ring the bell if it's a personal */
    if (!strcmp(owl_global_get_personalbell(&g), "on")) {
      if (!owl_message_is_loginout(m) &&
          !owl_message_is_mail(m) &&
          owl_message_is_personal(m)) {
        owl_function_beep();
      }
    } else if (!strcmp(owl_global_get_personalbell(&g), "off")) {
      /* do nothing */
    } else {
      f=owl_global_get_filter(&g, owl_global_get_personalbell(&g));
      if (f && owl_filter_message_match(f, m)) {
        owl_function_beep();
      }
    }

    /* if it matches the alert filter, do the alert action */
    f=owl_global_get_filter(&g, owl_global_get_alert_filter(&g));
    if (f && owl_filter_message_match(f, m)) {
      owl_function_command(owl_global_get_alert_action(&g));
    }

    /* if it's a zephyr login or logout, update the zbuddylist */
    if (owl_message_is_type_zephyr(m) && owl_message_is_loginout(m)) {
      if (owl_message_is_login(m)) {
        owl_zbuddylist_adduser(owl_global_get_zephyr_buddylist(&g), owl_message_get_sender(m));
      } else if (owl_message_is_logout(m)) {
        owl_zbuddylist_deluser(owl_global_get_zephyr_buddylist(&g), owl_message_get_sender(m));
      } else {
        owl_function_error("Internal error: received login notice that is neither login nor logout");
      }
    }
  }

  /* let perl know about it */
  owl_perlconfig_newmsg(m, NULL);
  /* log the message if we need to */
  owl_log_message(m);

  return 1;
}

/*
 * Process any new messages we have waiting in the message queue.
 * Returns 1 if any messages were added to the message list, and 0 otherwise.
 */
int owl_process_messages(owl_ps_action *d, void *p)
{
  int newmsgs=0;
  int followlast = owl_global_should_followlast(&g);
  owl_message *m;

  /* Grab incoming messages. */
  while (owl_global_messagequeue_pending(&g)) {
    m = owl_global_messagequeue_popmsg(&g);
    if (owl_process_message(m))
      newmsgs = 1;
  }

  if (newmsgs) {
    /* follow the last message if we're supposed to */
    if (followlast)
      owl_function_lastmsg_noredisplay();

    /* do the newmsgproc thing */
    owl_function_do_newmsgproc();

    /* redisplay if necessary */
    /* this should be optimized to not run if the new messages won't be displayed */
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    sepbar(NULL);
    owl_global_set_needrefresh(&g);
  }
  return newmsgs;
}

void owl_process_input(const owl_io_dispatch *d, void *data)
{
  owl_input j;

  while (1) {
    j.ch = wgetch(g.input_pad);
    if (j.ch == ERR) return;

    j.uch = '\0';
    if (j.ch >= KEY_MIN && j.ch <= KEY_MAX) {
      /* This is a curses control character. */
    }
    else if (j.ch > 0x7f && j.ch < 0xfe) {
      /* Pull in a full utf-8 character. */
      int bytes, i;
      char utf8buf[7];
      memset(utf8buf, '\0', 7);
      
      utf8buf[0] = j.ch;
      
      if ((j.ch & 0xc0) && (~j.ch & 0x20)) bytes = 2;
      else if ((j.ch & 0xe0) && (~j.ch & 0x10)) bytes = 3;
      else if ((j.ch & 0xf0) && (~j.ch & 0x08)) bytes = 4;
      else if ((j.ch & 0xf8) && (~j.ch & 0x04)) bytes = 5;
      else if ((j.ch & 0xfc) && (~j.ch & 0x02)) bytes = 6;
      else bytes = 1;
      
      for (i = 1; i < bytes; i++) {
        int tmp = wgetch(g.input_pad);
        /* If what we got was not a byte, or not a continuation byte */
        if (tmp > 0xff || !(tmp & 0x80 && ~tmp & 0x40)) {
          /* ill-formed UTF-8 code unit subsequence, put back the
             char we just got. */
          ungetch(tmp);
          j.ch = ERR;
          break;
        }
        utf8buf[i] = tmp;
      }
      
      if (j.ch != ERR) {
        if (g_utf8_validate(utf8buf, -1, NULL)) {
          j.uch = g_utf8_get_char(utf8buf);
        }
        else {
          j.ch = ERR;
        }
      }
    }
    else if (j.ch <= 0x7f) {
      j.uch = j.ch;
    }

    owl_process_input_char(j);
  }
}

void sig_handler(int sig, siginfo_t *si, void *data)
{
  if (sig==SIGWINCH) {
    /* we can't inturrupt a malloc here, so it just sets a flag
     * schedulding a resize for later
     */
    owl_function_resize();
  } else if (sig==SIGPIPE || sig==SIGCHLD) {
    /* Set a flag and some info that we got the sigpipe
     * so we can record that we got it and why... */
    owl_global_set_errsignal(&g, sig, si);
  } else if (sig==SIGTERM || sig==SIGHUP) {
    owl_function_quit();
  }
}

void sigint_handler(int sig, siginfo_t *si, void *data)
{
  owl_global_set_interrupted(&g);
}

void owl_register_signal_handlers(void) {
  struct sigaction sigact;

  /* signal handler */
  /*sigact.sa_handler=sig_handler;*/
  sigact.sa_sigaction=sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags=SA_SIGINFO;
  sigaction(SIGWINCH, &sigact, NULL);
  sigaction(SIGALRM, &sigact, NULL);
  sigaction(SIGPIPE, &sigact, NULL);
  sigaction(SIGTERM, &sigact, NULL);
  sigaction(SIGHUP, &sigact, NULL);

  sigact.sa_sigaction=sigint_handler;
  sigaction(SIGINT, &sigact, NULL);
}

#if OWL_STDERR_REDIR

/* Replaces stderr with a pipe so that we can read from it. 
 * Returns the fd of the pipe from which stderr can be read. */
int stderr_replace(void)
{
  int pipefds[2];
  if (0 != pipe(pipefds)) {
    perror("pipe");
    owl_function_debugmsg("stderr_replace: pipe FAILED\n");
    return -1;
  }
    owl_function_debugmsg("stderr_replace: pipe: %d,%d\n", pipefds[0], pipefds[1]);
  if (-1 == dup2(pipefds[1], 2 /*stderr*/)) {
    owl_function_debugmsg("stderr_replace: dup2 FAILED (%s)\n", strerror(errno));
    perror("dup2");
    return -1;
  }
  return pipefds[0];
}

/* Sends stderr (read from rfd) messages to the error console */
void stderr_redirect_handler(const owl_io_dispatch *d, void *data)
{
  int navail, bread;
  char buf[4096];
  int rfd = d->fd;
  char *err;

  if (rfd<0) return;
  if (-1 == ioctl(rfd, FIONREAD, &navail)) {
    return;
  }
  /*owl_function_debugmsg("stderr_redirect: navail = %d\n", navail);*/
  if (navail <= 0) return;
  if (navail > sizeof(buf)-1) {
    navail = sizeof(buf)-1;
  }
  bread = read(rfd, buf, navail);
  if (buf[navail-1] != '\0') {
    buf[navail] = '\0';
  }

  err = owl_sprintf("[stderr]\n%s", buf);

  owl_function_log_err(err);
  owl_free(err);
}

#endif /* OWL_STDERR_REDIR */

static int owl_refresh_pre_select_action(owl_ps_action *a, void *data)
{
  /* if a resize has been scheduled, deal with it */
  owl_global_resize(&g, 0, 0);
  /* also handle relayouts */
  owl_global_relayout(&g);

  /* update the terminal if we need to */
  if (owl_global_is_needrefresh(&g)) {
    /* these are here in case a relayout changes the windows */
    WINDOW *sepwin = owl_global_get_curs_sepwin(&g);
    WINDOW *typwin = owl_global_get_curs_typwin(&g);

    /* push all changed windows to screen */
    update_panels();
    /* leave the cursor in the appropriate window */
    if (!owl_popwin_is_active(owl_global_get_popwin(&g))
	&& owl_global_get_typwin(&g)) {
      owl_function_set_cursor(typwin);
    } else {
      owl_function_set_cursor(sepwin);
    }
    doupdate();
    owl_global_set_noneedrefresh(&g);
  }
  return 0;
}


int main(int argc, char **argv, char **env)
{
  int argcsave;
  const char *const *argvsave;
  char *perlout, *perlerr;
  const owl_style *s;
  const char *dir;
  owl_options opts;

  if (!GLIB_CHECK_VERSION (2, 12, 0))
    g_error ("GLib version 2.12.0 or above is needed.");

  argcsave=argc;
  argvsave=strs(argv);

  setlocale(LC_ALL, "");

  memset(&opts, 0, sizeof opts);
  opts.load_initial_subs = 1;
  owl_parse_options(argc, argv, &opts);
  g.load_initial_subs = opts.load_initial_subs;

  owl_function_debugmsg("startup: Finished parsing arguments");

  owl_register_signal_handlers();
  owl_start_curses();

  /* owl global init */
  owl_global_init(&g);
  if (opts.rm_debug) unlink(OWL_DEBUG_FILE);
  if (opts.debug) owl_global_set_debug_on(&g);
  if (opts.confdir) owl_global_set_confdir(&g, opts.confdir);
  owl_function_debugmsg("startup: first available debugging message");
  owl_global_set_startupargs(&g, argcsave, argvsave);
  owl_global_set_haveaim(&g);

  /* register STDIN dispatch; throw away return, we won't need it */
  owl_select_add_io_dispatch(STDIN, OWL_IO_READ, &owl_process_input, NULL, NULL);
  owl_zephyr_initialize();

#if OWL_STDERR_REDIR
  /* Do this only after we've started curses up... */
  owl_function_debugmsg("startup: doing stderr redirection");
  owl_select_add_io_dispatch(stderr_replace(), OWL_IO_READ, &stderr_redirect_handler, NULL, NULL);
#endif

  /* create the owl directory, in case it does not exist */
  owl_function_debugmsg("startup: creating owl directory, if not present");
  dir=owl_global_get_confdir(&g);
  mkdir(dir, S_IRWXU);

  /* set the tty, either from the command line, or by figuring it out */
  owl_function_debugmsg("startup: setting tty name");
  if (opts.tty) {
    owl_global_set_tty(&g, opts.tty);
  } else {
    char *tty = owl_util_get_default_tty();
    owl_global_set_tty(&g, tty);
    owl_free(tty);
  }

  /* Initialize perl */
  owl_function_debugmsg("startup: processing config file");

  owl_global_pop_context(&g);
  owl_global_push_context(&g, OWL_CTX_READCONFIG, NULL, NULL);

  perlerr=owl_perlconfig_initperl(opts.configfile, &argc, &argv, &env);
  if (perlerr) {
    endwin();
    fprintf(stderr, "Internal perl error: %s\n", perlerr);
    fflush(stderr);
    printf("Internal perl error: %s\n", perlerr);
    fflush(stdout);
    exit(1);
  }

  owl_global_complete_setup(&g);

  owl_setup_default_filters();

  /* set the current view */
  owl_function_debugmsg("startup: setting the current view");
  owl_view_create(owl_global_get_current_view(&g), "main",
                  owl_global_get_filter(&g, "all"),
                  owl_global_get_style_by_name(&g, "default"));

  /* AIM init */
  owl_function_debugmsg("startup: doing AIM initialization");
  owl_aim_init();

  /* execute the startup function in the configfile */
  owl_function_debugmsg("startup: executing perl startup, if applicable");
  perlout = owl_perlconfig_execute("BarnOwl::Hooks::_startup();");
  if (perlout) owl_free(perlout);

  /* welcome message */
  owl_function_debugmsg("startup: creating splash message");
  owl_function_adminmsg("",
    "-----------------------------------------------------------------------\n"
    "Welcome to barnowl version " OWL_VERSION_STRING ".  Press 'h' for on-line help.\n"
    "To see a quick introduction, type ':show quickstart'.                  \n"
    "                                                                       \n"
    "BarnOwl is free software. Type ':show license' for more                \n"
    "information.                                                     ^ ^   \n"
    "                                                                 OvO   \n"
    "Please report any bugs or suggestions to bug-barnowl@mit.edu    (   )  \n"
    "-----------------------------------------------------------------m-m---\n"
  );
  sepbar(NULL);

  /* process the startup file */
  owl_function_debugmsg("startup: processing startup file");
  owl_function_source(NULL);

  /* Set the default style */
  owl_function_debugmsg("startup: setting startup and default style");
  if (0 != strcmp(owl_global_get_default_style(&g), "__unspecified__")) {
    /* the style was set by the user: leave it alone */
  } else {
    owl_global_set_default_style(&g, "default");
  }

  owl_function_debugmsg("startup: set style for the view: %s", owl_global_get_default_style(&g));
  s = owl_global_get_style_by_name(&g, owl_global_get_default_style(&g));
  if(s)
      owl_view_set_style(owl_global_get_current_view(&g), s);
  else
      owl_function_error("No such style: %s", owl_global_get_default_style(&g));

  owl_function_debugmsg("startup: setting context interactive");

  owl_global_pop_context(&g);
  owl_global_push_context(&g, OWL_CTX_READCONFIG|OWL_CTX_RECV, NULL, "recv");

  owl_select_add_pre_select_action(owl_refresh_pre_select_action, NULL, NULL);
  owl_select_add_pre_select_action(owl_process_messages, NULL, NULL);

  owl_function_debugmsg("startup: entering main loop");
  /* main loop */
  while (1) {
    /* select on FDs we know about. */
    owl_select();

    /* Log any error signals */
    {
      siginfo_t si;
      int signum;
      if ((signum = owl_global_get_errsignal_and_clear(&g, &si)) > 0) {
	owl_function_error("Got unexpected signal: %d %s  (code: %d band: %ld  errno: %d)",
			   signum, signum==SIGPIPE?"SIGPIPE":"SIG????",
			   si.si_code, si.si_band, si.si_errno);
      }
    }

  }
}
