#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

void owl_global_init(owl_global *g) {
  struct hostent *hent;
  char hostname[MAXHOSTNAMELEN];

  g->malloced=0;
  g->freed=0;

  gethostname(hostname, MAXHOSTNAMELEN);
  hent=gethostbyname(hostname);
  if (!hent) {
    g->thishost=owl_strdup("localhost");
  } else {
    g->thishost=owl_strdup(hent->h_name);
  }

  owl_context_init(&g->ctx);
  owl_context_set_startup(&g->ctx);
  g->curmsg=0;
  g->topmsg=0;
  g->needrefresh=1;
  g->startupargs=NULL;

  owl_variable_dict_setup(&(g->vars));
  owl_cmddict_setup(&(g->cmds));

  g->lines=LINES;
  g->cols=COLS;

  g->rightshift=0;

  owl_editwin_init(&(g->tw), NULL, owl_global_get_typwin_lines(g), g->cols, OWL_EDITWIN_STYLE_ONELINE, NULL);

  owl_keyhandler_init(&g->kh);
  owl_keys_setup_keymaps(&g->kh);

  owl_list_create(&(g->filterlist));
  owl_list_create(&(g->puntlist));
  owl_list_create(&(g->messagequeue));
  owl_list_create(&(g->stylelist));
  g->curmsg_vert_offset=0;
  g->resizepending=0;
  g->typwinactive=0;
  g->direction=OWL_DIRECTION_DOWNWARDS;
  g->zaway=0;
  if (has_colors()) {
    g->hascolors=1;
  }
  g->colorpairs=COLOR_PAIRS;
  g->debug=OWL_DEBUG;
  g->searchactive=0;
  g->searchstring=NULL;
  g->starttime=time(NULL); /* assumes we call init only a start time */
  g->buffercommand=NULL;
  g->newmsgproc_pid=0;
  
  owl_global_set_config_format(g, 0);
  owl_global_set_userclue(g, OWL_USERCLUE_NONE);
  owl_global_set_no_have_config(g);
  owl_history_init(&(g->msghist));
  owl_history_init(&(g->cmdhist));
  g->nextmsgid=0;

  owl_filterelement_create_true(&(g->fe_true));
  owl_filterelement_create_false(&(g->fe_false));
  owl_filterelement_create_null(&(g->fe_null));

  _owl_global_setup_windows(g);

  /* Fill in some variables which don't have constant defaults */
  /* TODO: come back later and check passwd file first */
  g->homedir=owl_strdup(getenv("HOME"));

  owl_messagelist_create(&(g->msglist));
  owl_mainwin_init(&(g->mw));
  owl_popwin_init(&(g->pw));

  g->aim_screenname=NULL;
  g->aim_loggedin=0;
  owl_timer_create_countdown(&(g->aim_noop_timer), 30);
  owl_timer_create_countdown(&(g->aim_ignorelogin_timer), 0);
  owl_buddylist_init(&(g->buddylist));
  g->response=NULL;
  g->havezephyr=0;
  g->haveaim=0;
}

void _owl_global_setup_windows(owl_global *g) {
  int cols, typwin_lines;

  cols=g->cols;
  typwin_lines=owl_global_get_typwin_lines(g);

  /* set the new window sizes */
  g->recwinlines=g->lines-(typwin_lines+2);
  if (g->recwinlines<1) {
    /* this will screw things up.  I'm not sure what to do yet,
       but this is better than nothing */
    /* g->recwinlines=1; */
  }

  /* create the new windows */
  g->recwin=newwin(g->recwinlines, cols, 0, 0);
  if (g->recwin==NULL) {
    owl_function_debugmsg("\n\nI just received an error on creating a new receive window\n");
    owl_function_debugmsg("newwin was called with arguments (%i, %i, 0, 0) and returned NULL\n",
	   g->recwinlines, cols);
    endwin();

    exit(50);
  }
      
  g->sepwin=newwin(1, cols, g->recwinlines, 0);
  g->msgwin=newwin(1, cols, g->recwinlines+1, 0);
  g->typwin=newwin(typwin_lines, cols, g->recwinlines+2, 0);

  owl_editwin_set_curswin(&(g->tw), g->typwin, typwin_lines, g->cols);

  idlok(g->typwin, FALSE);
  idlok(g->recwin, FALSE);
  idlok(g->sepwin, FALSE);
  idlok(g->msgwin, FALSE);

  nodelay(g->typwin, 1);
  keypad(g->typwin, TRUE);
  wmove(g->typwin, 0, 0);

  meta(g->typwin, TRUE);
}

