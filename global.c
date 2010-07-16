#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include "owl.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 256
#endif

void owl_global_init(owl_global *g) {
  struct hostent *hent;
  char hostname[MAXHOSTNAMELEN];
  char *cd;

  g->malloced=0;
  g->freed=0;

  gethostname(hostname, MAXHOSTNAMELEN);
  hent=gethostbyname(hostname);
  if (!hent) {
    g->thishost=owl_strdup("localhost");
  } else {
    g->thishost=owl_strdup(hent->h_name);
  }

  g->context_stack = NULL;
  owl_global_push_context(g, OWL_CTX_STARTUP, NULL, NULL);

  g->curmsg=0;
  g->topmsg=0;
  g->markedmsgid=-1;
  g->needrefresh=1;
  g->startupargs=NULL;

  owl_variable_dict_setup(&(g->vars));

  g->lines=LINES;
  g->cols=COLS;

  g->rightshift=0;

  g->tw = NULL;

  owl_keyhandler_init(&g->kh);
  owl_keys_setup_keymaps(&g->kh);

  owl_dict_create(&(g->filters));
  g->filterlist = NULL;
  owl_list_create(&(g->puntlist));
  owl_list_create(&(g->messagequeue));
  owl_dict_create(&(g->styledict));
  g->curmsg_vert_offset=0;
  g->resizepending=0;
  g->relayoutpending = 0;
  g->direction=OWL_DIRECTION_DOWNWARDS;
  g->zaway=0;
  if (has_colors()) {
    g->hascolors=1;
  }
  g->colorpairs=COLOR_PAIRS;
  owl_fmtext_init_colorpair_mgr(&(g->cpmgr));
  g->debug=OWL_DEBUG;
  owl_regex_init(&g->search_re);
  g->starttime=time(NULL); /* assumes we call init only a start time */
  g->lastinputtime=g->starttime;
  g->newmsgproc_pid=0;
  
  owl_global_set_config_format(g, 0);
  owl_global_set_userclue(g, OWL_USERCLUE_NONE);
  owl_global_set_no_have_config(g);
  owl_history_init(&(g->msghist));
  owl_history_init(&(g->cmdhist));
  owl_history_set_norepeats(&(g->cmdhist));
  g->nextmsgid=0;

  _owl_global_setup_windows(g);

  /* Fill in some variables which don't have constant defaults */
  /* TODO: come back later and check passwd file first */
  g->homedir=owl_strdup(getenv("HOME"));

  g->confdir = NULL;
  g->startupfile = NULL;
  cd = owl_sprintf("%s/%s", g->homedir, OWL_CONFIG_DIR);
  owl_global_set_confdir(g, cd);
  owl_free(cd);

  owl_messagelist_create(&(g->msglist));
  owl_mainwin_init(&(g->mw));
  owl_popwin_init(&(g->pw));

  g->aim_screenname=NULL;
  g->aim_screenname_for_filters=NULL;
  g->aim_loggedin=0;
  owl_buddylist_init(&(g->buddylist));

  g->havezephyr=0;
  g->haveaim=0;
  g->ignoreaimlogin=0;
  owl_global_set_no_doaimevents(g);

  owl_errqueue_init(&(g->errqueue));
  g->got_err_signal=0;

  owl_zbuddylist_create(&(g->zbuddies));

  g->zaldlist = NULL;
  g->pseudologin_notify = 0;

  owl_obarray_init(&(g->obarray));

  owl_message_init_fmtext_cache();
  owl_list_create(&(g->io_dispatch_list));
  owl_list_create(&(g->psa_list));
  g->timerlist = NULL;
  g->interrupted = FALSE;

  /* set up a pad for input */
  g->input_pad = newpad(1, 1);
  nodelay(g->input_pad, 1);
  keypad(g->input_pad, 1);
  meta(g->input_pad, 1);
}

/* Called once perl has been initialized */
void owl_global_complete_setup(owl_global *g)
{
  owl_cmddict_setup(&(g->cmds));
}

/* If *pan does not exist, we create a new panel, otherwise we replace the
   window in *pan with win.

   libpanel PANEL objects cannot exist without owner a valid window. This
   maintains the invariant for _owl_global_setup_windows. */
