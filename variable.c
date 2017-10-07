#include "owl.h"
#include <stdio.h>
#include "gmarshal_funcs.h"

/* TODO(davidben): When we can require 2.30 and up, remove this. */
#ifndef G_VALUE_INIT
#define G_VALUE_INIT { 0, { { 0 } } }
#endif

typedef const char *(*get_string_t)(const owl_variable *);
typedef int (*get_int_t)(const owl_variable *);
typedef bool (*get_bool_t)(const owl_variable *);

typedef int (*set_string_t)(owl_variable *, const char *);
typedef int (*set_int_t)(owl_variable *, int);
typedef int (*set_bool_t)(owl_variable *, bool);

typedef int (*validate_string_t)(const owl_variable *, const char *);
typedef int (*validate_int_t)(const owl_variable *, int);
typedef int (*validate_bool_t)(const owl_variable *, bool);

static void owl_variable_dict_newvar_bool_full(owl_vardict *vd,
                                               const char *name,
                                               bool default_val,
                                               const char *summary,
                                               const char *description,
                                               validate_bool_t validate_fn,
                                               set_bool_t set_fn,
                                               get_bool_t get_fn);

static void owl_variable_dict_newvar_string_full(owl_vardict *vd,
                                                 const char *name,
                                                 const char *default_val,
                                                 const char *summary,
                                                 const char *description,
                                                 const char *validsettings,
                                                 validate_string_t validate_fn,
                                                 set_string_t set_fn,
                                                 get_string_t get_fn);

static void owl_variable_dict_newvar_int_full(owl_vardict *vd,
                                              const char *name,
                                              int default_val,
                                              const char *summary,
                                              const char *description,
                                              const char *validsettings,
                                              validate_int_t validate_fn,
                                              set_int_t set_fn,
                                              get_int_t get_fn);

static void owl_variable_dict_newvar_enum_full(owl_vardict *vd,
                                               const char *name,
                                               int default_val,
                                               const char *summary,
                                               const char *description,
                                               const char *validsettings,
                                               validate_int_t validate_fn,
                                               set_int_t set_fn,
                                               get_int_t get_fn);

#define OWLVAR_BOOL(name, default, summary, description) \
        owl_variable_dict_newvar_bool(vd, name, default, summary, description)

#define OWLVAR_BOOL_FULL(name, default, summary, description, validate, set, get) \
        owl_variable_dict_newvar_bool_full(vd, name, default, summary, description, \
                                           validate, set, get)

#define OWLVAR_INT(name, default, summary, description) \
        owl_variable_dict_newvar_int(vd, name, default, summary, description)

#define OWLVAR_INT_FULL(name,default,summary,description,validset,validate,set,get) \
        owl_variable_dict_newvar_int_full(vd, name, default, summary, description, \
                                          validset, validate, set, get)

#define OWLVAR_PATH(name, default, summary, description) \
        owl_variable_dict_newvar_path(vd, name, default, summary, description)

#define OWLVAR_STRING(name, default, summary, description) \
        owl_variable_dict_newvar_string(vd, name, default, summary, description)

#define OWLVAR_STRING_FULL(name, default, validset, summary, description, validate, set, get) \
        owl_variable_dict_newvar_string_full(vd, name, default, summary, description, \
                                             validset, validate, set, get)

/* enums are really integers, but where validset is a comma-separated
 * list of strings which can be specified.  The tokens, starting at 0,
 * correspond to the values that may be specified. */
#define OWLVAR_ENUM(name, default, summary, description, validset) \
        owl_variable_dict_newvar_enum(vd, name, default, summary, description, validset)

#define OWLVAR_ENUM_FULL(name,default,summary,description,validset,validate, set, get) \
        owl_variable_dict_newvar_enum_full(vd, name, default, summary, description, \
                                           validset, validate, set, get)