owl_context *owl_global_get_context(owl_global *g) {
  return(&g->ctx);
}
			 
int owl_global_get_lines(owl_global *g) {
  return(g->lines);
}

int owl_global_get_cols(owl_global *g) {
  return(g->cols);
}

int owl_global_get_recwin_lines(owl_global *g) {
  return(g->recwinlines);
}

/* curmsg */

int owl_global_get_curmsg(owl_global *g) {
  return(g->curmsg);
}

void owl_global_set_curmsg(owl_global *g, int i) {
  g->curmsg=i;
  /* we will reset the vertical offset from here */
  /* we might want to move this out to the functions later */
  owl_global_set_curmsg_vert_offset(g, 0);
}

/* topmsg */

int owl_global_get_topmsg(owl_global *g) {
  return(g->topmsg);
}

void owl_global_set_topmsg(owl_global *g, int i) {
  g->topmsg=i;
}

/* windows */

owl_mainwin *owl_global_get_mainwin(owl_global *g) {
  return(&(g->mw));
}

owl_popwin *owl_global_get_popwin(owl_global *g) {
  return(&(g->pw));
}

/* msglist */

owl_messagelist *owl_global_get_msglist(owl_global *g) {
  return(&(g->msglist));
}

/* keyhandler */

owl_keyhandler *owl_global_get_keyhandler(owl_global *g) {
  return(&(g->kh));
}

/* curses windows */

WINDOW *owl_global_get_curs_recwin(owl_global *g) {
  return(g->recwin);
}

WINDOW *owl_global_get_curs_sepwin(owl_global *g) {
  return(g->sepwin);
}

WINDOW *owl_global_get_curs_msgwin(owl_global *g) {
  return(g->msgwin);
}

WINDOW *owl_global_get_curs_typwin(owl_global *g) {
  return(g->typwin);
}

/* typwin */

owl_editwin *owl_global_get_typwin(owl_global *g) {
  return(&(g->tw));
}

/* buffercommand */

void owl_global_set_buffercommand(owl_global *g, char *command) {
  if (g->buffercommand) owl_free(g->buffercommand);
  g->buffercommand=owl_strdup(command);
}

char *owl_global_get_buffercommand(owl_global *g) {
  if (g->buffercommand) return(g->buffercommand);
  return("");
}

/* refresh */

int owl_global_is_needrefresh(owl_global *g) {
  if (g->needrefresh==1) return(1);
  return(0);
}

void owl_global_set_needrefresh(owl_global *g) {
  g->needrefresh=1;
}

void owl_global_set_noneedrefresh(owl_global *g) {
  g->needrefresh=0;
}

/* variable dictionary */

owl_vardict *owl_global_get_vardict(owl_global *g) {
  return &(g->vars);
}

/* command dictionary */

owl_cmddict *owl_global_get_cmddict(owl_global *g) {
  return &(g->cmds);
}

/* rightshift */

void owl_global_set_rightshift(owl_global *g, int i) {
  g->rightshift=i;
}

int owl_global_get_rightshift(owl_global *g) {
  return(g->rightshift);
}

/* typwin */

int owl_global_is_typwin_active(owl_global *g) {
  if (g->typwinactive==1) return(1);
  return(0);
}

void owl_global_set_typwin_active(owl_global *g) {
  g->typwinactive=1;
}

void owl_global_set_typwin_inactive(owl_global *g) {
  g->typwinactive=0;
}

/* resize */

void owl_global_set_resize_pending(owl_global *g) {
  g->resizepending=1;
}

char *owl_global_get_homedir(owl_global *g) {
  if (g->homedir) return(g->homedir);
  return("/");
}

int owl_global_get_direction(owl_global *g) {
  return(g->direction);
}

