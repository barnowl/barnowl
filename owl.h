/*  Copyright (c) 2006-2011 The BarnOwl Developers. All rights reserved.
 *  Copyright (c) 2004 James Kretchmar. All rights reserved.
 *
 *  This program is free software. You can redistribute it and/or
 *  modify under the terms of the Sleepycat License. See the COPYING
 *  file included with the distribution for more information.
 */

#ifndef INC_BARNOWL_OWL_H
#define INC_BARNOWL_OWL_H

#include <config.h>

#include "compat/compat.h"

#include <stdbool.h>

#ifndef OWL_PERL
#define NCURSES_ENABLE_STDBOOL_H 1
#include <ncursesw/curses.h>
#include <ncursesw/panel.h>
#endif
#include <sys/param.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <EXTERN.h>
#include <fcntl.h>
#include <netdb.h>
#include <regex.h>
#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>
#include <glib.h>
#ifdef HAVE_LIBZEPHYR
#include <zephyr/zephyr.h>
#endif
#ifdef HAVE_COM_ERR_H
#include <com_err.h>
#endif

/* Perl and curses don't play nice. */
#ifdef OWL_PERL
typedef struct _owl_fake_WINDOW WINDOW;
typedef struct _owl_fake_PANEL PANEL;
/* logout is defined in FreeBSD. */
#define logout logout_
#define HAS_BOOL
#include <perl.h>
#include "owl_perl.h"
#undef logout
#include "XSUB.h"
#else
typedef struct _owl_fake_SV SV;
typedef struct _owl_fake_AV AV;
typedef struct _owl_fake_HV HV;
#endif

#include "window.h"

extern const char *version;

/* Feature that is being tested to redirect stderr through a pipe. 
 * There may still be some portability problems with this. */
#define OWL_STDERR_REDIR 1

#define OWL_DEBUG 0
#define OWL_DEBUG_FILE "/var/tmp/barnowl-debug"

#define OWL_CONFIG_DIR "/.owl"             /* this is relative to the user's home directory */

#define OWL_FMTEXT_ATTR_NONE      0
#define OWL_FMTEXT_ATTR_BOLD      1
#define OWL_FMTEXT_ATTR_REVERSE   2
#define OWL_FMTEXT_ATTR_UNDERLINE 4

#define OWL_FMTEXT_UC_BASE 0x100000 /* Unicode Plane 16 - Supplementary Private Use Area-B*/
#define OWL_FMTEXT_UC_ATTR ( OWL_FMTEXT_UC_BASE | 0x800 )
#define OWL_FMTEXT_UC_ATTR_MASK 0x7
#define OWL_FMTEXT_UC_COLOR_BASE ( OWL_FMTEXT_UC_BASE | 0x400 )
#define OWL_FMTEXT_UC_FGCOLOR OWL_FMTEXT_UC_COLOR_BASE
#define OWL_FMTEXT_UC_BGCOLOR ( OWL_FMTEXT_UC_COLOR_BASE | 0x200 )
#define OWL_FMTEXT_UC_DEFAULT_COLOR 0x100
#define OWL_FMTEXT_UC_FGDEFAULT ( OWL_FMTEXT_UC_FGCOLOR | OWL_FMTEXT_UC_DEFAULT_COLOR )
#define OWL_FMTEXT_UC_BGDEFAULT ( OWL_FMTEXT_UC_BGCOLOR | OWL_FMTEXT_UC_DEFAULT_COLOR )
#define OWL_FMTEXT_UC_COLOR_MASK 0xFF
#define OWL_FMTEXT_UC_ALLCOLOR_MASK ( OWL_FMTEXT_UC_COLOR_MASK | OWL_FMTEXT_UC_DEFAULT_COLOR | 0x200)
#define OWL_FMTEXT_UC_STARTBYTE_UTF8 '\xf4'

#define OWL_COLOR_BLACK     0
#define OWL_COLOR_RED       1
#define OWL_COLOR_GREEN     2
#define OWL_COLOR_YELLOW    3
#define OWL_COLOR_BLUE      4
#define OWL_COLOR_MAGENTA   5
#define OWL_COLOR_CYAN      6
#define OWL_COLOR_WHITE     7
#define OWL_COLOR_DEFAULT   -1
#define OWL_COLOR_INVALID   -2