void owl_variable_add_defaults(owl_vardict *vd)
{
  OWLVAR_STRING( "personalbell" /* %OwlVarStub */, "off",
		 "ring the terminal bell when personal messages are received",
		 "Can be set to 'on', 'off', or the name of a filter which\n"
		 "messages need to match in order to ring the bell");

  OWLVAR_BOOL( "bell" /* %OwlVarStub */, 1,
	       "enable / disable the terminal bell", "" );

  OWLVAR_BOOL_FULL( "debug" /* %OwlVarStub */, OWL_DEBUG,
		    "whether debugging is enabled",
		    "If set to 'on', debugging messages are logged to the\n"
		    "file specified by the debugfile variable.\n",
		    NULL, owl_variable_debug_set, NULL);

  OWLVAR_BOOL( "startuplogin" /* %OwlVarStub */, 1,
	       "send a login message when BarnOwl starts", "" );

  OWLVAR_BOOL( "shutdownlogout" /* %OwlVarStub */, 1,
	       "send a logout message when BarnOwl exits", "" );

  OWLVAR_BOOL( "rxping" /* %OwlVarStub */, 0,
	       "display received pings", "" );

  OWLVAR_BOOL( "txping" /* %OwlVarStub */, 1,
	       "send pings", "" );

  OWLVAR_BOOL( "sepbar_disable" /* %OwlVarStub */, 0,
	       "disable printing information in the separator bar", "" );

  OWLVAR_BOOL( "smartstrip" /* %OwlVarStub */, 1,
	       "strip kerberos instance for reply", "");

  OWLVAR_BOOL( "newlinestrip" /* %OwlVarStub */, 1,
	       "strip leading and trailing newlines", "");

  OWLVAR_BOOL( "displayoutgoing" /* %OwlVarStub */, 1,
	       "display outgoing messages", "" );

  OWLVAR_BOOL( "loginsubs" /* %OwlVarStub */, 1,
	       "load logins from .anyone on startup", "" );

  OWLVAR_BOOL_FULL( "colorztext" /* %OwlVarStub */, 1,
                    "allow @color() in zephyrs to change color",
                    "", NULL, owl_variable_colorztext_set, NULL);

  OWLVAR_BOOL( "fancylines" /* %OwlVarStub */, 1,
	       "Use 'nice' line drawing on the terminal.",
	       "If turned off, dashes, pipes and pluses will be used\n"
	       "to draw lines on the screen.  Useful when the terminal\n"
	       "is causing problems" );

  OWLVAR_BOOL( "zcrypt" /* %OwlVarStub */, 1,
	       "Do automatic zcrypt processing",
	       "" );

  OWLVAR_BOOL_FULL( "pseudologins" /* %OwlVarStub */, 0,
		    "Enable zephyr pseudo logins",
		    "When this is enabled, BarnOwl will periodically check the zephyr\n"
		    "location of users in your .anyone file.  If a user is present\n"
		    "but sent no login message, or a user is not present that sent no\n"
		    "logout message, a pseudo login or logout message will be created\n",
		    NULL, owl_variable_pseudologins_set, NULL);

  OWLVAR_BOOL( "ignorelogins" /* %OwlVarStub */, 0,
	       "Enable printing of login notifications",
	       "When this is enabled, BarnOwl will print login and logout notifications\n"
	       "for zephyr or other protocols.  If disabled BarnOwl will not print\n"
	       "login or logout notifications.\n");

  OWLVAR_ENUM_FULL( "disable-ctrl-d" /* %OwlVarStub:lockout_ctrld */, 1,
		    "don't send zephyrs on C-d",
		    "If set to 'on', C-d won't send a zephyr from the edit\n"
		    "window.  If set to 'off', C-d will always send a zephyr\n"
		    "being composed in the edit window.  If set to 'middle',\n"
		    "C-d will only ever send a zephyr if the cursor is at\n"
		    "the end of the message being composed.\n\n"
		    "Note that this works by changing the C-d keybinding\n"
		    "in the editmulti keymap.\n",
		    "off,middle,on",
		    NULL, owl_variable_disable_ctrl_d_set, NULL);

  OWLVAR_PATH( "debug_file" /* %OwlVarStub */, OWL_DEBUG_FILE,
	       "path for logging debug messages when debugging is enabled",
	       "This file will be logged to if 'debug' is set to 'on'.\n"
               "BarnOwl will append a dot and the current process's pid to the filename.");
  
  OWLVAR_PATH( "zsigproc" /* %OwlVarStub:zsigproc */, NULL,
	       "name of a program to run that will generate zsigs",
	       "This program should produce a zsig on stdout when run.\n"
	       "Note that it is important that this program not block.\n\n"
               "See the documentation for 'zsig' for more information about\n"
               "how the outgoing zsig is chosen."
               );

  OWLVAR_PATH( "newmsgproc" /* %OwlVarStub:newmsgproc */, NULL,
	       "name of a program to run when new messages are present",
	       "The named program will be run when BarnOwl receives new\n"
	       "messages.  It will not be run again until the first\n"
	       "instance exits");

  OWLVAR_STRING( "zsender" /* %OwlVarStub */, "",
             "zephyr sender name",
         "Allows you to customize the outgoing username in\n"
         "zephyrs.  If this is unset, it will use your Kerberos\n"
         "principal. Note that customizing the sender name will\n"
         "cause your zephyrs to be sent unauthenticated.");

  OWLVAR_STRING( "zsigfunc" /* %OwlVarStub */, "BarnOwl::default_zephyr_signature()",
		 "zsig perl function",
		 "Called every time you start a zephyrgram without an\n"
		 "explicit zsig.  The default setting implements the policy\n"
		 "described in the documentation for the 'zsig' variable.\n"
		 "See also BarnOwl::random_zephyr_signature().\n");

  OWLVAR_STRING( "zsig" /* %OwlVarStub */, "",
	         "zephyr signature",
		 "The zsig to get on outgoing messages. If this variable is\n"
		 "unset, 'zsigproc' will be run to generate a zsig. If that is\n"
		 "also unset, the 'zwrite-signature' zephyr variable will be\n"
		 "used instead.\n");

  OWLVAR_STRING( "appendtosepbar" /* %OwlVarStub */, "",
	         "string to append to the end of the sepbar",
		 "The sepbar is the bar separating the top and bottom\n"
		 "of the BarnOwl screen.  Any string specified here will\n"
		 "be displayed on the right of the sepbar\n");

  OWLVAR_BOOL( "zaway" /* %OwlVarStub */, 0,
	       "turn zaway on or off", "" );

  OWLVAR_STRING( "zaway_msg" /* %OwlVarStub */, 
		 OWL_DEFAULT_ZAWAYMSG,
	         "zaway msg for responding to zephyrs when away", "" );

  OWLVAR_STRING( "zaway_msg_default" /* %OwlVarStub */, 
		 OWL_DEFAULT_ZAWAYMSG,
	         "default zaway message", "" );

  OWLVAR_STRING( "view_home" /* %OwlVarStub */, "all",
	         "home view to switch to after 'X' and 'V'", 
		 "SEE ALSO: view, filter\n" );

  OWLVAR_STRING( "alert_filter" /* %OwlVarStub */, "none",
		 "filter on which to trigger alert actions",
		 "" );

  OWLVAR_STRING( "alert_action" /* %OwlVarStub */, "nop",
		 "BarnOwl command to execute for alert actions",
		 "" );

  OWLVAR_STRING_FULL( "tty" /* %OwlVarStub */, "", "<string>", "tty name for zephyr location", "",
		      NULL, owl_variable_tty_set, NULL);

  OWLVAR_STRING( "default_style" /* %OwlVarStub */, "default",
		 "name of the default formatting style",
		 "This sets the default message formatting style.\n"
		 "Styles may be created with the 'style' command.\n"
		 "Some built-in styles include:\n"
		 "   default  - the default BarnOwl formatting\n"
		 "   oneline  - one line per-message\n"
		 "   perl     - legacy perl interface\n"
		 "\nSEE ALSO: style, show styles, view -s <style>\n"
		 );


  OWLVAR_INT(    "edit:maxfillcols" /* %OwlVarStub:edit_maxfillcols */, 70,
	         "maximum number of columns for M-q (edit:fill-paragraph) to fill text to",
                 "This specifies the maximum number of columns for M-q to fill text\n"
                 "to.  If set to 0, M-q will wrap to the width of the window, and\n"
                 "values less than 0 disable M-q entirely.\n");

  OWLVAR_INT(    "edit:maxwrapcols" /* %OwlVarStub:edit_maxwrapcols */, 70,
	         "maximum number of columns for line-wrapping",
                 "This specifies the maximum number of columns for\n"
                 "auto-line-wrapping.  If set to 0, text will be wrapped at the\n"
                 "window width. Values less than 0 disable automatic wrapping.\n"
                 "\n"
                 "As a courtesy to recipients, it is recommended that outgoing\n"
                 "Zephyr messages be no wider than 70 columns.\n");

  OWLVAR_INT_FULL( "typewinsize" /* %OwlVarStub:typwin_lines */, 
		   OWL_TYPWIN_SIZE,
		  "number of lines in the typing window", 
		   "This specifies the height of the window at the\n"
		   "bottom of the screen where commands are entered\n"
		   "and where messages are composed.\n",
		   "int > 0",
		   owl_variable_int_validate_gt0,
		   owl_variable_typewinsize_set,
		   NULL /* use default for get */
		   );

  OWLVAR_INT( "typewindelta" /* %OwlVarStub */, 0,
		  "number of lines to add to the typing window when in use",
		   "On small screens you may want the typing window to\n"
		   "auto-hide when not entering a command or message.\n"
		   "This variable is the number of lines to add to the\n"
           "typing window when it is in use; you can then set\n"
           "typewinsize to 1.\n\n"
           "This works a lot better with a non-default scrollmode;\n"
           "try :set scrollmode pagedcenter.\n");

  OWLVAR_ENUM( "scrollmode" /* %OwlVarStub */, OWL_SCROLLMODE_NORMAL,
	       "how to scroll up and down",
	       "This controls how the screen is scrolled as the\n"
	       "cursor moves between messages being displayed.\n"
	       "The following modes are supported:\n\n"
	       "   normal      - This is the BarnOwl default.  Scrolling happens\n"
	       "                 when it needs to, and an attempt is made to\n"
	       "                 keep the current message roughly near\n"
	       "                 the middle of the screen.\n"
	       "   top         - The current message will always be the\n"
	       "                 the top message displayed.\n"
	       "   neartop     - The current message will be one down\n"
	       "                 from the top message displayed,\n"
	       "                 where possible.\n"
	       "   center      - An attempt is made to keep the current\n"
	       "                 message near the center of the screen.\n"
	       "   paged       - The top message displayed only changes\n"
	       "                 when user moves the cursor to the top\n"
	       "                 or bottom of the screen.  When it moves,\n"
	       "                 the screen will be paged up or down and\n"
	       "                 the cursor will be near the top or\n"
	       "                 the bottom.\n"
	       "   pagedcenter - The top message displayed only changes\n"
	       "                 when user moves the cursor to the top\n"
	       "                 or bottom of the screen.  When it moves,\n"
	       "                 the screen will be paged up or down and\n"
	       "                 the cursor will be near the center.\n",
	       "normal,top,neartop,center,paged,pagedcenter" );

  OWLVAR_BOOL( "narrow-related" /* %OwlVarStub:narrow_related */, 1,
               "Make smartnarrow use broader filters",
               "Causes smartfilter to narrow to messages \"related\" to \n"
               "the current message, as well as ones to the same place.\n\n"
               "for Zephyr, this controls whether to narrow to e.g. class-help or\n"
               "class-help.d alone, or to related-class-help, which includes\n"
               "help, unhelp, help.d, etc.\n\nDefault is true (include unclasses, etc.).\n" );

  OWLVAR_BOOL( "_followlast" /* %OwlVarStub */, 0,
	       "enable automatic following of the last zephyr",
	       "If the cursor is at the last message, it will\n"
	       "continue to follow the last message if this is set.\n"
	       "Note that this is currently risky as you might accidentally\n"
	       "delete a message right as it came in.\n" );

  OWLVAR_STRING_FULL( "default_exposure" /* %OwlVarStub */, "",
                      "none,opstaff,realm-visible,realm-announced,net-visible,net-announced",
                      "controls the persistent value for exposure",
                      "The default exposure level corresponds to the Zephyr exposure value\n"
                      "in ~/.zephyr.vars.  Defaults to realm-visible if there is no value in\n"
                      "~/.zephyr.vars.\n"
                      "See the description of exposure for the values this can be.",
                      NULL, owl_variable_default_exposure_set, owl_variable_default_exposure_get );

  OWLVAR_STRING_FULL( "exposure" /* %OwlVarStub */, "",
                      "none,opstaff,realm-visible,realm-announced,net-visible,net-announced",
                      "controls who can zlocate you",
                      "The exposure level, defaulting to the value of default_exposure,\n"
                      "can be one of the following (from least exposure to widest exposure,\n"
                      "as listed in zctl(1)):\n"
                      "\n"
                      "   none            - This completely disables Zephyr for the user. \n"
                      "                     The user is not registered with Zephyr.  No user\n"
                      "                     location information is retained by Zephyr.  No\n"
                      "                     login or logout announcements will be sent.  No\n"
                      "                     subscriptions will be entered for the user, and\n"
                      "                     no notices will be displayed by zwgc(1).\n"
                      "   opstaff         - The user is registered with Zephyr.  No login or\n"
                      "                     logout announcements will be sent, and location\n"
                      "                     information will only be visible to Operations\n"
                      "                     staff.  Default subscriptions and any additional\n"
                      "                     personal subscriptions will be entered for the\n"
                      "                     user.\n"
                      "   realm-visible   - The user is registered with Zephyr.  User\n"
                      "                     location information is retained by Zephyr and\n"
                      "                     made available only to users within the user’s\n"
                      "                     Kerberos realm.  No login or logout\n"
                      "                     announcements will be sent.  This is the system\n"
                      "                     default.  Default subscriptions and any\n"
                      "                     additional personal subscriptions will be\n"
                      "                     entered for the user.\n"
                      "   realm-announced - The user is registered with Zephyr.  User\n"
                      "                     location information is retained by Zephyr and\n"
                      "                     made available only to users authenticated\n"
                      "                     within the user’s Kerberos realm.  Login and\n"
                      "                     logout announcements will be sent, but only to\n"
                      "                     users within the user’s Kerberos realm who have\n"
                      "                     explicitly requested such via subscriptions. \n"
                      "                     Default subscriptions and any additional\n"
                      "                     personal subscriptions will be entered for the\n"
                      "                     user.\n"
                      "   net-visible     - The user is registered with Zephyr.  User\n"
                      "                     location information is retained by Zephyr and\n"
                      "                     made available to any authenticated user who\n"
                      "                     requests such.  Login and logout announcements\n"
                      "                     will be sent only to users within the user’s\n"
                      "                     Kerberos realm who have explicitly requested\n"
                      "                     such via subscriptions.  Default subscriptions\n"
                      "                     and any additional personal subscriptions will\n"
                      "                     be entered for the user.\n"
                      "   net-announced   - The user is registered with Zephyr.  User\n"
                      "                     location information is retained by Zephyr and\n"
                      "                     made available to any authenticated user who\n"
                      "                     requests such.  Login and logout announcements\n"
                      "                     will be sent to any user has requested such. \n"
                      "                     Default subscriptions and any additional\n"
                      "                     personal subscriptions will be entered for the\n"
                      "                     user.\n",
                      NULL, owl_variable_exposure_set, NULL /* use default for get */ );
}