void owl_global_set_direction_downwards(owl_global *g) {
  g->direction=OWL_DIRECTION_DOWNWARDS;
}

void owl_global_set_direction_upwards(owl_global *g) {
  g->direction=OWL_DIRECTION_UPWARDS;
}

/* perl stuff */

void owl_global_set_perlinterp(owl_global *g, void *p) {
  g->perl=p;
}

void *owl_global_get_perlinterp(owl_global *g) {
  return(g->perl);
}

int owl_global_is_config_format(owl_global *g) {
  if (g->config_format) return(1);
  return(0);
}

void owl_global_set_config_format(owl_global *g, int state) {
  if (state==1) {
    g->config_format=1;
  } else {
    g->config_format=0;
  }
}

void owl_global_set_have_config(owl_global *g) {
  g->haveconfig=1;
}

void owl_global_set_no_have_config(owl_global *g) {
  g->haveconfig=0;
}

int owl_global_have_config(owl_global *g) {
  if (g->haveconfig) return(1);
  return(0);
}

void owl_global_resize(owl_global *g, int x, int y) {
  /* resize the screen.  If x or y is 0 use the terminal size */
  struct winsize size;
    
  if (!g->resizepending) return;

  /* delete the current windows */
  delwin(g->recwin);
  delwin(g->sepwin);
  delwin(g->msgwin);
  delwin(g->typwin);
  if (!isendwin()) {
    endwin();
  }

  refresh();

  /* get the new size */
  ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
  if (x==0) {
    g->lines=size.ws_row;
  } else {
    g->lines=x;
  }

  if (y==0) {
    g->cols=size.ws_col;
  } else {
    g->cols=y;
  }

  /* resizeterm(size.ws_row, size.ws_col); */

  /* re-initialize the windows */
  _owl_global_setup_windows(g);

  /* refresh stuff */
  g->needrefresh=1;
  owl_mainwin_redisplay(&(g->mw));
  sepbar(NULL);

  if (owl_global_is_typwin_active(g)) {
    owl_editwin_redisplay(&(g->tw), 0);
  }	
  /* TODO: this should handle other forms of popwins */
  if (owl_popwin_is_active(owl_global_get_popwin(g)) 
      && owl_global_get_viewwin(g)) {
    owl_popwin_refresh(owl_global_get_popwin(g));
    owl_viewwin_redisplay(owl_global_get_viewwin(g), 0);
  }

  owl_function_debugmsg("New size is %i lines, %i cols.", size.ws_row, size.ws_col);
  owl_function_makemsg("");
  g->resizepending=0;
}

/* debug */

int owl_global_is_debug_fast(owl_global *g) {
  if (g->debug) return(1);
  return(0);
}

/* starttime */

time_t owl_global_get_starttime(owl_global *g) {
  return(g->starttime);
}

time_t owl_global_get_runtime(owl_global *g) {
  return(time(NULL)-g->starttime);
}

void owl_global_get_runtime_string(owl_global *g, char *buff) {
  time_t diff;

  diff=time(NULL)-owl_global_get_starttime(g);

  /* print something nicer later */   
  sprintf(buff, "%i seconds", (int) diff);
}

char *owl_global_get_hostname(owl_global *g) {
  if (g->thishost) return(g->thishost);
  return("");
}

/* userclue */

void owl_global_set_userclue(owl_global *g, int clue) {
  g->userclue=clue;
}

void owl_global_add_userclue(owl_global *g, int clue) {
  g->userclue|=clue;
}

int owl_global_get_userclue(owl_global *g) {
  return(g->userclue);
}

int owl_global_is_userclue(owl_global *g, int clue) {
  if (g->userclue & clue) return(1);
  return(0);
}

/* viewwin */

owl_viewwin *owl_global_get_viewwin(owl_global *g) {
  return(&(g->vw));
}


/* vert offset */

int owl_global_get_curmsg_vert_offset(owl_global *g) {
  return(g->curmsg_vert_offset);
}

void owl_global_set_curmsg_vert_offset(owl_global *g, int i) {
  g->curmsg_vert_offset=i;
}

/* startup args */

