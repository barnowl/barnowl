/* Written by James Kretchmar, MIT
 *
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
#include <sys/wait.h>
#include <errno.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int main(int argc, char **argv, char **env) {
  WINDOW *recwin, *sepwin, *typwin, *msgwin;
  owl_editwin *tw;
  owl_popwin *pw;
  int j, ret, initialsubs, debug, newzephyrs, argcsave, followlast;
  struct sigaction sigact;
  char *configfile, *tty, *perlout;
  char buff[LINE], startupmsg[LINE];
  char **argvsave;
  owl_filter *f;
  time_t nexttime;
  int nexttimediff;

  argcsave=argc;
  argvsave=argv;
  configfile=NULL;
  tty=NULL;
  debug=0;
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

  /* zephyr init */
  if ((ret = ZInitialize()) != ZERR_NONE) {
    com_err("owl",ret,"while initializing");
    exit(1);
  }
  if ((ret = ZOpenPort(NULL)) != ZERR_NONE) {
    com_err("owl",ret,"while opening port");
    exit(1);
  }

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
  use_default_colors();
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
    
  /* owl init */
  owl_global_init(&g);
  if (debug) owl_global_set_debug_on(&g);
  owl_global_set_startupargs(&g, argcsave, argvsave);
  owl_context_set_readconfig(owl_global_get_context(&g));

  if (tty) {
    owl_global_set_tty(&g, tty);
  } else {
    owl_global_set_tty(&g, owl_util_get_default_tty());
  }

  /* setup the default filters */
  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "personal", "class ^message$ and instance ^personal$ and ( recipient ^%me%$ or sender ^%me%$ )"); /* fix to use admintype */
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "trash", "class ^mail$ or opcode ^ping$ or type ^admin$ or class ^login$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "ping", "opcode ^ping$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "auto", "opcode ^auto$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "login", "class ^login$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "reply-lockout", "class ^noc or class ^mail$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "out", "direction ^out$");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  f=malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, "all", "class .*");
  owl_list_append_element(owl_global_get_filterlist(&g), f);

  /* set the current view */
  owl_view_create(owl_global_get_current_view(&g), f);

  /* read the config file */
  ret=owl_readconfig(configfile);
  if (ret) {
    endwin();
    printf("\nError parsing configfile\n");
    exit(1);
  }
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
  ret=owl_zephyr_loadsubs(NULL);
  if (ret!=-1) {
    owl_global_add_userclue(&g, OWL_USERCLUE_CLASSES);
  }

  /* load login subs */
  if (owl_global_is_loginsubs(&g)) {
    loadloginsubs(NULL);
  }

  /* zlog in if we need to */
  if (owl_global_is_startuplogin(&g)) {
    owl_function_zlog_in();
  }

  /* welcome message */
  strcpy(startupmsg, "-------------------------------------------------------------------------\n");
  sprintf(buff,      "Welcome to owl version %s.  Press 'h' for on line help. \n", OWL_VERSION_STRING);
  strcat(startupmsg, buff);
  strcat(startupmsg, "                                                                         \n");
  strcat(startupmsg, "If you would like to receive release announcments about owl you can join \n");
  strcat(startupmsg, "the owl-users@mit.edu mailing list.  MIT users can add themselves,       \n");
  strcat(startupmsg, "otherwise send a request to owner-owl-users@mit.edu.               ^ ^   \n");
  strcat(startupmsg, "                                                                   OvO   \n");
  strcat(startupmsg, "Please report any bugs or suggestions to bug-owl@mit.edu          (   )  \n");
  strcat(startupmsg, "-------------------------------------------------------------------m-m---\n");
  
  owl_function_adminmsg("", startupmsg);
  sepbar(NULL);
  
  owl_context_set_interactive(owl_global_get_context(&g));

  nexttimediff=20;
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

    /* little hack */
    if (0 && owl_global_get_runtime(&g)<300) {
      if (time(NULL)>nexttime) {
	if (nexttimediff==1) {
	  nexttimediff=20;
	} else {
	  nexttimediff=1;
	}
	nexttime+=nexttimediff;
	owl_hack_animate();
      }
    }

    /* grab incoming zephyrs */
    newzephyrs=0;
    while(ZPending()) {
      ZNotice_t notice;
      struct sockaddr_in from;
      owl_message *m;

      ZReceiveNotice(&notice, &from);

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
      owl_message_create_from_zephyr(m, &notice);
      
      /* if it's on the puntlist then, nuke it and continue */
      if (owl_global_message_is_puntable(&g, m)) {
	owl_message_free(m);
	continue;
      }

      /* otherwise add it to the global list */
      owl_messagelist_append_element(owl_global_get_msglist(&g), m);
      newzephyrs=1;

      /* let the config know the new message has been received */
      owl_config_getmsg(m, 0);

      /* add it to any necessary views; right now there's only the current view */
      owl_view_consider_message(owl_global_get_current_view(&g), m);

      /* do we need to autoreply? */
      if (owl_global_is_zaway(&g)) {
	owl_zephyr_zaway(m);
      }

      /* ring the bell if it's a personal */
      if (owl_global_is_personalbell(&g) && owl_message_is_personal(m)) {
	owl_function_beep();
	owl_global_set_needrefresh(&g);
      }

      /* check for burning ears message */
      /* this is an unsupported feature */
      if (owl_global_is_burningears(&g) && owl_message_is_burningears(m)) {
	char *buff;
	buff = owl_sprintf("@i(Burning ears message on class %s)", owl_message_get_class(m));
	/* owl_function_makemsg(buff); */
	owl_function_adminmsg(buff, "");
	owl_free(buff);
	owl_function_beep();
      }

      /* log the zephyr if we need to */
      if (owl_global_is_logging(&g) || owl_global_is_classlogging(&g)) {
	owl_log_incoming(m);
      }
    }

    /* follow the last message if we're supposed to */
    if (newzephyrs && followlast) {
      owl_function_lastmsg_noredisplay();
    }

    /* check if newmsgproc is active, if not but the option is on,
       make it active */
    if (newzephyrs) {
      if (owl_global_get_newmsgproc(&g) && strcmp(owl_global_get_newmsgproc(&g), "")) {
	/* if there's a process out there, we need to check on it */
	if (owl_global_get_newmsgproc_pid(&g)) {
	  owl_function_debugmsg("Checking on newmsgproc pid==%i", owl_global_get_newmsgproc_pid(&g));
	  owl_function_debugmsg("Waitpid return is %i", waitpid(owl_global_get_newmsgproc_pid(&g), NULL, WNOHANG));
	  waitpid(owl_global_get_newmsgproc_pid(&g), NULL, WNOHANG);
	  if (waitpid(owl_global_get_newmsgproc_pid(&g), NULL, WNOHANG)==-1) {
	    /* it exited */
	    owl_global_set_newmsgproc_pid(&g, 0);
	    owl_function_debugmsg("newmsgproc exited");
	  } else {
	    owl_function_debugmsg("newmsgproc did not exit");
	  }
	}
	
	/* if it exited, fork & exec a new one */
	if (owl_global_get_newmsgproc_pid(&g)==0) {
	  int i, myargc;
	  i=fork();
	  if (i) {
	    /* parent set the child's pid */
	    owl_global_set_newmsgproc_pid(&g, i);
	    waitpid(i, NULL, WNOHANG);
	    owl_function_debugmsg("I'm the parent and I started a new newmsgproc with pid %i", i);
	  } else {
	    /* child exec's the program */
	    char **parsed;
	    parsed=owl_parseline(owl_global_get_newmsgproc(&g), &myargc);
	    parsed=realloc(parsed, strlen(owl_global_get_newmsgproc(&g)+300));
	    parsed[myargc]=(char *) NULL;

	    owl_function_debugmsg("About to exec: %s with %i arguments", parsed[0], myargc);

	    execvp(parsed[0], (char **) parsed);
	    

	    /* was there an error exec'ing? */
	    owl_function_debugmsg("Error execing: %s", strerror(errno));
	    _exit(127);
	  }
	}
      }
    }
    
    /* redisplay if necessary */
    if (newzephyrs) {
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

    /* handle all keypresses */
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
      owl_context_set_editline(owl_global_get_context(&g), tw);
      owl_function_activate_keymap("editline");
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
    /* we can't inturrupt a malloc here, so it just sets a flag */
    owl_function_resize();
  }
}

void usage() {
  fprintf(stderr, "Usage: owl [-n] [-d] [-v] [-c <configfile>] [-t <ttyname>]\n");
}