/**************************************************************************/
/*********************** SPECIFIC TO VARIABLES ****************************/
/**************************************************************************/


/* commonly useful */

int owl_variable_int_validate_gt0(const owl_variable *v, int newval)
{
  return !(newval < 1);
}

int owl_variable_int_validate_positive(const owl_variable *v, int newval)
{
  return !(newval < 0);
}

/* typewinsize */
int owl_variable_typewinsize_set(owl_variable *v, int newval)
{
  int rv;
  rv = owl_variable_int_set_default(v, newval);
  if (0 == rv) owl_mainpanel_layout_contents(&g.mainpanel);
  return(rv);
}

/* debug (cache value in g->debug) */
int owl_variable_debug_set(owl_variable *v, bool newval)
{
  g.debug = newval;
  return owl_variable_bool_set_default(v, newval);
}

int owl_variable_colorztext_set(owl_variable *v, bool newval)
{
  int ret = owl_variable_bool_set_default(v, newval);
  /* flush the format cache so that we see the update, but only if we're done initializing BarnOwl */
  if (owl_global_get_msglist(&g) != NULL)
    owl_messagelist_invalidate_formats(owl_global_get_msglist(&g));
  if (owl_global_get_mainwin(&g) != NULL) {
    owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  }
  return ret;
}

