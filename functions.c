#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <com_err.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_function_noop(void)
{
  return;
}

char *owl_function_command(char *cmdbuff)
{
  owl_function_debugmsg("executing command: %s", cmdbuff);
  return owl_cmddict_execute(owl_global_get_cmddict(&g), 
			     owl_global_get_context(&g), cmdbuff);
}

void owl_function_command_norv(char *cmdbuff)
{
  char *rv;
  rv=owl_function_command(cmdbuff);
  if (rv) owl_free(rv);
}

void owl_function_command_alias(char *alias_from, char *alias_to)
{
  owl_cmddict_add_alias(owl_global_get_cmddict(&g), alias_from, alias_to);
}

owl_cmd *owl_function_get_cmd(char *name)
{
  return owl_cmddict_find(owl_global_get_cmddict(&g), name);
}

void owl_function_show_commands()
{
  owl_list l;
  owl_fmtext fm;

  owl_fmtext_init_null(&fm);
  owl_fmtext_append_bold(&fm, "Commands:  ");
  owl_fmtext_append_normal(&fm, "(use 'show command <name>' for details)\n");
  owl_cmddict_get_names(owl_global_get_cmddict(&g), &l);
  owl_fmtext_append_list(&fm, &l, "\n", owl_function_cmd_describe);
  owl_fmtext_append_normal(&fm, "\n");
  owl_function_popless_fmtext(&fm);
  owl_cmddict_namelist_free(&l);
  owl_fmtext_free(&fm);
}

char *owl_function_cmd_describe(void *name)
{
  owl_cmd *cmd = owl_cmddict_find(owl_global_get_cmddict(&g), name);
  if (cmd) return owl_cmd_describe(cmd);
  else return(NULL);
}

void owl_function_show_command(char *name)
{
  owl_function_help_for_command(name);
}

void owl_function_adminmsg(char *header, char *body)
{
  owl_message *m;
  int followlast;

  followlast=owl_global_should_followlast(&g);
  m=owl_malloc(sizeof(owl_message));
  owl_message_create_admin(m, header, body);
  owl_messagelist_append_element(owl_global_get_msglist(&g), m);
  owl_view_consider_message(owl_global_get_current_view(&g), m);

  if (followlast) owl_function_lastmsg_noredisplay();

  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
    owl_popwin_refresh(owl_global_get_popwin(&g));
  }
  
  wnoutrefresh(owl_global_get_curs_recwin(&g));
  owl_global_set_needrefresh(&g);
}

void owl_function_make_outgoing_zephyr(char *body, char *zwriteline, char *zsig)
{
  owl_message *m;
  int followlast;
  owl_zwrite z;
  
  followlast=owl_global_should_followlast(&g);

  /* create a zwrite for the purpose of filling in other message fields */
  owl_zwrite_create_from_line(&z, zwriteline);

  /* create the message */
  m=owl_malloc(sizeof(owl_message));
  owl_message_create_from_zwriteline(m, zwriteline, body, zsig);

  /* add it to the global list and current view */
  owl_messagelist_append_element(owl_global_get_msglist(&g), m);
  owl_view_consider_message(owl_global_get_current_view(&g), m);

  if (followlast) owl_function_lastmsg_noredisplay();

  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
    owl_popwin_refresh(owl_global_get_popwin(&g));
  }
  
  wnoutrefresh(owl_global_get_curs_recwin(&g));
  owl_global_set_needrefresh(&g);
  owl_zwrite_free(&z);
}

int owl_function_make_outgoing_aim(char *body, char *to)
{
  owl_message *m;
  int followlast;


  if (!owl_global_is_aimloggedin(&g)) {
    return(-1);
  }
  
  followlast=owl_global_should_followlast(&g);

  /* create the message */
  m=owl_malloc(sizeof(owl_message));
  owl_message_create_aim(m,
			 owl_global_get_aim_screenname(&g),
			 to,
			 body,
			 OWL_MESSAGE_DIRECTION_OUT,
			 0);

  /* add it to the global list and current view */
  owl_messagelist_append_element(owl_global_get_msglist(&g), m);
  owl_view_consider_message(owl_global_get_current_view(&g), m);

  if (followlast) owl_function_lastmsg_noredisplay();

  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  if (owl_popwin_is_active(owl_global_get_popwin(&g))) {
    owl_popwin_refresh(owl_global_get_popwin(&g));
  }
  
  wnoutrefresh(owl_global_get_curs_recwin(&g));
  owl_global_set_needrefresh(&g);
  return(0);
}

void owl_function_zwrite_setup(char *line)
{
  owl_editwin *e;
  char buff[1024];
  owl_zwrite z;
  int ret;

  /* check the arguments */
  ret=owl_zwrite_create_from_line(&z, line);
  if (ret) {
    owl_function_makemsg("Error in zwrite arugments");
    owl_zwrite_free(&z);
    return;
  }

  /* send a ping if necessary */
  if (owl_global_is_txping(&g)) {
    owl_zwrite_send_ping(&z);
  }
  owl_zwrite_free(&z);

  /* create and setup the editwin */
  e=owl_global_get_typwin(&g);
  owl_editwin_new_style(e, OWL_EDITWIN_STYLE_MULTILINE, owl_global_get_msg_history(&g));

  if (!owl_global_get_lockout_ctrld(&g)) {
    owl_function_makemsg("Type your zephyr below.  End with ^D or a dot on a line by itself.  ^C will quit.");
  } else {
    owl_function_makemsg("Type your zephyr below.  End with a dot on a line by itself.  ^C will quit.");
  }

  owl_editwin_clear(e);
  owl_editwin_set_dotsend(e);
  strcpy(buff, "----> ");
  strcat(buff, line);
  strcat(buff, "\n");
  owl_editwin_set_locktext(e, buff);

  /* make it active */
  owl_global_set_typwin_active(&g);
}

void owl_function_aimwrite_setup(char *line)
{
  owl_editwin *e;
  char buff[1024];

  /* check the arguments */

  /* create and setup the editwin */
  e=owl_global_get_typwin(&g);
  owl_editwin_new_style(e, OWL_EDITWIN_STYLE_MULTILINE, owl_global_get_msg_history(&g));

  if (!owl_global_get_lockout_ctrld(&g)) {
    owl_function_makemsg("Type your message below.  End with ^D or a dot on a line by itself.  ^C will quit.");
  } else {
    owl_function_makemsg("Type your message below.  End with a dot on a line by itself.  ^C will quit.");
  }

  owl_editwin_clear(e);
  owl_editwin_set_dotsend(e);
  strcpy(buff, "----> ");
  strcat(buff, line);
  strcat(buff, "\n");
  owl_editwin_set_locktext(e, buff);

  /* make it active */
  owl_global_set_typwin_active(&g);
}



void owl_function_zcrypt_setup(char *line)
{
  owl_editwin *e;
  char buff[1024];
  owl_zwrite z;
  int ret;

  /* check the arguments */
  ret=owl_zwrite_create_from_line(&z, line);
  if (ret) {
    owl_function_makemsg("Error in zwrite arugments");
    owl_zwrite_free(&z);
    return;
  }

  if (owl_zwrite_get_numrecips(&z)>0) {
    owl_function_makemsg("You may not specifiy a recipient for a zcrypt message");
    owl_zwrite_free(&z);
    return;
  }

  /* send a ping if necessary */
  if (owl_global_is_txping(&g)) {
    owl_zwrite_send_ping(&z);
  }
  owl_zwrite_free(&z);

  /* create and setup the editwin */
  e=owl_global_get_typwin(&g);
  owl_editwin_new_style(e, OWL_EDITWIN_STYLE_MULTILINE, owl_global_get_msg_history(&g));

  if (!owl_global_get_lockout_ctrld(&g)) {
    owl_function_makemsg("Type your zephyr below.  End with ^D or a dot on a line by itself.  ^C will quit.");
  } else {
    owl_function_makemsg("Type your zephyr below.  End with a dot on a line by itself.  ^C will quit.");
  }

  owl_editwin_clear(e);
  owl_editwin_set_dotsend(e);
  strcpy(buff, "----> ");
  strcat(buff, line);
  strcat(buff, "\n");
  owl_editwin_set_locktext(e, buff);

  /* make it active */
  owl_global_set_typwin_active(&g);
}

void owl_function_zwrite(char *line)
{
  owl_zwrite z;
  int i, j;

  /* create the zwrite and send the message */
  owl_zwrite_create_from_line(&z, line);
  owl_zwrite_send_message(&z, owl_editwin_get_text(owl_global_get_typwin(&g)));
  owl_function_makemsg("Waiting for ack...");

  /* display the message as an outgoing message in the receive window */
  if (owl_global_is_displayoutgoing(&g) && owl_zwrite_is_personal(&z)) {
    owl_function_make_outgoing_zephyr(owl_editwin_get_text(owl_global_get_typwin(&g)), line, owl_zwrite_get_zsig(&z));
  }

  /* log it if we have logging turned on */
  if (owl_global_is_logging(&g) && owl_zwrite_is_personal(&z)) {
    j=owl_zwrite_get_numrecips(&z);
    for (i=0; i<j; i++) {
      owl_log_outgoing_zephyr(owl_zwrite_get_recip_n(&z, i),
		       owl_editwin_get_text(owl_global_get_typwin(&g)));
    }
  }

  /* free the zwrite */
  owl_zwrite_free(&z);
}


void owl_function_aimwrite(char *to)
{
  /*  send the message */
  owl_aim_send_im(to, owl_editwin_get_text(owl_global_get_typwin(&g)));
  owl_function_makemsg("AIM message sent.");

  /* display the message as an outgoing message in the receive window */
  if (owl_global_is_displayoutgoing(&g)) {
    owl_function_make_outgoing_aim(owl_editwin_get_text(owl_global_get_typwin(&g)), to);
  }

  /* log it if we have logging turned on */
  if (owl_global_is_logging(&g)) {
    owl_log_outgoing_aim(to, owl_editwin_get_text(owl_global_get_typwin(&g)));
  }
}



/* If filter is non-null, looks for the next message matching
 * that filter.  If skip_deleted, skips any deleted messages. 
 * If last_if_none, will stop at the last message in the view
 * if no matching messages are found.  */
void owl_function_nextmsg_full(char *filter, int skip_deleted, int last_if_none)
{
  int curmsg, i, viewsize, found;
  owl_view *v;
  owl_filter *f = NULL;
  owl_message *m;

  v=owl_global_get_current_view(&g);

  if (filter) {
    f=owl_global_get_filter(&g, filter);
    if (!f) {
      owl_function_makemsg("No %s filter defined", filter);
      return;
    }
  }

  curmsg=owl_global_get_curmsg(&g);
  viewsize=owl_view_get_size(v);
  found=0;

  /* just check to make sure we're in bounds... */
  if (curmsg>viewsize-1) curmsg=viewsize-1;
  if (curmsg<0) curmsg=0;

  for (i=curmsg+1; i<viewsize; i++) {
    m=owl_view_get_element(v, i);
    if (skip_deleted && owl_message_is_delete(m)) continue;
    if (f && !owl_filter_message_match(f, m)) continue;
    found = 1;
    break;
  }

  if (i>owl_view_get_size(v)-1) i=owl_view_get_size(v)-1;

  if (!found) {
    owl_function_makemsg("already at last%s message%s%s",
			 skip_deleted?" non-deleted":"",
			 filter?" in ":"", filter?filter:"");
    /* if (!skip_deleted) owl_function_beep(); */
  }

  if (last_if_none || found) {
    owl_global_set_curmsg(&g, i);
    owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    owl_global_set_direction_downwards(&g);
  }
}