#define OWL_TAB_WIDTH 8

#define OWL_EDITWIN_STYLE_MULTILINE 0
#define OWL_EDITWIN_STYLE_ONELINE   1

#define OWL_MESSAGE_DIRECTION_NONE  0
#define OWL_MESSAGE_DIRECTION_IN    1
#define OWL_MESSAGE_DIRECTION_OUT   2

#define OWL_DIRECTION_NONE      0
#define OWL_DIRECTION_DOWNWARDS 1
#define OWL_DIRECTION_UPWARDS   2

#define OWL_LOGGING_DIRECTION_BOTH 0
#define OWL_LOGGING_DIRECTION_IN   1
#define OWL_LOGGING_DIRECTION_OUT  2

#define OWL_SCROLLMODE_NORMAL      0
#define OWL_SCROLLMODE_TOP         1
#define OWL_SCROLLMODE_NEARTOP     2
#define OWL_SCROLLMODE_CENTER      3
#define OWL_SCROLLMODE_PAGED       4
#define OWL_SCROLLMODE_PAGEDCENTER 5

#define OWL_TAB               3  /* This *HAS* to be the size of TABSTR below */
#define OWL_TABSTR        "   "
#define OWL_MSGTAB            7
#define OWL_TYPWIN_SIZE       8
#define OWL_HISTORYSIZE       50

/* Indicate current state, as well as what is allowed */
#define OWL_CTX_ANY          0xffff
/* Only one of these may be active at a time... */
#define OWL_CTX_MODE_BITS    0x000f
#define OWL_CTX_STARTUP      0x0001
#define OWL_CTX_READCONFIG   0x0002
#define OWL_CTX_INTERACTIVE  0x0004
/* Only one of these may be active at a time... */
#define OWL_CTX_ACTIVE_BITS  0xfff0
#define OWL_CTX_POPWIN       0x00f0
#define OWL_CTX_POPLESS      0x0010
#define OWL_CTX_RECWIN       0x0f00
#define OWL_CTX_RECV         0x0100
#define OWL_CTX_TYPWIN       0xf000
#define OWL_CTX_EDIT         0x7000
#define OWL_CTX_EDITLINE     0x1000
#define OWL_CTX_EDITMULTI    0x2000
#define OWL_CTX_EDITRESPONSE 0x4000

#define OWL_VARIABLE_OTHER      0
#define OWL_VARIABLE_INT        1
#define OWL_VARIABLE_BOOL       2
#define OWL_VARIABLE_STRING     3

#define OWL_OUTPUT_RETURN       0
#define OWL_OUTPUT_POPUP        1
#define OWL_OUTPUT_ADMINMSG     2

#define OWL_FILTER_MAX_DEPTH    300

#define OWL_KEYMAP_MAXSTACK     20

#define OWL_KEYBINDING_NOOP     0   /* dummy binding that does nothing */
#define OWL_KEYBINDING_COMMAND  1   /* command string */
#define OWL_KEYBINDING_FUNCTION 2   /* function taking no args */

#define OWL_DEFAULT_ZAWAYMSG    "I'm sorry, but I am currently away from the terminal and am\nnot able to receive your message.\n"

#define OWL_CMD_ALIAS_SUMMARY_PREFIX "command alias to: "

#define OWL_WEBZEPHYR_PRINCIPAL "daemon/webzephyr.mit.edu"
#define OWL_WEBZEPHYR_CLASS     "webzephyr"
#define OWL_WEBZEPHYR_OPCODE    "webzephyr"

#define OWL_ZEPHYR_NOSTRIP_HOST         "host/"
#define OWL_ZEPHYR_NOSTRIP_RCMD         "rcmd."
#define OWL_ZEPHYR_NOSTRIP_DAEMON5      "daemon/"
#define OWL_ZEPHYR_NOSTRIP_DAEMON4      "daemon."

#define OWL_REGEX_QUOTECHARS    "!+*.?[]^\\${}()|"
#define OWL_REGEX_QUOTEWITH     "\\"