int owl_variable_pseudologins_set(owl_variable *v, bool newval)
{
  static guint timer = 0;
  if (newval) {
    owl_function_zephyr_buddy_check(0);
    if (timer == 0) {
      timer = g_timeout_add_seconds(180, owl_zephyr_buddycheck_timer, NULL);
    }
  } else {
    if (timer != 0) {
      g_source_remove(timer);
      timer = 0;
    }
  }
  return owl_variable_bool_set_default(v, newval);
}

/* note that changing the value of this will clobber 
 * any user setting of this */
int owl_variable_disable_ctrl_d_set(owl_variable *v, int newval)
{
  if (!owl_context_is_startup(owl_global_get_context(&g))) {
    if (newval == 2) {
      owl_function_command_norv("bindkey editmulti C-d command edit:delete-next-char");
    } else if (newval == 1) {
      owl_function_command_norv("bindkey editmulti C-d command edit:done-or-delete");
    } else {
      owl_function_command_norv("bindkey editmulti C-d command edit:done");
    }
  }  
  return owl_variable_int_set_default(v, newval);
}

int owl_variable_tty_set(owl_variable *v, const char *newval)
{
  owl_zephyr_set_locationinfo(g_get_host_name(), newval);
  return owl_variable_string_set_default(v, newval);
}

