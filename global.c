#include "owl.h"
#include <stdio.h>
#include <sys/ioctl.h>

static void _owl_global_init_windows(owl_global *g);

void owl_global_init(owl_global *g) {
  char *cd;
  const char *homedir;

#if !GLIB_CHECK_VERSION(2, 35, 0)
  g_type_init();
#endif
#if !GLIB_CHECK_VERSION(2, 31, 0)
  g_thread_init(NULL);
#endif

  owl_select_init();

  g->lines=LINES;
  g->cols=COLS;
  /* We shouldn't need this if we initialize lines and cols before the first
   * owl_window_get_screen, but to be safe, we synchronize. */
  owl_window_resize(owl_window_get_screen(), g->lines, g->cols);

  g->context_stack = NULL;
  owl_global_push_context(g, OWL_CTX_STARTUP, NULL, NULL, NULL);

  g->curmsg=0;
  g->topmsg=0;
  g->markedmsgid=-1;
  g->startupargs=NULL;

  owl_variable_dict_setup(&(g->vars));

  g->rightshift=0;

  g->pw = NULL;
  g->vw = NULL;
  g->tw = NULL;

  owl_keyhandler_init(&g->kh);
  owl_keys_setup_keymaps(&g->kh);

  owl_dict_create(&(g->filters));
  g->filterlist = NULL;
  g->puntlist = g_ptr_array_new();
  g->messagequeue = g_queue_new();
  owl_dict_create(&(g->styledict));
  g->curmsg_vert_offset=0;
  g->resizepending=0;
  g->direction=OWL_DIRECTION_DOWNWARDS;
  owl_fmtext_init_colorpair_mgr(&(g->cpmgr));
  g->debug=OWL_DEBUG;
  owl_regex_init(&g->search_re);
  g->starttime=time(NULL); /* assumes we call init only a start time */
  g->lastinputtime=g->starttime;
  g->last_wakeup_time = g->starttime;
  g->newmsgproc_pid=0;
  
  owl_global_set_config_format(g, 0);
  owl_global_set_no_have_config(g);
  owl_history_init(&(g->msghist));
  owl_history_init(&(g->cmdhist));
  g->nextmsgid=0;

  /* Fill in some variables which don't have constant defaults */

  /* glib's g_get_home_dir prefers passwd entries to $HOME, so we
   * explicitly check getenv first. */
  homedir = getenv("HOME");
  if (!homedir)
    homedir = g_get_home_dir();
  g->homedir = g_strdup(homedir);

  g->confdir = NULL;
  g->startupfile = NULL;
  cd = g_build_filename(g->homedir, OWL_CONFIG_DIR, NULL);
  owl_global_set_confdir(g, cd);
  g_free(cd);

  g->msglist = owl_messagelist_new();

  _owl_global_init_windows(g);

  g->havezephyr=0;

  owl_errqueue_init(&(g->errqueue));

  owl_zbuddylist_create(&(g->zbuddies));

  g->zaldlist = NULL;
  g->pseudologin_notify = 0;

  owl_message_init_fmtext_cache();
  g->kill_buffer = NULL;

  g->interrupt_count = 0;
#if GLIB_CHECK_VERSION(2, 31, 0)
  g_mutex_init(&g->interrupt_lock);
#else
  g->interrupt_lock = g_mutex_new();
#endif
}

static void _owl_global_init_windows(owl_global *g)
{
  /* Create the main window */
  owl_mainpanel_init(&(g->mainpanel));

  /* Create the widgets */
  g->mw = owl_mainwin_new(g->mainpanel.recwin);
  owl_msgwin_init(&(g->msgwin), g->mainpanel.msgwin);
  owl_sepbar_init(g->mainpanel.sepwin);

  owl_window_set_default_cursor(g->mainpanel.sepwin);

  /* set up a pad for input */
  g->input_pad = newpad(1, 1);
  nodelay(g->input_pad, 1);
  keypad(g->input_pad, 1);
  meta(g->input_pad, 1);
}

void owl_global_sepbar_dirty(owl_global *g)
{
  owl_window_dirty(g->mainpanel.sepwin);
}

/* Called once perl has been initialized */
void owl_global_complete_setup(owl_global *g)
{
  owl_cmddict_setup(&(g->cmds));
}

owl_context *owl_global_get_context(const owl_global *g) {
  if (!g->context_stack)
    return NULL;
  return g->context_stack->data;
}

static void owl_global_activate_context(owl_global *g, owl_context *c) {
  if (!c)
    return;

  if (c->keymap) {
    if (!owl_keyhandler_activate(owl_global_get_keyhandler(g), c->keymap)) {
      owl_function_error("Unable to activate keymap '%s'", c->keymap);
    }
  }
  owl_window_set_cursor(c->cursor);
}