#if defined(HAVE_DES_STRING_TO_KEY) && defined(HAVE_DES_KEY_SCHED) && defined(HAVE_DES_ECB_ENCRYPT)
#define OWL_ENABLE_ZCRYPT 1
#endif

/* Annotate functions in which the caller owns the return value and is
 * responsible for ensuring it is freed. */
#define CALLER_OWN G_GNUC_WARN_UNUSED_RESULT

#define OWL_META(key) ((key)|010000)
/* OWL_CTRL is definied in kepress.c */

#ifdef HAVE_LIBZEPHYR
/* libzephyr doesn't use const, so we appease the type system with this kludge.
 * This just casts const char * to char * in a way that doesn't yield a warning
 * from gcc -Wcast-qual. */
static inline char *zstr(const char *str)
{
  union { char *rw; const char *ro; } u;
  u.ro = str;
  return u.rw;
}
#endif

/* Convert char *const * into const char *const *.  This conversion is safe,
 * and implicit in C++ (conv.qual 4) but for some reason not in C. */
static inline const char *const *strs(char *const *pstr)
{
  return (const char *const *)pstr;
}

typedef struct _owl_variable {
  char *name;
  int   type;  /* OWL_VARIABLE_* */
  char *default_str;            /* the default value as a string */
  char *validsettings;	        /* documentation of valid settings */
  char *summary;		/* summary of usage */
  char *description;		/* detailed description */
  bool takes_on_off;            /* allow passing on/off in argument-less set/unset */
  GClosure *set_fromstring_fn;
                                /* sets the variable to a value
                                 * of the appropriate type.
                                 * unless documented, this
                                 * should make a copy.
                                 * returns 0 on success. */
  GClosure *get_tostring_fn;
                                /* converts val to a string;
                                 * caller must free the result */

  /* These are only valid for OWL_VARIABLE_{INT,BOOL,STRING} */
  GValue val;                   /* current value, if default get_fn/set_fn */

  GCallback get_fn;
                                /* returns a reference to the current value.
                                 * WARNING:  this approach is hard to make
                                 * thread-safe... */
  GCallback validate_fn;
                                /* returns 1 if newval is valid */
  GCallback set_fn;
                                /* sets the variable to a value
                                 * of the appropriate type.
                                 * unless documented, this
                                 * should make a copy.
                                 * returns 0 on success. */
} owl_variable;


typedef struct _owl_input {
  int ch;
  gunichar uch;
} owl_input;

typedef struct _owl_fmtext {
  GString *buff;
} owl_fmtext;

typedef struct _owl_dict_el {
  char *k;			/* key   */
  void *v;			/* value */
} owl_dict_el;

typedef struct _owl_dict {
  int size;
  int avail;
  owl_dict_el *els;		/* invariant: sorted by k */
} owl_dict;
typedef owl_dict owl_vardict;	/* dict of variables */
typedef owl_dict owl_cmddict;	/* dict of commands */

typedef struct _owl_context {
  int   mode;
  void *data;		/* determined by mode */
  char *keymap;
  owl_window *cursor;
  void (*deactivate_cb)(struct _owl_context*);
  void (*delete_cb)(struct _owl_context*);
  void *cbdata;
} owl_context;

typedef struct _owl_cmd {	/* command */
  char *name;

  char *summary;		/* one line summary of command */
  char *usage;			/* usage synopsis */
  char *description;		/* long description of command */

  int validctx;			/* bitmask of valid contexts */

  /* we should probably have a type here that says which of
   * the following is valid, and maybe make the below into a union... */

  /* Only one of these may be non-NULL ... */

  char *cmd_aliased_to;		/* what this command is aliased to... */
  
  /* These don't take any context */
  CALLER_OWN char *(*cmd_args_fn)(int argc, const char *const *argv, const char *buff);
				/* takes argv and the full command as buff.
				 * caller must free return value if !NULL */
  void (*cmd_v_fn)(void);	/* takes no args */
  void (*cmd_i_fn)(int i);	/* takes an int as an arg */

  /* The following also take the active context if it's valid */
  CALLER_OWN char *(*cmd_ctxargs_fn)(void *ctx, int argc, const char *const *argv, const char *buff);
				/* takes argv and the full command as buff.
				 * caller must free return value if !NULL */
  void (*cmd_ctxv_fn)(void *ctx);	        /* takes no args */
  void (*cmd_ctxi_fn)(void *ctx, int i);	/* takes an int as an arg */
  SV *cmd_perl;                                /* Perl closure that takes a list of args */
} owl_cmd;


