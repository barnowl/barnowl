/*  Copyright (c) 2006-2011 The BarnOwl Developers. All rights reserved.
 *  Copyright (c) 2004 James Kretchmar. All rights reserved.
 *
 *  This program is free software. You can redistribute it and/or
 *  modify under the terms of the Sleepycat License. See the COPYING
 *  file included with the distribution for more information.
 */

#include "owl.h"
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>
#include <locale.h>
#include <unistd.h>

#if OWL_STDERR_REDIR
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
int stderr_replace(void);
#endif

owl_global g;

typedef struct _owl_options {
  bool load_initial_subs;
  char *configfile;
  char *tty;
  char *confdir;
  bool debug;
} owl_options;

void usage(FILE *file)
{
  fprintf(file, "BarnOwl version %s\n", version);
  fprintf(file, "Usage: barnowl [-n] [-d] [-D] [-v] [-h] [-c <configfile>] [-s <confdir>] [-t <ttyname>]\n");
  fprintf(file, "  -n,--no-subs        don't load zephyr subscriptions\n");
  fprintf(file, "  -d,--debug          enable debugging\n");
  fprintf(file, "  -v,--version        print the BarnOwl version number and exit\n");
  fprintf(file, "  -h,--help           print this help message\n");
  fprintf(file, "  -s,--config-dir     specify an alternate config dir (default ~/.owl)\n");
  fprintf(file, "  -c,--config-file    specify an alternate config file (default ~/.owl/init.pl)\n");
  fprintf(file, "  -t,--tty            set the tty name\n");
}

/* TODO: free owl_options after init is done? */
void owl_parse_options(int argc, char *argv[], owl_options *opts) {
  static const struct option long_options[] = {
    { "no-subs",         0, 0, 'n' },
    { "config-file",     1, 0, 'c' },
    { "config-dir",      1, 0, 's' },
    { "tty",             1, 0, 't' },
    { "debug",           0, 0, 'd' },
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
      opts->configfile = g_strdup(optarg);
      break;
    case 's':
      opts->confdir = g_strdup(optarg);
      break;
    case 't':
      opts->tty = g_strdup(optarg);
      break;
    case 'd':
      opts->debug = 1;
      break;
    case 'v':
      printf("This is BarnOwl version %s\n", version);
      exit(0);
    case 'h':
      usage(stdout);
      exit(0);
    default:
      usage(stderr);
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
  tcgetattr(STDIN_FILENO, owl_global_get_startup_tio(&g));

  tcgetattr(STDIN_FILENO, &tio);
  tio.c_iflag &= ~(ISTRIP|IEXTEN);
  tio.c_cc[VQUIT] = fpathconf(STDIN_FILENO, _PC_VDISABLE);
  tio.c_cc[VSUSP] = fpathconf(STDIN_FILENO, _PC_VDISABLE);
  tio.c_cc[VSTART] = fpathconf(STDIN_FILENO, _PC_VDISABLE);
  tio.c_cc[VSTOP] = fpathconf(STDIN_FILENO, _PC_VDISABLE);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio);

  /* screen init */
  initscr();
  cbreak();
  noecho();

  owl_start_color();
}

void owl_shutdown_curses(void) {
  endwin();
  /* restore terminal settings */
  tcsetattr(STDIN_FILENO, TCSAFLUSH, owl_global_get_startup_tio(&g));
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
static int owl_process_message(owl_message *m) {
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
      owl_function_command_norv(owl_global_get_alert_action(&g));
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
  /* redraw the sepbar; TODO: don't violate layering */
  owl_global_sepbar_dirty(&g);

  return 1;
}

static gboolean owl_process_messages_prepare(GSource *source, int *timeout) {
  *timeout = -1;
  return owl_global_messagequeue_pending(&g);
}

static gboolean owl_process_messages_check(GSource *source) {
  return owl_global_messagequeue_pending(&g);
}

/*
 * Process any new messages we have waiting in the message queue.
 */
static gboolean owl_process_messages_dispatch(GSource *source, GSourceFunc callback, gpointer user_data) {
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
      owl_function_lastmsg();

    /* do the newmsgproc thing */
    owl_function_do_newmsgproc();

    /* redisplay if necessary */
    /* this should be optimized to not run if the new messages won't be displayed */
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  }
  return TRUE;
}

static GSourceFuncs owl_process_messages_funcs = {
  owl_process_messages_prepare,
  owl_process_messages_check,
  owl_process_messages_dispatch,
  NULL
};

void owl_process_input_char(owl_input j)
{
  int ret;

  owl_global_set_lastinputtime(&g, time(NULL));
  owl_global_wakeup(&g);
  ret = owl_keyhandler_process(owl_global_get_keyhandler(&g), j);
  if (ret!=0 && ret!=1) {
    owl_function_makemsg("Unable to handle keypress");
  }
}