int owl_variable_default_exposure_set(owl_variable *v, const char *newval)
{
  return owl_zephyr_set_default_exposure(newval);
}

const char *owl_variable_default_exposure_get(const owl_variable *v)
{
  return owl_zephyr_get_default_exposure();
}

int owl_variable_exposure_set(owl_variable *v, const char *newval)
{
  int ret = owl_zephyr_set_exposure(newval);
  if (ret != 0)
    return ret;
  return owl_variable_string_set_default(v, owl_zephyr_normalize_exposure(newval));
}

/**************************************************************************/
/****************************** GENERAL ***********************************/
/**************************************************************************/

void owl_variable_dict_setup(owl_vardict *vd) {
  owl_dict_create(vd);
  owl_variable_add_defaults(vd);
}

CALLER_OWN GClosure *owl_variable_make_closure(owl_variable *v,
                                               GCallback fn,
                                               GClosureMarshal marshal) {
  GClosure *closure = g_cclosure_new_swap(fn, v, NULL);
  g_closure_set_marshal(closure,marshal);
  g_closure_ref(closure);
  g_closure_sink(closure);
  return closure;
}

void owl_variable_dict_add_variable(owl_vardict * vardict,
                                    owl_variable * var) {
  char *oldvalue = NULL;
  owl_variable *oldvar = owl_variable_get_var(vardict, var->name);
  /* Save the old value as a string. */
  if (oldvar) {
    oldvalue = owl_variable_get_tostring(oldvar);
  }
  owl_dict_insert_element(vardict, var->name, var, (void (*)(void *))owl_variable_delete);
  /* Restore the old value. */
  if (oldvalue) {
    owl_variable_set_fromstring(var, oldvalue, 0);
    g_free(oldvalue);
  }
}

static owl_variable *owl_variable_newvar(int type, const char *name, const char *summary, const char *description, const char *validsettings) {
  owl_variable *var = g_slice_new0(owl_variable);
  var->type = type;
  var->name = g_strdup(name);
  var->summary = g_strdup(summary);
  var->description = g_strdup(description);
  var->validsettings = g_strdup(validsettings);
  return var;
}

static void owl_variable_dict_newvar_int_full(owl_vardict *vd, const char *name, int default_val, const char *summary, const char *description, const char *validsettings, validate_int_t validate_fn, set_int_t set_fn, get_int_t get_fn)
{
  owl_variable *var = owl_variable_newvar(OWL_VARIABLE_INT, name, summary,
                                          description, validsettings);
  var->takes_on_off = false;
  var->get_fn = G_CALLBACK(get_fn ? get_fn : owl_variable_int_get_default);
  var->set_fn = G_CALLBACK(set_fn ? set_fn : owl_variable_int_set_default);
  var->validate_fn = G_CALLBACK(validate_fn ? validate_fn : owl_variable_int_validate_default);

  var->get_tostring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_int_get_tostring_default),
      g_cclosure_user_marshal_STRING__VOID);
  var->set_fromstring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_int_set_fromstring_default),
      g_cclosure_user_marshal_INT__STRING);

  g_value_init(&var->val, G_TYPE_INT);
  owl_variable_set_int(var, default_val);

  var->default_str = owl_variable_get_tostring(var);
  owl_variable_dict_add_variable(vd, var);
}

void owl_variable_dict_newvar_int(owl_vardict *vd, const char *name, int default_val, const char *summary, const char *description) {
  owl_variable_dict_newvar_int_full(vd, name, default_val, summary, description,
                                    "<int>", NULL, NULL, NULL);
}

static void owl_variable_dict_newvar_bool_full(owl_vardict *vd, const char *name, bool default_val, const char *summary, const char *description, validate_bool_t validate_fn, set_bool_t set_fn, get_bool_t get_fn)
{
  owl_variable *var = owl_variable_newvar(OWL_VARIABLE_BOOL, name, summary,
                                          description, "on,off");
  var->takes_on_off = true;
  var->get_fn = G_CALLBACK(get_fn ? get_fn : owl_variable_bool_get_default);
  var->set_fn = G_CALLBACK(set_fn ? set_fn : owl_variable_bool_set_default);
  var->validate_fn = G_CALLBACK(validate_fn ? validate_fn : owl_variable_bool_validate_default);

  var->get_tostring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_bool_get_tostring_default),
      g_cclosure_user_marshal_STRING__VOID);
  var->set_fromstring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_bool_set_fromstring_default),
      g_cclosure_user_marshal_INT__STRING);

  g_value_init(&var->val, G_TYPE_BOOLEAN);
  owl_variable_set_bool(var, default_val);

  var->default_str = owl_variable_get_tostring(var);
  owl_variable_dict_add_variable(vd, var);
}