typedef struct _owl_zwrite {
  char *cmd;
  char *zwriteline;
  char *class;
  char *inst;
  char *realm;
  char *opcode;
  char *zsig;
  char *message;
  GPtrArray *recips;
  int cc;
  int noping;
} owl_zwrite;

typedef struct _owl_pair {
  const char *key;
  char *value;
} owl_pair;

struct _owl_fmtext_cache;

typedef struct _owl_message {
  int id;
  int direction;
#ifdef HAVE_LIBZEPHYR
  bool has_notice;
  ZNotice_t notice;
#endif
  struct _owl_fmtext_cache * fmtext;
  int delete;
  const char *hostname;
  GPtrArray *attributes;          /* this is a list of pairs */
  char *timestr;
  time_t time;
} owl_message;

#define OWL_FMTEXT_CACHE_SIZE 1000
/* We cache the saved fmtexts for the last bunch of messages we
   rendered */
typedef struct _owl_fmtext_cache {
    owl_message * message;
    owl_fmtext fmtext;
} owl_fmtext_cache;

typedef struct _owl_style {
  char *name;
  SV *perlobj;
} owl_style;

typedef struct _owl_mainwin {
  int curtruncated;
  int lasttruncated;
  int lastdisplayed;
  owl_window *window;
} owl_mainwin;

typedef struct _owl_editwin owl_editwin;
typedef struct _owl_editwin_excursion owl_editwin_excursion;

typedef struct _owl_viewwin {
  owl_fmtext fmtext;
  int textlines;
  int topline;
  int rightshift;
  owl_window *window;
  void (*onclose_hook) (struct _owl_viewwin *vwin, void *data);
  void *onclose_hook_data;

  gulong sig_resize_id;
  owl_window *content;
  gulong sig_content_redraw_id;
  owl_window *status;
  gulong sig_status_redraw_id;
  owl_window *cmdwin;
} owl_viewwin;
  
typedef struct _owl_popwin {
  owl_window *border;
  owl_window *content;
  gulong sig_redraw_id;
  gulong sig_resize_id;
} owl_popwin;
  
typedef struct _owl_msgwin {
  char *msg;
  owl_window *window;
  gulong redraw_id;
} owl_msgwin;

typedef struct _owl_messagelist {
  GPtrArray *list;
} owl_messagelist;

typedef struct _owl_regex {
  int negate;
  char *string;
  regex_t re;
} owl_regex;

typedef struct _owl_filterelement {
  int (*match_message)(const struct _owl_filterelement *fe, const owl_message *m);
  /* Append a string representation of the filterelement onto buf*/
  void (*print_elt)(const struct _owl_filterelement *fe, GString *buf);
  /* Operands for and,or,not*/
  struct _owl_filterelement *left, *right;
  /* For regex filters*/
  owl_regex re;
  /* Used by regexes, filter references, and perl */
  char *field;
} owl_filterelement;

typedef struct _owl_filter {
  char *name;
  owl_filterelement * root;
  int fgcolor;
  int bgcolor;
} owl_filter;

typedef struct _owl_view {
  char *name;
  owl_filter *filter;
  owl_messagelist *ml;
  const owl_style *style;
  int cachedmsgid;
} owl_view;

typedef struct _owl_history {
  GQueue hist;
  GList *cur;
  bool partial;
} owl_history;

typedef struct _owl_mainpanel {
  owl_window *panel;
  owl_window *typwin;
  owl_window *sepwin;
  owl_window *msgwin;
  owl_window *recwin;
  int recwinlines;
} owl_mainpanel;

typedef struct _owl_keybinding {
  int  *keys;			/* keypress stack */
  int   len;                    /* length of stack */
  int   type;			/* command or function? */
  char *desc;			/* description (or "*user*") */
  char *command;		/* command, if of type command */
  void (*function_fn)(void);	/* function ptr, if of type function */
} owl_keybinding;