void _owl_panel_set_window(PANEL **pan, WINDOW *win)
{
  WINDOW *oldwin;

  if (win == NULL) {
    owl_function_debugmsg("_owl_panel_set_window: passed NULL win (failed to allocate?)");
    endwin();
    exit(50);
  }

  if (*pan) {
    oldwin = panel_window(*pan);
    replace_panel(*pan, win);
    delwin(oldwin);
  } else {
    *pan = new_panel(win);
  }
}

void _owl_global_setup_windows(owl_global *g) {
  int cols, typwin_lines;

  cols=g->cols;
  typwin_lines=owl_global_get_typwin_lines(g);

  /* set the new window sizes */
  g->recwinlines=g->lines-(typwin_lines+2);
  if (g->recwinlines<0) {
    /* gotta deal with this */
    g->recwinlines=0;
  }

  /* create the new windows */
  _owl_panel_set_window(&g->recpan, newwin(g->recwinlines, cols, 0, 0));
  _owl_panel_set_window(&g->seppan, newwin(1, cols, g->recwinlines, 0));
  _owl_panel_set_window(&g->msgpan, newwin(1, cols, g->recwinlines+1, 0));
  _owl_panel_set_window(&g->typpan, newwin(typwin_lines, cols, g->recwinlines+2, 0));

  if (g->tw)
      owl_editwin_set_curswin(g->tw, owl_global_get_curs_typwin(g), typwin_lines, g->cols);

  idlok(owl_global_get_curs_typwin(g), FALSE);
  idlok(owl_global_get_curs_recwin(g), FALSE);
  idlok(owl_global_get_curs_sepwin(g), FALSE);
  idlok(owl_global_get_curs_msgwin(g), FALSE);

  wmove(owl_global_get_curs_typwin(g), 0, 0);
}

owl_context *owl_global_get_context(owl_global *g) {
  if (!g->context_stack)
    return NULL;
  return g->context_stack->data;
}

static void owl_global_lookup_keymap(owl_global *g) {
  owl_context *c = owl_global_get_context(g);
  if (!c || !c->keymap)
    return;

  if (!owl_keyhandler_activate(owl_global_get_keyhandler(g), c->keymap)) {
    owl_function_error("Unable to activate keymap '%s'", c->keymap);
  }
}

void owl_global_push_context(owl_global *g, int mode, void *data, const char *keymap) {
  owl_context *c;
  if (!(mode & OWL_CTX_MODE_BITS))
    mode |= OWL_CTX_INTERACTIVE;
  c = owl_malloc(sizeof *c);
  c->mode = mode;
  c->data = data;
  c->keymap = owl_strdup(keymap);
  g->context_stack = g_list_prepend(g->context_stack, c);
  owl_global_lookup_keymap(g);
}

void owl_global_pop_context(owl_global *g) {
  owl_context *c;
  if (!g->context_stack)
    return;
  c = owl_global_get_context(g);
  g->context_stack = g_list_delete_link(g->context_stack,
                                        g->context_stack);
  owl_free(c->keymap);
  owl_free(c);
  owl_global_lookup_keymap(g);
}

int owl_global_get_lines(const owl_global *g) {
  return(g->lines);
}

int owl_global_get_cols(const owl_global *g) {
  return(g->cols);
}

int owl_global_get_recwin_lines(const owl_global *g) {
  return(g->recwinlines);
}

/* curmsg */

int owl_global_get_curmsg(const owl_global *g) {
  return(g->curmsg);
}

void owl_global_set_curmsg(owl_global *g, int i) {
  g->curmsg=i;
  /* we will reset the vertical offset from here */
  /* we might want to move this out to the functions later */
  owl_global_set_curmsg_vert_offset(g, 0);
}

/* topmsg */

int owl_global_get_topmsg(const owl_global *g) {
  return(g->topmsg);
}

void owl_global_set_topmsg(owl_global *g, int i) {
  g->topmsg=i;
}

/* markedmsgid */

int owl_global_get_markedmsgid(const owl_global *g) {
  return(g->markedmsgid);
}