void owl_variable_dict_newvar_bool(owl_vardict *vd, const char *name, bool default_val, const char *summary, const char *description) {
  owl_variable_dict_newvar_bool_full(vd, name, default_val, summary, description,
                                     NULL, NULL, NULL);
}

static void owl_variable_dict_newvar_string_full(owl_vardict *vd, const char *name, const char *default_val, const char *summary, const char *description, const char *validsettings, validate_string_t validate_fn, set_string_t set_fn, get_string_t get_fn)
{
  owl_variable *var = owl_variable_newvar(OWL_VARIABLE_STRING, name, summary,
                                          description, validsettings);
  var->takes_on_off = false;
  var->get_fn = G_CALLBACK(get_fn ? get_fn : owl_variable_string_get_default);
  var->set_fn = G_CALLBACK(set_fn ? set_fn : owl_variable_string_set_default);
  var->validate_fn = G_CALLBACK(validate_fn ? validate_fn : owl_variable_string_validate_default);

  var->get_tostring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_string_get_tostring_default),
      g_cclosure_user_marshal_STRING__VOID);
  var->set_fromstring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_string_set_fromstring_default),
      g_cclosure_user_marshal_INT__STRING);

  g_value_init(&var->val, G_TYPE_STRING);
  owl_variable_set_string(var, default_val);

  var->default_str = owl_variable_get_tostring(var);
  owl_variable_dict_add_variable(vd, var);
}

void owl_variable_dict_newvar_string(owl_vardict *vd, const char *name, const char *default_val, const char *summary, const char *description) {
  owl_variable_dict_newvar_string_full(vd, name, default_val, summary, description,
                                       "<string>", NULL, NULL, NULL);
}

void owl_variable_dict_newvar_path(owl_vardict *vd, const char *name, const char *default_val, const char *summary, const char *description) {
  owl_variable_dict_newvar_string_full(vd, name, default_val, summary, description,
                                       "<path>", NULL, NULL, NULL);
}

static void owl_variable_dict_newvar_enum_full(owl_vardict *vd, const char *name, int default_val, const char *summary, const char *description, const char *validsettings, validate_int_t validate_fn, set_int_t set_fn, get_int_t get_fn)
{
  owl_variable *var = owl_variable_newvar(OWL_VARIABLE_INT, name, summary,
                                          description, validsettings);
  var->takes_on_off = false;
  var->get_fn = G_CALLBACK(get_fn ? get_fn : owl_variable_int_get_default);
  var->set_fn = G_CALLBACK(set_fn ? set_fn : owl_variable_int_set_default);
  var->validate_fn = G_CALLBACK(validate_fn ? validate_fn : owl_variable_enum_validate);

  var->get_tostring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_enum_get_tostring),
      g_cclosure_user_marshal_STRING__VOID);
  var->set_fromstring_fn = owl_variable_make_closure(
      var, G_CALLBACK(owl_variable_enum_set_fromstring),
      g_cclosure_user_marshal_INT__STRING);

  g_value_init(&var->val, G_TYPE_INT);
  owl_variable_set_int(var, default_val);

  var->default_str = owl_variable_get_tostring(var);
  owl_variable_dict_add_variable(vd, var);
}

void owl_variable_dict_newvar_enum(owl_vardict *vd, const char *name, int default_val, const char *summary, const char *description, const char *validset) {
  owl_variable_dict_newvar_enum_full(vd, name, default_val, summary, description,
                                     validset, NULL, NULL, NULL);
}

void owl_variable_dict_newvar_other(owl_vardict *vd, const char *name, const char *summary, const char *description, const char *validsettings, bool takes_on_off, GClosure *get_tostring_fn, GClosure *set_fromstring_fn)
{
  owl_variable *var = owl_variable_newvar(OWL_VARIABLE_OTHER, name, summary,
                                          description, validsettings);
  var->takes_on_off = takes_on_off;

  var->get_tostring_fn = g_closure_ref(get_tostring_fn);
  g_closure_sink(get_tostring_fn);

  var->set_fromstring_fn = g_closure_ref(set_fromstring_fn);
  g_closure_sink(set_fromstring_fn);

  var->default_str = owl_variable_get_tostring(var);

  /* Note: this'll overwrite any existing variable of that name, even a C one,
     but it's consistent with previous behavior and commands. */
  owl_variable_dict_add_variable(vd, var);
}

void owl_variable_dict_cleanup(owl_vardict *d)
{
  owl_dict_cleanup(d, (void (*)(void *))owl_variable_delete);
}

CALLER_OWN GPtrArray *owl_variable_dict_get_names(const owl_vardict *d) {
  return owl_dict_get_keys(d);
}

void owl_variable_delete(owl_variable *v)
{
  g_free(v->name);
  g_free(v->summary);
  g_free(v->description);
  g_free(v->default_str);
  g_free(v->validsettings);
  if (v->type != OWL_VARIABLE_OTHER)
    g_value_unset(&(v->val));
  g_closure_unref(v->get_tostring_fn);
  g_closure_unref(v->set_fromstring_fn);

  g_slice_free(owl_variable, v);
}


const char *owl_variable_get_name(const owl_variable *v)
{
  return v->name;
}