void owl_function_prevmsg_full(char *filter, int skip_deleted, int first_if_none)
{
  int curmsg, i, viewsize, found;
  owl_view *v;
  owl_filter *f = NULL;
  owl_message *m;

  v=owl_global_get_current_view(&g);

  if (filter) {
    f=owl_global_get_filter(&g, filter);
    if (!f) {
      owl_function_makemsg("No %s filter defined", filter);
      return;
    }
  }

  curmsg=owl_global_get_curmsg(&g);
  viewsize=owl_view_get_size(v);
  found=0;

  /* just check to make sure we're in bounds... */
  if (curmsg<0) curmsg=0;

  for (i=curmsg-1; i>=0; i--) {
    m=owl_view_get_element(v, i);
    if (skip_deleted && owl_message_is_delete(m)) continue;
    if (f && !owl_filter_message_match(f, m)) continue;
    found = 1;
    break;
  }

  if (i<0) i=0;

  if (!found) {
    owl_function_makemsg("already at first%s message%s%s",
			 skip_deleted?" non-deleted":"",
			 filter?" in ":"", filter?filter:"");
    /* if (!skip_deleted) owl_function_beep(); */
  }

  if (first_if_none || found) {
    owl_global_set_curmsg(&g, i);
    owl_function_calculate_topmsg(OWL_DIRECTION_UPWARDS);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    owl_global_set_direction_upwards(&g);
  }
}

void owl_function_nextmsg()
{
  owl_function_nextmsg_full(NULL, 0, 1);
}


void owl_function_prevmsg()
{
  owl_function_prevmsg_full(NULL, 0, 1);
}

void owl_function_nextmsg_notdeleted()
{
  owl_function_nextmsg_full(NULL, 1, 1);
}

void owl_function_prevmsg_notdeleted()
{
  owl_function_prevmsg_full(NULL, 1, 1);
}


void owl_function_nextmsg_personal()
{
  owl_function_nextmsg_full("personal", 0, 0);
}

void owl_function_prevmsg_personal()
{
  owl_function_prevmsg_full("personal", 0, 0);
}


/* if move_after is 1, moves after the delete */
void owl_function_deletecur(int move_after)
{
  int curmsg;
  owl_view *v;

  v=owl_global_get_current_view(&g);

  /* bail if there's no current message */
  if (owl_view_get_size(v) < 1) {
    owl_function_makemsg("No current message to delete");
    return;
  }

  /* mark the message for deletion */
  curmsg=owl_global_get_curmsg(&g);
  owl_view_delete_element(v, curmsg);

  if (move_after) {
    /* move the poiner in the appropriate direction 
     * to the next undeleted msg */
    if (owl_global_get_direction(&g)==OWL_DIRECTION_UPWARDS) {
      owl_function_prevmsg_notdeleted();
    } else {
      owl_function_nextmsg_notdeleted();
    }
  }
}


void owl_function_undeletecur(int move_after)
{
  int curmsg;
  owl_view *v;

  v=owl_global_get_current_view(&g);
  
  if (owl_view_get_size(v) < 1) {
    owl_function_makemsg("No current message to undelete");
    return;
  }
  curmsg=owl_global_get_curmsg(&g);

  owl_view_undelete_element(v, curmsg);

  if (move_after) {
    if (owl_global_get_direction(&g)==OWL_DIRECTION_UPWARDS) {
      if (curmsg>0) {
	owl_function_prevmsg();
      } else {
	owl_function_nextmsg();
      }
    } else {
      owl_function_nextmsg();
    }
  }

  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
}


void owl_function_expunge()
{
  int curmsg;
  owl_message *m;
  owl_messagelist *ml;
  owl_view *v;
  int lastmsgid=0;

  curmsg=owl_global_get_curmsg(&g);
  v=owl_global_get_current_view(&g);
  ml=owl_global_get_msglist(&g);

  m=owl_view_get_element(v, curmsg);
  if (m) lastmsgid = owl_message_get_id(m);

  /* expunge the message list */
  owl_messagelist_expunge(ml);

  /* update all views (we only have one right now) */
  owl_view_recalculate(v);

  /* find where the new position should be
     (as close as possible to where we last where) */
  curmsg = owl_view_get_nearest_to_msgid(v, lastmsgid);
  if (curmsg>owl_view_get_size(v)-1) curmsg = owl_view_get_size(v)-1;
  if (curmsg<0) curmsg = 0;
  owl_global_set_curmsg(&g, curmsg);
  owl_function_calculate_topmsg(OWL_DIRECTION_NONE);
  /* if there are no messages set the direction to down in case we
     delete everything upwards */
  owl_global_set_direction_downwards(&g);
  
  owl_function_makemsg("Messages expunged");
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
}


void owl_function_firstmsg()
{
  owl_global_set_curmsg(&g, 0);
  owl_global_set_topmsg(&g, 0);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_direction_downwards(&g);
}

void owl_function_lastmsg_noredisplay()
{
  int oldcurmsg, curmsg;
  owl_view *v;

  v=owl_global_get_current_view(&g);
  oldcurmsg=owl_global_get_curmsg(&g);
  curmsg=owl_view_get_size(v)-1;  
  if (curmsg<0) curmsg=0;
  owl_global_set_curmsg(&g, curmsg);
  if (oldcurmsg < curmsg) {
    owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
  } else if (curmsg<owl_view_get_size(v)) {
    /* If already at the end, blank the screen and move curmsg
     * past the end of the messages. */
    owl_global_set_topmsg(&g, curmsg+1);
    owl_global_set_curmsg(&g, curmsg+1);
  } 
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_direction_downwards(&g);
}

void owl_function_lastmsg()
{
  owl_function_lastmsg_noredisplay();
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));  
}

void owl_function_shift_right()
{
  owl_global_set_rightshift(&g, owl_global_get_rightshift(&g)+10);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_needrefresh(&g);
}


void owl_function_shift_left()
{
  int shift;

  shift=owl_global_get_rightshift(&g);
  if (shift>=10) {
    owl_global_set_rightshift(&g, shift-10);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    owl_global_set_needrefresh(&g);
  } else {
    owl_function_beep();
    owl_function_makemsg("Already full left");
  }
}


void owl_function_unsuball()
{
  unsuball();
  owl_function_makemsg("Unsubscribed from all messages.");
}

void owl_function_loadsubs(char *file)
{
  int ret;

  ret=owl_zephyr_loadsubs(file);

  if (!owl_context_is_interactive(owl_global_get_context(&g))) return;
  if (ret==0) {
    owl_function_makemsg("Subscribed to messages from file.");
  } else if (ret==-1) {
    owl_function_makemsg("Could not open file.");
  } else {
    owl_function_makemsg("Error subscribing to messages from file.");
  }
}

void owl_function_loadloginsubs(char *file)
{
  int ret;

  ret=owl_zephyr_loadloginsubs(file);

  if (!owl_context_is_interactive(owl_global_get_context(&g))) return;
  if (ret==0) {
    owl_function_makemsg("Subscribed to login messages from file.");
  } else if (ret==-1) {
    owl_function_makemsg("Could not open file for login subscriptions.");
  } else {
    owl_function_makemsg("Error subscribing to login messages from file.");
  }
}

void owl_function_suspend()
{
  endwin();
  printf("\n");
  kill(getpid(), SIGSTOP);

  /* resize to reinitialize all the windows when we come back */
  owl_command_resize();
}

void owl_function_zaway_toggle()
{
  if (!owl_global_is_zaway(&g)) {
    owl_global_set_zaway_msg(&g, owl_global_get_zaway_msg_default(&g));
    owl_function_zaway_on();
  } else {
    owl_function_zaway_off();
  }
}

void owl_function_zaway_on()
{
  owl_global_set_zaway_on(&g);
  owl_function_makemsg("zaway set (%s)", owl_global_get_zaway_msg(&g));
}

void owl_function_zaway_off()
{
  owl_global_set_zaway_off(&g);
  owl_function_makemsg("zaway off");
}

void owl_function_quit()
{
  char *ret;
  
  /* zlog out if we need to */
  if (owl_global_is_shutdownlogout(&g)) {
    owl_zephyr_zlog_out();
  }

  /* execute the commands in shutdown */
  ret = owl_config_execute("owl::shutdown();");
  if (ret) owl_free(ret);

  /* signal our child process, if any */
  if (owl_global_get_newmsgproc_pid(&g)) {
    kill(owl_global_get_newmsgproc_pid(&g), SIGHUP);
  }

  /* final clean up */
  unsuball();
  ZClosePort();
  endwin();
  owl_function_debugmsg("Quitting Owl");
  exit(0);
}


void owl_function_makemsg(char *fmt, ...)
{
  va_list ap;
  char buff[2048];

  va_start(ap, fmt);
  werase(owl_global_get_curs_msgwin(&g));
  
  vsnprintf(buff, 2048, fmt, ap);
  owl_function_debugmsg("makemsg: %s", buff);
  waddstr(owl_global_get_curs_msgwin(&g), buff);  
  wnoutrefresh(owl_global_get_curs_msgwin(&g));
  owl_global_set_needrefresh(&g);
  va_end(ap);
}

void owl_function_errormsg(char *fmt, ...)
{
  va_list ap;
  char buff[2048];

  va_start(ap, fmt);
  werase(owl_global_get_curs_msgwin(&g));
  
  vsnprintf(buff, 2048, fmt, ap);
  owl_function_debugmsg("ERROR: %s", buff);
  waddstr(owl_global_get_curs_msgwin(&g), buff);  
  waddstr(owl_global_get_curs_msgwin(&g), "ERROR: ");  
  wnoutrefresh(owl_global_get_curs_msgwin(&g));
  owl_global_set_needrefresh(&g);
  va_end(ap);
}


void owl_function_openurl()
{
  /* visit the first url in the current message */
  owl_message *m;
  owl_view *v;
  char *ptr1, *ptr2, *text, url[LINE], tmpbuff[LINE];
  int webbrowser;

  webbrowser = owl_global_get_webbrowser(&g);

  if (webbrowser < 0 || webbrowser == OWL_WEBBROWSER_NONE) {
    owl_function_makemsg("No browser selected");
    return;
  }

  v=owl_global_get_current_view(&g);
  
  m=owl_view_get_element(v, owl_global_get_curmsg(&g));

  if (!m || owl_view_get_size(v)==0) {
    owl_function_makemsg("No current message selected");
    return;
  }

  text=owl_message_get_text(m);

  /* First look for a good URL */  
  if ((ptr1=strstr(text, "http://"))!=NULL) {
    ptr2=strpbrk(ptr1, " \n\t");
    if (ptr2) {
      strncpy(url, ptr1, ptr2-ptr1+1);
      url[ptr2-ptr1+1]='\0';
    } else {
      strcpy(url, ptr1);
    }

    /* if we had <http strip a trailing > */
    if (ptr1>text && ptr1[-1]=='<') {
      if (url[strlen(url)-1]=='>') {
	url[strlen(url)-1]='\0';
      }
    }
  } else if ((ptr1=strstr(text, "https://"))!=NULL) {
    /* Look for an https URL */  
    ptr2=strpbrk(ptr1, " \n\t");
    if (ptr2) {
      strncpy(url, ptr1, ptr2-ptr1+1);
      url[ptr2-ptr1+1]='\0';
    } else {
      strcpy(url, ptr1);
    }
    
    /* if we had <http strip a trailing > */
    if (ptr1>text && ptr1[-1]=='<') {
      if (url[strlen(url)-1]=='>') {
	url[strlen(url)-1]='\0';
      }
    }
  } else if ((ptr1=strstr(text, "www."))!=NULL) {
    /* if we can't find a real url look for www.something */
    ptr2=strpbrk(ptr1, " \n\t");
    if (ptr2) {
      strncpy(url, ptr1, ptr2-ptr1+1);
      url[ptr2-ptr1+1]='\0';
    } else {
      strcpy(url, ptr1);
    }
  } else {
    owl_function_beep();
    owl_function_makemsg("Could not find URL to open.");
    return;
  }

  /* Make sure there aren't any quotes or \'s in the url */
  for (ptr1 = url; *ptr1; ptr1++) {
    if (*ptr1 == '"' || *ptr1 == '\\') {
      owl_function_beep();
      owl_function_makemsg("URL contains invalid characters.");
      return;
    }
  }
  
  /* NOTE: There are potentially serious security issues here... */

  /* open the page */
  owl_function_makemsg("Opening %s", url);
  if (webbrowser == OWL_WEBBROWSER_NETSCAPE) {
    snprintf(tmpbuff, LINE, "netscape -remote \"openURL(%s)\" > /dev/null 2> /dev/null", url);
    system(tmpbuff); 
  } else if (webbrowser == OWL_WEBBROWSER_GALEON) {
    snprintf(tmpbuff, LINE, "galeon \"%s\" > /dev/null 2> /dev/null &", url);
    system(tmpbuff); 
  } else if (webbrowser == OWL_WEBBROWSER_OPERA) {
    snprintf(tmpbuff, LINE, "opera \"%s\" > /dev/null 2> /dev/null &", url);
    system(tmpbuff); 
  }
}