void owl_global_set_startupargs(owl_global *g, int argc, char **argv) {
  int i, len;

  if (g->startupargs) owl_free(g->startupargs);
  
  len=0;
  for (i=0; i<argc; i++) {
    len+=strlen(argv[i])+5;
  }
  g->startupargs=malloc(len+5);

  strcpy(g->startupargs, "");
  for (i=0; i<argc; i++) {
    sprintf(g->startupargs, "%s%s ", g->startupargs, argv[i]);
  }
  g->startupargs[strlen(g->startupargs)-1]='\0';
}

char *owl_global_get_startupargs(owl_global *g) {
  if (g->startupargs) return(g->startupargs);
  return("");
}

/* history */

owl_history *owl_global_get_msg_history(owl_global *g) {
  return(&(g->msghist));
}

owl_history *owl_global_get_cmd_history(owl_global *g) {
  return(&(g->cmdhist));
}

/* filterlist */

owl_list *owl_global_get_filterlist(owl_global *g) {
  return(&(g->filterlist));
}

owl_filter *owl_global_get_filter(owl_global *g, char *name) {
  int i, j;
  owl_filter *f;

  j=owl_list_get_size(&(g->filterlist));
  for (i=0; i<j; i++) {
    f=owl_list_get_element(&(g->filterlist), i);
    if (!strcmp(name, owl_filter_get_name(f))) {
      return(f);
    }
  }
  return(NULL);
}

void owl_global_add_filter(owl_global *g, owl_filter *f) {
  owl_list_append_element(&(g->filterlist), f);
}

void owl_global_remove_filter(owl_global *g, char *name) {
  int i, j;
  owl_filter *f;

  j=owl_list_get_size(&(g->filterlist));
  for (i=0; i<j; i++) {
    f=owl_list_get_element(&(g->filterlist), i);
    if (!strcmp(name, owl_filter_get_name(f))) {
      owl_filter_free(f);
      owl_list_remove_element(&(g->filterlist), i);
      break;
    }
  }
}

/* nextmsgid */

int owl_global_get_nextmsgid(owl_global *g) {
  return(g->nextmsgid++);
}

/* current view */

owl_view *owl_global_get_current_view(owl_global *g) {
  return(&(g->current_view));
}

owl_filterelement *owl_global_get_filterelement_true(owl_global *g) {
  return(&(g->fe_true));
}

owl_filterelement *owl_global_get_filterelement_false(owl_global *g) {
  return(&(g->fe_false));
}

owl_filterelement *owl_global_get_filterelement_null(owl_global *g) {
  return(&(g->fe_null));
}

/* has colors */

int owl_global_get_hascolors(owl_global *g) {
  if (g->hascolors) return(1);
  return(0);
}

/* color pairs */

int owl_global_get_colorpairs(owl_global *g) {
  return(g->colorpairs);
}

/* puntlist */

owl_list *owl_global_get_puntlist(owl_global *g) {
  return(&(g->puntlist));
}

int owl_global_message_is_puntable(owl_global *g, owl_message *m) {
  owl_list *pl;
  int i, j;

  pl=owl_global_get_puntlist(g);
  j=owl_list_get_size(pl);
  for (i=0; i<j; i++) {
    if (owl_filter_message_match(owl_list_get_element(pl, i), m)) return(1);
  }
  return(0);
}

int owl_global_should_followlast(owl_global *g) {
  owl_view *v;
  
  if (!owl_global_is__followlast(g)) return(0);
  
  v=owl_global_get_current_view(g);
  
  if (owl_global_get_curmsg(g)==owl_view_get_size(v)-1) return(1);
  return(0);
}

int owl_global_is_search_active(owl_global *g) {
  if (g->searchactive) return(1);
  return(0);
}

void owl_global_set_search_active(owl_global *g, char *string) {
  g->searchactive=1;
  if (g->searchstring != NULL) owl_free(g->searchstring);
  g->searchstring=owl_strdup(string);
}

void owl_global_set_search_inactive(owl_global *g) {
  g->searchactive=0;
}

char *owl_global_get_search_string(owl_global *g) {
  if (g->searchstring==NULL) return("");
  return(g->searchstring);
}