void owl_global_push_context(owl_global *g, int mode, void *data, const char *keymap, owl_window *cursor) {
  owl_context *c;
  c = owl_context_new(mode, data, keymap, cursor);
  owl_global_push_context_obj(g, c);
}

void owl_global_push_context_obj(owl_global *g, owl_context *c)
{
  g->context_stack = g_list_prepend(g->context_stack, c);
  owl_global_activate_context(g, owl_global_get_context(g));
}

/* Pops the current context from the context stack and returns it. Caller is
 * responsible for freeing. */
CALLER_OWN owl_context *owl_global_pop_context_no_delete(owl_global *g)
{
  owl_context *c;
  if (!g->context_stack)
    return NULL;
  c = owl_global_get_context(g);
  owl_context_deactivate(c);
  g->context_stack = g_list_delete_link(g->context_stack,
                                        g->context_stack);
  owl_global_activate_context(g, owl_global_get_context(g));
  return c;
}

/* Pops the current context from the context stack and deletes it. */
void owl_global_pop_context(owl_global *g) {
  owl_context *c;
  c = owl_global_pop_context_no_delete(g);
  if (c)
    owl_context_delete(c);
}

int owl_global_get_lines(const owl_global *g) {
  return(g->lines);
}

int owl_global_get_cols(const owl_global *g) {
  return(g->cols);
}