const char *owl_variable_get_description(const owl_variable *v) {
  return v->description;
}

const char *owl_variable_get_summary(const owl_variable *v) {
  return v->summary;
}

const char *owl_variable_get_validsettings(const owl_variable *v) {
  return v->validsettings;
}

bool owl_variable_takes_on_off(const owl_variable *v) {
  return v->takes_on_off;
}

int owl_variable_get_type(const owl_variable *v)
{
  return v->type;
}

/* returns 0 on success, prints a status msg if msg is true */
int owl_variable_set_fromstring(owl_variable *v, const char *value, int msg) {
  char *tostring;
  GValue values[] = {G_VALUE_INIT, G_VALUE_INIT};
  GValue return_box = G_VALUE_INIT;
  int set_successfully;

  g_value_init(&values[0], G_TYPE_POINTER);
  g_value_set_pointer(&values[0], NULL);
  g_value_init(&values[1], G_TYPE_STRING);
  g_value_set_static_string(&values[1], value);
  g_value_init(&return_box, G_TYPE_INT);
  g_closure_invoke(v->set_fromstring_fn, &return_box, 2, values, NULL);

  set_successfully = g_value_get_int(&return_box);
  if (0 != set_successfully) {
    if (msg) owl_function_error("Unable to set %s (must be %s)", owl_variable_get_name(v),
                                owl_variable_get_validsettings(v));
  } else if (msg) {
    tostring = owl_variable_get_tostring(v);
    if (tostring) {
      owl_function_makemsg("%s = '%s'", owl_variable_get_name(v), tostring);
    } else {
      owl_function_makemsg("%s = <null>", owl_variable_get_name(v));
    }
    g_free(tostring);
  }

  g_value_unset(&return_box);
  g_value_unset(&values[1]);
  g_value_unset(&values[0]);
  return set_successfully;
}
 
int owl_variable_set_string(owl_variable *v, const char *newval)
{
  g_return_val_if_fail(v->type == OWL_VARIABLE_STRING, -1);

  set_string_t cb = (set_string_t) v->set_fn;
  return cb(v, newval);
}

int owl_variable_set_int(owl_variable *v, int newval)
{
  g_return_val_if_fail(v->type == OWL_VARIABLE_INT, -1);

  set_int_t cb = (set_int_t) v->set_fn;
  return cb(v, newval);
}

int owl_variable_set_bool(owl_variable *v, bool newval) {
  g_return_val_if_fail(v->type == OWL_VARIABLE_BOOL, -1);

  set_bool_t cb = (set_bool_t) v->set_fn;
  return cb(v, newval);
}

int owl_variable_set_bool_on(owl_variable *v)
{
  if (v->type != OWL_VARIABLE_BOOL) return -1;
  return owl_variable_set_bool(v, true);
}

int owl_variable_set_bool_off(owl_variable *v)
{
  if (v->type != OWL_VARIABLE_BOOL) return -1;
  return owl_variable_set_bool(v, false);
}

CALLER_OWN char *owl_variable_get_tostring(const owl_variable *v)
{
  GValue instance = G_VALUE_INIT;
  GValue tostring_box = G_VALUE_INIT;
  char *ret = NULL;

  g_value_init(&instance, G_TYPE_POINTER);
  g_value_set_pointer(&instance, NULL);
  g_value_init(&tostring_box, G_TYPE_STRING);
  g_closure_invoke(v->get_tostring_fn, &tostring_box, 1, &instance, NULL);

  ret = g_value_dup_string(&tostring_box);

  g_value_unset(&tostring_box);
  g_value_unset(&instance);
  return ret;
}

const char *owl_variable_get_default_tostring(const owl_variable *v)
{
  return v->default_str;
}

owl_variable *owl_variable_get_var(const owl_vardict *d, const char *name)
{
  return owl_dict_find_element(d, name);
}

const char *owl_variable_get_string(const owl_variable *v)
{
  g_return_val_if_fail(v->type == OWL_VARIABLE_STRING, NULL);

  get_string_t cb = (get_string_t) v->get_fn;
  return cb(v);
}

int owl_variable_get_int(const owl_variable *v)
{
  g_return_val_if_fail(v->type == OWL_VARIABLE_INT, 0);

  get_int_t cb = (get_int_t) v->get_fn;
  return cb(v);
}

bool owl_variable_get_bool(const owl_variable *v)
{
  g_return_val_if_fail(v->type == OWL_VARIABLE_BOOL, FALSE);

  get_bool_t cb = (get_bool_t) v->get_fn;
  return cb(v);
}

void owl_variable_describe(const owl_variable *v, owl_fmtext *fm)
{
  const char *default_str = owl_variable_get_default_tostring(v);
  char *default_buf;

  if (default_str)
    default_buf = g_strdup_printf("'%s'", default_str);
  else
    default_buf = g_strdup("<null>");
  owl_fmtext_appendf_normal(fm, OWL_TABSTR "%-20s - %s (default: %s)\n",
                            owl_variable_get_name(v),
                            owl_variable_get_summary(v), default_buf);
  g_free(default_buf);
}

