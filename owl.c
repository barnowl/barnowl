/*
 * Copyright 2001 Massachusetts Institute of Technology
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation for any purpose and without fee is hereby
 * granted, provided that the above copyright notice and this
 * permission notice appear in all copies and in supporting
 * documentation.  No representation is made about the suitability of
 * this software for any purpose.  It is provided "as is" without
 * express or implied warranty.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <zephyr/zephyr.h>
#include <com_err.h>
#include <signal.h>
#include <time.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

owl_global g;

int main(int argc, char **argv, char **env) {
  WINDOW *recwin, *sepwin, *typwin, *msgwin;
  owl_editwin *tw;
  owl_popwin *pw;
  int j, ret, initialsubs, debug, argcsave, followlast;
  int newmsgs, zpendcount, nexttimediff;
  struct sigaction sigact;
  char *configfile, *tty, *perlout, **argvsave, buff[LINE], startupmsg[LINE];
  owl_filter *f;
  owl_style *s;
  time_t nexttime, now;
  struct tm *today;
  char *dir;
  ZNotice_t notice;

  argcsave=argc;
  argvsave=argv;
  configfile=NULL;
  tty=NULL;
  debug=0;
  initialsubs=1;
  if (argc>0) {
    argv++;
    argc--;
  }
  while (argc>0) {
    if (!strcmp(argv[0], "-n")) {
      initialsubs=0;
      argv++;
      argc--;
    } else if (!strcmp(argv[0], "-c")) {
      if (argc<2) {
	fprintf(stderr, "Too few arguments to -c\n");
	usage();
	exit(1);
      }
      configfile=argv[1];
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-t")) {
      if (argc<2) {
	fprintf(stderr, "Too few arguments to -t\n");
	usage();
	exit(1);
      }
      tty=argv[1];
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-d")) {
      debug=1;
      argv++;
      argc--;
    } else if (!strcmp(argv[0], "-v")) {
      printf("This is owl version %s\n", OWL_VERSION_STRING);
      exit(0);
    } else {
      fprintf(stderr, "Uknown argument\n");
      usage();	      
      exit(1);
    }
  }

#ifdef HAVE_LIBZEPHYR
  /* zephyr init */
  ret=owl_zephyr_initialize();
  if (ret) {
    exit(1);
  }
#endif
  
  /* signal handler */
  sigact.sa_handler=sig_handler;
  sigemptyset(&sigact.sa_mask);
  sigact.sa_flags=0;
  sigaction(SIGWINCH, &sigact, NULL);
  sigaction(SIGALRM, &sigact, NULL);

  /* screen init */
  sprintf(buff, "TERMINFO=%s", TERMINFO);
  putenv(buff);
  initscr();
  start_color();
#ifdef HAVE_USE_DEFAULT_COLORS
  use_default_colors();