int owl_global_get_recwin_lines(const owl_global *g) {
  return g->mainpanel.recwinlines;
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

owl_mainwin *owl_global_get_mainwin(owl_global *g)
{
  return g->mw;
}

owl_popwin *owl_global_get_popwin(owl_global *g) {
  return g->pw;
}

void owl_global_set_popwin(owl_global *g, owl_popwin *pw) {
  g->pw = pw;
}

/* msglist */

owl_messagelist *owl_global_get_msglist(owl_global *g) {
  return g->msglist;
}

/* keyhandler */

owl_keyhandler *owl_global_get_keyhandler(owl_global *g) {
  return(&(g->kh));
}

/* Gets the currently active typwin out of the current context. */
owl_editwin *owl_global_current_typwin(const owl_global *g) {
  owl_context *ctx = owl_global_get_context(g);
  return owl_editcontext_get_editwin(ctx);
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
  g->rightshift = i;
  owl_mainwin_redisplay(owl_global_get_mainwin(g));
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

  if (g->typwin_erase_id) {
    g_signal_handler_disconnect(g->mainpanel.typwin, g->typwin_erase_id);
    g->typwin_erase_id = 0;
  }

  g->tw = owl_editwin_new(g->mainpanel.typwin,
                          owl_global_get_typwin_lines(g),
                          g->cols,
                          style,
                          hist);
  return g->tw;
}

void owl_global_deactivate_editcontext(owl_context *ctx) {
  owl_global *g = ctx->cbdata;
  owl_global_set_typwin_inactive(g);
}

void owl_global_set_typwin_inactive(owl_global *g) {
  int d = owl_global_get_typewindelta(g);
  if (d > 0 && owl_editwin_get_style(g->tw) == OWL_EDITWIN_STYLE_MULTILINE)
      owl_function_resize_typwin(owl_global_get_typwin_lines(g) - d);

  if (!g->typwin_erase_id) {
    g->typwin_erase_id =
      g_signal_connect(g->mainpanel.typwin, "redraw", G_CALLBACK(owl_window_erase_cb), NULL);
  }
  owl_window_dirty(g->mainpanel.typwin);

  owl_editwin_unref(g->tw);
  g->tw = NULL;
}

/* resize */

void owl_global_set_resize_pending(owl_global *g) {
  g->resizepending = true;
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
  g_free(g->confdir);
  g->confdir = g_strdup(cd);
  g_free(g->startupfile);
  g->startupfile = g_build_filename(cd, "startup", NULL);
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
void owl_global_get_terminal_size(int *lines, int *cols) {
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

void owl_global_check_resize(owl_global *g) {
  /* resize the screen.  If lines or cols is 0 use the terminal size */
  if (!g->resizepending) return;
  g->resizepending = false;

  owl_global_get_terminal_size(&g->lines, &g->cols);
  owl_window_resize(owl_window_get_screen(), g->lines, g->cols);

  owl_function_debugmsg("New size is %i lines, %i cols.", g->lines, g->cols);
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

void owl_global_wakeup(owl_global *g)
{
  if (time(NULL) - g->last_wakeup_time >= 1) {
    g_free(owl_perlconfig_execute("BarnOwl::Hooks::_wakeup()"));
    g->last_wakeup_time = time(NULL);
  }
}

/* viewwin */

owl_viewwin *owl_global_get_viewwin(owl_global *g) {
  return g->vw;
}

void owl_global_set_viewwin(owl_global *g, owl_viewwin *vw) {
  g->vw = vw;
}


/* vert offset */

int owl_global_get_curmsg_vert_offset(const owl_global *g) {
  return(g->curmsg_vert_offset);
}

void owl_global_set_curmsg_vert_offset(owl_global *g, int i) {
  g->curmsg_vert_offset = i;
}

/* startup args */

void owl_global_set_startupargs(owl_global *g, int argc, char **argv) {
  g_free(g->startupargs);
  g->startupargs = g_strjoinv(" ", argv);
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
  g_slice_free(owl_global_filter_ent, e);
}

void owl_global_add_filter(owl_global *g, owl_filter *f) {
  owl_global_filter_ent *e = g_slice_new(owl_global_filter_ent);
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

owl_colorpair_mgr *owl_global_get_colorpair_mgr(owl_global *g) {
  return(&(g->cpmgr));
}

/* puntlist */

GPtrArray *owl_global_get_puntlist(owl_global *g) {
  return g->puntlist;
}

int owl_global_message_is_puntable(owl_global *g, const owl_message *m) {
  const GPtrArray *pl;
  int i;

  pl = owl_global_get_puntlist(g);
  for (i = 0; i < pl->len; i++) {
    if (owl_filter_message_match(pl->pdata[i], m)) return 1;
  }
  return 0;
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
  /* TODO: Emit a signal so we don't depend on the viewwin and mainwin */
  if (owl_global_get_viewwin(g))
    owl_viewwin_dirty(owl_global_get_viewwin(g));
  owl_mainwin_redisplay(owl_global_get_mainwin(g));
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

/* message queue */

void owl_global_messagequeue_addmsg(owl_global *g, owl_message *m)
{
  g_queue_push_tail(g->messagequeue, m);
}

/* pop off the first message and return it.  Return NULL if the queue
 * is empty.  The caller should free the message after using it, if
 * necessary.
 */
CALLER_OWN owl_message *owl_global_messagequeue_popmsg(owl_global *g)
{
  owl_message *out;

  if (g_queue_is_empty(g->messagequeue))
    return NULL;
  out = g_queue_pop_head(g->messagequeue);
  return out;
}

int owl_global_messagequeue_pending(owl_global *g)
{
  return !g_queue_is_empty(g->messagequeue);
}

/* style */

/* Return the style with name 'name'.  If it does not exist return
 * NULL */
const owl_style *owl_global_get_style_by_name(const owl_global *g, const char *name)
{
  return owl_dict_find_element(&(g->styledict), name);
}

CALLER_OWN GPtrArray *owl_global_get_style_names(const owl_global *g)
{
  return owl_dict_get_keys(&g->styledict);
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

void owl_global_setup_default_filters(owl_global *g)
{
  int i;
  static const struct {
    const char *name;
    const char *desc;
  } filters[] = {
    { "personal",
      "isprivate ^true$ and ( not type ^zephyr$ or ( class ^message$ ) )" },
    { "trash",
      "class ^mail$ or opcode ^ping$ or type ^admin$ or ( not login ^none$ )" },
    { "wordwrap", "not ( type ^admin$ or type ^zephyr$ )" },
    { "ping", "opcode ^ping$" },
    { "auto", "opcode ^auto$" },
    { "login", "not login ^none$" },
    { "reply-lockout", "class ^mail$ or class ^filsrv$" },
    { "out", "direction ^out$" },
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
    char *path;
    int fd;

    if (g->debug_file)
      fclose(g->debug_file);

    g->debug_file = NULL;

    path = g_strdup_printf("%s.%d", filename, getpid());
    fd = open(path, O_CREAT|O_WRONLY|O_EXCL, 0600);
    g_free(path);

    if (fd >= 0)
      g->debug_file = fdopen(fd, "a");

    g_free(open_file);
    open_file = g_strdup(filename);
  }
  return g->debug_file;
}

const char *owl_global_get_kill_buffer(owl_global *g) {
  return g->kill_buffer;
}

void owl_global_set_kill_buffer(owl_global *g, const char *kill, int len) {
  g_free(g->kill_buffer);
  g->kill_buffer = g_strndup(kill, len);
}

static GMutex *owl_global_get_interrupt_lock(owl_global *g)
{
#if GLIB_CHECK_VERSION(2, 31, 0)
  return &g->interrupt_lock;
#else
  return g->interrupt_lock;
#endif
}

void owl_global_add_interrupt(owl_global *g) {
  /* TODO: This can almost certainly be done with atomic
   * operations. Whatever. */
  g_mutex_lock(owl_global_get_interrupt_lock(g));
  g->interrupt_count++;
  g_mutex_unlock(owl_global_get_interrupt_lock(g));
}

bool owl_global_take_interrupt(owl_global *g) {
  bool ans = false;
  g_mutex_lock(owl_global_get_interrupt_lock(g));
  if (g->interrupt_count > 0) {
    ans = true;
    g->interrupt_count--;
  }
  g_mutex_unlock(owl_global_get_interrupt_lock(g));
  return ans;
}