void owl_function_calculate_topmsg(int direction)
{
  int recwinlines, topmsg, curmsg;
  owl_view *v;

  v=owl_global_get_current_view(&g);
  curmsg=owl_global_get_curmsg(&g);
  topmsg=owl_global_get_topmsg(&g);
  recwinlines=owl_global_get_recwin_lines(&g);

  /*
  if (owl_view_get_size(v) < 1) {
    return;
  }
  */

  switch (owl_global_get_scrollmode(&g)) {
  case OWL_SCROLLMODE_TOP:
    topmsg = owl_function_calculate_topmsg_top(direction, v, curmsg, topmsg, recwinlines);
    break;
  case OWL_SCROLLMODE_NEARTOP:
    topmsg = owl_function_calculate_topmsg_neartop(direction, v, curmsg, topmsg, recwinlines);
    break;
  case OWL_SCROLLMODE_CENTER:
    topmsg = owl_function_calculate_topmsg_center(direction, v, curmsg, topmsg, recwinlines);
    break;
  case OWL_SCROLLMODE_PAGED:
    topmsg = owl_function_calculate_topmsg_paged(direction, v, curmsg, topmsg, recwinlines, 0);
    break;
  case OWL_SCROLLMODE_PAGEDCENTER:
    topmsg = owl_function_calculate_topmsg_paged(direction, v, curmsg, topmsg, recwinlines, 1);
    break;
  case OWL_SCROLLMODE_NORMAL:
  default:
    topmsg = owl_function_calculate_topmsg_normal(direction, v, curmsg, topmsg, recwinlines);
  }
  owl_function_debugmsg("Calculated a topmsg of %i", topmsg);
  owl_global_set_topmsg(&g, topmsg);
}

/* Returns what the new topmsg should be.  
 * Passed the last direction of movement, 
 * the current view,
 * the current message number in the view,
 * the top message currently being displayed,
 * and the number of lines in the recwin.
 */
int owl_function_calculate_topmsg_top(int direction, owl_view *v, int curmsg, int topmsg, int recwinlines)
{
  return(curmsg);
}

int owl_function_calculate_topmsg_neartop(int direction, owl_view *v, int curmsg, int topmsg, int recwinlines)
{
  if (curmsg>0 
      && (owl_message_get_numlines(owl_view_get_element(v, curmsg-1))
	  <  recwinlines/2)) {
    return(curmsg-1);
  } else {
    return(curmsg);
  }
}
  
int owl_function_calculate_topmsg_center(int direction, owl_view *v, int curmsg, int topmsg, int recwinlines)
{
  int i, last, lines;

  last = curmsg;
  lines = 0;
  for (i=curmsg-1; i>=0; i--) {
    lines += owl_message_get_numlines(owl_view_get_element(v, i));
    if (lines > recwinlines/2) break;
    last = i;
  }
  return(last);
}
  
int owl_function_calculate_topmsg_paged(int direction, owl_view *v, int curmsg, int topmsg, int recwinlines, int center_on_page)
{
  int i, last, lines, savey;
  
  /* If we're off the top of the screen, scroll up such that the 
   * curmsg is near the botton of the screen. */
  if (curmsg < topmsg) {
    last = curmsg;
    lines = 0;
    for (i=curmsg; i>=0; i--) {
      lines += owl_message_get_numlines(owl_view_get_element(v, i));
      if (lines > recwinlines) break;
    last = i;
    }
    if (center_on_page) {
      return(owl_function_calculate_topmsg_center(direction, v, curmsg, 0, recwinlines));
    } else {
      return(last);
    }
  }

  /* Find number of lines from top to bottom of curmsg (store in savey) */
  savey=0;
  for (i=topmsg; i<=curmsg; i++) {
    savey+=owl_message_get_numlines(owl_view_get_element(v, i));
  }

  /* if we're off the bottom of the screen, scroll down */
  if (savey > recwinlines) {
    if (center_on_page) {
      return(owl_function_calculate_topmsg_center(direction, v, curmsg, 0, recwinlines));
    } else {
      return(curmsg);
    }
  }

  /* else just stay as we are... */
  return(topmsg);
}


int owl_function_calculate_topmsg_normal(int direction, owl_view *v, int curmsg, int topmsg, int recwinlines)
{
  int savey, j, i, foo, y;

  if (curmsg<0) return(topmsg);
    
  /* If we're off the top of the screen then center */
  if (curmsg<topmsg) {
    topmsg=owl_function_calculate_topmsg_center(direction, v, curmsg, 0, recwinlines);
  }

  /* Find number of lines from top to bottom of curmsg (store in savey) */
  savey=0;
  for (i=topmsg; i<=curmsg; i++) {
    savey+=owl_message_get_numlines(owl_view_get_element(v, i));
  }

  /* If we're off the bottom of the screen, set the topmsg to curmsg
   * and scroll upwards */
  if (savey > recwinlines) {
    topmsg=curmsg;
    savey=owl_message_get_numlines(owl_view_get_element(v, i));
    direction=OWL_DIRECTION_UPWARDS;
  }
  
  /* If our bottom line is less than 1/4 down the screen then scroll up */
  if (direction == OWL_DIRECTION_UPWARDS || direction == OWL_DIRECTION_NONE) {
    if (savey < (recwinlines / 4)) {
      y=0;
      for (j=curmsg; j>=0; j--) {
	foo=owl_message_get_numlines(owl_view_get_element(v, j));
	/* will we run the curmsg off the screen? */
	if ((foo+y) >= recwinlines) {
	  j++;
	  if (j>curmsg) j=curmsg;
	  break;
	}
	/* have saved 1/2 the screen space? */
	y+=foo;
	if (y > (recwinlines / 2)) break;
      }
      if (j<0) j=0;
      return(j);
    }
  }

  if (direction == OWL_DIRECTION_DOWNWARDS || direction == OWL_DIRECTION_NONE) {
    /* If curmsg bottom line is more than 3/4 down the screen then scroll down */
    if (savey > ((recwinlines * 3)/4)) {
      y=0;
      /* count lines from the top until we can save 1/2 the screen size */
      for (j=topmsg; j<curmsg; j++) {
	y+=owl_message_get_numlines(owl_view_get_element(v, j));
	if (y > (recwinlines / 2)) break;
      }
      if (j==curmsg) {
	j--;
      }
      return(j+1);
    }
  }

  return(topmsg);
}


void owl_function_resize()
{
  owl_global_set_resize_pending(&g);
}


void owl_function_run_buffercommand()
{
  char *buff;

  buff=owl_global_get_buffercommand(&g);
  if (!strncmp(buff, "zwrite ", 7)) {
    owl_function_zwrite(buff);
  } else if (!strncmp(buff, "aimwrite ", 9)) {
    owl_function_aimwrite(buff+9);
  }
}

void owl_function_debugmsg(char *fmt, ...)
{
  FILE *file;
  time_t now;
  char buff1[LINE], buff2[LINE];
  va_list ap;
  va_start(ap, fmt);

  if (!owl_global_is_debug_fast(&g)) return;

  file=fopen(owl_global_get_debug_file(&g), "a");
  if (!file) return;

  now=time(NULL);
  strcpy(buff1, ctime(&now));
  buff1[strlen(buff1)-1]='\0';

  owl_global_get_runtime_string(&g, buff2);
  
  fprintf(file, "[%i -  %s - %s]: ", (int) getpid(), buff1, buff2);
  vfprintf(file, fmt, ap);
  fprintf(file, "\n");
  fclose(file);

  va_end(ap);
}


void owl_function_refresh()
{
  owl_function_resize();
}

void owl_function_beep()
{
  if (owl_global_is_bell(&g)) {
    beep();
    owl_global_set_needrefresh(&g); /* do we really need this? */
  }
}


void owl_function_subscribe(char *class, char *inst, char *recip)
{
  int ret;

  ret=owl_zephyr_sub(class, inst, recip);
  if (ret) {
    owl_function_makemsg("Error subscribing.");
  } else {
    owl_function_makemsg("Subscribed.");
  }
}


void owl_function_unsubscribe(char *class, char *inst, char *recip)
{
  int ret;

  ret=owl_zephyr_unsub(class, inst, recip);
  if (ret) {
    owl_function_makemsg("Error subscribing.");
  } else {
    owl_function_makemsg("Unsubscribed.");
  }
}


void owl_function_set_cursor(WINDOW *win)
{
  wnoutrefresh(win);
}


void owl_function_full_redisplay()
{
  redrawwin(owl_global_get_curs_recwin(&g));
  redrawwin(owl_global_get_curs_sepwin(&g));
  redrawwin(owl_global_get_curs_typwin(&g));
  redrawwin(owl_global_get_curs_msgwin(&g));

  wnoutrefresh(owl_global_get_curs_recwin(&g));
  wnoutrefresh(owl_global_get_curs_sepwin(&g));
  wnoutrefresh(owl_global_get_curs_typwin(&g));
  wnoutrefresh(owl_global_get_curs_msgwin(&g));
  
  sepbar("");
  owl_function_makemsg("");

  owl_global_set_needrefresh(&g);
}


void owl_function_popless_text(char *text)
{
  owl_popwin *pw;
  owl_viewwin *v;

  pw=owl_global_get_popwin(&g);
  v=owl_global_get_viewwin(&g);

  owl_popwin_up(pw);
  owl_viewwin_init_text(v, owl_popwin_get_curswin(pw),
			owl_popwin_get_lines(pw), owl_popwin_get_cols(pw),
			text);
  owl_popwin_refresh(pw);
  owl_viewwin_redisplay(v, 0);
  owl_global_set_needrefresh(&g);
}


void owl_function_popless_fmtext(owl_fmtext *fm)
{
  owl_popwin *pw;
  owl_viewwin *v;

  pw=owl_global_get_popwin(&g);
  v=owl_global_get_viewwin(&g);

  owl_popwin_up(pw);
  owl_viewwin_init_fmtext(v, owl_popwin_get_curswin(pw),
		   owl_popwin_get_lines(pw), owl_popwin_get_cols(pw),
		   fm);
  owl_popwin_refresh(pw);
  owl_viewwin_redisplay(v, 0);
  owl_global_set_needrefresh(&g);
}