#endif
  raw();
  noecho();

  /* define simple color pairs */
  if (has_colors() && COLOR_PAIRS>=8) {
    init_pair(OWL_COLOR_BLACK,   COLOR_BLACK,   -1);
    init_pair(OWL_COLOR_RED,     COLOR_RED,     -1);
    init_pair(OWL_COLOR_GREEN,   COLOR_GREEN,   -1);
    init_pair(OWL_COLOR_YELLOW,  COLOR_YELLOW,  -1);
    init_pair(OWL_COLOR_BLUE,    COLOR_BLUE,    -1);
    init_pair(OWL_COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(OWL_COLOR_CYAN,    COLOR_CYAN,    -1);
    init_pair(OWL_COLOR_WHITE,   COLOR_WHITE,   -1);
  }

    
  /* owl global init */
  owl_global_init(&g);
  if (debug) owl_global_set_debug_on(&g);
  owl_global_set_startupargs(&g, argcsave, argvsave);

  /* create the owl directory, in case it does not exist */
  dir=owl_sprintf("%s/%s", owl_global_get_homedir(&g), OWL_CONFIG_DIR);
  mkdir(dir, S_IRWXU);
  owl_free(dir);

  /* set the tty, either from the command line, or by figuring it out */
  if (tty) {
    owl_global_set_tty(&g, tty);
  } else {
    owl_global_set_tty(&g, owl_util_get_default_tty());
  }

  /* setup the built-in styles */
  s=owl_malloc(sizeof(owl_style));
  owl_style_create_internal(s, "default", &owl_stylefunc_default);
  owl_global_add_style(&g, s);

  s=owl_malloc(sizeof(owl_style));
  owl_style_create_internal(s, "basic", &owl_stylefunc_basic);
  owl_global_add_style(&g, s);

  s=owl_malloc(sizeof(owl_style));
  owl_style_create_internal(s, "oneline", &owl_stylefunc_oneline);
  owl_global_add_style(&g, s);

  /* setup the default filters */
  /* the personal filter will need to change again when AIM chat's are
   *  included.  Also, there should be an %aimme% */
  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "personal", "( type ^zephyr$ "
			     "and class ^message$ and instance ^personal$ "
			     "and ( recipient ^%me%$ or sender ^%me%$ ) ) "
			     "or ( type ^aim$ and login ^none$ )");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "trash", "class ^mail$ or opcode ^ping$ or type ^admin$ or ( not login ^none$ )");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "ping", "opcode ^ping$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "auto", "opcode ^auto$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "login", "not login ^none$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "reply-lockout", "class ^noc or class ^mail$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "out", "direction ^out$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "aim", "type ^aim$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "zephyr", "type ^zephyr$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "none", "false");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "all", "true");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  /* set the current view */
  owl_view_create(owl_global_get_current_view(&g), "main", f, owl_global_get_style_by_name(&g, "default"));

  /* AIM init */
  owl_aim_init();

  /* process the startup file */
  owl_function_execstartup();

  /* read the config file */
  owl_context_set_readconfig(owl_global_get_context(&g));
  ret=owl_readconfig(configfile);
  if (ret) {
    endwin();
    printf("\nError parsing configfile\n");
    exit(1);
  }

  /* if the config defines a formatting function, add 'perl' as a style */
  if (owl_global_is_config_format(&g)) {
    owl_function_debugmsg("Found perl formatting");
    s=owl_malloc(sizeof(owl_style));
    owl_style_create_perl(s, "perl", "owl::format_msg");
    owl_global_add_style(&g, s);
    owl_global_set_default_style(&g, "perl");
  }

  /* execute the startup function in the configfile */
  perlout = owl_config_execute("owl::startup();");
  if (perlout) owl_free(perlout);
  owl_function_debugmsg("-- Owl Startup --");
  
  /* hold on to the window names for convenience */
  msgwin=owl_global_get_curs_msgwin(&g);
  recwin=owl_global_get_curs_recwin(&g);
  sepwin=owl_global_get_curs_sepwin(&g);
  typwin=owl_global_get_curs_typwin(&g);
  tw=owl_global_get_typwin(&g);

  wrefresh(sepwin);

  /* load zephyr subs */
  if (initialsubs) {
    /* load normal subscriptions */
    ret=owl_zephyr_loadsubs(NULL);
    if (ret!=-1) {
      owl_global_add_userclue(&g, OWL_USERCLUE_CLASSES);
    }

    /* load login subscriptions */
    if (owl_global_is_loginsubs(&g)) {
      owl_function_loadloginsubs(NULL);
    }
  }

  /* zlog in if we need to */
  if (owl_global_is_startuplogin(&g)) {
    owl_zephyr_zlog_in();
  }

  /* set the startup and default style, based on userclue and presence of a
   * formatting function */
  if (owl_global_is_config_format(&g)) {
    owl_view_set_style(owl_global_get_current_view(&g), owl_global_get_style_by_name(&g, "perl"));
    owl_global_set_default_style(&g, "perl");
  } else if (owl_global_is_userclue(&g, OWL_USERCLUE_CLASSES)) {
    owl_view_set_style(owl_global_get_current_view(&g), owl_global_get_style_by_name(&g, "default"));
    owl_global_set_default_style(&g, "default");
  } else {
    owl_view_set_style(owl_global_get_current_view(&g), owl_global_get_style_by_name(&g, "basic"));
    owl_global_set_default_style(&g, "basic");
  }

  /* welcome message */
  strcpy(startupmsg, "-------------------------------------------------------------------------\n");
  sprintf(buff,      "Welcome to Owl version %s.  Press 'h' for on line help. \n", OWL_VERSION_STRING);
  strcat(startupmsg, buff);
  strcat(startupmsg, "                                                                         \n");
  strcat(startupmsg, "If you would like to receive release announcements about owl you can join \n");
  strcat(startupmsg, "the owl-users@mit.edu mailing list.  MIT users can add themselves,       \n");
  strcat(startupmsg, "otherwise send a request to owner-owl-users@mit.edu.               ^ ^   \n");
  strcat(startupmsg, "                                                                   OvO   \n");
  strcat(startupmsg, "Please report any bugs or suggestions to bug-owl@mit.edu          (   )  \n");
  strcat(startupmsg, "-------------------------------------------------------------------m-m---\n");
  owl_function_adminmsg("", startupmsg);
  sepbar(NULL);
  
  owl_context_set_interactive(owl_global_get_context(&g));

  nexttimediff=10;
  nexttime=time(NULL);

  /* main loop */
  while (1) {

    /* if a resize has been scheduled, deal with it */
    owl_global_resize(&g, 0, 0);

    /* these are here in case a resize changes the windows */
    msgwin=owl_global_get_curs_msgwin(&g);
    recwin=owl_global_get_curs_recwin(&g);
    sepwin=owl_global_get_curs_sepwin(&g);
    typwin=owl_global_get_curs_typwin(&g);

    followlast=owl_global_should_followlast(&g);
    
    /* If we're logged into AIM, do AIM stuff */
    if (owl_global_is_aimloggedin(&g)) {
      owl_function_debugmsg("Doing aim processing");
      owl_aim_process_events();
    }

    /* little hack */
    now=time(NULL);
    today=localtime(&now);
    if (today->tm_mon==9 && today->tm_mday==31 && owl_global_get_runtime(&g)<600) {
      if (time(NULL)>nexttime) {
	if (nexttimediff==1) {
	  nexttimediff=10;
	} else {
	  nexttimediff=1;
	}
	nexttime+=nexttimediff;
	owl_hack_animate();
      }
    }

    /* Grab incoming messages. */
    newmsgs=0;
    zpendcount=0;
    while(owl_zephyr_zpending() || owl_global_messagequeue_pending(&g)) {
      struct sockaddr_in from;
      owl_message *m;
      owl_filter *f;
      
      /* grab the new message, stick it in 'm' */
      if (owl_zephyr_zpending()) {
	/* grab a zephyr notice, but if we've done 20 without stopping,
	   take a break to process keystrokes etc. */
	if (zpendcount>20) break;
	ZReceiveNotice(&notice, &from);
	zpendcount++;
	
	/* is this an ack from a zephyr we sent? */
	if (owl_zephyr_notice_is_ack(&notice)) {
	  owl_zephyr_handle_ack(&notice);
	  continue;
	}
	
	/* if it's a ping and we're not viewing pings then skip it */
	if (!owl_global_is_rxping(&g) && !strcasecmp(notice.z_opcode, "ping")) {
	  continue;
	}

	/* create the new message */
	m=owl_malloc(sizeof(owl_message));
	owl_message_create_from_znotice(m, &notice);
      } else if (owl_global_messagequeue_pending(&g)) {
	m=owl_global_messageuque_popmsg(&g);
      }
      
      /* if this message it on the puntlist, nuke it and continue */
      if (owl_global_message_is_puntable(&g, m)) {
	owl_message_free(m);
	continue;
      }

      /* otherwise add it to the global list */
      owl_messagelist_append_element(owl_global_get_msglist(&g), m);
      newmsgs=1;

      /* let the config know the new message has been received */
      owl_config_getmsg(m, "owl::receive_msg();");

      /* add it to any necessary views; right now there's only the current view */
      owl_view_consider_message(owl_global_get_current_view(&g), m);

      /* do we need to autoreply? */
      if (owl_message_is_type_zephyr(m) && owl_global_is_zaway(&g)) {
	owl_zephyr_zaway(m);
      }

      /* ring the bell if it's a personal */
      if (owl_global_is_personalbell(&g) &&
	  !owl_message_is_loginout(m) &&
	  !owl_message_is_mail(m) &&
	  owl_message_is_private(m)) {
	owl_function_beep();
      }

      /* if it matches the alert filter, do the alert action */
      f=owl_global_get_filter(&g, owl_global_get_alert_filter(&g));
      if (f && owl_filter_message_match(f, m)) {
	owl_function_command(owl_global_get_alert_action(&g));
      }

      /* check for burning ears message */
      /* this is an unsupported feature */
      if (owl_global_is_burningears(&g) && owl_message_is_burningears(m)) {
	char *buff;
	buff = owl_sprintf("@i(Burning ears message on class %s)", owl_message_get_class(m));
	owl_function_adminmsg(buff, "");
	owl_free(buff);
	owl_function_beep();
      }

      /* log the message if we need to */
      if (owl_global_is_logging(&g) || owl_global_is_classlogging(&g)) {
	owl_log_incoming(m);
      }
    }

    /* follow the last message if we're supposed to */
    if (newmsgs && followlast) {
      owl_function_lastmsg_noredisplay();
    }

    /* do the newmsgproc thing */
    if (newmsgs) {
      owl_function_do_newmsgproc();
    }
    
    /* redisplay if necessary */
    /* this should be optimized to not run if the new messages won't be displayed */
    if (newmsgs) {
      owl_mainwin_redisplay(owl_global_get_mainwin(&g));
      sepbar(NULL);
      if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
	owl_popwin_refresh(owl_global_get_popwin(&g));
	/* TODO: this is a broken kludge */
	if (owl_global_get_viewwin(&g)) {
	  owl_viewwin_redisplay(owl_global_get_viewwin(&g), 0);
	}
      }
      owl_global_set_needrefresh(&g);
    }

    /* if a popwin just came up, refresh it */
    pw=owl_global_get_popwin(&g);
    if (owl_popwin_is_active(pw) && owl_popwin_needs_first_refresh(pw)) {
      owl_popwin_refresh(pw);
      owl_popwin_no_needs_first_refresh(pw);
      owl_global_set_needrefresh(&g);
      /* TODO: this is a broken kludge */
      if (owl_global_get_viewwin(&g)) {
	owl_viewwin_redisplay(owl_global_get_viewwin(&g), 0);
      }
    }

    /* update the terminal if we need to */
    if (owl_global_is_needrefresh(&g)) {
      /* leave the cursor in the appropriate window */
      if (owl_global_is_typwin_active(&g)) {
	owl_function_set_cursor(typwin);
      } else {
	owl_function_set_cursor(sepwin);
      }
      doupdate();
      owl_global_set_noneedrefresh(&g);
    }

    /* Handle all keypresses.  If no key has been pressed, sleep for a
     * little bit, but otherwise do not.  This lets input be grabbed
     * as quickly as possbile */
    j=wgetch(typwin);
    if (j==ERR) {
      usleep(10);
      continue;
    }
    /* find and activate the current keymap.
     * TODO: this should really get fixed by activating
     * keymaps as we switch between windows... 
     */
    if (pw && owl_popwin_is_active(pw) && owl_global_get_viewwin(&g)) {
      owl_context_set_popless(owl_global_get_context(&g), 
			      owl_global_get_viewwin(&g));
      owl_function_activate_keymap("popless");
    } else if (owl_global_is_typwin_active(&g) 
	       && owl_editwin_get_style(tw)==OWL_EDITWIN_STYLE_ONELINE) {
      /*
      owl_context_set_editline(owl_global_get_context(&g), tw);
      owl_function_activate_keymap("editline");
      */
    } else if (owl_global_is_typwin_active(&g) 
	       && owl_editwin_get_style(tw)==OWL_EDITWIN_STYLE_MULTILINE) {
      owl_context_set_editmulti(owl_global_get_context(&g), tw);
      owl_function_activate_keymap("editmulti");
    } else {
      owl_context_set_recv(owl_global_get_context(&g));
      owl_function_activate_keymap("recv");
    }
    /* now actually handle the keypress */
    ret = owl_keyhandler_process(owl_global_get_keyhandler(&g), j);
    if (ret!=0 && ret!=1) {
      owl_function_makemsg("Unable to handle keypress");
    }
  }
}

void sig_handler(int sig) {
  if (sig==SIGWINCH) {
    /* we can't inturrupt a malloc here, so it just sets a flag
     * schedulding a resize for later
     */
    owl_function_resize();
  }
}

void usage() {
  fprintf(stderr, "Owl version %s\n", OWL_VERSION_STRING);
  fprintf(stderr, "Usage: owl [-n] [-d] [-v] [-h] [-c <configfile>] [-t <ttyname>]\n");
  fprintf(stderr, "  -n      don't load zephyr subscriptions\n");
  fprintf(stderr, "  -d      enable debugging\n");
  fprintf(stderr, "  -v      print the Owl version number and exit\n");
  fprintf(stderr, "  -h      print this help message\n");
  fprintf(stderr, "  -c      specify an alternate config file\n");
  fprintf(stderr, "  -t      set the tty name\n");
  
}
