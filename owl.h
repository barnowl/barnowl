#ifndef INC_OWL_H
#define INC_OWL_H

#include <zephyr/zephyr.h>
#include <curses.h>
#include <sys/param.h>
#include <EXTERN.h>
#include <netdb.h>
#include <regex.h>
#include "config.h"

static const char owl_h_fileIdent[] = "$Id$";

#define OWL_VERSION         1.2.3
#define OWL_VERSION_STRING "1.2.3"

#define OWL_DEBUG 0
#define OWL_DEBUG_FILE "/var/tmp/owldebug"

#define OWL_FMTEXT_ATTR_NONE      0
#define OWL_FMTEXT_ATTR_BOLD      1
#define OWL_FMTEXT_ATTR_REVERSE   2
#define OWL_FMTEXT_ATTR_UNDERLINE 4

#define OWL_COLOR_BLACK     0
#define OWL_COLOR_RED       1
#define OWL_COLOR_GREEN     2
#define OWL_COLOR_YELLOW    3
#define OWL_COLOR_BLUE      4
#define OWL_COLOR_MAGENTA   5
#define OWL_COLOR_CYAN      6
#define OWL_COLOR_WHITE     7
#define OWL_COLOR_DEFAULT   8

#define OWL_EDITWIN_STYLE_MULTILINE 0
#define OWL_EDITWIN_STYLE_ONELINE   1

#define OWL_MESSAGE_TYPE_ADMIN  0
#define OWL_MESSAGE_TYPE_ZEPHYR 1

#define OWL_MESSAGE_ADMINTYPE_GENERIC  0
#define OWL_MESSAGE_ADMINTYPE_OUTGOING 1
#define OWL_MESSAGE_ADMINTYPE_NOTADMIN 2

#define OWL_DIRECTION_NONE      0
#define OWL_DIRECTION_DOWNWARDS 1
#define OWL_DIRECTION_UPWARDS   2

#define OWL_SCROLLMODE_NORMAL   0
#define OWL_SCROLLMODE_TOP      1
#define OWL_SCROLLMODE_NEARTOP  2
#define OWL_SCROLLMODE_CENTER   3
#define OWL_SCROLLMODE_PAGED    4
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
#define OWL_CTX_EDIT         0x3000
#define OWL_CTX_EDITLINE     0x1000
#define OWL_CTX_EDITMULTI    0x2000

#define OWL_USERCLUE_NONE       0
#define OWL_USERCLUE_CLASSES    1
#define OWL_USERCLUE_FOOBAR     2
#define OWL_USERCLUE_BAZ        4

#define OWL_WEBBROWSER_NONE     0
#define OWL_WEBBROWSER_NETSCAPE 1
#define OWL_WEBBROWSER_GALEON   2
#define OWL_WEBBROWSER_OPERA    3

#define OWL_VARIABLE_OTHER      0
#define OWL_VARIABLE_INT        1
#define OWL_VARIABLE_BOOL       2
#define OWL_VARIABLE_STRING     3

#define OWL_FILTER_MAX_DEPTH    300

#define OWL_KEYMAP_MAXSTACK 20

#define OWL_KEYBINDING_COMMAND   1 /* command string */
#define OWL_KEYBINDING_FUNCTION  2 /* function taking no args */

#define OWL_DEFAULT_ZAWAYMSG "I'm sorry, but I am currently away from the terminal and am\nnot able to receive your message.\n"

#define OWL_INCLUDE_REG_TESTS  1  /* whether to build in regression tests */

#define OWL_CMD_ALIAS_SUMMARY_PREFIX "command alias to: "

#ifndef CTRL
#define CTRL(key) ((key)&037)
#endif
#ifndef META
#define META(key) ((key)|0200)
#endif

#define LINE 2048

typedef struct _owl_variable {
  char *name;
  int   type;  /* OWL_VARIABLE_* */
  void *pval_default;  /* for types other and string */
  int   ival_default;  /* for types int and bool     */
  char *validsettings;		/* documentation of valid settings */
  char *summary;		/* summary of usage */
  char *description;		/* detailed description */
  void *val;                    /* current value */
  int  (*validate_fn)(struct _owl_variable *v, void *newval);
                                /* returns 1 if newval is valid */
  int  (*set_fn)(struct _owl_variable *v, void *newval); 
                                /* sets the variable to a value
				 * of the appropriate type.
				 * unless documented, this 
				 * should make a copy. 
				 * returns 0 on success. */
  int  (*set_fromstring_fn)(struct _owl_variable *v, char *newval);
                                /* sets the variable to a value
				 * of the appropriate type.
				 * unless documented, this 
				 * should make a copy. 
				 * returns 0 on success. */
  void *(*get_fn)(struct _owl_variable *v);
				/* returns a reference to the current value.
				 * WARNING:  this approach is hard to make
				 * thread-safe... */
  int  (*get_tostring_fn)(struct _owl_variable *v, 
			  char *buf, int bufsize, void *val); 
                                /* converts val to a string 
				 * and puts into buf */
  void  (*free_fn)(struct _owl_variable *v);
				/* frees val as needed */
} owl_variable;

typedef struct _owl_fmtext {
  int textlen;
  char *textbuff;
  char *fmbuff;
  char *colorbuff;
} owl_fmtext;