void owl_function_about()
{
  char buff[5000];

  sprintf(buff, "This is owl version %s\n", OWL_VERSION_STRING);
  strcat(buff, "\nOwl was written by James Kretchmar at the Massachusetts\n");
  strcat(buff, "Institute of Technology.  The first version, 0.5, was\n");
  strcat(buff, "released in March 2002.\n");
  strcat(buff, "\n");
  strcat(buff, "The name 'owl' was chosen in reference to the owls in the\n");
  strcat(buff, "Harry Potter novels, who are tasked with carrying messages\n");
  strcat(buff, "between Witches and Wizards.\n");
  strcat(buff, "\n");
  strcat(buff, "Copyright 2002 Massachusetts Institute of Technology\n");
  strcat(buff, "\n");
  strcat(buff, "Permission to use, copy, modify, and distribute this\n");
  strcat(buff, "software and its documentation for any purpose and without\n");
  strcat(buff, "fee is hereby granted, provided that the above copyright\n");
  strcat(buff, "notice and this permission notice appear in all copies\n");
  strcat(buff, "and in supporting documentation.  No representation is\n");
  strcat(buff, "made about the suitability of this software for any\n");
  strcat(buff, "purpose.  It is provided \"as is\" without express\n");
  strcat(buff, "or implied warranty.\n");
  owl_function_popless_text(buff);
}

void owl_function_info()
{
  owl_message *m;
  owl_fmtext fm, attrfm;
  ZNotice_t *n;
  char buff[10000], tmpbuff[1024];
  char *ptr;
  int i, j, fields, len;
  owl_view *v;

  owl_fmtext_init_null(&fm);
  
  v=owl_global_get_current_view(&g);
  m=owl_view_get_element(v, owl_global_get_curmsg(&g));
  if (!m || owl_view_get_size(v)==0) {
    owl_function_makemsg("No message selected\n");
    return;
  }

  owl_fmtext_append_bold(&fm, "General Information:\n");
  owl_fmtext_append_normal(&fm, "  Msg Id    : ");
  sprintf(buff, "%i", owl_message_get_id(m));
  owl_fmtext_append_normal(&fm, buff);
  owl_fmtext_append_normal(&fm, "\n");

  owl_fmtext_append_normal(&fm, "  Type      : ");
  owl_fmtext_append_bold(&fm, owl_message_type_to_string(m));
  owl_fmtext_append_normal(&fm, "\n");

  if (owl_message_is_direction_in(m)) {
    owl_fmtext_append_normal(&fm, "  Direction : in\n");
  } else if (owl_message_is_direction_out(m)) {
    owl_fmtext_append_normal(&fm, "  Direction : out\n");
  } else if (owl_message_is_direction_none(m)) {
    owl_fmtext_append_normal(&fm, "  Direction : none\n");
  } else {
    owl_fmtext_append_normal(&fm, "  Direction : unknown\n");
  }

  owl_fmtext_append_normal(&fm, "  Time      : ");
  owl_fmtext_append_normal(&fm, owl_message_get_timestr(m));
  owl_fmtext_append_normal(&fm, "\n");

  if (!owl_message_is_type_admin(m)) {
    owl_fmtext_append_normal(&fm, "  Sender    : ");
    owl_fmtext_append_normal(&fm, owl_message_get_sender(m));
    owl_fmtext_append_normal(&fm, "\n");
    
    owl_fmtext_append_normal(&fm, "  Recipient : ");
    owl_fmtext_append_normal(&fm, owl_message_get_recipient(m));
    owl_fmtext_append_normal(&fm, "\n");
  }
    
  if (owl_message_is_type_zephyr(m)) {
    owl_fmtext_append_bold(&fm, "\nZephyr Specific Information:\n");
    
    owl_fmtext_append_normal(&fm, "  Class     : ");
    owl_fmtext_append_normal(&fm, owl_message_get_class(m));
    owl_fmtext_append_normal(&fm, "\n");
    owl_fmtext_append_normal(&fm, "  Instance  : ");
    owl_fmtext_append_normal(&fm, owl_message_get_instance(m));
    owl_fmtext_append_normal(&fm, "\n");
    owl_fmtext_append_normal(&fm, "  Opcode    : ");
    owl_fmtext_append_normal(&fm, owl_message_get_opcode(m));
    owl_fmtext_append_normal(&fm, "\n");
    
    owl_fmtext_append_normal(&fm, "  Time      : ");
    owl_fmtext_append_normal(&fm, owl_message_get_timestr(m));
    owl_fmtext_append_normal(&fm, "\n");

    if (owl_message_is_direction_in(m)) {
      n=owl_message_get_notice(m);

      owl_fmtext_append_normal(&fm, "  Kind      : ");
      if (n->z_kind==UNSAFE) {
	owl_fmtext_append_normal(&fm, "UNSAFE\n");
      } else if (n->z_kind==UNACKED) {
	owl_fmtext_append_normal(&fm, "UNACKED\n");
      } else if (n->z_kind==ACKED) {
	owl_fmtext_append_normal(&fm, "ACKED\n");
      } else if (n->z_kind==HMACK) {
	owl_fmtext_append_normal(&fm, "HMACK\n");
      } else if (n->z_kind==HMCTL) {
	owl_fmtext_append_normal(&fm, "HMCTL\n");
      } else if (n->z_kind==SERVACK) {
	owl_fmtext_append_normal(&fm, "SERVACK\n");
      } else if (n->z_kind==SERVNAK) {
	owl_fmtext_append_normal(&fm, "SERVNACK\n");
      } else if (n->z_kind==CLIENTACK) {
	owl_fmtext_append_normal(&fm, "CLIENTACK\n");
      } else if (n->z_kind==STAT) {
	owl_fmtext_append_normal(&fm, "STAT\n");
      } else {
	owl_fmtext_append_normal(&fm, "ILLEGAL VALUE\n");
      }
      owl_fmtext_append_normal(&fm, "  Host      : ");
      owl_fmtext_append_normal(&fm, owl_message_get_hostname(m));
      owl_fmtext_append_normal(&fm, "\n");
      sprintf(buff, "  Port      : %i\n", n->z_port);
      owl_fmtext_append_normal(&fm, buff);

      owl_fmtext_append_normal(&fm,    "  Auth      : ");
      if (n->z_auth == ZAUTH_FAILED) {
	owl_fmtext_append_normal(&fm, "FAILED\n");
      } else if (n->z_auth == ZAUTH_NO) {
	owl_fmtext_append_normal(&fm, "NO\n");
      } else if (n->z_auth == ZAUTH_YES) {
	owl_fmtext_append_normal(&fm, "YES\n");
      } else {
	sprintf(buff, "Unknown State (%i)\n", n->z_auth);
	owl_fmtext_append_normal(&fm, buff);
      }

      /* fix this */
      sprintf(buff, "  Checkd Ath: %i\n", n->z_checked_auth);
      sprintf(buff, "%s  Multi notc: %s\n", buff, n->z_multinotice);
      sprintf(buff, "%s  Num other : %i\n", buff, n->z_num_other_fields);
      sprintf(buff, "%s  Msg Len   : %i\n", buff, n->z_message_len);
      owl_fmtext_append_normal(&fm, buff);
      
      sprintf(buff, "  Fields    : %i\n", owl_zephyr_get_num_fields(n));
      owl_fmtext_append_normal(&fm, buff);
      
      fields=owl_zephyr_get_num_fields(n);
      for (i=0; i<fields; i++) {
	sprintf(buff, "  Field %i   : ", i+1);
	
	ptr=owl_zephyr_get_field(n, i+1, &len);
	if (!ptr) break;
	if (len<30) {
	  strncpy(tmpbuff, ptr, len);
	  tmpbuff[len]='\0';
	} else {
	  strncpy(tmpbuff, ptr, 30);
	  tmpbuff[30]='\0';
	  strcat(tmpbuff, "...");
	}
	
	for (j=0; j<strlen(tmpbuff); j++) {
	  if (tmpbuff[j]=='\n') tmpbuff[j]='~';
	  if (tmpbuff[j]=='\r') tmpbuff[j]='!';
	}
	
	strcat(buff, tmpbuff);
	strcat(buff, "\n");
	owl_fmtext_append_normal(&fm, buff);
      }
      owl_fmtext_append_normal(&fm, "  Default Fm:");
      owl_fmtext_append_normal(&fm, n->z_default_format);
    }
  }

  if (owl_message_is_type_aim(m)) {
    owl_fmtext_append_bold(&fm, "\nAIM Specific Information:\n");
  }

  owl_fmtext_append_bold(&fm, "\nOwl Message Attributes:\n");
  owl_message_attributes_tofmtext(m, &attrfm);
  owl_fmtext_append_fmtext(&fm, &attrfm);
  
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
  owl_fmtext_free(&attrfm);
}


/* print the current message in a popup window.
 * Use the 'default' style regardless of whatever
 * style the user may be using
 */
void owl_function_curmsg_to_popwin()
{
  owl_popwin *pw;
  owl_view *v;
  owl_message *m;
  owl_style *s;
  owl_fmtext fm;

  v=owl_global_get_current_view(&g);
  s=owl_global_get_style_by_name(&g, "default");
  pw=owl_global_get_popwin(&g);

  m=owl_view_get_element(v, owl_global_get_curmsg(&g));

  if (!m || owl_view_get_size(v)==0) {
    owl_function_makemsg("No current message");
    return;
  }

  owl_fmtext_init_null(&fm);
  owl_style_get_formattext(s, &fm, m);

  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}


void owl_function_page_curmsg(int step)
{
  /* scroll down or up within the current message IF the message is truncated */

  int offset, curmsg, lines;
  owl_view *v;
  owl_message *m;

  offset=owl_global_get_curmsg_vert_offset(&g);
  v=owl_global_get_current_view(&g);
  curmsg=owl_global_get_curmsg(&g);
  m=owl_view_get_element(v, curmsg);
  if (!m || owl_view_get_size(v)==0) return;
  lines=owl_message_get_numlines(m);

  if (offset==0) {
    /* Bail if the curmsg isn't the last one displayed */
    if (curmsg != owl_mainwin_get_last_msg(owl_global_get_mainwin(&g))) {
      owl_function_makemsg("The entire message is already displayed");
      return;
    }
    
    /* Bail if we're not truncated */
    if (!owl_mainwin_is_curmsg_truncated(owl_global_get_mainwin(&g))) {
      owl_function_makemsg("The entire message is already displayed");
      return;
    }
  }
  
  
  /* don't scroll past the last line */
  if (step>0) {
    if (offset+step > lines-1) {
      owl_global_set_curmsg_vert_offset(&g, lines-1);
    } else {
      owl_global_set_curmsg_vert_offset(&g, offset+step);
    }
  }

  /* would we be before the beginning of the message? */
  if (step<0) {
    if (offset+step<0) {
      owl_global_set_curmsg_vert_offset(&g, 0);
    } else {
      owl_global_set_curmsg_vert_offset(&g, offset+step);
    }
  }
  
  /* redisplay */
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_needrefresh(&g);
}

void owl_function_resize_typwin(int newsize)
{
  owl_global_set_typwin_lines(&g, newsize);
  owl_function_resize();
}

void owl_function_typwin_grow()
{
  int i;

  i=owl_global_get_typwin_lines(&g);
  owl_function_resize_typwin(i+1);
}

void owl_function_typwin_shrink()
{
  int i;

  i=owl_global_get_typwin_lines(&g);
  if (i>2) {
    owl_function_resize_typwin(i-1);
  }
}

void owl_function_mainwin_pagedown()
{
  int i;

  i=owl_mainwin_get_last_msg(owl_global_get_mainwin(&g));
  if (i<0) return;
  if (owl_mainwin_is_last_msg_truncated(owl_global_get_mainwin(&g))
      && (owl_global_get_curmsg(&g) < i)
      && (i>0)) {
    i--;
  }
  owl_global_set_curmsg(&g, i);
  owl_function_nextmsg();
}