typedef struct _owl_keymap {
  char     *name;		/* name of keymap */
  char     *desc;		/* description */
  GPtrArray *bindings;		/* key bindings */
  const struct _owl_keymap *parent;	/* parent */
  void (*default_fn)(owl_input j);	/* default action (takes a keypress) */
  void (*prealways_fn)(owl_input  j);	/* always called before a keypress is received */
  void (*postalways_fn)(owl_input  j);	/* always called after keypress is processed */
} owl_keymap;

typedef struct _owl_keyhandler {
  owl_dict  keymaps;		/* dictionary of keymaps */
  const owl_keymap *active;		/* currently active keymap */
  int	    in_esc;		/* escape pressed? */
  int       kpstack[OWL_KEYMAP_MAXSTACK+1]; /* current stack of keypresses */
  int       kpstackpos;		/* location in stack (-1 = none) */
} owl_keyhandler;

typedef struct _owl_zbuddylist {
  GPtrArray *zusers;
} owl_zbuddylist;

typedef struct _owl_errqueue {
  GPtrArray *errlist;
} owl_errqueue;

typedef struct _owl_colorpair_mgr {
  int next;
  short **pairs;
  bool overflow;
} owl_colorpair_mgr;

typedef struct _owl_popexec {
  int refcount;
  owl_viewwin *vwin;
  int winactive;
  pid_t pid;			/* or 0 if it has terminated */
  guint io_watch;
} owl_popexec;

typedef struct _owl_global {
  owl_mainwin *mw;
  owl_popwin *pw;
  owl_msgwin msgwin;
  owl_history cmdhist;		/* command history */
  owl_history msghist;		/* outgoing message history */
  owl_keyhandler kh;
  owl_dict filters;
  GList *filterlist;
  GPtrArray *puntlist;
  owl_vardict vars;
  owl_cmddict cmds;
  GList *context_stack;
  owl_errqueue errqueue;
  int lines, cols;
  int curmsg, topmsg;
  int markedmsgid;              /* for finding the marked message when it has moved. */
  int curmsg_vert_offset;
  owl_view current_view;
  owl_messagelist *msglist;
  WINDOW *input_pad;
  owl_mainpanel mainpanel;
  gulong typwin_erase_id;
  int rightshift;
  bool resizepending;
  char *homedir;
  char *confdir;
  char *startupfile;
  int direction;
  int haveconfig;
  int config_format;
  owl_editwin *tw;
  owl_viewwin *vw;
  void *perl;
  int debug;
  time_t starttime;
  time_t lastinputtime;
  time_t last_wakeup_time;
  char *startupargs;
  int nextmsgid;
  owl_colorpair_mgr cpmgr;
  pid_t newmsgproc_pid;
  owl_regex search_re;
  GQueue *messagequeue;     /* for queueing up messages */
  owl_dict styledict;       /* global dictionary of available styles */
  char *response;           /* response to the last question asked */
  int havezephyr;
  owl_zbuddylist zbuddies;
  GList *zaldlist;
  int pseudologin_notify;
  struct termios startup_tio;
  int load_initial_subs;
  FILE *debug_file;
  char *kill_buffer;
  int interrupt_count;
#if GLIB_CHECK_VERSION(2, 31, 0)
  GMutex interrupt_lock;
#else
  GMutex *interrupt_lock;
#endif
} owl_global;

/* globals */
extern owl_global g;

#include <owl_prototypes.h>

/* These were missing from the Zephyr includes before Zephyr 3. */
#if defined HAVE_LIBZEPHYR && defined ZCONST
int ZGetSubscriptions(ZSubscription_t *, int *);
int ZGetLocations(ZLocations_t *,int *);
#endif

/* We have to dynamically bind these ourselves */
extern gboolean (*gvalue_from_sv) (GValue * value, SV * sv);
extern SV * (*sv_from_gvalue) (const GValue * value);
extern GClosure * (*perl_closure_new) (SV * callback, SV * data, gboolean swap);


#endif /* INC_BARNOWL_OWL_H */