void owl_global_set_newmsgproc_pid(owl_global *g, int i) {
  g->newmsgproc_pid=i;
}

int owl_global_get_newmsgproc_pid(owl_global *g) {
  return(g->newmsgproc_pid);
}

void owl_global_add_to_malloced(owl_global *g, int i) {
  g->malloced+=i;
}

void owl_global_add_to_freed(owl_global *g, int i) {
  g->freed+=1;
}

int owl_global_get_malloced(owl_global *g) {
  return(g->malloced);
}

int owl_global_get_freed(owl_global *g) {
  return(g->freed);
}

int owl_global_get_meminuse(owl_global *g) {
  return(g->malloced-g->freed);
}

/* AIM stuff */

int owl_global_is_aimloggedin(owl_global *g)
{
  if (g->aim_loggedin) return(1);
  return(0);
}

char *owl_global_get_aim_screenname(owl_global *g)
{
  return (g->aim_screenname);
}

void owl_global_set_aimloggedin(owl_global *g, char *screenname)
{
  g->aim_loggedin=1;
  if (g->aim_screenname) owl_free(g->aim_screenname);
  g->aim_screenname=owl_strdup(screenname);
}

void owl_global_set_aimnologgedin(owl_global *g)
{
  g->aim_loggedin=0;
}

aim_session_t *owl_global_get_aimsess(owl_global *g)
{
  return(&(g->aimsess));
}

aim_conn_t *owl_global_get_waitingconn(owl_global *g)
{
  return(&(g->waitingconn));
}

int owl_global_is_aimnop_time(owl_global *g)
{
  if (owl_timer_is_expired(&(g->aim_noop_timer))) return(1);
  return(0);
}

void owl_global_aimnop_sent(owl_global *g)
{
  owl_timer_reset(&(g->aim_noop_timer));
}

owl_timer *owl_global_get_aim_login_timer(owl_global *g)
{
  return(&(g->aim_ignorelogin_timer));
}

/* message queue */

void owl_global_messagequeue_addmsg(owl_global *g, owl_message *m)
{
  owl_list_append_element(&(g->messagequeue), m);
}

/* pop off the first message and return it.  Return NULL if the queue
 * is empty.  The caller should free the message after using it, if
 * necessary.
 */
owl_message *owl_global_messageuque_popmsg(owl_global *g)
{
  owl_message *out;

  if (owl_list_get_size(&(g->messagequeue))==0) return(NULL);
  out=owl_list_get_element(&(g->messagequeue), 0);
  owl_list_remove_element(&(g->messagequeue), 0);
  return(out);
}

int owl_global_messagequeue_pending(owl_global *g)
{
  if (owl_list_get_size(&(g->messagequeue))==0) return(0);
  return(1);
}

owl_buddylist *owl_global_get_buddylist(owl_global *g)
{
  return(&(g->buddylist));
}
  
/* style */

/* Return the style with name 'name'.  If it does not exist return
 * NULL */
owl_style *owl_global_get_style_by_name(owl_global *g, char *name)
{
  int i, j;
  owl_style *s;
  
  j=owl_list_get_size(&(g->stylelist));
  for (i=0; i<j; i++) {
    s=owl_list_get_element(&(g->stylelist), i);
    if (owl_style_matches_name(s, name)) {
      return(s);
    }
  }
  return(NULL);
}

void owl_global_add_style(owl_global *g, owl_style *s)
{
  owl_list_append_element(&(g->stylelist), s);
}

char *owl_global_get_response(owl_global *g)
{
  if (g->response==NULL) return("");
  return(g->response);
}

void owl_global_set_response(owl_global *g, char *resp)
{
  if (g->response) owl_free(g->response);
  g->response=owl_strdup(resp);
}


void owl_global_set_haveaim(owl_global *g)
{
  g->haveaim=1;
}

int owl_global_is_haveaim(owl_global *g)
{
  if (g->haveaim) return(1);
  return(0);
}

void owl_global_set_havezephyr(owl_global *g)
{
  g->havezephyr=1;
}

int owl_global_is_havezephyr(owl_global *g)
{
  if (g->havezephyr) return(1);
  return(0);
}