void owl_function_mainwin_pageup()
{
  owl_global_set_curmsg(&g, owl_global_get_topmsg(&g));
  owl_function_prevmsg();
}

void owl_function_getsubs()
{
  int ret, num, i, one;
  ZSubscription_t sub;
  char *buff, *tmpbuff;

  one = 1;

  ret=ZRetrieveSubscriptions(0, &num);
  if (ret == ZERR_TOOMANYSUBS) {
    owl_function_makemsg("Zephyr: too many subscriptions");
    return;
  }

  buff=owl_malloc(num*500);
  tmpbuff=owl_malloc(num*500);
  strcpy(buff, "");
  for (i=0; i<num; i++) {
    if ((ret = ZGetSubscriptions(&sub, &one)) != ZERR_NONE) {
      owl_function_makemsg("Error while getting subscriptions");
      owl_free(buff);
      owl_free(tmpbuff);
      ZFlushSubscriptions();
      return;
    } else {
      sprintf(tmpbuff, "<%s,%s,%s>\n%s", sub.zsub_class, sub.zsub_classinst, sub.zsub_recipient, buff);
      strcpy(buff, tmpbuff);
    }
  }

  owl_function_popless_text(buff);
  owl_free(buff);
  owl_free(tmpbuff);
  ZFlushSubscriptions();
}

#define PABUFLEN 5000
void owl_function_printallvars()
{
  char buff[PABUFLEN], *pos, *name;
  owl_list varnames;
  int i, numvarnames, rem;

  pos = buff;
  pos += sprintf(pos, "%-20s = %s\n", "VARIABLE", "VALUE");
  pos += sprintf(pos, "%-20s   %s\n",  "--------", "-----");
  owl_variable_dict_get_names(owl_global_get_vardict(&g), &varnames);
  rem = (buff+PABUFLEN)-pos-1;
  numvarnames = owl_list_get_size(&varnames);
  for (i=0; i<numvarnames; i++) {
    name = owl_list_get_element(&varnames, i);
    if (name && name[0]!='_') {
      rem = (buff+PABUFLEN)-pos-1;    
      pos += snprintf(pos, rem, "\n%-20s = ", name);
      rem = (buff+PABUFLEN)-pos-1;    
      owl_variable_get_tostring(owl_global_get_vardict(&g), name, pos, rem);
      pos = buff+strlen(buff);
    }
  }
  rem = (buff+PABUFLEN)-pos-1;    
  snprintf(pos, rem, "\n");
  owl_variable_dict_namelist_free(&varnames);
  
  owl_function_popless_text(buff);
}

void owl_function_show_variables()
{
  owl_list varnames;
  owl_fmtext fm;  
  int i, numvarnames;
  char *varname;

  owl_fmtext_init_null(&fm);
  owl_fmtext_append_bold(&fm, 
      "Variables: (use 'show variable <name>' for details)\n");
  owl_variable_dict_get_names(owl_global_get_vardict(&g), &varnames);
  numvarnames = owl_list_get_size(&varnames);
  for (i=0; i<numvarnames; i++) {
    varname = owl_list_get_element(&varnames, i);
    if (varname && varname[0]!='_') {
      owl_variable_describe(owl_global_get_vardict(&g), varname, &fm);
    }
  }
  owl_variable_dict_namelist_free(&varnames);
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_show_variable(char *name)
{
  owl_fmtext fm;  

  owl_fmtext_init_null(&fm);
  owl_variable_get_help(owl_global_get_vardict(&g), name, &fm);
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);  
}

/* note: this applies to global message list, not to view.
 * If flag is 1, deletes.  If flag is 0, undeletes. */
void owl_function_delete_by_id(int id, int flag)
{
  owl_messagelist *ml;
  owl_message *m;
  ml = owl_global_get_msglist(&g);
  m = owl_messagelist_get_by_id(ml, id);
  if (m) {
    if (flag == 1) {
      owl_message_mark_delete(m);
    } else if (flag == 0) {
      owl_message_unmark_delete(m);
    }
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    owl_global_set_needrefresh(&g);
  } else {
    owl_function_makemsg("No message with id %d: unable to mark for (un)delete",id);
  }
}

void owl_function_delete_automsgs()
{
  /* mark for deletion all messages in the current view that match the
   * 'trash' filter */

  int i, j, count;
  owl_message *m;
  owl_view *v;
  owl_filter *f;

  /* get the trash filter */
  f=owl_global_get_filter(&g, "trash");
  if (!f) {
    owl_function_makemsg("No trash filter defined");
    return;
  }

  v=owl_global_get_current_view(&g);

  count=0;
  j=owl_view_get_size(v);
  for (i=0; i<j; i++) {
    m=owl_view_get_element(v, i);
    if (owl_filter_message_match(f, m)) {
      count++;
      owl_message_mark_delete(m);
    }
  }
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_makemsg("%i messages marked for deletion", count);
  owl_global_set_needrefresh(&g);
}


void owl_function_status()
{
  char buff[5000];
  time_t start;
  int up, days, hours, minutes;

  start=owl_global_get_starttime(&g);

  sprintf(buff, "Version: %s\n", OWL_VERSION_STRING);
  sprintf(buff, "%sScreen size: %i lines, %i columns\n", buff, owl_global_get_lines(&g), owl_global_get_cols(&g));
  sprintf(buff, "%sStartup Arugments: %s\n", buff, owl_global_get_startupargs(&g));
  sprintf(buff, "%sStartup Time: %s", buff, ctime(&start));

  up=owl_global_get_runtime(&g);
  days=up/86400;
  up-=days*86400;
  hours=up/3600;
  up-=hours*3600;
  minutes=up/60;
  up-=minutes*60;
  sprintf(buff, "%sRun Time: %i days %2.2i:%2.2i:%2.2i\n", buff, days, hours, minutes, up);

  if (owl_global_get_hascolors(&g)) {
    sprintf(buff, "%sColor: Yes, %i color pairs.\n", buff, owl_global_get_colorpairs(&g));
  } else {
    strcat(buff, "Color: No.\n");
  }

  sprintf(buff, "%sMemory Malloced: %i\n", buff, owl_global_get_malloced(&g));
  sprintf(buff, "%sMemory Freed: %i\n", buff, owl_global_get_freed(&g));
  sprintf(buff, "%sMemory In Use: %i\n", buff, owl_global_get_meminuse(&g));

  owl_function_popless_text(buff);
}

void owl_function_show_term()
{
  owl_fmtext fm;
  char buff[LINE];

  owl_fmtext_init_null(&fm);
  sprintf(buff, "Terminal Lines: %i\nTerminal Columns: %i\n",
	  owl_global_get_lines(&g),
	  owl_global_get_cols(&g));
  owl_fmtext_append_normal(&fm, buff);

  if (owl_global_get_hascolors(&g)) {
    owl_fmtext_append_normal(&fm, "Color: Yes\n");
    sprintf(buff, "Number of color pairs: %i\n", owl_global_get_colorpairs(&g));
    owl_fmtext_append_normal(&fm, buff);
    sprintf(buff, "Can change colors: %s\n", can_change_color() ? "yes" : "no");
    owl_fmtext_append_normal(&fm, buff);
  } else {
    owl_fmtext_append_normal(&fm, "Color: No\n");
  }

  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}


void owl_function_reply(int type, int enter)
{
  /* if type = 0 then normal reply.
   * if type = 1 then it's a reply to sender
   * if enter = 0 then allow the command to be edited
   * if enter = 1 then don't wait for editing
   */
  char *buff, *oldbuff;
  owl_message *m;
  owl_filter *f;
  
  if (owl_view_get_size(owl_global_get_current_view(&g))==0) {
    owl_function_makemsg("No message selected");
  } else {
    char *class, *inst, *to, *cc=NULL;
    
    m=owl_view_get_element(owl_global_get_current_view(&g), owl_global_get_curmsg(&g));
    if (!m) {
      owl_function_makemsg("No message selected");
      return;
    }

    /* first check if we catch the reply-lockout filter */
    f=owl_global_get_filter(&g, "reply-lockout");
    if (f) {
      if (owl_filter_message_match(f, m)) {
	owl_function_makemsg("Sorry, replies to this message have been disabled by the reply-lockout filter");
	return;
      }
    }

    /* admin */
    if (owl_message_is_type_admin(m)) {
      owl_function_makemsg("You cannot reply to an admin message");
      return;
    }

    /* zephyr */
    if (owl_message_is_type_zephyr(m)) {
      /* for now we disable replies to zcrypt messages, since we can't
	 support an encrypted reply */
      if (!strcasecmp(owl_message_get_opcode(m), "crypt")) {
	owl_function_makemsg("Replies to zcrypt messages are not enabled in this release");
	return;
      }

      if (owl_message_is_direction_out(m)) {
	owl_function_zwrite_setup(owl_message_get_zwriteline(m));
	owl_global_set_buffercommand(&g, owl_message_get_zwriteline(m));
	return;
      }
      
      if (owl_message_is_loginout(m)) {
	class="MESSAGE";
	inst="PERSONAL";
	to=owl_message_get_sender(m);
      } else if (type==1) {
	class="MESSAGE";
	inst="PERSONAL";
	to=owl_message_get_sender(m);
      } else {
	class=owl_message_get_class(m);
	inst=owl_message_get_instance(m);
	to=owl_message_get_recipient(m);
	cc=owl_message_get_cc(m);
	if (!strcmp(to, "") || !strcmp(to, "*")) {
	  to="";
	} else if (to[0]=='@') {
	  /* leave it, to get the realm */
	} else {
	  to=owl_message_get_sender(m);
	}
      }
	
      /* create the command line */
      buff = owl_strdup("zwrite");
      if (strcasecmp(class, "message")) {
	buff = owl_sprintf("%s -c %s%s%s", oldbuff=buff, owl_getquoting(class), class, owl_getquoting(class));
	owl_free(oldbuff);
      }
      if (strcasecmp(inst, "personal")) {
	buff = owl_sprintf("%s -i %s%s%s", oldbuff=buff, owl_getquoting(inst), inst, owl_getquoting(inst));
	owl_free(oldbuff);
      }
      if (*to != '\0') {
	char *tmp, *oldtmp, *tmp2;
	tmp=short_zuser(to);
	if (cc) {
	  tmp = owl_util_uniq(oldtmp=tmp, cc, "-");
	  owl_free(oldtmp);
	  buff = owl_sprintf("%s -C %s", oldbuff=buff, tmp);
	  owl_free(oldbuff);
	} else {
	  if (owl_global_is_smartstrip(&g)) {
	    tmp2=tmp;
	    tmp=owl_util_smartstripped_user(tmp2);
	    owl_free(tmp2);
	  }
	  buff = owl_sprintf("%s %s", oldbuff=buff, tmp);
	  owl_free(oldbuff);
	}
	owl_free(tmp);
      }
      if (cc) owl_free(cc);
    }

    /* aim */
    if (owl_message_is_type_aim(m)) {
      if (owl_message_is_direction_out(m)) {
	buff=owl_sprintf("aimwrite %s", owl_message_get_recipient(m));
      } else {
	buff=owl_sprintf("aimwrite %s", owl_message_get_sender(m));
      }
    }
    
    if (enter) {
      owl_history *hist = owl_global_get_cmd_history(&g);
      owl_history_store(hist, buff);
      owl_history_reset(hist);
      owl_function_command_norv(buff);
    } else {
      owl_function_start_command(buff);
    }
    owl_free(buff);
  }
}