gboolean owl_process_input(GIOChannel *source, GIOCondition condition, void *data)
{
  owl_global *g = data;
  owl_input j;

  while (1) {
    j.ch = wgetch(g->input_pad);
    if (j.ch == ERR) return TRUE;

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
        int tmp = wgetch(g->input_pad);
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
  return TRUE;
}

static void sig_handler_main_thread(void *data) {
  int sig = GPOINTER_TO_INT(data);

  owl_function_debugmsg("Got signal %d", sig);
  if (sig == SIGWINCH) {
    owl_function_resize();
  } else if (sig == SIGTERM || sig == SIGHUP) {
    owl_function_quit();
  } else if (sig == SIGINT && owl_global_take_interrupt(&g)) {
    owl_input in;
    in.ch = in.uch = owl_global_get_startup_tio(&g)->c_cc[VINTR];
    owl_process_input_char(in);
  }
}

static void sig_handler(const siginfo_t *siginfo, void *data) {
  /* If it was an interrupt, set a flag so we can handle it earlier if
   * needbe. sig_handler_main_thread will check the flag to make sure
   * no one else took it. */
  if (siginfo->si_signo == SIGINT) {
    owl_global_add_interrupt(&g);
  }
  /* Send a message to the main thread. */
  owl_select_post_task(sig_handler_main_thread,
                       GINT_TO_POINTER(siginfo->si_signo), 
		       NULL, g_main_context_default());
}

#define OR_DIE(s, syscall)       \
  G_STMT_START {		 \
    if ((syscall) == -1) {	 \
      perror((s));		 \
      exit(1);			 \
    }				 \
  } G_STMT_END

void owl_register_signal_handlers(void) {
  struct sigaction sig_ignore = { .sa_handler = SIG_IGN };
  struct sigaction sig_default = { .sa_handler = SIG_DFL };
  sigset_t sigset;
  int ret, i;
  const int reset_signals[] = { SIGABRT, SIGBUS, SIGCHLD, SIGFPE, SIGILL,
                                SIGQUIT, SIGSEGV, };
  /* Don't bother resetting watched ones because owl_signal_init will. */
  const int watch_signals[] = { SIGWINCH, SIGTERM, SIGHUP, SIGINT, };

  /* Sanitize our signals; the mask and dispositions from our parent
   * aren't really useful. Signal list taken from equivalent code in
   * Chromium. */
  OR_DIE("sigemptyset", sigemptyset(&sigset));
  if ((ret = pthread_sigmask(SIG_SETMASK, &sigset, NULL)) != 0) {
    errno = ret;
    perror("pthread_sigmask");
    exit(1);
  }
  for (i = 0; i < G_N_ELEMENTS(reset_signals); i++) {
    OR_DIE("sigaction", sigaction(reset_signals[i], &sig_default, NULL));
  }

  /* Turn off SIGPIPE; we check the return value of write. */
  OR_DIE("sigaction", sigaction(SIGPIPE, &sig_ignore, NULL));

  /* Register some signals with the signal thread. */
  owl_signal_init(watch_signals, G_N_ELEMENTS(watch_signals),
                  sig_handler, NULL);
}

#if OWL_STDERR_REDIR

/* Replaces stderr with a pipe so that we can read from it. 
 * Returns the fd of the pipe from which stderr can be read. */
int stderr_replace(void)
{
  int pipefds[2];
  if (0 != pipe(pipefds)) {
    perror("pipe");
    owl_function_debugmsg("stderr_replace: pipe FAILED");
    return -1;
  }
    owl_function_debugmsg("stderr_replace: pipe: %d,%d", pipefds[0], pipefds[1]);
  if (-1 == dup2(pipefds[1], STDERR_FILENO)) {
    owl_function_debugmsg("stderr_replace: dup2 FAILED (%s)", strerror(errno));
    perror("dup2");
    return -1;
  }
  return pipefds[0];
}

/* Sends stderr (read from rfd) messages to the error console */
gboolean stderr_redirect_handler(GIOChannel *source, GIOCondition condition, void *data)
{
  int navail, bread;
  char buf[4096];
  int rfd = g_io_channel_unix_get_fd(source);
  char *err;

  /* TODO: Use g_io_channel_read_line? We'd have to be careful about
   * blocking on the read. */

  if (rfd<0) return TRUE;
  if (-1 == ioctl(rfd, FIONREAD, &navail)) {
    return TRUE;
  }
  /*owl_function_debugmsg("stderr_redirect: navail = %d\n", navail);*/
  if (navail <= 0) return TRUE;
  if (navail > sizeof(buf)-1) {
    navail = sizeof(buf)-1;
  }
  bread = read(rfd, buf, navail);
  if (bread == -1)
    return TRUE;

  err = g_strdup_printf("[stderr]\n%.*s", bread, buf);

  owl_function_log_err(err);
  g_free(err);
  return TRUE;
}

#endif /* OWL_STDERR_REDIR */

int main(int argc, char **argv, char **env)
{
  int argc_copy;
  char **argv_copy;
  char *perlout, *perlerr;
  const owl_style *s;
  const char *dir;
  owl_options opts;
  GSource *source;
  GIOChannel *channel;

  argc_copy = argc;
  argv_copy = g_strdupv(argv);

  setlocale(LC_ALL, "");

  memset(&opts, 0, sizeof opts);
  opts.load_initial_subs = 1;
  owl_parse_options(argc, argv, &opts);
  g.load_initial_subs = opts.load_initial_subs;

  owl_start_curses();

  /* owl global init */
  owl_global_init(&g);
  if (opts.debug) owl_global_set_debug_on(&g);
  if (opts.confdir) owl_global_set_confdir(&g, opts.confdir);
  owl_function_debugmsg("startup: first available debugging message");
  owl_global_set_startupargs(&g, argc_copy, argv_copy);
  g_strfreev(argv_copy);

  owl_register_signal_handlers();

  /* register STDIN dispatch; throw away return, we won't need it */
  channel = g_io_channel_unix_new(STDIN_FILENO);
  g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR, &owl_process_input, &g);
  g_io_channel_unref(channel);
  owl_zephyr_initialize();

#if OWL_STDERR_REDIR
  /* Do this only after we've started curses up... */
  if (isatty(STDERR_FILENO)) {
    owl_function_debugmsg("startup: doing stderr redirection");
    channel = g_io_channel_unix_new(stderr_replace());
    g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_ERR, &stderr_redirect_handler, NULL);
    g_io_channel_unref(channel);
  }
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
    g_free(tty);
  }

  /* Initialize perl */
  owl_function_debugmsg("startup: processing config file");

  owl_global_pop_context(&g);
  owl_global_push_context(&g, OWL_CTX_READCONFIG, NULL, NULL, NULL);

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

  owl_global_setup_default_filters(&g);

  /* set the current view */
  owl_function_debugmsg("startup: setting the current view");
  owl_view_create(owl_global_get_current_view(&g), "main",
                  owl_global_get_filter(&g, "all"),
                  owl_global_get_style_by_name(&g, "default"));

  /* execute the startup function in the configfile */
  owl_function_debugmsg("startup: executing perl startup, if applicable");
  perlout = owl_perlconfig_execute("BarnOwl::Hooks::_startup();");
  g_free(perlout);

  /* welcome message */
  owl_function_debugmsg("startup: creating splash message");
  char *welcome = g_strdup_printf(
    "-----------------------------------------------------------------------\n"
    "Welcome to BarnOwl version %s.\n"
    "To see a quick introduction, type ':show quickstart'.                  \n"
    "Press 'h' for on-line help.                                            \n"
    "                                                                       \n"
    "BarnOwl is free software. Type ':show license' for more                \n"
    "information.                                                     ^ ^   \n"
    "                                                                 OvO   \n"
    "Please report any bugs or suggestions to bug-barnowl@mit.edu    (   )  \n"
    "-----------------------------------------------------------------m-m---\n",
    version);
  owl_function_adminmsg("", welcome);
  g_free(welcome);

  owl_function_debugmsg("startup: setting context interactive");

  owl_global_pop_context(&g);
  owl_global_push_context(&g, OWL_CTX_INTERACTIVE|OWL_CTX_RECV, NULL, "recv", NULL);

  /* process the startup file */
  owl_function_debugmsg("startup: processing startup file");
  owl_function_source(NULL);

  owl_function_debugmsg("startup: set style for the view: %s", owl_global_get_default_style(&g));
  s = owl_global_get_style_by_name(&g, owl_global_get_default_style(&g));
  if(s)
      owl_view_set_style(owl_global_get_current_view(&g), s);
  else
      owl_function_error("No such style: %s", owl_global_get_default_style(&g));

  source = owl_window_redraw_source_new();
  g_source_attach(source, NULL);
  g_source_unref(source);

  source = g_source_new(&owl_process_messages_funcs, sizeof(GSource));
  g_source_attach(source, NULL);
  g_source_unref(source);

  owl_log_init();

  owl_function_debugmsg("startup: entering main loop");
  owl_select_run_loop();

  /* Shut down everything. */
  owl_zephyr_shutdown();
  owl_signal_shutdown();
  owl_shutdown_curses();
  owl_log_shutdown();
  return 0;
}