typedef struct _owl_list {
  int size;
  int avail;
  void **list;
} owl_list;

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
  char *(*cmd_args_fn)(int argc, char **argv, char *buff);  
				/* takes argv and the full command as buff.
				 * caller must free return value if !NULL */
  void (*cmd_v_fn)(void);	/* takes no args */
  void (*cmd_i_fn)(int i);	/* takes an int as an arg */

  /* The following also take the active context if it's valid */
  char *(*cmd_ctxargs_fn)(void *ctx, int argc, char **argv, char *buff);  
				/* takes argv and the full command as buff.
				 * caller must free return value if !NULL */
  void (*cmd_ctxv_fn)(void *ctx);	        /* takes no args */
  void (*cmd_ctxi_fn)(void *ctx, int i);	/* takes an int as an arg */
} owl_cmd;


typedef struct _owl_zwrite {
  char class[LINE];
  char inst[LINE];
  char realm[LINE];
  char opcode[LINE];
  owl_list recips;
  int cc;
  int noping;
} owl_zwrite;

typedef struct _owl_message {
  int id;
  int type;
  int admintype;
  ZNotice_t notice;
  owl_fmtext fmtext;
  int delete;
  char hostname[MAXHOSTNAMELEN];
  char *sender;
  char *recip;
  char *class;
  char *inst;
  char *opcode;
  char *time;
  char *realm;
  char *body;
  char *zwriteline;
} owl_message;

typedef struct _owl_mainwin {
  int curtruncated;
  int lasttruncated;
  int lastdisplayed;
} owl_mainwin;

typedef struct _owl_viewwin {
  owl_fmtext fmtext;
  int textlines;
  int topline;
  int rightshift;
  int winlines, wincols;
  WINDOW *curswin;
} owl_viewwin;
  
typedef struct _owl_popwin {
  WINDOW *borderwin;
  WINDOW *popwin;
  int lines;
  int cols;
  int active;
  int needsfirstrefresh;
  void (*handler) (int ch);
} owl_popwin;

typedef struct _owl_messagelist {
  owl_list list;
} owl_messagelist;

typedef struct _owl_regex {
  int negate;
  char *string;
  regex_t re;
} owl_regex;

typedef struct _owl_filterelement {
  int type;
  char *field;
  owl_regex re;
} owl_filterelement;

typedef struct _owl_filter {
  char *name;
  int polarity;
  owl_list fes; /* filterelements */
  int color;
  int cachedmsgid;  /* cached msgid: should move into view eventually */
} owl_filter;

typedef struct _owl_view {
  owl_filter *filter;
  owl_messagelist ml;
} owl_view;

typedef struct _owl_history {
  owl_list hist;
  int cur;
  int touched;
  int partial;
} owl_history;

typedef struct _owl_editwin {
  char *buff;
  owl_history *hist;
  int bufflen;
  int allocated;
  int buffx, buffy;
  int topline;
  int winlines, wincols, fillcol, wrapcol;
  WINDOW *curswin;
  int style;
  int lock;
  int dotsend;
} owl_editwin;

typedef struct _owl_keybinding {
  int  *j;			/* keypress stack (0-terminated) */  
  int   type;			/* command or function? */
  char *desc;			/* description (or "*user*") */
  char *command;		/* command, if of type command */
  void (*function_fn)(void);	/* function ptr, if of type function */
} owl_keybinding;

typedef struct _owl_keymap {
  char     *name;		/* name of keymap */
  char     *desc;		/* description */
  owl_list  bindings;		/* key bindings */
  struct _owl_keymap *submap;	/* submap */
  void (*default_fn)(int j);	/* default action (takes a keypress) */
  void (*prealways_fn)(int j);	/* always called before a keypress is received */
  void (*postalways_fn)(int j);	/* always called after keypress is processed */
} owl_keymap;

typedef struct _owl_keyhandler {
  owl_dict  keymaps;		/* dictionary of keymaps */
  owl_keymap *active;		/* currently active keymap */
  int	    in_esc;		/* escape pressed? */
  int       kpstack[OWL_KEYMAP_MAXSTACK+1]; /* current stack of keypresses */
  int       kpstackpos;		/* location in stack (-1 = none) */
} owl_keyhandler;

typedef struct _owl_global {
  owl_mainwin mw;
  owl_popwin pw;
  owl_history cmdhist;		/* command history */
  owl_history msghist;		/* outgoing message history */
  owl_keyhandler kh;
  owl_list filterlist;
  owl_list puntlist;
  owl_vardict vars;
  owl_cmddict cmds;
  owl_context ctx;
  int lines, cols;
  int curmsg, topmsg;
  int curmsg_vert_offset;
  owl_view current_view;
  owl_messagelist msglist;
  WINDOW *recwin, *sepwin, *msgwin, *typwin;
  int needrefresh;
  int rightshift;
  int resizepending;
  int recwinlines;
  int typwinactive;
  char thishost[LINE];
  char thistty[LINE];
  char homedir[LINE];
  int direction;
  int zaway;
  char *cur_zaway_msg;
  int haveconfig;
  int config_format;
  char buffercommand[1024];
  owl_editwin tw;
  owl_viewwin vw;
  void *perl;
  int debug;
  int starttime;
  char startupargs[LINE];
  int userclue;
  int nextmsgid;
  int hascolors;
  int colorpairs;
  int searchactive;
  char *searchstring;
  owl_filterelement fe_true;
  owl_filterelement fe_false;
  owl_filterelement fe_null;
} owl_global;

/* globals */
owl_global g;

#include "owl_prototypes.h"

/* these are missing from the zephyr includes for some reason */
int ZGetSubscriptions(ZSubscription_t *, int *);
int ZGetLocations(ZLocations_t *,int *);

#endif /* INC_OWL_H */