void owl_function_zlocate(int argc, char **argv, int auth)
{
  owl_fmtext fm;
  char *ptr, buff[LINE];
  int i;

  owl_fmtext_init_null(&fm);

  for (i=0; i<argc; i++) {
    ptr=long_zuser(argv[i]);
    owl_zephyr_zlocate(ptr, buff, auth);
    owl_fmtext_append_normal(&fm, buff);
    owl_free(ptr);
  }

  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_start_command(char *line)
{
  int i, j;
  owl_editwin *tw;

  tw=owl_global_get_typwin(&g);
  owl_global_set_typwin_active(&g);
  owl_editwin_new_style(tw, OWL_EDITWIN_STYLE_ONELINE, 
			owl_global_get_cmd_history(&g));

  owl_editwin_set_locktext(tw, "command: ");
  owl_global_set_needrefresh(&g);

  j=strlen(line);
  for (i=0; i<j; i++) {
    owl_editwin_process_char(tw, line[i]);
  }
  owl_editwin_redisplay(tw, 0);
}

char *owl_function_exec(int argc, char **argv, char *buff, int type)
{
  /* if type == 1 display in a popup
   * if type == 2 display an admin messages
   * if type == 0 return output
   * else display in a popup
   */
  char *newbuff, *redirect = " 2>&1 < /dev/null";
  char *out, buff2[1024];
  int size;
  FILE *p;

  if (argc<2) {
    owl_function_makemsg("Wrong number of arguments to the exec command");
    return NULL;
  }

  buff = skiptokens(buff, 1);
  newbuff = owl_malloc(strlen(buff)+strlen(redirect)+1);
  strcpy(newbuff, buff);
  strcat(newbuff, redirect);

  p=popen(newbuff, "r");
  out=owl_malloc(1024);
  size=1024;
  strcpy(out, "");
  while (fgets(buff2, 1024, p)!=NULL) {
    size+=1024;
    out=owl_realloc(out, size);
    strcat(out, buff2);
  }
  pclose(p);

  if (type==1) {
    owl_function_popless_text(out);
  } else if (type==0) {
    return out;
  } else if (type==2) {
    owl_function_adminmsg(buff, out);
  } else {
    owl_function_popless_text(out);
  }
  owl_free(out);
  return NULL;
}


char *owl_function_perl(int argc, char **argv, char *buff, int type)
{
  /* if type == 1 display in a popup
   * if type == 2 display an admin messages
   * if type == 0 return output
   * else display in a popup
   */
  char *perlout;

  if (argc<2) {
    owl_function_makemsg("Wrong number of arguments to perl command");
    return NULL;
  }

  /* consume first token (argv[0]) */
  buff = skiptokens(buff, 1);

  perlout = owl_config_execute(buff);
  if (perlout) { 
    if (type==1) {
      owl_function_popless_text(perlout);
    } else if (type==2) {
      owl_function_adminmsg(buff, perlout);
    } else if (type==0) {
      return perlout;
    } else {
      owl_function_popless_text(perlout);
    }
    owl_free(perlout);
  }
  return NULL;
}

#if 0
void owl_function_change_view_old(char *filtname)
{
  owl_view *v;
  owl_filter *f;
  int curid=-1, newpos, curmsg;
  owl_message *curm=NULL;

  v=owl_global_get_current_view(&g);
  curmsg=owl_global_get_curmsg(&g);
  if (curmsg==-1) {
    owl_function_debugmsg("Hit the curmsg==-1 case in change_view");
  } else {
    curm=owl_view_get_element(v, curmsg);
    if (curm) {
      curid=owl_message_get_id(curm);
      owl_view_save_curmsgid(v, curid);
    }
  }

  /* grab the filter */;
  f=owl_global_get_filter(&g, filtname);
  if (!f) {
    owl_function_makemsg("Unknown filter");
    return;
  }

  /* free the existing view and create a new one based on the filter */
  owl_view_free(v);
  owl_view_create(v, f);

  /* Figure out what to set the current message to.
   * - If the view we're leaving has messages in it, go to the closest message
   *   to the last message pointed to in that view. 
   * - If the view we're leaving is empty, try to restore the position
   *   from the last time we were in the new view.  */
  if (curm) {
    newpos = owl_view_get_nearest_to_msgid(v, curid);
  } else {
    newpos = owl_view_get_nearest_to_saved(v);
  }

  owl_global_set_curmsg(&g, newpos);

  owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_direction_downwards(&g);
}
#endif

void owl_function_change_view(char *filtname)
{
  owl_view *v;
  owl_filter *f;
  int curid=-1, newpos, curmsg;
  owl_message *curm=NULL;

  v=owl_global_get_current_view(&g);

  curmsg=owl_global_get_curmsg(&g);
  if (curmsg==-1) {
    owl_function_debugmsg("Hit the curmsg==-1 case in change_view");
  } else {
    curm=owl_view_get_element(v, curmsg);
    if (curm) {
      curid=owl_message_get_id(curm);
      owl_view_save_curmsgid(v, curid);
    }
  }

  f=owl_global_get_filter(&g, filtname);
  if (!f) {
    owl_function_makemsg("Unknown filter");
    return;
  }

  owl_view_new_filter(v, f);

  /* Figure out what to set the current message to.
   * - If the view we're leaving has messages in it, go to the closest message
   *   to the last message pointed to in that view. 
   * - If the view we're leaving is empty, try to restore the position
   *   from the last time we were in the new view.  */
  if (curm) {
    newpos = owl_view_get_nearest_to_msgid(v, curid);
  } else {
    newpos = owl_view_get_nearest_to_saved(v);
  }

  owl_global_set_curmsg(&g, newpos);
  owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_global_set_direction_downwards(&g);
}

void owl_function_create_filter(int argc, char **argv)
{
  owl_filter *f;
  owl_view *v;
  int ret, inuse=0;

  if (argc < 2) {
    owl_function_makemsg("Wrong number of arguments to filter command");
    return;
  }

  v=owl_global_get_current_view(&g);

  /* don't touch the all filter */
  if (!strcmp(argv[1], "all")) {
    owl_function_makemsg("You may not change the 'all' filter.");
    return;
  }

  /* deal with the case of trying change the filter color */
  if (argc==4 && !strcmp(argv[2], "-c")) {
    f=owl_global_get_filter(&g, argv[1]);
    if (!f) {
      owl_function_makemsg("The filter '%s' does not exist.", argv[1]);
      return;
    }
    owl_filter_set_color(f, owl_util_string_to_color(argv[3]));
    owl_global_set_needrefresh(&g);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
    return;
  }

  /* create the filter and check for errors */
  f=owl_malloc(sizeof(owl_filter));
  ret=owl_filter_init(f, argv[1], argc-2, argv+2);
  if (ret==-1) {
    owl_free(f);
    owl_function_makemsg("Invalid filter syntax");
    return;
  }

  /* if the named filter is in use by the current view, remember it */
  if (!strcmp(owl_view_get_filtname(v), argv[1])) {
    inuse=1;
  }

  /* if the named filter already exists, nuke it */
  if (owl_global_get_filter(&g, argv[1])) {
    owl_global_remove_filter(&g, argv[1]);
  }

  /* add the filter */
  owl_global_add_filter(&g, f);

  /* if it was in use by the current view then update */
  if (inuse) {
    owl_function_change_view(argv[1]);
  }
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
}

void owl_function_show_filters()
{
  owl_list *l;
  owl_filter *f;
  int i, j;
  owl_fmtext fm;

  owl_fmtext_init_null(&fm);

  l=owl_global_get_filterlist(&g);
  j=owl_list_get_size(l);

  owl_fmtext_append_bold(&fm, "Filters:\n");

  for (i=0; i<j; i++) {
    f=owl_list_get_element(l, i);
    owl_fmtext_append_normal(&fm, "   ");
    if (owl_global_get_hascolors(&g)) {
      owl_fmtext_append_normal_color(&fm, owl_filter_get_name(f), owl_filter_get_color(f));
    } else {
      owl_fmtext_append_normal(&fm, owl_filter_get_name(f));
    }
    owl_fmtext_append_normal(&fm, "\n");
  }
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_show_filter(char *name)
{
  owl_filter *f;
  char buff[5000];

  f=owl_global_get_filter(&g, name);
  if (!f) {
    owl_function_makemsg("There is no filter with that name");
    return;
  }
  owl_filter_print(f, buff);
  owl_function_popless_text(buff);
}

void owl_function_show_zpunts()
{
  owl_filter *f;
  owl_list *fl;
  char buff[5000];
  owl_fmtext fm;
  int i, j;

  owl_fmtext_init_null(&fm);

  fl=owl_global_get_puntlist(&g);
  j=owl_list_get_size(fl);
  owl_fmtext_append_bold(&fm, "Active zpunt filters:\n");

  for (i=0; i<j; i++) {
    f=owl_list_get_element(fl, i);
    owl_filter_print(f, buff);
    owl_fmtext_append_normal(&fm, buff);
  }
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

/* Create a filter for a class, instance if one doesn't exist.  If
 * instance is NULL then catch all messgaes in the class.  Returns the
 * name of the filter, which the caller must free.
 */
char *owl_function_classinstfilt(char *class, char *instance) 
{
  owl_list *fl;
  owl_filter *f;
  char *argbuff, *filtname;
  char *tmpclass, *tmpinstance = NULL;
  int len;

  fl=owl_global_get_filterlist(&g);

  /* name for the filter */
  len=strlen(class)+30;
  if (instance) len+=strlen(instance);
  filtname=owl_malloc(len);
  if (!instance) {
    sprintf(filtname, "class-%s", class);
  } else {
    sprintf(filtname, "class-%s-instance-%s", class, instance);
  }
  /* downcase it */
  downstr(filtname);
  /* turn spaces into hyphens */
  owl_util_tr(filtname, ' ', '.');
  
  /* if it already exists then go with it.  This lets users override */
  if (owl_global_get_filter(&g, filtname)) {
    return(filtname);
  }

  /* create the new filter */
  argbuff=owl_malloc(len+20);
  tmpclass=owl_strdup(class);
  owl_util_tr(tmpclass, ' ', '.');
  if (instance) {
    tmpinstance=owl_strdup(instance);
    owl_util_tr(tmpinstance, ' ', '.');
  }
  sprintf(argbuff, "( class ^%s$ )", tmpclass);
  if (tmpinstance) {
    sprintf(argbuff, "%s and ( instance ^%s$ )", argbuff, tmpinstance);
  }
  owl_free(tmpclass);
  if (tmpinstance) owl_free(tmpinstance);

  f=owl_malloc(sizeof(owl_filter));
  owl_filter_init_fromstring(f, filtname, argbuff);

  /* add it to the global list */
  owl_global_add_filter(&g, f);

  owl_free(argbuff);
  return(filtname);
}

/* Create a filter for personal zephyrs to or from the specified
 * zephyr user.  Includes login/logout notifications for the user.
 * The name of the filter will be 'user-<user>'.  If a filter already
 * exists with this name, no new filter will be created.  This allows
 * the configuration to override this function.  Returns the name of
 * the filter, which the caller must free.
 */
char *owl_function_zuserfilt(char *user)
{
  owl_filter *f;
  char *argbuff, *longuser, *shortuser, *filtname;

  /* stick the local realm on if it's not there */
  longuser=long_zuser(user);
  shortuser=short_zuser(user);

  /* name for the filter */
  filtname=owl_malloc(strlen(shortuser)+20);
  sprintf(filtname, "user-%s", shortuser);

  /* if it already exists then go with it.  This lets users override */
  if (owl_global_get_filter(&g, filtname)) {
    return(owl_strdup(filtname));
  }

  /* create the new-internal filter */
  f=owl_malloc(sizeof(owl_filter));

  argbuff=owl_malloc(strlen(longuser)+1000);
  sprintf(argbuff, "( type ^zephyr$ and ( class ^message$ and instance ^personal$ and ");
  sprintf(argbuff, "%s ( ( direction ^in$ and sender ^%s$ ) or ( direction ^out$ and recipient ^%s$ ) ) )", argbuff, longuser, longuser);
  sprintf(argbuff, "%s or ( ( class ^login$ ) and ( sender ^%s$ ) ) )", argbuff, longuser);

  owl_filter_init_fromstring(f, filtname, argbuff);

  /* add it to the global list */
  owl_global_add_filter(&g, f);

  /* free stuff */
  owl_free(argbuff);
  owl_free(longuser);
  owl_free(shortuser);

  return(filtname);
}

/* Create a filter for AIM IM messages to or from the specified
 * screenname.  The name of the filter will be 'aimuser-<user>'.  If a
 * filter already exists with this name, no new filter will be
 * created.  This allows the configuration to override this function.
 * Returns the name of the filter, which the caller must free.
 */
char *owl_function_aimuserfilt(char *user)
{
  owl_filter *f;
  char *argbuff, *filtname;

  /* name for the filter */
  filtname=owl_malloc(strlen(user)+40);
  sprintf(filtname, "aimuser-%s", user);

  /* if it already exists then go with it.  This lets users override */
  if (owl_global_get_filter(&g, filtname)) {
    return(owl_strdup(filtname));
  }

  /* create the new-internal filter */
  f=owl_malloc(sizeof(owl_filter));

  argbuff=owl_malloc(1000);
  sprintf(argbuff,
	  "( type ^aim$ and ( ( sender ^%s$ and recipient ^%s$ ) or ( sender ^%s$ and recipient ^%s$ ) ) )",
	  user, owl_global_get_aim_screenname(&g), owl_global_get_aim_screenname(&g), user);

  owl_filter_init_fromstring(f, filtname, argbuff);

  /* add it to the global list */
  owl_global_add_filter(&g, f);

  /* free stuff */
  owl_free(argbuff);

  return(filtname);
}

char *owl_function_typefilt(char *type)
{
  owl_filter *f;
  char *argbuff, *filtname;

  /* name for the filter */
  filtname=owl_sprintf("type-%s", type);

  /* if it already exists then go with it.  This lets users override */
  if (owl_global_get_filter(&g, filtname)) {
    return filtname;
  }

  /* create the new-internal filter */
  f=owl_malloc(sizeof(owl_filter));

  argbuff = owl_sprintf("type ^%s$", type);

  owl_filter_init_fromstring(f, filtname, argbuff);

  /* add it to the global list */
  owl_global_add_filter(&g, f);

  /* free stuff */
  owl_free(argbuff);

  return filtname;
}

/* If flag is 1, marks for deletion.  If flag is 0,
 * unmarks for deletion. */
void owl_function_delete_curview_msgs(int flag)
{
  owl_view *v;
  int i, j;

  v=owl_global_get_current_view(&g);
  j=owl_view_get_size(v);
  for (i=0; i<j; i++) {
    if (flag == 1) {
      owl_message_mark_delete(owl_view_get_element(v, i));
    } else if (flag == 0) {
      owl_message_unmark_delete(owl_view_get_element(v, i));
    }
  }

  owl_function_makemsg("%i messages marked for %sdeletion", j, flag?"":"un");

  owl_mainwin_redisplay(owl_global_get_mainwin(&g));  
}

/* Create a filter based on the current message.  Returns the name of
 * a filter or null.  The caller must free this name.
 *
 * if the curmsg is a personal zephyr return a filter name
 *    to the zephyr converstaion with that user.
 * If the curmsg is a zephyr class message, instance foo, recip *,
 *    return a filter name to the class, inst.
 * If the curmsg is a zephyr class message and type==0 then 
 *    return a filter name for just the class.
 * If the curmsg is a zephyr class message and type==1 then 
 *    return a filter name for the class and instance.
 * If the curmsg is a personal AIM message returna  filter
 *    name to the AIM conversation with that user 
 */
char *owl_function_smartfilter(int type)
{
  owl_view *v;
  owl_message *m;
  char *zperson, *filtname=NULL;
  
  v=owl_global_get_current_view(&g);
  m=owl_view_get_element(v, owl_global_get_curmsg(&g));

  if (!m || owl_view_get_size(v)==0) {
    owl_function_makemsg("No message selected\n");
    return(NULL);
  }

  /* very simple handling of admin messages for now */
  if (owl_message_is_type_admin(m)) {
    return(owl_function_typefilt("admin"));
  }

  /* aim messages */
  if (owl_message_is_type_aim(m)) {
    if (owl_message_is_direction_in(m)) {
      filtname=owl_function_aimuserfilt(owl_message_get_sender(m));
    } else if (owl_message_is_direction_out(m)) {
      filtname=owl_function_aimuserfilt(owl_message_get_recipient(m));
    }
    return(filtname);
  }

  /* narrow personal and login messages to the sender or recip as appropriate */
  if (owl_message_is_personal(m) || owl_message_is_loginout(m)) {
    if (owl_message_is_type_zephyr(m)) {
      if (owl_message_is_direction_in(m)) {
	zperson=short_zuser(owl_message_get_sender(m));
      } else {
	zperson=short_zuser(owl_message_get_recipient(m));
      }
      filtname=owl_function_zuserfilt(zperson);
      owl_free(zperson);
      return(filtname);
    }
    return(NULL);
  }

  /* narrow class MESSAGE, instance foo, recip * messages to class, inst */
  if (!strcasecmp(owl_message_get_class(m), "message") && !owl_message_is_personal(m)) {
    filtname=owl_function_classinstfilt(owl_message_get_class(m), owl_message_get_instance(m));
    return(filtname);
  }

  /* otherwise narrow to the class */
  if (type==0) {
    filtname=owl_function_classinstfilt(owl_message_get_class(m), NULL);
  } else if (type==1) {
    filtname=owl_function_classinstfilt(owl_message_get_class(m), owl_message_get_instance(m));
  }
  return(filtname);
}

void owl_function_smartzpunt(int type)
{
  /* Starts a zpunt command based on the current class,instance pair. 
   * If type=0, uses just class.  If type=1, uses instance as well. */
  owl_view *v;
  owl_message *m;
  char *cmd, *cmdprefix, *mclass, *minst;
  
  v=owl_global_get_current_view(&g);
  m=owl_view_get_element(v, owl_global_get_curmsg(&g));

  if (!m || owl_view_get_size(v)==0) {
    owl_function_makemsg("No message selected\n");
    return;
  }

  /* for now we skip admin messages. */
  if (owl_message_is_type_admin(m)
      || owl_message_is_loginout(m)
      || !owl_message_is_type_zephyr(m)) {
    owl_function_makemsg("smartzpunt doesn't support this message type.");
    return;
  }

  mclass = owl_message_get_class(m);
  minst = owl_message_get_instance(m);
  if (!mclass || !*mclass || *mclass==' '
      || (!strcasecmp(mclass, "message") && !strcasecmp(minst, "personal"))
      || (type && (!minst || !*minst|| *minst==' '))) {
    owl_function_makemsg("smartzpunt can't safely do this for <%s,%s>",
			 mclass, minst);
  } else {
    cmdprefix = "start-command zpunt ";
    cmd = owl_malloc(strlen(cmdprefix)+strlen(mclass)+strlen(minst)+3);
    strcpy(cmd, cmdprefix);
    strcat(cmd, mclass);
    if (type) {
      strcat(cmd, " ");
      strcat(cmd, minst);
    } else {
      strcat(cmd, " *");
    }
    owl_function_command(cmd);
    owl_free(cmd);
  }
}



void owl_function_color_current_filter(char *color)
{
  owl_filter *f;
  char *name;

  name=owl_view_get_filtname(owl_global_get_current_view(&g));
  f=owl_global_get_filter(&g, name);
  if (!f) {
    owl_function_makemsg("Unknown filter");
    return;
  }

  /* don't touch the all filter */
  if (!strcmp(name, "all")) {
    owl_function_makemsg("You may not change the 'all' filter.");
    return;
  }

  /* deal with the case of trying change the filter color */
  owl_filter_set_color(f, owl_util_string_to_color(color));
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
}

void owl_function_show_colors()
{
  owl_fmtext fm;

  owl_fmtext_init_null(&fm);
  owl_fmtext_append_normal_color(&fm, "default\n", OWL_COLOR_DEFAULT);
  owl_fmtext_append_normal_color(&fm, "red\n", OWL_COLOR_RED);
  owl_fmtext_append_normal_color(&fm, "green\n", OWL_COLOR_GREEN);
  owl_fmtext_append_normal_color(&fm, "yellow\n", OWL_COLOR_YELLOW);
  owl_fmtext_append_normal_color(&fm, "blue\n", OWL_COLOR_BLUE);
  owl_fmtext_append_normal_color(&fm, "magenta\n", OWL_COLOR_MAGENTA);
  owl_fmtext_append_normal_color(&fm, "cyan\n", OWL_COLOR_CYAN);
  owl_fmtext_append_normal_color(&fm, "white\n", OWL_COLOR_WHITE);

  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_zpunt(char *class, char *inst, char *recip, int direction)
{
  /* add the given class, inst, recip to the punt list for filtering.
   *   if direction==0 then punt
   *   if direction==1 then unpunt */
  owl_filter *f;
  owl_list *fl;
  char *buff;
  int ret, i, j;

  fl=owl_global_get_puntlist(&g);

  /* first, create the filter */
  f=malloc(sizeof(owl_filter));
  buff=malloc(strlen(class)+strlen(inst)+strlen(recip)+100);
  if (!strcmp(recip, "*")) {
    sprintf(buff, "class ^%s$ and instance ^%s$", class, inst);
  } else {
    sprintf(buff, "class ^%s$ and instance ^%s$ and recipient %s", class, inst, recip);
  }
  owl_function_debugmsg("About to filter %s", buff);
  ret=owl_filter_init_fromstring(f, "punt-filter", buff);
  owl_free(buff);
  if (ret) {
    owl_function_makemsg("Error creating filter for zpunt");
    owl_filter_free(f);
    return;
  }

  /* Check for an identical filter */
  j=owl_list_get_size(fl);
  for (i=0; i<j; i++) {
    if (owl_filter_equiv(f, owl_list_get_element(fl, i))) {
      /* if we're punting, then just silently bow out on this duplicate */
      if (direction==0) {
	owl_filter_free(f);
	return;
      }

      /* if we're unpunting, then remove this filter from the puntlist */
      if (direction==1) {
	owl_filter_free(owl_list_get_element(fl, i));
	owl_list_remove_element(fl, i);
	return;
      }
    }
  }

  /* If we're punting, add the filter to the global punt list */
  if (direction==0) {
    owl_list_append_element(fl, f);
  }
}

void owl_function_activate_keymap(char *keymap)
{
  if (!owl_keyhandler_activate(owl_global_get_keyhandler(&g), keymap)) {
    owl_function_makemsg("Unable to activate keymap '%s'", keymap);
  }
}


void owl_function_show_keymaps()
{
  owl_list l;
  owl_fmtext fm;
  owl_keymap *km;
  owl_keyhandler *kh;
  int i, numkm;
  char *kmname;

  kh = owl_global_get_keyhandler(&g);
  owl_fmtext_init_null(&fm);
  owl_fmtext_append_bold(&fm, "Keymaps:   ");
  owl_fmtext_append_normal(&fm, "(use 'show keymap <name>' for details)\n");
  owl_keyhandler_get_keymap_names(kh, &l);
  owl_fmtext_append_list(&fm, &l, "\n", owl_function_keymap_summary);
  owl_fmtext_append_normal(&fm, "\n");

  numkm = owl_list_get_size(&l);
  for (i=0; i<numkm; i++) {
    kmname = owl_list_get_element(&l, i);
    km = owl_keyhandler_get_keymap(kh, kmname);
    owl_fmtext_append_bold(&fm, "\n\n----------------------------------------------------------------------------------------------------\n\n");
    owl_keymap_get_details(km, &fm);    
  }
  owl_fmtext_append_normal(&fm, "\n");
  
  owl_function_popless_fmtext(&fm);
  owl_keyhandler_keymap_namelist_free(&l);
  owl_fmtext_free(&fm);
}

char *owl_function_keymap_summary(void *name)
{
  owl_keymap *km 
    = owl_keyhandler_get_keymap(owl_global_get_keyhandler(&g), name);
  if (km) return owl_keymap_summary(km);
  else return(NULL);
}

/* TODO: implement for real */
void owl_function_show_keymap(char *name)
{
  owl_fmtext fm;
  owl_keymap *km;

  owl_fmtext_init_null(&fm);
  km = owl_keyhandler_get_keymap(owl_global_get_keyhandler(&g), name);
  if (km) {
    owl_keymap_get_details(km, &fm);
  } else {
    owl_fmtext_append_normal(&fm, "No such keymap...\n");
  }  
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_help_for_command(char *cmdname)
{
  owl_fmtext fm;

  owl_fmtext_init_null(&fm);
  owl_cmd_get_help(owl_global_get_cmddict(&g), cmdname, &fm);
  owl_function_popless_fmtext(&fm);  
  owl_fmtext_free(&fm);
}

void owl_function_search_start(char *string, int direction)
{
  /* direction is OWL_DIRECTION_DOWNWARDS or OWL_DIRECTION_UPWARDS */
  owl_global_set_search_active(&g, string);
  owl_function_search_helper(0, direction);
}

void owl_function_search_continue(int direction)
{
  /* direction is OWL_DIRECTION_DOWNWARDS or OWL_DIRECTION_UPWARDS */
  owl_function_search_helper(1, direction);
}

void owl_function_search_helper(int mode, int direction)
{
  /* move to a message that contains the string.  If direction is
   * OWL_DIRECTION_DOWNWARDS then search fowards, if direction is
   * OWL_DIRECTION_UPWARDS then search backwards.
   *
   * If mode==0 then it will stay on the current message if it
   * contains the string.
   */

  owl_view *v;
  int viewsize, i, curmsg, start;
  owl_message *m;

  v=owl_global_get_current_view(&g);
  viewsize=owl_view_get_size(v);
  curmsg=owl_global_get_curmsg(&g);
  
  if (viewsize==0) {
    owl_function_makemsg("No messages present");
    return;
  }

  if (mode==0) {
    start=curmsg;
  } else if (direction==OWL_DIRECTION_DOWNWARDS) {
    start=curmsg+1;
  } else {
    start=curmsg-1;
  }

  /* bounds check */
  if (start>=viewsize || start<0) {
    owl_function_makemsg("No further matches found");
    return;
  }

  for (i=start; i<viewsize && i>=0;) {
    m=owl_view_get_element(v, i);
    if (owl_message_search(m, owl_global_get_search_string(&g))) {
      owl_global_set_curmsg(&g, i);
      owl_function_calculate_topmsg(direction);
      owl_mainwin_redisplay(owl_global_get_mainwin(&g));
      if (direction==OWL_DIRECTION_DOWNWARDS) {
	owl_global_set_direction_downwards(&g);
      } else {
	owl_global_set_direction_upwards(&g);
      }
      return;
    }
    if (direction==OWL_DIRECTION_DOWNWARDS) {
      i++;
    } else {
      i--;
    }
  }
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_makemsg("No matches found");
}


/* strips formatting from ztext and returns the unformatted text. 
 * caller is responsible for freeing. */
char *owl_function_ztext_stylestrip(char *zt)
{
  owl_fmtext fm;
  char *plaintext;

  owl_fmtext_init_null(&fm);
  owl_fmtext_append_ztext(&fm, zt);
  plaintext = owl_fmtext_print_plain(&fm);
  owl_fmtext_free(&fm);
  return(plaintext);
}

/* Popup a buddylisting.  If file is NULL use the default .anyone */
void owl_function_buddylist(int aim, int zephyr, char *file)
{
  char *ourfile, *tmp, buff[LINE], *line;
  FILE *f;
  int numlocs, ret, i, j;
  ZLocations_t location[200];
  owl_fmtext fm;
  owl_buddylist *b;

  owl_fmtext_init_null(&fm);

  if (aim && owl_global_is_aimloggedin(&g)) {
    b=owl_global_get_buddylist(&g);

    owl_fmtext_append_bold(&fm, "AIM users logged in:\n");
    j=owl_buddylist_get_size(b);
    for (i=0; i<j; i++) {
      owl_fmtext_append_normal(&fm, "  ");
      owl_fmtext_append_normal(&fm, owl_buddylist_get_buddy(b, i));
      owl_fmtext_append_normal(&fm, "\n");
    }
  }

  if (zephyr) {
    if (file==NULL) {
      tmp=owl_global_get_homedir(&g);
      if (!tmp) {
	owl_function_makemsg("Could not determine home directory");
	return;
      }
      ourfile=owl_malloc(strlen(tmp)+50);
      sprintf(ourfile, "%s/.anyone", owl_global_get_homedir(&g));
    } else {
      ourfile=owl_strdup(file);
    }
    
    f=fopen(ourfile, "r");
    if (!f) {
      owl_function_makemsg("Error opening file %s: %s",
			   ourfile,
			   strerror(errno) ? strerror(errno) : "");
      return;
    }
    
    owl_fmtext_append_bold(&fm, "Zephyr users logged in:\n");
    
    while (fgets(buff, LINE, f)!=NULL) {
      /* ignore comments, blank lines etc. */
      if (buff[0]=='#') continue;
      if (buff[0]=='\n') continue;
      if (buff[0]=='\0') continue;
      
      /* strip the \n */
      buff[strlen(buff)-1]='\0';
      
      /* ingore from # on */
      tmp=strchr(buff, '#');
      if (tmp) tmp[0]='\0';
      
      /* ingore from SPC */
      tmp=strchr(buff, ' ');
      if (tmp) tmp[0]='\0';
      
      /* stick on the local realm. */
      if (!strchr(buff, '@')) {
	strcat(buff, "@");
	strcat(buff, ZGetRealm());
      }
      
      ret=ZLocateUser(buff, &numlocs, ZAUTH);
      if (ret!=ZERR_NONE) {
	owl_function_makemsg("Error getting location for %s", buff);
	continue;
      }
      
      numlocs=200;
      ret=ZGetLocations(location, &numlocs);
      if (ret==0) {
	for (i=0; i<numlocs; i++) {
	  line=malloc(strlen(location[i].host)+strlen(location[i].time)+strlen(location[i].tty)+100);
	  tmp=short_zuser(buff);
	  sprintf(line, "  %-10.10s %-24.24s %-12.12s  %20.20s\n",
		  tmp,
		  location[i].host,
		  location[i].tty,
		  location[i].time);
	  owl_fmtext_append_normal(&fm, line);
	  owl_free(tmp);
	}
	if (numlocs>=200) {
	  owl_fmtext_append_normal(&fm, "  Too many locations found for this user, truncating.\n");
	}
      }
    }
    fclose(f);
    owl_free(ourfile);
  }
  
  owl_function_popless_fmtext(&fm);
  owl_fmtext_free(&fm);
}

void owl_function_dump(char *filename) 
{
  int i, j, count;
  owl_message *m;
  owl_view *v;
  FILE *file;
  /* struct stat sbuf; */

  v=owl_global_get_current_view(&g);

  /* in the future make it ask yes/no */
  /*
  ret=stat(filename, &sbuf);
  if (!ret) {
    ret=owl_function_askyesno("File exists, continue? [Y/n]");
    if (!ret) return;
  }
  */

  file=fopen(filename, "w");
  if (!file) {
    owl_function_makemsg("Error opening file");
    return;
  }

  count=0;
  j=owl_view_get_size(v);
  for (i=0; i<j; i++) {
    m=owl_view_get_element(v, i);
    fputs(owl_message_get_text(m), file);
  }
  fclose(file);
}



void owl_function_do_newmsgproc(void)
{
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
	owl_function_debugmsg("I'm the parent and I started a new newmsgproc with pid %i", i);
      } else {
	/* child exec's the program */
	char **parsed;
	parsed=owl_parseline(owl_global_get_newmsgproc(&g), &myargc);
	if (myargc < 0) {
	  owl_function_debugmsg("Could not parse newmsgproc '%s': unbalanced quotes?", owl_global_get_newmsgproc(&g));
	}
	if (myargc <= 0) {
	  _exit(127);
	}
	parsed=realloc(parsed, sizeof(*parsed) * (myargc+1));
	parsed[myargc] = NULL;
	
	owl_function_debugmsg("About to exec \"%s\" with %d arguments", parsed[0], myargc);
	
	execvp(parsed[0], parsed);
	
	
	/* was there an error exec'ing? */
	owl_function_debugmsg("Cannot run newmsgproc '%s': cannot exec '%s': %s", 
			      owl_global_get_newmsgproc(&g), parsed[0], strerror(errno));
	_exit(127);
      }
    }
  }
}

/* print the xterm escape sequence to raise the window */
void owl_function_xterm_raise(void)
{
  printf("\033[5t");
}

/* print the xterm escape sequence to deiconify the window */
void owl_function_xterm_deiconify(void)
{
  printf("\033[1t");
}

/* Add the specified command to the startup file.  Eventually this
 * should be clever, and rewriting settings that will obviosly
 * override earlier settings with 'set' 'bindkey' and 'alias'
 * commands.  For now though we just remove any line that would
 * duplicate this one and then append this line to the end of
 * startupfile.
 */
void owl_function_addstartup(char *buff)
{
  FILE *file;
  char *filename;

  filename=owl_sprintf("%s/%s", owl_global_get_homedir(&g), OWL_STARTUP_FILE);
  file=fopen(filename, "a");
  if (!file) {
    owl_function_makemsg("Error opening startupfile for new command");
    owl_free(filename);
    return;
  }

  /* delete earlier copies */
  owl_util_file_deleteline(filename, buff, 1);
  owl_free(filename);

  /* add this line */
  fprintf(file, "%s\n", buff);

  fclose(file);
}

/* Remove the specified command from the startup file. */
void owl_function_delstartup(char *buff)
{
  char *filename;
  filename=owl_sprintf("%s/%s", owl_global_get_homedir(&g), OWL_STARTUP_FILE);
  owl_util_file_deleteline(filename, buff, 1);
  owl_free(filename);
}

void owl_function_execstartup(void)
{
  FILE *file;
  char *filename;
  char buff[LINE];

  filename=owl_sprintf("%s/%s", owl_global_get_homedir(&g), OWL_STARTUP_FILE);
  file=fopen(filename, "r");
  owl_free(filename);
  if (!file) {
    /* just fail silently if it doesn't exist */
    return;
  }
  while (fgets(buff, LINE, file)!=NULL) {
    buff[strlen(buff)-1]='\0';
    owl_function_command(buff);
  }
  fclose(file);
}

void owl_function_toggleoneline()
{
  char *style;

  style=owl_global_get_style(&g);

  if (strcmp(style, "oneline")) {
    owl_global_set_style(&g, "oneline");
  } else {
    owl_global_set_style(&g, owl_global_get_default_style(&g));
  }
}