void owl_variable_get_help(const owl_variable *v, owl_fmtext *fm) {
  char *tostring;
  const char *default_str;

  owl_fmtext_append_bold(fm, "OWL VARIABLE\n\n");
  owl_fmtext_append_normal(fm, OWL_TABSTR);
  owl_fmtext_append_normal(fm, owl_variable_get_name(v));
  owl_fmtext_append_normal(fm, " - ");
  owl_fmtext_append_normal(fm, owl_variable_get_summary(v));
  owl_fmtext_append_normal(fm, "\n\n");

  owl_fmtext_append_normal(fm, "Current:        ");
  tostring = owl_variable_get_tostring(v);
  owl_fmtext_append_normal(fm, (tostring ? tostring : "<null>"));
  g_free(tostring);
  owl_fmtext_append_normal(fm, "\n\n");

  default_str = owl_variable_get_default_tostring(v);
  owl_fmtext_append_normal(fm, "Default:        ");
  owl_fmtext_append_normal(fm, (default_str ? default_str : "<null>"));
  owl_fmtext_append_normal(fm, "\n\n");

  owl_fmtext_append_normal(fm, "Valid Settings: ");
  owl_fmtext_append_normal(fm, owl_variable_get_validsettings(v));
  owl_fmtext_append_normal(fm, "\n\n");

  if (v->description && *v->description) {
    owl_fmtext_append_normal(fm, "Description:\n");
    owl_fmtext_append_normal(fm, owl_variable_get_description(v));
    owl_fmtext_append_normal(fm, "\n\n");
  }
}




/**************************************************************************/
/*********************** GENERAL TYPE-SPECIFIC ****************************/
/**************************************************************************/

/* default common functions */

const char *owl_variable_string_get_default(const owl_variable *v) {
  return g_value_get_string(&(v->val));
}

int owl_variable_int_get_default(const owl_variable *v) {
  return g_value_get_int(&(v->val));
}

bool owl_variable_bool_get_default(const owl_variable *v) {
  return g_value_get_boolean(&(v->val));
}

/* default functions for booleans */

int owl_variable_bool_validate_default(const owl_variable *v, bool newval) {
  return (newval == 1) || (newval == 0);
}

int owl_variable_bool_set_default(owl_variable *v, bool newval) {
  if (!((validate_bool_t)v->validate_fn)(v, newval))
    return -1;

  g_value_set_boolean(&(v->val), newval);
  return(0);
}

int owl_variable_bool_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) {
  bool i;
  if (!strcmp(newval, "on")) {
    i = true;
  } else if (!strcmp(newval, "off")) {
    i = false;
  } else {
    return(-1);
  }

  return owl_variable_set_bool(v, i);
}

CALLER_OWN char *owl_variable_bool_get_tostring_default(const owl_variable *v, void *dummy)
{
  return g_strdup(owl_variable_get_bool(v) ? "on" : "off");
}

/* default functions for integers */

int owl_variable_int_validate_default(const owl_variable *v, int newval)
{
  return (1);
}

int owl_variable_int_set_default(owl_variable *v, int newval) {
  if (!((validate_int_t)v->validate_fn)(v, newval))
    return -1;

  g_value_set_int(&(v->val), newval);
  return(0);
}

int owl_variable_int_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) {
  int i;
  char *ep;
  i = strtol(newval, &ep, 10);
  if (*ep || ep==newval) return(-1);
  return owl_variable_set_int(v, i);
}

CALLER_OWN char *owl_variable_int_get_tostring_default(const owl_variable *v, void *dummy)
{
  return g_strdup_printf("%d", owl_variable_get_int(v));
}

/* default functions for enums (a variant of integers) */

int owl_variable_enum_validate(const owl_variable *v, int newval) {
  char **enums;
  int nenums, val;
  enums = g_strsplit_set(v->validsettings, ",", 0);
  nenums = g_strv_length(enums);
  g_strfreev(enums);
  val = newval;
  if (val < 0 || val >= nenums) {
    return(0);
  }
  return(1);
}

int owl_variable_enum_set_fromstring(owl_variable *v, const char *newval, void *dummy) {
  char **enums;
  int i, val=-1;
  if (newval == NULL) return(-1);
  enums = g_strsplit_set(v->validsettings, ",", 0);
  for (i = 0; enums[i] != NULL; i++) {
    if (0==strcmp(newval, enums[i])) {
      val = i;
    }
  }
  g_strfreev(enums);
  if (val == -1) return(-1);
  return owl_variable_set_int(v, val);
}

CALLER_OWN char *owl_variable_enum_get_tostring(const owl_variable *v, void *dummy)
{
  char **enums;
  int nenums, i;
  char *tostring;

  enums = g_strsplit_set(v->validsettings, ",", 0);
  nenums = g_strv_length(enums);
  i = owl_variable_get_int(v);
  if (i<0 || i>=nenums) {
    g_strfreev(enums);
    return g_strdup_printf("<invalid:%d>", i);
  }
  tostring = g_strdup(enums[i]);
  g_strfreev(enums);
  return tostring;
}

/* default functions for stringeans */

int owl_variable_string_validate_default(const owl_variable *v, const char *newval) {
  if (newval == NULL) return(0);
  else return (1);
}

int owl_variable_string_set_default(owl_variable *v, const char *newval) {
  if (!((validate_string_t)v->validate_fn)(v, newval))
    return -1;

  g_value_set_string(&(v->val), newval);
  return(0);
}

int owl_variable_string_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) 
{
  return owl_variable_set_string(v, newval);
}

CALLER_OWN char *owl_variable_string_get_tostring_default(const owl_variable *v, void *dummy)
{
  return g_strdup(owl_variable_get_string(v));
}