void owl_global_set_markedmsgid(owl_global *g, int i) {
  g->markedmsgid=i;
  /* i; index of message in the current view.
  const owl_message *m;
  owl_view *v;

  v = owl_global_get_current_view(&g);
  m = owl_view_get_element(v, i);
  g->markedmsgid = m ? owl_message_get_id(m) : 0;
  */
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

WINDOW *owl_global_get_curs_recwin(const owl_global *g) {
  return panel_window(g->recpan);
}

WINDOW *owl_global_get_curs_sepwin(const owl_global *g) {
  return panel_window(g->seppan);
}

WINDOW *owl_global_get_curs_msgwin(const owl_global *g) {
  return panel_window(g->msgpan);
}

WINDOW *owl_global_get_curs_typwin(const owl_global *g) {
  return panel_window(g->typpan);
}

/* typwin */

owl_editwin *owl_global_get_typwin(const owl_global *g) {
  return(g->tw);
}

/* refresh */

int owl_global_is_needrefresh(const owl_global *g) {
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

int owl_global_get_rightshift(const owl_global *g) {
  return(g->rightshift);
}

/* typwin */

owl_editwin *owl_global_set_typwin_active(owl_global *g, int style, owl_history *hist) {
  int d;
  d = owl_global_get_typewindelta(g);
  if (d > 0 && style == OWL_EDITWIN_STYLE_MULTILINE)
      owl_function_resize_typwin(owl_global_get_typwin_lines(g) + d);

  g->tw = owl_editwin_new(owl_global_get_curs_typwin(g),
                          owl_global_get_typwin_lines(g),
                          g->cols,
                          style,
                          hist);
  return g->tw;
}

void owl_global_set_typwin_inactive(owl_global *g) {
  int d = owl_global_get_typewindelta(g);
  if (d > 0 && owl_editwin_get_style(g->tw) == OWL_EDITWIN_STYLE_MULTILINE)
      owl_function_resize_typwin(owl_global_get_typwin_lines(g) - d);

  werase(owl_global_get_curs_typwin(g));
  g->tw = NULL;
}

/* resize */

void owl_global_set_resize_pending(owl_global *g) {
  g->resizepending=1;
}

void owl_global_set_relayout_pending(owl_global *g) {
  g->relayoutpending = 1;
}

const char *owl_global_get_homedir(const owl_global *g) {
  if (g->homedir) return(g->homedir);
  return("/");
}

const char *owl_global_get_confdir(const owl_global *g) {
  if (g->confdir) return(g->confdir);
  return("/");
}

/*
 * Setting this also sets startupfile to confdir/startup
 */
void owl_global_set_confdir(owl_global *g, const char *cd) {
  owl_free(g->confdir);
  g->confdir = owl_strdup(cd);
  owl_free(g->startupfile);
  g->startupfile = owl_sprintf("%s/startup", cd);
}

const char *owl_global_get_startupfile(const owl_global *g) {
  if(g->startupfile) return(g->startupfile);
  return("/");
}

int owl_global_get_direction(const owl_global *g) {
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

void *owl_global_get_perlinterp(const owl_global *g) {
  return(g->perl);
}

int owl_global_is_config_format(const owl_global *g) {
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

/*
 * Compute the size of the terminal. Try a ioctl, fallback to other stuff on
 * fail.
 */
static void _owl_global_get_size(int *lines, int *cols) {
  struct winsize size;
  /* get the new size */
  ioctl(STDIN_FILENO, TIOCGWINSZ, &size);
  if (size.ws_row) {
    *lines = size.ws_row;
  } else {
    *lines = LINES;
  }

  if (size.ws_col) {
    *cols = size.ws_col;
  } else {
    *cols = COLS;
  }
}

void owl_global_resize(owl_global *g, int x, int y) {
  /* resize the screen.  If x or y is 0 use the terminal size */
  if (!g->resizepending) return;
  g->resizepending = 0;

  _owl_global_get_size(&g->lines, &g->cols);
  if (x != 0) {
    g->lines = x;
  }
  if (y != 0) {
    g->cols = y;
  }

  resizeterm(g->lines, g->cols);

  owl_function_debugmsg("New size is %i lines, %i cols.", g->lines, g->cols);
  owl_global_set_relayout_pending(g);
}

void owl_global_relayout(owl_global *g) {
  owl_popwin *pw;
  owl_viewwin *vw;

  if (!g->relayoutpending) return;
  g->relayoutpending = 0;

  owl_function_debugmsg("Relayouting...");

  /* re-initialize the windows */
  _owl_global_setup_windows(g);

  /* in case any styles rely on the current width */
  owl_messagelist_invalidate_formats(owl_global_get_msglist(g));

  /* recalculate the topmsg to make sure the current message is on
   * screen */
  owl_function_calculate_topmsg(OWL_DIRECTION_NONE);

  /* recreate the popwin */
  pw = owl_global_get_popwin(g);
  if (owl_popwin_is_active(pw)) {
    /*
     * This is somewhat hacky; we probably want a proper windowing layer. We
     * destroy the popwin and recreate it. Then the viewwin is redirected to
     * the new window.
     */
    vw = owl_global_get_viewwin(g);
    owl_popwin_close(pw);
    owl_popwin_up(pw);
    owl_viewwin_set_curswin(vw, owl_popwin_get_curswin(pw),
	owl_popwin_get_lines(pw), owl_popwin_get_cols(pw));
    owl_viewwin_redisplay(vw);
  }

  /* refresh stuff */
  g->needrefresh=1;
  owl_mainwin_redisplay(&(g->mw));
  sepbar(NULL);
  if (g->tw)
      owl_editwin_redisplay(g->tw);
  else
    werase(owl_global_get_curs_typwin(g));

  owl_function_full_redisplay();

  owl_function_makemsg("");
}

/* debug */

int owl_global_is_debug_fast(const owl_global *g) {
  if (g->debug) return(1);
  return(0);
}

/* starttime */

time_t owl_global_get_starttime(const owl_global *g) {
  return(g->starttime);
}

time_t owl_global_get_runtime(const owl_global *g) {
  return(time(NULL)-g->starttime);
}

time_t owl_global_get_lastinputtime(const owl_global *g) {
  return(g->lastinputtime);
}

void owl_global_set_lastinputtime(owl_global *g, time_t time) {
  g->lastinputtime = time;
}

time_t owl_global_get_idletime(const owl_global *g) {
  return(time(NULL)-g->lastinputtime);
}

const char *owl_global_get_hostname(const owl_global *g) {
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

int owl_global_get_userclue(const owl_global *g) {
  return(g->userclue);
}

int owl_global_is_userclue(const owl_global *g, int clue) {
  if (g->userclue & clue) return(1);
  return(0);
}

/* viewwin */

owl_viewwin *owl_global_get_viewwin(owl_global *g) {
  return(&(g->vw));
}


/* vert offset */

int owl_global_get_curmsg_vert_offset(const owl_global *g) {
  return(g->curmsg_vert_offset);
}

void owl_global_set_curmsg_vert_offset(owl_global *g, int i) {
  g->curmsg_vert_offset=i;
}

/* startup args */

void owl_global_set_startupargs(owl_global *g, int argc, const char *const *argv) {
  int i, len;

  if (g->startupargs) owl_free(g->startupargs);
  
  len=0;
  for (i=0; i<argc; i++) {
    len+=strlen(argv[i])+5;
  }
  g->startupargs=owl_malloc(len+5);

  strcpy(g->startupargs, "");
  for (i=0; i<argc; i++) {
    sprintf(g->startupargs + strlen(g->startupargs), "%s ", argv[i]);
  }
  g->startupargs[strlen(g->startupargs)-1]='\0';
}

const char *owl_global_get_startupargs(const owl_global *g) {
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
typedef struct _owl_global_filter_ent {         /* noproto */
  owl_global *g;
  owl_filter *f;
} owl_global_filter_ent;

owl_filter *owl_global_get_filter(const owl_global *g, const char *name) {
  owl_global_filter_ent *e = owl_dict_find_element(&(g->filters), name);
  if (e) return e->f;
  return NULL;
}

static void owl_global_delete_filter_ent(void *data)
{
  owl_global_filter_ent *e = data;
  e->g->filterlist = g_list_remove(e->g->filterlist, e->f);
  owl_filter_delete(e->f);
  owl_free(e);
}

void owl_global_add_filter(owl_global *g, owl_filter *f) {
  owl_global_filter_ent *e = owl_malloc(sizeof *e);
  e->g = g;
  e->f = f;

  owl_dict_insert_element(&(g->filters), owl_filter_get_name(f),
                          e, owl_global_delete_filter_ent);
  g->filterlist = g_list_append(g->filterlist, f);
}

void owl_global_remove_filter(owl_global *g, const char *name) {
  owl_global_filter_ent *e = owl_dict_remove_element(&(g->filters), name);
  if (e)
    owl_global_delete_filter_ent(e);
}

/* nextmsgid */

int owl_global_get_nextmsgid(owl_global *g) {
  return(g->nextmsgid++);
}

/* current view */

owl_view *owl_global_get_current_view(owl_global *g) {
  return(&(g->current_view));
}

/* has colors */

int owl_global_get_hascolors(const owl_global *g) {
  if (g->hascolors) return(1);
  return(0);
}

/* color pairs */

int owl_global_get_colorpairs(const owl_global *g) {
  return(g->colorpairs);
}

owl_colorpair_mgr *owl_global_get_colorpair_mgr(owl_global *g) {
  return(&(g->cpmgr));
}

/* puntlist */

owl_list *owl_global_get_puntlist(owl_global *g) {
  return(&(g->puntlist));
}

int owl_global_message_is_puntable(owl_global *g, const owl_message *m) {
  const owl_list *pl;
  int i, j;

  pl=owl_global_get_puntlist(g);
  j=owl_list_get_size(pl);
  for (i=0; i<j; i++) {
    if (owl_filter_message_match(owl_list_get_element(pl, i), m)) return(1);
  }
  return(0);
}

int owl_global_should_followlast(owl_global *g) {
  const owl_view *v;
  
  if (!owl_global_is__followlast(g)) return(0);
  
  v=owl_global_get_current_view(g);
  
  if (owl_global_get_curmsg(g)==owl_view_get_size(v)-1) return(1);
  return(0);
}

int owl_global_is_search_active(const owl_global *g) {
  if (owl_regex_is_set(&g->search_re)) return(1);
  return(0);
}

void owl_global_set_search_re(owl_global *g, const owl_regex *re) {
  if (owl_regex_is_set(&g->search_re)) {
    owl_regex_cleanup(&g->search_re);
    owl_regex_init(&g->search_re);
  }
  if (re != NULL)
    owl_regex_copy(re, &g->search_re);
}

const owl_regex *owl_global_get_search_re(const owl_global *g) {
  return &g->search_re;
}

void owl_global_set_newmsgproc_pid(owl_global *g, pid_t i) {
  g->newmsgproc_pid=i;
}

pid_t owl_global_get_newmsgproc_pid(const owl_global *g) {
  return(g->newmsgproc_pid);
}

/* AIM stuff */

int owl_global_is_aimloggedin(const owl_global *g)
{
  if (g->aim_loggedin) return(1);
  return(0);
}

const char *owl_global_get_aim_screenname(const owl_global *g)
{
  if (owl_global_is_aimloggedin(g)) {
    return (g->aim_screenname);
  }
  return("");
}

const char *owl_global_get_aim_screenname_for_filters(const owl_global *g)
{
  if (owl_global_is_aimloggedin(g)) {
    return (g->aim_screenname_for_filters);
  }
  return("");
}

void owl_global_set_aimloggedin(owl_global *g, const char *screenname)
{
  char *sn_escaped;
  const char *quote;
  g->aim_loggedin=1;
  if (g->aim_screenname) owl_free(g->aim_screenname);
  if (g->aim_screenname_for_filters) owl_free(g->aim_screenname_for_filters);
  g->aim_screenname=owl_strdup(screenname);
  sn_escaped = owl_text_quote(screenname, OWL_REGEX_QUOTECHARS, OWL_REGEX_QUOTEWITH);
  quote = owl_getquoting(sn_escaped);
  g->aim_screenname_for_filters=owl_sprintf("%s%s%s", quote, sn_escaped, quote);
  owl_free(sn_escaped);
}

void owl_global_set_aimnologgedin(owl_global *g)
{
  g->aim_loggedin=0;
}

int owl_global_is_doaimevents(const owl_global *g)
{
  if (g->aim_doprocessing) return(1);
  return(0);
}

void owl_global_set_doaimevents(owl_global *g)
{
  g->aim_doprocessing=1;
}

void owl_global_set_no_doaimevents(owl_global *g)
{
  g->aim_doprocessing=0;
}

aim_session_t *owl_global_get_aimsess(owl_global *g)
{
  return(&(g->aimsess));
}

aim_conn_t *owl_global_get_bosconn(owl_global *g)
{
  return(&(g->bosconn));
}

void owl_global_set_bossconn(owl_global *g, aim_conn_t *conn)
{
  g->bosconn=*conn;
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
owl_message *owl_global_messagequeue_popmsg(owl_global *g)
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
const owl_style *owl_global_get_style_by_name(const owl_global *g, const char *name)
{
  return owl_dict_find_element(&(g->styledict), name);
}

/* creates a list and fills it in with keys.  duplicates the keys, 
 * so they will need to be freed by the caller. */
int owl_global_get_style_names(const owl_global *g, owl_list *l) {
  return owl_dict_get_keys(&(g->styledict), l);
}

void owl_global_add_style(owl_global *g, owl_style *s)
{
  /*
   * If we're redefining the current style, make sure to update
   * pointers to it.
   */
  if(g->current_view.style
     && !strcmp(owl_style_get_name(g->current_view.style),
                owl_style_get_name(s)))
    g->current_view.style = s;
  owl_dict_insert_element(&(g->styledict), owl_style_get_name(s),
                          s, (void (*)(void *))owl_style_delete);
}

void owl_global_set_haveaim(owl_global *g)
{
  g->haveaim=1;
}

int owl_global_is_haveaim(const owl_global *g)
{
  if (g->haveaim) return(1);
  return(0);
}

void owl_global_set_ignore_aimlogin(owl_global *g)
{
    g->ignoreaimlogin = 1;
}

void owl_global_unset_ignore_aimlogin(owl_global *g)
{
    g->ignoreaimlogin = 0;
}

int owl_global_is_ignore_aimlogin(const owl_global *g)
{
    return g->ignoreaimlogin;
}

void owl_global_set_havezephyr(owl_global *g)
{
  g->havezephyr=1;
}

int owl_global_is_havezephyr(const owl_global *g)
{
  if (g->havezephyr) return(1);
  return(0);
}

owl_errqueue *owl_global_get_errqueue(owl_global *g)
{
  return(&(g->errqueue));
}

void owl_global_set_errsignal(owl_global *g, int signum, siginfo_t *siginfo)
{
  g->got_err_signal = signum;
  if (siginfo) {
    g->err_signal_info = *siginfo;
  } else {
    siginfo_t si;
    memset(&si, 0, sizeof(si));
    g->err_signal_info = si;
  }
}

int owl_global_get_errsignal_and_clear(owl_global *g, siginfo_t *siginfo)
{
  int signum;
  if (siginfo && g->got_err_signal) {
    *siginfo = g->err_signal_info;
  } 
  signum = g->got_err_signal;
  g->got_err_signal = 0;
  return signum;
}


owl_zbuddylist *owl_global_get_zephyr_buddylist(owl_global *g)
{
  return(&(g->zbuddies));
}

GList **owl_global_get_zaldlist(owl_global *g)
{
  return &(g->zaldlist);
}

int owl_global_get_pseudologin_notify(owl_global *g)
{
  return g->pseudologin_notify;
}

void owl_global_set_pseudologin_notify(owl_global *g, int notify)
{
  g->pseudologin_notify = notify;
}

struct termios *owl_global_get_startup_tio(owl_global *g)
{
  return(&(g->startup_tio));
}

const char * owl_global_intern(owl_global *g, const char * string)
{
  return owl_obarray_insert(&(g->obarray), string);
}

owl_list *owl_global_get_io_dispatch_list(owl_global *g)
{
  return &(g->io_dispatch_list);
}

owl_list *owl_global_get_psa_list(owl_global *g)
{
  return &(g->psa_list);
}

GList **owl_global_get_timerlist(owl_global *g)
{
  return &(g->timerlist);
}

int owl_global_is_interrupted(const owl_global *g) {
  return g->interrupted;
}

void owl_global_set_interrupted(owl_global *g) {
  g->interrupted = 1;
}

void owl_global_unset_interrupted(owl_global *g) {
  g->interrupted = 0;
}

void owl_global_setup_default_filters(owl_global *g)
{
  int i;
  static const struct {
    const char *name;
    const char *desc;
  } filters[] = {
    { "personal",
      "private ^true$ and ( not type ^zephyr$ or "
      "( class ^message and ( instance ^personal$ or instance ^urgent$ ) ) )" },
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
    owl_global_add_filter(g, owl_filter_new_fromstring(filters[i].name,
                                                       filters[i].desc));
}

FILE *owl_global_get_debug_file_handle(owl_global *g) {
  static char *open_file = NULL;
  const char *filename = owl_global_get_debug_file(g);
  if (g->debug_file == NULL ||
      (open_file && strcmp(filename, open_file) != 0)) {
    if (g->debug_file)
      fclose(g->debug_file);
    g->debug_file = fopen(filename, "a");

    owl_free(open_file);
    open_file = owl_strdup(filename);
  }
  return g->debug_file;
}
