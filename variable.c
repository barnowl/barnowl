#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include "owl.h"

#define OWLVAR_BOOL(name,default,summary,description) \
        { name, OWL_VARIABLE_BOOL, NULL, default, "on,off", summary,description, NULL, \
        NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_BOOL_FULL(name,default,summary,description,validate,set,get) \
        { name, OWL_VARIABLE_BOOL, NULL, default, "on,off", summary,description, NULL, \
        validate, set, NULL, get, NULL, NULL }

#define OWLVAR_INT(name,default,summary,description) \
        { name, OWL_VARIABLE_INT, NULL, default, "<int>", summary,description, NULL, \
        NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_INT_FULL(name,default,summary,description,validset,validate,set,get) \
        { name, OWL_VARIABLE_INT, NULL, default, validset, summary,description, NULL, \
        validate, set, NULL, get, NULL, NULL }

#define OWLVAR_PATH(name,default,summary,description) \
        { name, OWL_VARIABLE_STRING, default, 0, "<path>", summary,description,  NULL, \
        NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_STRING(name,default,summary,description) \
        { name, OWL_VARIABLE_STRING, default, 0, "<string>", summary,description, NULL, \
        NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_STRING_FULL(name,default,summary,description,validate,set,get) \
        { name, OWL_VARIABLE_STRING, default, 0, "<string>", summary,description, NULL, \
        validate, set, NULL, get, NULL, NULL }

/* enums are really integers, but where validset is a comma-separated
 * list of strings which can be specified.  The tokens, starting at 0,
 * correspond to the values that may be specified. */
#define OWLVAR_ENUM(name,default,summary,description,validset) \
        { name, OWL_VARIABLE_INT, NULL, default, validset, summary,description, NULL, \
        owl_variable_enum_validate, \
        NULL, owl_variable_enum_set_fromstring, \
        NULL, owl_variable_enum_get_tostring, \
        NULL }

#define OWLVAR_ENUM_FULL(name,default,summary,description,validset,validate, set, get) \
        { name, OWL_VARIABLE_INT, NULL, default, validset, summary,description, NULL, \
        validate, \
        set, owl_variable_enum_set_fromstring, \
        get, owl_variable_enum_get_tostring, \
        NULL }

static owl_variable variables_to_init[] = {

  OWLVAR_STRING( "personalbell" /* %OwlVarStub */, "off",
		 "ring the terminal bell when personal messages are received",
		 "Can be set to 'on', 'off', or the name of a filter which\n"
		 "messages need to match in order to ring the bell"),

  OWLVAR_BOOL( "bell" /* %OwlVarStub */, 1,
	       "enable / disable the terminal bell", "" ),

  OWLVAR_BOOL_FULL( "debug" /* %OwlVarStub */, OWL_DEBUG,
		    "whether debugging is enabled",
		    "If set to 'on', debugging messages are logged to the\n"
		    "file specified by the debugfile variable.\n",
		    NULL, owl_variable_debug_set, NULL),

  OWLVAR_BOOL( "startuplogin" /* %OwlVarStub */, 1,
	       "send a login message when owl starts", "" ),

  OWLVAR_BOOL( "shutdownlogout" /* %OwlVarStub */, 1,
	       "send a logout message when owl exits", "" ),

  OWLVAR_BOOL( "rxping" /* %OwlVarStub */, 0,
	       "display received pings", "" ),

  OWLVAR_BOOL( "txping" /* %OwlVarStub */, 1,
	       "send pings", "" ),

  OWLVAR_BOOL( "sepbar_disable" /* %OwlVarStub */, 0,
	       "disable printing information in the separator bar", "" ),

  OWLVAR_BOOL( "smartstrip" /* %OwlVarStub */, 1,
	       "strip kerberos instance for reply", ""),

  OWLVAR_BOOL( "newlinestrip" /* %OwlVarStub */, 1,
	       "strip leading and trailing newlines", ""),

  OWLVAR_BOOL( "displayoutgoing" /* %OwlVarStub */, 1,
	       "display outgoing messages", "" ),

  OWLVAR_BOOL( "loginsubs" /* %OwlVarStub */, 1,
	       "load logins from .anyone on startup", "" ),

  OWLVAR_BOOL( "logging" /* %OwlVarStub */, 0,
	       "turn personal logging on or off", 
	       "If this is set to on, personal messages are\n"
	       "logged in the directory specified\n"
	       "by the 'logpath' variable.  The filename in that\n"
	       "directory is derived from the sender of the message.\n" ),

  OWLVAR_BOOL( "classlogging" /* %OwlVarStub */, 0,
	       "turn class logging on or off",
	       "If this is set to on, class messages are\n"
	       "logged in the directory specified\n"
	       "by the 'classlogpath' variable.\n" 
	       "The filename in that directory is derived from\n"
	       "the name of the class to which the message was sent.\n" ),

  OWLVAR_ENUM( "loggingdirection" /* %OwlVarStub */, OWL_LOGGING_DIRECTION_BOTH,
	       "specifices which kind of messages should be logged",
	       "Can be one of 'both', 'in', or 'out'.  If 'in' is\n"
	       "selected, only incoming messages are logged, if 'out'\n"
	       "is selected only outgoing messages are logged.  If 'both'\n"
	       "is selected both incoming and outgoing messages are\n"
	       "logged.",
	       "both,in,out"),

  OWLVAR_BOOL( "colorztext" /* %OwlVarStub */, 1,
	       "allow @color() in zephyrs to change color",
	       "Note that only messages received after this variable\n"
	       "is set will be affected." ),

  OWLVAR_BOOL( "fancylines" /* %OwlVarStub */, 1,
	       "Use 'nice' line drawing on the terminal.",
	       "If turned off, dashes, pipes and pluses will be used\n"
	       "to draw lines on the screen.  Useful when the terminal\n"
	       "is causing problems" ),

  OWLVAR_BOOL( "zcrypt" /* %OwlVarStub */, 1,
	       "Do automatic zcrypt processing",
	       "" ),

  OWLVAR_BOOL_FULL( "pseudologins" /* %OwlVarStub */, 0,
		    "Enable zephyr pseudo logins",
		    "When this is enabled, Owl will periodically check the zephyr\n"
		    "location of users in your .anyone file.  If a user is present\n"
		    "but sent no login message, or a user is not present that sent no\n"
		    "logout message, a pseudo login or logout message wil be created\n",
		    NULL, owl_variable_pseudologins_set, NULL),

  OWLVAR_BOOL( "ignorelogins" /* %OwlVarStub */, 0,
	       "Enable printing of login notifications",
	       "When this is enabled, Owl will print login and logout notifications\n"
	       "for AIM, zephyr, or other protocols.  If disabled Owl will not print\n"
	       "login or logout notifications.\n"),

  OWLVAR_STRING( "logfilter" /* %OwlVarStub */, "",
		 "name of a filter controlling which messages to log",

		 "If non empty, any messages matching the given filter will be logged.\n"
		 "This is a completely separate mechanisim from the other logging\n"
		 "variables like logging, classlogging, loglogins, loggingdirection,\n"
		 "etc.  If you want this variable to control all logging, make sure\n"
		 "all other logging variables are in their default state.\n"),

  OWLVAR_BOOL( "loglogins" /* %OwlVarStub */, 0,
	       "Enable logging of login notifications",
	       "When this is enabled, Owl will login login and logout notifications\n"
	       "for AIM, zephyr, or other protocols.  If disabled Owl will not print\n"
	       "login or logout notifications.\n"),

  OWLVAR_ENUM_FULL( "disable-ctrl-d" /* %OwlVarStub:lockout_ctrld */, 1,
		    "don't send zephyrs on C-d",
		    "If set to 'off', C-d won't send a zephyr from the edit\n"
		    "window.  If set to 'on', C-d will always send a zephyr\n"
		    "being composed in the edit window.  If set to 'middle',\n"
		    "C-d will only ever send a zephyr if the cursor is at\n"
		    "the end of the message being composed.\n\n"
		    "Note that this works by changing the C-d keybinding\n"
		    "in the editmulti keymap.\n",
		    "off,middle,on",
		    NULL, owl_variable_disable_ctrl_d_set, NULL),

  OWLVAR_PATH( "logpath" /* %OwlVarStub */, "~/zlog/people",
	       "path for logging personal zephyrs", 
	       "Specifies a directory which must exist.\n"
	       "Files will be created in the directory for each sender.\n"),

  OWLVAR_PATH( "classlogpath" /* %OwlVarStub:classlogpath */, "~/zlog/class",
	       "path for logging class zephyrs",
	       "Specifies a directory which must exist.\n"
	       "Files will be created in the directory for each class.\n"),

  OWLVAR_PATH( "debug_file" /* %OwlVarStub */, OWL_DEBUG_FILE,
	       "path for logging debug messages when debugging is enabled",
	       "This file will be logged to if 'debug' is set to 'on'.\n"),
  
  OWLVAR_PATH( "zsigproc" /* %OwlVarStub:zsigproc */, NULL,
	       "name of a program to run that will generate zsigs",
	       "This program should produce a zsig on stdout when run.\n"
	       "Note that it is important that this program not block.\n\n"
               "See the documentation for 'zsig' for more information about\n"
               "how the outgoing zsig is chosen."
               ),

  OWLVAR_PATH( "newmsgproc" /* %OwlVarStub:newmsgproc */, NULL,
	       "name of a program to run when new messages are present",
	       "The named program will be run when owl recevies new.\n"
	       "messages.  It will not be run again until the first\n"
	       "instance exits"),

  OWLVAR_STRING( "zsender" /* %OwlVarStub */, "",
             "zephyr sender name",
         "Allows you to customize the outgoing username in\n"
         "zephyrs.  If this is unset, it will use your Kerberos\n"
         "principal. Note that customizing the sender name will\n"
         "cause your zephyrs to be sent unauthenticated."),

  OWLVAR_STRING( "zsigfunc" /* %OwlVarStub */, "BarnOwl::default_zephyr_signature()",
		 "zsig perl function",
		 "Called every time you start a zephyrgram without an\n"
		 "explicit zsig.  The default setting implements the policy\n"
		 "descripted in the documentation for the 'zsig' variable.\n"),

  OWLVAR_STRING( "zsig" /* %OwlVarStub */, "",
	         "zephyr signature",
		 "The zsig to get on outgoing messages. If this variable is\n"
		 "unset, 'zsigproc' will be run to generate a zsig. If that is\n"
		 "also unset, the 'zwrite-signature' zephyr variable will be\n"
		 "used instead.\n"),

  OWLVAR_STRING( "appendtosepbar" /* %OwlVarStub */, "",
	         "string to append to the end of the sepbar",
		 "The sepbar is the bar separating the top and bottom\n"
		 "of the owl screen.  Any string specified here will\n"
		 "be displayed on the right of the sepbar\n"),

  OWLVAR_BOOL( "zaway" /* %OwlVarStub */, 0,
	       "turn zaway on or off", "" ),

  OWLVAR_STRING( "zaway_msg" /* %OwlVarStub */, 
		 OWL_DEFAULT_ZAWAYMSG,
	         "zaway msg for responding to zephyrs when away", "" ),

  OWLVAR_STRING( "zaway_msg_default" /* %OwlVarStub */, 
		 OWL_DEFAULT_ZAWAYMSG,
	         "default zaway message", "" ),

  OWLVAR_BOOL_FULL( "aaway" /* %OwlVarStub */, 0,
		    "Set AIM away status",
		    "",
		    NULL, owl_variable_aaway_set, NULL),

  OWLVAR_STRING( "aaway_msg" /* %OwlVarStub */, 
		 OWL_DEFAULT_AAWAYMSG,
	         "AIM away msg for responding when away", "" ),

  OWLVAR_STRING( "aaway_msg_default" /* %OwlVarStub */, 
		 OWL_DEFAULT_AAWAYMSG,
	         "default AIM away message", "" ),

  OWLVAR_STRING( "view_home" /* %OwlVarStub */, "all",
	         "home view to switch to after 'X' and 'V'", 
		 "SEE ALSO: view, filter\n" ),

  OWLVAR_STRING( "alert_filter" /* %OwlVarStub */, "none",
		 "filter on which to trigger alert actions",
		 "" ),

  OWLVAR_STRING( "alert_action" /* %OwlVarStub */, "nop",
		 "owl command to execute for alert actions",
		 "" ),

  OWLVAR_STRING_FULL( "tty" /* %OwlVarStub */, "", "tty name for zephyr location", "",
		      NULL, owl_variable_tty_set, NULL),

  OWLVAR_STRING( "default_style" /* %OwlVarStub */, "__unspecified__",
		 "name of the default formatting style",
		 "This sets the default message formatting style.\n"
		 "Styles may be created with the 'style' command.\n"
		 "Some built-in styles include:\n"
		 "   default  - the default owl formatting\n"
		 "   oneline  - one line per-message\n"
		 "   perl     - legacy perl interface\n"
		 "\nSEE ALSO: style, show styles, view -s <style>\n"
		 ),


  OWLVAR_INT(    "edit:maxfillcols" /* %OwlVarStub:edit_maxfillcols */, 70,
	         "maximum number of columns for M-q (edit:fill-paragraph) to fill text to",
                 "This specifies the maximum number of columns for M-q to fill text\n"
                 "to.  If set to 0, M-q will wrap to the width of the window, and\n"
                 "values less than 0 disable M-q entirely.\n"),

  OWLVAR_INT(    "edit:maxwrapcols" /* %OwlVarStub:edit_maxwrapcols */, 70,
	         "maximum number of columns for line-wrapping",
                 "This specifies the maximum number of columns for\n"
                 "auto-line-wrapping.  If set to 0, text will be wrapped at the\n"
                 "window width. Values less than 0 disable automatic wrapping.\n"
                 "\n"
                 "As a courtesy to recipients, it is recommended that outgoing\n"
                 "Zephyr messages be no wider than 70 columns.\n"),

  OWLVAR_INT( "aim_ignorelogin_timer" /* %OwlVarStub */, 15,
	      "number of seconds after AIM login to ignore login messages",
	      "This specifies the number of seconds to wait after an\n"
	      "AIM login before allowing the recipt of AIM login notifications.\n"
	      "By default this is set to 15.  If you would like to view login\n"
	      "notifications of buddies as soon as you login, set it to 0 instead."),

	      
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
		   ),

  OWLVAR_INT( "typewindelta" /* %OwlVarStub */, 0,
		  "number of lines to add to the typing window when in use",
		   "On small screens you may want the typing window to\n"
		   "auto-hide when not entering a command or message.\n"
		   "This variable is the number of lines to add to the\n"
           "typing window when it is in use; you can then set\n"
           "typewinsize to 1.\n\n"
           "This works a lot better with a non-default scrollmode;\n"
           "try :set scrollmode pagedcenter.\n"),

  OWLVAR_ENUM( "scrollmode" /* %OwlVarStub */, OWL_SCROLLMODE_NORMAL,
	       "how to scroll up and down",
	       "This controls how the screen is scrolled as the\n"
	       "cursor moves between messages being displayed.\n"
	       "The following modes are supported:\n\n"
	       "   normal      - This is the owl default.  Scrolling happens\n"
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
	       "normal,top,neartop,center,paged,pagedcenter" ),

  OWLVAR_BOOL( "narrow-related" /* %OwlVarStub:narrow_related */, 1,
               "Make smartnarrow use broader filters",
               "Causes smartfiler to narrow to messages \"related\" to \n"
               "the current message, as well as ones to the same place.\n\n"
               "for Zephyr, this controls whether to narrow to e.g. class-help or\n"
               "class-help.d alone, or to related-class-help, which includes\n"
               "help, unhelp, help.d, etc.\n\nDefault is true (include unclasses, etc.).\n" ),

  OWLVAR_BOOL( "_followlast" /* %OwlVarStub */, 0,
	       "enable automatic following of the last zephyr",
	       "If the cursor is at the last message, it will\n"
	       "continue to follow the last message if this is set.\n"
	       "Note that this is currently risky as you might accidentally\n"
	       "delete a message right as it came in.\n" ),

  /* This MUST be last... */
  { NULL, 0, NULL, 0, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL }

};

/**************************************************************************/
/*********************** SPECIFIC TO VARIABLES ****************************/
/**************************************************************************/


/* commonly useful */

int owl_variable_int_validate_gt0(const owl_variable *v, const void *newval)
{
  if (newval == NULL) return(0);
  else if (*(const int*)newval < 1) return(0);
  else return (1);
}

int owl_variable_int_validate_positive(const owl_variable *v, const void *newval)
{
  if (newval == NULL) return(0);
  else if (*(const int*)newval < 0) return(0);
  else return (1);
}

/* typewinsize */
int owl_variable_typewinsize_set(owl_variable *v, const void *newval)
{
  int rv;
  rv = owl_variable_int_set_default(v, newval);
  if (0 == rv) owl_global_set_relayout_pending(&g);
  return(rv);
}

/* debug (cache value in g->debug) */
int owl_variable_debug_set(owl_variable *v, const void *newval)
{
  if (newval && (*(const int*)newval == 1 || *(const int*)newval == 0)) {
    g.debug = *(const int*)newval;
  }
  return owl_variable_bool_set_default(v, newval);
}

/* When 'aaway' is changed, need to notify the AIM server */
int owl_variable_aaway_set(owl_variable *v, const void *newval)
{
  if (newval) {
    if (*(const int*)newval == 1) {
      owl_aim_set_awaymsg(owl_global_get_aaway_msg(&g));
    } else if (*(const int*)newval == 0) {
      owl_aim_set_awaymsg("");
    }
  }
  return owl_variable_bool_set_default(v, newval);
}

int owl_variable_pseudologins_set(owl_variable *v, const void *newval)
{
  static owl_timer *timer = NULL;
  if (newval) {
    if (*(const int*)newval == 1) {
      owl_function_zephyr_buddy_check(0);
      if (timer == NULL) {
        timer = owl_select_add_timer(180, 180, owl_zephyr_buddycheck_timer, NULL, NULL);
      }
    } else {
      if (timer != NULL) {
        owl_select_remove_timer(timer);
        timer = NULL;
      }
    }
  }
  return owl_variable_bool_set_default(v, newval);
}

/* note that changing the value of this will clobber 
 * any user setting of this */
int owl_variable_disable_ctrl_d_set(owl_variable *v, const void *newval)
{
  if (newval && !owl_context_is_startup(owl_global_get_context(&g))) {
    if (*(const int*)newval == 2) {
      owl_function_command_norv("bindkey editmulti C-d command edit:delete-next-char");
    } else if (*(const int*)newval == 1) {
      owl_function_command_norv("bindkey editmulti C-d command edit:done-or-delete");
    } else {
      owl_function_command_norv("bindkey editmulti C-d command edit:done");
    }
  }  
  return owl_variable_int_set_default(v, newval);  
}

int owl_variable_tty_set(owl_variable *v, const void *newval)
{
  owl_zephyr_set_locationinfo(owl_global_get_hostname(&g), newval);
  return(owl_variable_string_set_default(v, newval));
}


/**************************************************************************/
/****************************** GENERAL ***********************************/
/**************************************************************************/

int owl_variable_dict_setup(owl_vardict *vd) {
  owl_variable *var, *cur;
  if (owl_dict_create(vd)) return(-1);
  for (var = variables_to_init; var->name != NULL; var++) {
    cur = owl_malloc(sizeof(owl_variable));
    *cur = *var;
    switch (cur->type) {
    case OWL_VARIABLE_OTHER:
      cur->set_fn(cur, cur->pval_default);
      break;
    case OWL_VARIABLE_STRING:
      if (!cur->validate_fn) 
	cur->validate_fn = owl_variable_string_validate_default;
      if (!cur->set_fn) 
	cur->set_fn = owl_variable_string_set_default;
      if (!cur->set_fromstring_fn) 
	cur->set_fromstring_fn = owl_variable_string_set_fromstring_default;
      if (!cur->get_fn) 
	cur->get_fn = owl_variable_get_default;
      if (!cur->get_tostring_fn) 
	cur->get_tostring_fn = owl_variable_string_get_tostring_default;      
      if (!cur->delete_fn)
	cur->delete_fn = owl_variable_delete_default;
      cur->set_fn(cur, cur->pval_default);
      break;
    case OWL_VARIABLE_BOOL:
      if (!cur->validate_fn) 
	cur->validate_fn = owl_variable_bool_validate_default;
      if (!cur->set_fn) 
	cur->set_fn = owl_variable_bool_set_default;
      if (!cur->set_fromstring_fn) 
	cur->set_fromstring_fn = owl_variable_bool_set_fromstring_default;
      if (!cur->get_fn) 
	cur->get_fn = owl_variable_get_default;
      if (!cur->get_tostring_fn) 
	cur->get_tostring_fn = owl_variable_bool_get_tostring_default;      
      if (!cur->delete_fn)
	cur->delete_fn = owl_variable_delete_default;
      cur->val = owl_malloc(sizeof(int));
      cur->set_fn(cur, &cur->ival_default);
      break;
    case OWL_VARIABLE_INT:
      if (!cur->validate_fn) 
	cur->validate_fn = owl_variable_int_validate_default;
      if (!cur->set_fn) 
	cur->set_fn = owl_variable_int_set_default;
      if (!cur->set_fromstring_fn) 
	cur->set_fromstring_fn = owl_variable_int_set_fromstring_default;
      if (!cur->get_fn) 
	cur->get_fn = owl_variable_get_default;
      if (!cur->get_tostring_fn) 
	cur->get_tostring_fn = owl_variable_int_get_tostring_default;      
      if (!cur->delete_fn)
	cur->delete_fn = owl_variable_delete_default;
      cur->val = owl_malloc(sizeof(int));
      cur->set_fn(cur, &cur->ival_default);
      break;
    default:
      fprintf(stderr, "owl_variable_setup: invalid variable type\n");
      return(-2);
    }
    owl_dict_insert_element(vd, cur->name, cur, NULL);
  }
  return 0;
}

void owl_variable_dict_add_variable(owl_vardict * vardict,
                                    owl_variable * var) {
  owl_dict_insert_element(vardict, var->name, var, (void (*)(void *))owl_variable_delete);
}

owl_variable * owl_variable_newvar(const char *name, const char *summary, const char * description) {
  owl_variable * var = owl_malloc(sizeof(owl_variable));
  memset(var, 0, sizeof(owl_variable));
  var->name = owl_strdup(name);
  var->summary = owl_strdup(summary);
  var->description = owl_strdup(description);
  return var;
}

void owl_variable_update(owl_variable *var, const char *summary, const char *desc) {
  if(var->summary) owl_free(var->summary);
  var->summary = owl_strdup(summary);
  if(var->description) owl_free(var->description);
  var->description = owl_strdup(desc);
}

void owl_variable_dict_newvar_string(owl_vardict * vd, const char *name, const char *summ, const char * desc, const char * initval) {
  owl_variable *old = owl_variable_get_var(vd, name, OWL_VARIABLE_STRING);
  if(old) {
    owl_variable_update(old, summ, desc);
    if(old->pval_default) owl_free(old->pval_default);
    old->pval_default = owl_strdup(initval);
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_STRING;
    var->pval_default = owl_strdup(initval);
    var->set_fn = owl_variable_string_set_default;
    var->set_fromstring_fn = owl_variable_string_set_fromstring_default;
    var->get_fn = owl_variable_get_default;
    var->get_tostring_fn = owl_variable_string_get_tostring_default;
    var->delete_fn = owl_variable_delete_default;
    var->set_fn(var, initval);
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_newvar_int(owl_vardict * vd, const char *name, const char *summ, const char * desc, int initval) {
  owl_variable *old = owl_variable_get_var(vd, name, OWL_VARIABLE_INT);
  if(old) {
    owl_variable_update(old, summ, desc);
    old->ival_default = initval;
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_INT;
    var->ival_default = initval;
    var->validate_fn = owl_variable_int_validate_default;
    var->set_fn = owl_variable_int_set_default;
    var->set_fromstring_fn = owl_variable_int_set_fromstring_default;
    var->get_fn = owl_variable_get_default;
    var->get_tostring_fn = owl_variable_int_get_tostring_default;
    var->delete_fn = owl_variable_delete_default;
    var->val = owl_malloc(sizeof(int));
    var->set_fn(var, &initval);
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_newvar_bool(owl_vardict * vd, const char *name, const char *summ, const char * desc, int initval) {
  owl_variable *old = owl_variable_get_var(vd, name, OWL_VARIABLE_BOOL);
  if(old) {
    owl_variable_update(old, summ, desc);
    old->ival_default = initval;
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_BOOL;
    var->ival_default = initval;
    var->validate_fn = owl_variable_bool_validate_default;
    var->set_fn = owl_variable_bool_set_default;
    var->set_fromstring_fn = owl_variable_bool_set_fromstring_default;
    var->get_fn = owl_variable_get_default;
    var->get_tostring_fn = owl_variable_bool_get_tostring_default;
    var->delete_fn = owl_variable_delete_default;
    var->val = owl_malloc(sizeof(int));
    var->set_fn(var, &initval);
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_cleanup(owl_vardict *d)
{
  owl_dict_cleanup(d, (void (*)(void *))owl_variable_delete);
}

/* free the list with owl_variable_dict_namelist_cleanup */
void owl_variable_dict_get_names(const owl_vardict *d, owl_list *l) {
  owl_dict_get_keys(d, l);
}

void owl_variable_dict_namelist_cleanup(owl_list *l)
{
  owl_list_cleanup(l, owl_free);
}

void owl_variable_delete(owl_variable *v)
{
  if (v->delete_fn) v->delete_fn(v);
  owl_free(v);
}


const char *owl_variable_get_description(const owl_variable *v) {
  return v->description;
}

const char *owl_variable_get_summary(const owl_variable *v) {
  return v->summary;
}

const char *owl_variable_get_validsettings(const owl_variable *v) {
  if (v->validsettings) {
    return v->validsettings;
  } else {
    return "";
  }
}

/* functions for getting and setting variable values */

/* returns 0 on success, prints a status msg if msg is true */
int owl_variable_set_fromstring(owl_vardict *d, const char *name, const char *value, int msg, int requirebool) {
  owl_variable *v;
  char buff2[1024];
  if (!name) return(-1);
  v = owl_dict_find_element(d, name);
  if (v == NULL) {
    if (msg) owl_function_error("Unknown variable %s", name);
    return -1;
  }
  if (!v->set_fromstring_fn) {
    if (msg) owl_function_error("Variable %s is read-only", name);
    return -1;   
  }
  if (requirebool && v->type!=OWL_VARIABLE_BOOL) {
    if (msg) owl_function_error("Variable %s is not a boolean", name);
    return -1;   
  }
  if (0 != v->set_fromstring_fn(v, value)) {
    if (msg) owl_function_error("Unable to set %s (must be %s)", name, 
				  owl_variable_get_validsettings(v));
    return -1;
  }
  if (msg && v->get_tostring_fn) {
    v->get_tostring_fn(v, buff2, 1024, v->val);
    owl_function_makemsg("%s = '%s'", name, buff2);
  }    
  return 0;
}
 
int owl_variable_set_string(owl_vardict *d, const char *name, const char *newval) {
  owl_variable *v;
  if (!name) return(-1);
  v = owl_dict_find_element(d, name);
  if (v == NULL || !v->set_fn) return(-1);
  if (v->type!=OWL_VARIABLE_STRING) return(-1);
  return v->set_fn(v, newval);
}
 
int owl_variable_set_int(owl_vardict *d, const char *name, int newval) {
  owl_variable *v;
  if (!name) return(-1);
  v = owl_dict_find_element(d, name);
  if (v == NULL || !v->set_fn) return(-1);
  if (v->type!=OWL_VARIABLE_INT && v->type!=OWL_VARIABLE_BOOL) return(-1);
  return v->set_fn(v, &newval);
}
 
int owl_variable_set_bool_on(owl_vardict *d, const char *name) {
  return owl_variable_set_int(d,name,1);
}

int owl_variable_set_bool_off(owl_vardict *d, const char *name) {
  return owl_variable_set_int(d,name,0);
}

int owl_variable_get_tostring(const owl_vardict *d, const char *name, char *buf, int bufsize) {
  owl_variable *v;
  if (!name) return(-1);
  v = owl_dict_find_element(d, name);
  if (v == NULL || !v->get_tostring_fn) return(-1);
  return v->get_tostring_fn(v, buf, bufsize, v->val);
}

int owl_variable_get_default_tostring(const owl_vardict *d, const char *name, char *buf, int bufsize) {
  owl_variable *v;
  if (!name) return(-1);
  v = owl_dict_find_element(d, name);
  if (v == NULL || !v->get_tostring_fn) return(-1);
  if (v->type == OWL_VARIABLE_INT || v->type == OWL_VARIABLE_BOOL) {
    return v->get_tostring_fn(v, buf, bufsize, &(v->ival_default));
  } else {
    return v->get_tostring_fn(v, buf, bufsize, v->pval_default);
  }
}

owl_variable *owl_variable_get_var(const owl_vardict *d, const char *name, int require_type) {
  owl_variable *v;
  if (!name) return(NULL);
  v = owl_dict_find_element(d, name);
  if (v == NULL || !v->get_fn || v->type != require_type) return(NULL);
  return v;
}

/* returns a reference */
const void *owl_variable_get(const owl_vardict *d, const char *name, int require_type) {
  owl_variable *v = owl_variable_get_var(d, name, require_type);
  if(v == NULL) return NULL;
  return v->get_fn(v);
}

/* returns a reference */
const char *owl_variable_get_string(const owl_vardict *d, const char *name) {
  return owl_variable_get(d,name, OWL_VARIABLE_STRING);
}

/* returns a reference */
const void *owl_variable_get_other(const owl_vardict *d, const char *name) {
  return owl_variable_get(d,name, OWL_VARIABLE_OTHER);
}

int owl_variable_get_int(const owl_vardict *d, const char *name) {
  const int *pi;
  pi = owl_variable_get(d,name,OWL_VARIABLE_INT);
  if (!pi) return(-1);
  return(*pi);
}

int owl_variable_get_bool(const owl_vardict *d, const char *name) {
  const int *pi;
  pi = owl_variable_get(d,name,OWL_VARIABLE_BOOL);
  if (!pi) return(-1);
  return(*pi);
}

void owl_variable_describe(const owl_vardict *d, const char *name, owl_fmtext *fm) {
  char defaultbuf[50];
  char buf[1024];
  int buflen = 1023;
  owl_variable *v;

  if (!name
      || (v = owl_dict_find_element(d, name)) == NULL 
      || !v->get_fn) {
    snprintf(buf, buflen, "     No such variable '%s'\n", name);     
    owl_fmtext_append_normal(fm, buf);
    return;
  }
  if (v->type == OWL_VARIABLE_INT || v->type == OWL_VARIABLE_BOOL) {
    v->get_tostring_fn(v, defaultbuf, 50, &(v->ival_default));
  } else {
    v->get_tostring_fn(v, defaultbuf, 50, v->pval_default);
  }
  snprintf(buf, buflen, OWL_TABSTR "%-20s - %s (default: '%s')\n", 
		  v->name, 
		  owl_variable_get_summary(v), defaultbuf);
  owl_fmtext_append_normal(fm, buf);
}

void owl_variable_get_help(const owl_vardict *d, const char *name, owl_fmtext *fm) {
  char buff[1024];
  int bufflen = 1023;
  owl_variable *v;

  if (!name
      || (v = owl_dict_find_element(d, name)) == NULL 
      || !v->get_fn) {
    owl_fmtext_append_normal(fm, "No such variable...\n");
    return;
  }

  owl_fmtext_append_bold(fm, "OWL VARIABLE\n\n");
  owl_fmtext_append_normal(fm, OWL_TABSTR);
  owl_fmtext_append_normal(fm, name);
  owl_fmtext_append_normal(fm, " - ");
  owl_fmtext_append_normal(fm, v->summary);
  owl_fmtext_append_normal(fm, "\n\n");

  owl_fmtext_append_normal(fm, "Current:        ");
  owl_variable_get_tostring(d, name, buff, bufflen);
  owl_fmtext_append_normal(fm, buff);
  owl_fmtext_append_normal(fm, "\n\n");


  if (v->type == OWL_VARIABLE_INT || v->type == OWL_VARIABLE_BOOL) {
    v->get_tostring_fn(v, buff, bufflen, &(v->ival_default));
  } else {
    v->get_tostring_fn(v, buff, bufflen, v->pval_default);
  }
  owl_fmtext_append_normal(fm, "Default:        ");
  owl_fmtext_append_normal(fm, buff);
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

const void *owl_variable_get_default(const owl_variable *v) {
  return v->val;
}

void owl_variable_delete_default(owl_variable *v)
{
  if (v->val) owl_free(v->val);
}

/* default functions for booleans */

int owl_variable_bool_validate_default(const owl_variable *v, const void *newval) {
  if (newval == NULL) return(0);
  else if (*(const int*)newval==1 || *(const int*)newval==0) return(1);
  else return (0);
}

int owl_variable_bool_set_default(owl_variable *v, const void *newval) {
  if (v->validate_fn) {
    if (!v->validate_fn(v, newval)) return(-1);
  }
  *(int*)v->val = *(const int*)newval;
  return(0);
}

int owl_variable_bool_set_fromstring_default(owl_variable *v, const char *newval) {
  int i;
  if (!strcmp(newval, "on")) i=1;
  else if (!strcmp(newval, "off")) i=0;
  else return(-1);
  return (v->set_fn(v, &i));
}

int owl_variable_bool_get_tostring_default(const owl_variable *v, char* buf, int bufsize, const void *val) {
  if (val == NULL) {
    snprintf(buf, bufsize, "<null>");
    return -1;
  } else if (*(const int*)val == 0) {
    snprintf(buf, bufsize, "off");
    return 0;
  } else if (*(const int*)val == 1) {
    snprintf(buf, bufsize, "on");
    return 0;
  } else {
    snprintf(buf, bufsize, "<invalid>");
    return -1;
  }
}

/* default functions for integers */

int owl_variable_int_validate_default(const owl_variable *v, const void *newval) {
  if (newval == NULL) return(0);
  else return (1);
}

int owl_variable_int_set_default(owl_variable *v, const void *newval) {
  if (v->validate_fn) {
    if (!v->validate_fn(v, newval)) return(-1);
  }
  *(int*)v->val = *(const int*)newval;
  return(0);
}

int owl_variable_int_set_fromstring_default(owl_variable *v, const char *newval) {
  int i;
  char *ep;
  i = strtol(newval, &ep, 10);
  if (*ep || ep==newval) return(-1);
  return (v->set_fn(v, &i));
}

int owl_variable_int_get_tostring_default(const owl_variable *v, char* buf, int bufsize, const void *val) {
  if (val == NULL) {
    snprintf(buf, bufsize, "<null>");
    return -1;
  } else {
    snprintf(buf, bufsize, "%d", *(const int*)val);
    return 0;
  } 
}

/* default functions for enums (a variant of integers) */

int owl_variable_enum_validate(const owl_variable *v, const void *newval) {  
  char **enums;
  int nenums, val;
  if (newval == NULL) return(0);
  enums = atokenize(v->validsettings, ",", &nenums);
  if (enums == NULL) return(0);
  atokenize_delete(enums, nenums);
  val = *(const int*)newval;
  if (val < 0 || val >= nenums) {
    return(0);
  }
  return(1);
}

int owl_variable_enum_set_fromstring(owl_variable *v, const char *newval) {
  char **enums;
  int nenums, i, val=-1;
  if (newval == NULL) return(-1);
  enums = atokenize(v->validsettings, ",", &nenums);
  if (enums == NULL) return(-1);
  for (i=0; i<nenums; i++) {
    if (0==strcmp(newval, enums[i])) {
      val = i;
    }
  }
  atokenize_delete(enums, nenums);
  if (val == -1) return(-1);
  return (v->set_fn(v, &val));
}

int owl_variable_enum_get_tostring(const owl_variable *v, char* buf, int bufsize, const void *val) {
  char **enums;
  int nenums, i;

  if (val == NULL) {
    snprintf(buf, bufsize, "<null>");
    return -1;
  }
  enums = atokenize(v->validsettings, ",", &nenums);
  i = *(const int*)val;
  if (i<0 || i>=nenums) {
    snprintf(buf, bufsize, "<invalid:%d>",i);
    atokenize_delete(enums, nenums);
    return(-1);
  }
  snprintf(buf, bufsize, "%s", enums[i]);
  atokenize_delete(enums, nenums);
  return 0;
}

/* default functions for stringeans */

int owl_variable_string_validate_default(const struct _owl_variable *v, const void *newval) {
  if (newval == NULL) return(0);
  else return (1);
}

int owl_variable_string_set_default(owl_variable *v, const void *newval) {
  if (v->validate_fn) {
    if (!v->validate_fn(v, newval)) return(-1);
  }
  if (v->val) owl_free(v->val);
  v->val = owl_strdup(newval);
  return(0);
}

int owl_variable_string_set_fromstring_default(owl_variable *v, const char *newval) {
  return (v->set_fn(v, newval));
}

int owl_variable_string_get_tostring_default(const owl_variable *v, char* buf, int bufsize, const void *val) {
  if (val == NULL) {
    snprintf(buf, bufsize, "<null>");
    return -1;
  } else {
    snprintf(buf, bufsize, "%s", (const char*)val);
    return 0;
  }
}

