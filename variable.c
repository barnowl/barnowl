#include "owl.h"
#include <stdio.h>
#include "gmarshal_funcs.h"

static const GType owl_variable_gtype_map[] = {G_TYPE_POINTER, 
					       G_TYPE_INT, G_TYPE_BOOLEAN,
					       G_TYPE_STRING };

#define OWLVAR_BOOL(name,default,summary,description) \
        { g_strdup(name), OWL_VARIABLE_BOOL, NULL, default, "on,off", g_strdup(summary), g_strdup(description), NULL, \
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_BOOL_FULL(name,default,summary,description,validate,set,get) \
        { g_strdup(name), OWL_VARIABLE_BOOL, NULL, default, "on,off", g_strdup(summary), g_strdup(description), NULL, \
	    G_CALLBACK(validate), G_CALLBACK(set), NULL, G_CALLBACK(get), NULL, NULL, NULL }

#define OWLVAR_INT(name,default,summary,description) \
        { g_strdup(name), OWL_VARIABLE_INT, NULL, default, "<int>", g_strdup(summary), g_strdup(description), NULL, \
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_INT_FULL(name,default,summary,description,validset,validate,set,get) \
        { g_strdup(name), OWL_VARIABLE_INT, NULL, default, validset, g_strdup(summary), g_strdup(description), NULL, \
	    G_CALLBACK(validate), G_CALLBACK(set), NULL, G_CALLBACK(get), NULL, NULL, NULL }

#define OWLVAR_PATH(name,default,summary,description) \
        { g_strdup(name), OWL_VARIABLE_STRING, g_strdup(default), 0, "<path>", g_strdup(summary), g_strdup(description),  NULL, \
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_STRING(name,default,summary,description) \
        { g_strdup(name), OWL_VARIABLE_STRING, g_strdup(default), 0, "<string>", g_strdup(summary), g_strdup(description), NULL, \
	    NULL, NULL, NULL, NULL, NULL, NULL, NULL }

#define OWLVAR_STRING_FULL(name,default,validset,summary,description,validate,set,get) \
        { g_strdup(name), OWL_VARIABLE_STRING, g_strdup(default), 0, validset, g_strdup(summary), g_strdup(description), NULL, \
	    G_CALLBACK(validate), G_CALLBACK(set), NULL, G_CALLBACK(get), NULL, NULL, NULL }

/* enums are really integers, but where validset is a comma-separated
 * list of strings which can be specified.  The tokens, starting at 0,
 * correspond to the values that may be specified. */
#define OWLVAR_ENUM(name,default,summary,description,validset) \
        { g_strdup(name), OWL_VARIABLE_INT, NULL, default, validset, g_strdup(summary), g_strdup(description), NULL, \
	    G_CALLBACK(owl_variable_enum_validate),			\
	    NULL, G_CALLBACK(owl_variable_enum_set_fromstring),		\
	    NULL, G_CALLBACK(owl_variable_enum_get_tostring),		\
	    NULL, NULL }

#define OWLVAR_ENUM_FULL(name,default,summary,description,validset,validate, set, get) \
        { g_strdup(name), OWL_VARIABLE_INT, NULL, default, validset, g_strdup(summary), g_strdup(description), NULL, \
	    G_CALLBACK(validate),					\
	    G_CALLBACK(set), G_CALLBACK(owl_variable_enum_set_fromstring), \
	    G_CALLBACK(get), G_CALLBACK(owl_variable_enum_get_tostring), \
	    NULL, NULL }

int owl_variable_add_defaults(owl_vardict *vd)
{
  owl_variable_init_params variables_to_init[] = {

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
	       "send a login message when BarnOwl starts", "" ),

  OWLVAR_BOOL( "shutdownlogout" /* %OwlVarStub */, 1,
	       "send a logout message when BarnOwl exits", "" ),

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
	       "specifies which kind of messages should be logged",
	       "Can be one of 'both', 'in', or 'out'.  If 'in' is\n"
	       "selected, only incoming messages are logged, if 'out'\n"
	       "is selected only outgoing messages are logged.  If 'both'\n"
	       "is selected both incoming and outgoing messages are\n"
	       "logged.",
	       "both,in,out"),

  OWLVAR_BOOL_FULL( "colorztext" /* %OwlVarStub */, 1,
                    "allow @color() in zephyrs to change color",
                    NULL, NULL, owl_variable_colorztext_set, NULL),

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
		    "When this is enabled, BarnOwl will periodically check the zephyr\n"
		    "location of users in your .anyone file.  If a user is present\n"
		    "but sent no login message, or a user is not present that sent no\n"
		    "logout message, a pseudo login or logout message will be created\n",
		    NULL, owl_variable_pseudologins_set, NULL),

  OWLVAR_BOOL( "ignorelogins" /* %OwlVarStub */, 0,
	       "Enable printing of login notifications",
	       "When this is enabled, BarnOwl will print login and logout notifications\n"
	       "for AIM, zephyr, or other protocols.  If disabled BarnOwl will not print\n"
	       "login or logout notifications.\n"),

  OWLVAR_STRING( "logfilter" /* %OwlVarStub */, "",
		 "name of a filter controlling which messages to log",

		 "If non empty, any messages matching the given filter will be logged.\n"
		 "This is a completely separate mechanism from the other logging\n"
		 "variables like logging, classlogging, loglogins, loggingdirection,\n"
		 "etc.  If you want this variable to control all logging, make sure\n"
		 "all other logging variables are in their default state.\n"),

  OWLVAR_BOOL( "loglogins" /* %OwlVarStub */, 0,
	       "Enable logging of login notifications",
	       "When this is enabled, BarnOwl will log login and logout notifications\n"
	       "for AIM, zephyr, or other protocols.  If disabled BarnOwl will not print\n"
	       "login or logout notifications.\n"),

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
	       "This file will be logged to if 'debug' is set to 'on'.\n"
               "BarnOwl will append a dot and the current process's pid to the filename."),
  
  OWLVAR_PATH( "zsigproc" /* %OwlVarStub:zsigproc */, NULL,
	       "name of a program to run that will generate zsigs",
	       "This program should produce a zsig on stdout when run.\n"
	       "Note that it is important that this program not block.\n\n"
               "See the documentation for 'zsig' for more information about\n"
               "how the outgoing zsig is chosen."
               ),

  OWLVAR_PATH( "newmsgproc" /* %OwlVarStub:newmsgproc */, NULL,
	       "name of a program to run when new messages are present",
	       "The named program will be run when BarnOwl receives new\n"
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
		 "described in the documentation for the 'zsig' variable.\n"
		 "See also BarnOwl::random_zephyr_signature().\n"),

  OWLVAR_STRING( "zsig" /* %OwlVarStub */, "",
	         "zephyr signature",
		 "The zsig to get on outgoing messages. If this variable is\n"
		 "unset, 'zsigproc' will be run to generate a zsig. If that is\n"
		 "also unset, the 'zwrite-signature' zephyr variable will be\n"
		 "used instead.\n"),

  OWLVAR_STRING( "appendtosepbar" /* %OwlVarStub */, "",
	         "string to append to the end of the sepbar",
		 "The sepbar is the bar separating the top and bottom\n"
		 "of the BarnOwl screen.  Any string specified here will\n"
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
		 "BarnOwl command to execute for alert actions",
		 "" ),

  OWLVAR_STRING_FULL( "tty" /* %OwlVarStub */, "", "<string>", "tty name for zephyr location", "",
		      NULL, owl_variable_tty_set, NULL),

  OWLVAR_STRING( "default_style" /* %OwlVarStub */, "default",
		 "name of the default formatting style",
		 "This sets the default message formatting style.\n"
		 "Styles may be created with the 'style' command.\n"
		 "Some built-in styles include:\n"
		 "   default  - the default BarnOwl formatting\n"
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
	      "AIM login before allowing the receipt of AIM login notifications.\n"
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
	       "normal,top,neartop,center,paged,pagedcenter" ),

  OWLVAR_BOOL( "narrow-related" /* %OwlVarStub:narrow_related */, 1,
               "Make smartnarrow use broader filters",
               "Causes smartfilter to narrow to messages \"related\" to \n"
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

  OWLVAR_STRING_FULL( "default_exposure" /* %OwlVarStub */, "",
                      "none,opstaff,realm-visible,realm-announced,net-visible,net-announced",
                      "controls the persistent value for exposure",
                      "The default exposure level corresponds to the Zephyr exposure value\n"
                      "in ~/.zephyr.vars.  Defaults to realm-visible if there is no value in\n"
                      "~/.zephyr.vars.\n"
                      "See the description of exposure for the values this can be.",
                      NULL, owl_variable_default_exposure_set, owl_variable_default_exposure_get ),

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
                      NULL, owl_variable_exposure_set, NULL /* use default for get */ ),

  /* This MUST be last... */
  { NULL, 0, NULL, 0, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL }

  };

  int ret = owl_variable_dict_add_from_list(vd, variables_to_init);
  owl_variable_init_params *var;
  for (var = variables_to_init; var->name != NULL; var++)
    owl_variable_cleanup_initializer(var);
  return ret;
}

/**************************************************************************/
/*********************** SPECIFIC TO VARIABLES ****************************/
/**************************************************************************/


/* commonly useful */

int owl_variable_int_validate_gt0(const owl_variable *v, const int newval, void *dummy)
{
  return !(newval < 1);
}

int owl_variable_int_validate_positive(const owl_variable *v, const int newval, void *dummy)
{
  return !(newval < 0);
}

/* typewinsize */
int owl_variable_typewinsize_set(owl_variable *v, const int newval, void *dummy)
{
  int rv;
  rv = owl_variable_int_set_default(v, newval, dummy);
  if (0 == rv) owl_mainpanel_layout_contents(&g.mainpanel);
  return(rv);
}

/* debug (cache value in g->debug) */
int owl_variable_debug_set(owl_variable *v, const int newval, void *dummy)
{
  if (newval == 1 || newval == 0) {
    g.debug = newval;
  }
  return owl_variable_bool_set_default(v, newval, dummy);
}

/* When 'aaway' is changed, need to notify the AIM server */
int owl_variable_aaway_set(owl_variable *v, const gboolean newval, void *dummy)
{
  if (newval == 1) {
    owl_aim_set_awaymsg(owl_global_get_aaway_msg(&g));
  } else if (newval == 0) {
    owl_aim_set_awaymsg("");
  }
  return owl_variable_bool_set_default(v, newval, dummy);
}

int owl_variable_colorztext_set(owl_variable *v, const void *newval, void *dummy)
{
  int ret = owl_variable_bool_set_default(v, newval, dummy);
  /* flush the format cache so that we see the update, but only if we're done initializing BarnOwl */
  if (owl_global_get_msglist(&g) != NULL)
    owl_messagelist_invalidate_formats(owl_global_get_msglist(&g));
  if (owl_global_get_mainwin(&g) != NULL) {
    owl_function_calculate_topmsg(OWL_DIRECTION_DOWNWARDS);
    owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  }
  return ret;
}

int owl_variable_pseudologins_set(owl_variable *v, const int newval, void *dummy)
{
  static guint timer = 0;
  if (newval == 1) {
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
  return owl_variable_bool_set_default(v, newval, dummy);
}

/* note that changing the value of this will clobber 
 * any user setting of this */
int owl_variable_disable_ctrl_d_set(owl_variable *v, const int newval, void *dummy)
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
  return owl_variable_int_set_default(v, newval, dummy);  
}

int owl_variable_tty_set(owl_variable *v, const char *newval, void *dummy)
{
  owl_zephyr_set_locationinfo(g_get_host_name(), newval);
  return(owl_variable_string_set_default(v, newval, dummy));
}

int owl_variable_default_exposure_set(owl_variable *v, const char *newval, void *dummy)
{
  return owl_zephyr_set_default_exposure(newval);
}

const char *owl_variable_default_exposure_get(const owl_variable *v, void *dummy)
{
  return owl_zephyr_get_default_exposure();
}

int owl_variable_exposure_set(owl_variable *v, const char *newval, void *dummy)
{
  int ret = owl_zephyr_set_exposure(newval);
  if (ret != 0)
    return ret;
  return owl_variable_string_set_default(v, owl_zephyr_normalize_exposure(newval), dummy);
}

/**************************************************************************/
/****************************** GENERAL ***********************************/
/**************************************************************************/

int owl_variable_dict_setup(owl_vardict *vd) {
  owl_dict_create(vd);
  return owl_variable_add_defaults(vd);
}

CALLER_OWN GClosure *owl_variable_make_closure(owl_variable *v,
                                                      GCallback fn,
                                                      GClosureMarshal marshal) {
  GClosure *closure = g_cclosure_new(fn, NULL, NULL);
  g_closure_set_marshal(closure,marshal);
  g_closure_ref(closure);
  g_closure_sink(closure);
  return closure;
}

#define OWL_VARIABLE_SETUP_FUNC(variable, initializer, func_name, default_func, marshal_func, temp) do { \
  if(initializer->func_name) { \
    temp = initializer->func_name; \
  } else { \
    temp = default_func; \
  } \
  variable->func_name = owl_variable_make_closure(variable, G_CALLBACK(temp), \
						  marshal_func);	\
  } while(0)
  

int owl_variable_dict_add_from_list(owl_vardict *vd, owl_variable_init_params *variables_to_init)
{
  owl_variable *newvar = NULL;
  owl_variable_init_params *init_params = NULL;
  for (init_params = variables_to_init; init_params->name; init_params++) {
    newvar = g_new0(owl_variable, 1);
    newvar->type = init_params->type;
    /* strdup all the strings so we can delete them consistently. */
    newvar->name = g_strdup(init_params->name);
    newvar->summary = g_strdup(init_params->summary);
    newvar->description = g_strdup(init_params->description);
    newvar->validsettings = init_params->validsettings;
    GValue values[] = {{0}, {0}};
    GValue *value = values+1;
    GValue ret = {0};
    GCallback fn = NULL;
    switch (init_params->type) {
    case OWL_VARIABLE_STRING:
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_fn, 
                              G_CALLBACK(owl_variable_string_get_default), 
                              g_cclosure_user_marshal_STRING__VOID, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fn, 
                              G_CALLBACK(owl_variable_string_set_default), 
                              g_cclosure_user_marshal_INT__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, validate_fn,
                              G_CALLBACK(owl_variable_string_validate_default),
                              g_cclosure_user_marshal_INT__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fromstring_fn,
                              G_CALLBACK(owl_variable_string_set_fromstring_default),
                              g_cclosure_user_marshal_INT__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_tostring_fn,
                              G_CALLBACK(owl_variable_string_get_tostring_default),
                              g_cclosure_user_marshal_STRING__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_default_fn, 
                              G_CALLBACK(owl_variable_string_get_default_default), 
                              g_cclosure_user_marshal_STRING__VOID, fn);

      g_value_init(value,G_TYPE_STRING);
      g_value_set_string(value, init_params->pval_default);
      break;
    case OWL_VARIABLE_BOOL:
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_fn, 
                              G_CALLBACK(owl_variable_bool_get_default), 
                              g_cclosure_user_marshal_BOOLEAN__VOID, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fn, 
                              G_CALLBACK(owl_variable_bool_set_default), 
                              g_cclosure_user_marshal_INT__BOOLEAN, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, validate_fn,
                              G_CALLBACK(owl_variable_bool_validate_default),
                              g_cclosure_user_marshal_INT__BOOLEAN, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fromstring_fn,
                              G_CALLBACK(owl_variable_bool_set_fromstring_default),
                              g_cclosure_user_marshal_INT__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_tostring_fn,
                              G_CALLBACK(owl_variable_bool_get_tostring_default),
                              g_cclosure_user_marshal_STRING__BOOLEAN, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_default_fn,
                              G_CALLBACK(owl_variable_bool_get_default_default),
                              g_cclosure_user_marshal_BOOLEAN__VOID, fn);

      g_value_init(value,G_TYPE_BOOLEAN);
      g_value_set_boolean(value, !!(init_params->ival_default));
      break;
    case OWL_VARIABLE_INT:
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_fn, 
                              G_CALLBACK(owl_variable_int_get_default), 
                              g_cclosure_user_marshal_INT__VOID, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fn, 
                              G_CALLBACK(owl_variable_int_set_default), 
                              g_cclosure_user_marshal_INT__INT, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, validate_fn,
                              G_CALLBACK(owl_variable_int_validate_default),
                              g_cclosure_user_marshal_INT__INT, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, set_fromstring_fn,
                              G_CALLBACK(owl_variable_int_set_fromstring_default),
                              g_cclosure_user_marshal_INT__STRING, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_tostring_fn,
                              G_CALLBACK(owl_variable_int_get_tostring_default),
                              g_cclosure_user_marshal_STRING__INT, fn);
      OWL_VARIABLE_SETUP_FUNC(newvar, init_params, get_default_fn, 
                              G_CALLBACK(owl_variable_int_get_default_default), 
                              g_cclosure_user_marshal_INT__VOID, fn);

      g_value_init(value,G_TYPE_INT);
      g_value_set_int(value, init_params->ival_default);
      break;
    default:
      fprintf(stderr, "owl_variable_setup: invalid variable type\n");
      return(-2);
    }
    OWL_VARIABLE_SETUP_FUNC(newvar, init_params, delete_fn, 
                            G_CALLBACK(owl_variable_delete_default),
                            g_cclosure_marshal_VOID__VOID, fn);
    
    g_value_init(&(newvar->gval_default), G_VALUE_TYPE(value));
    g_value_init(&(newvar->val), G_VALUE_TYPE(value));
    g_value_copy(value, &(newvar->gval_default));
    /* we have the value boxed up already *anyway*, so... */
    g_value_init(values, G_TYPE_POINTER);
    g_value_set_pointer(values, newvar);
    g_value_init(&ret, G_TYPE_INT);
    g_closure_invoke(newvar->set_fn, &ret, 2, values, NULL);
    g_value_unset(value);
    owl_dict_insert_element(vd, newvar->name, newvar, NULL);
  }
  return 0;
}

void owl_variable_dict_add_variable(owl_vardict * vardict,
                                    owl_variable * var) {
  owl_dict_insert_element(vardict, var->name, var, (void (*)(void *))owl_variable_delete);
}

CALLER_OWN owl_variable *owl_variable_newvar(const char *name, const char *summary, const char *description)
{
  owl_variable *var = g_new0(owl_variable, 1);
  var->name = g_strdup(name);
  var->summary = g_strdup(summary);
  var->description = g_strdup(description);
  return var;
}

void owl_variable_update(owl_variable *var, const char *summary, const char *desc) {
  g_free(var->summary);
  var->summary = g_strdup(summary);
  g_free(var->description);
  var->description = g_strdup(desc);
}

#define OWL_VARIABLE_SETUP_DEFAULT_FUNCS(variable, type, gtype) do { \
  variable->set_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_set_default), \
    g_cclosure_user_marshal_INT__##gtype); \
  variable->get_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_get_default), \
    g_cclosure_user_marshal_##gtype##__##VOID); \
  variable->validate_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_validate_default), \
    g_cclosure_user_marshal_INT__##gtype); \
  variable->set_fromstring_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_set_fromstring_default), \
    g_cclosure_user_marshal_INT__STRING); \
  variable->get_tostring_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_get_tostring_default), \
    g_cclosure_user_marshal_STRING__##gtype); \
  variable->get_default_fn = owl_variable_make_closure(variable, \
    G_CALLBACK(owl_variable_##type##_get_default_default), \
    g_cclosure_user_marshal_##gtype##__##VOID); \
  } while(0);

void owl_variable_dict_newvar_string(owl_vardict *vd, const char *name, const char *summ, const char *desc, const char *initval)
{
  GValue default_gvals[] = {{0}, {0}};
  GValue *default_gval = default_gvals+1;
  GValue retval = {0}; 
  g_value_init(default_gval, G_TYPE_STRING);
  g_value_init(&retval, G_TYPE_INT);
  g_value_set_static_string(default_gval, initval);
  owl_variable *old = owl_variable_get_var(vd, name);
  if (old && owl_variable_get_type(old) == OWL_VARIABLE_STRING) {
    owl_variable_update(old, summ, desc);
    g_value_copy(default_gval, &(old->gval_default));
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_STRING;
    var->validsettings = "<string>";
    g_value_init(&(var->val), G_TYPE_STRING);
    g_value_init(default_gvals, G_TYPE_POINTER);
    g_value_set_pointer(default_gvals, var);
    g_value_init(&(var->gval_default), G_TYPE_STRING);
    g_value_set_string(&(var->gval_default), initval);
    OWL_VARIABLE_SETUP_DEFAULT_FUNCS(var, string, STRING);

    g_closure_invoke(var->set_fn, &retval, 2, default_gvals, NULL);
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_newvar_int(owl_vardict *vd, const char *name, const char *summ, const char *desc, int initval)
{
  GValue default_gvals[] = {{0}, {0}};
  GValue *default_gval = default_gvals+1;
  GValue retval = {0}; 
  g_value_init(default_gval, G_TYPE_INT);
  g_value_init(&retval, G_TYPE_INT);
  g_value_set_int(default_gval, initval);
  owl_variable *old = owl_variable_get_var(vd, name);
  if (old && owl_variable_get_type(old) == OWL_VARIABLE_INT) {
    owl_variable_update(old, summ, desc);
    g_value_copy(default_gval, &(old->gval_default));
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_INT;
    var->validsettings = "<int>";
    g_value_init(&(var->val), G_TYPE_INT);
    g_value_init(&(var->gval_default), G_TYPE_INT);
    OWL_VARIABLE_SETUP_DEFAULT_FUNCS(var, int, INT);
    g_value_init(default_gvals, G_TYPE_POINTER);
    g_value_set_pointer(default_gvals, var);
    g_value_set_int(&(var->gval_default), initval);
    g_closure_invoke(var->set_fn, &retval, 2, default_gvals, NULL);
    /*    g_value_unset(&retval); */
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_newvar_bool(owl_vardict *vd, const char *name, const char *summ, const char *desc, gboolean initval)
{
  GValue default_gvals[] = {{0}, {0}};
  GValue *default_gval = default_gvals+1;
  GValue retval = {0}; 
  g_value_init(default_gval, G_TYPE_BOOLEAN);
  g_value_init(&retval, G_TYPE_INT);
  g_value_set_boolean(default_gval, initval);
  owl_variable *old = owl_variable_get_var(vd, name);
  if (old && owl_variable_get_type(old) == OWL_VARIABLE_BOOL) {
    owl_variable_update(old, summ, desc);
    g_value_copy(default_gval, &(old->gval_default));
  } else {
    owl_variable * var = owl_variable_newvar(name, summ, desc);
    var->type = OWL_VARIABLE_BOOL;
    var->validsettings = "on,off";
    g_value_init(&(var->val), G_TYPE_BOOLEAN);
    g_value_init(&(var->gval_default), G_TYPE_BOOLEAN);
    OWL_VARIABLE_SETUP_DEFAULT_FUNCS(var, bool, BOOLEAN);

    g_value_init(default_gvals, G_TYPE_POINTER);
    g_value_set_pointer(default_gvals, var);
    g_value_set_int(&(var->gval_default), initval);
    g_closure_invoke(var->set_fn, &retval, 2, default_gvals, NULL);  
    owl_variable_dict_add_variable(vd, var);
  }
}

void owl_variable_dict_cleanup(owl_vardict *d)
{
  owl_dict_cleanup(d, (void (*)(void *))owl_variable_delete);
}

CALLER_OWN GPtrArray *owl_variable_dict_get_names(const owl_vardict *d) {
  return owl_dict_get_keys(d);
}

void owl_variable_cleanup(owl_variable *v)
{
  GValue val = {0};
  if (v->delete_fn) {
    g_value_init(&val, G_TYPE_POINTER);
    g_value_set_pointer(&val, v);
    g_closure_invoke(v->delete_fn, NULL, 1, &val, NULL);
  }
  g_free(v->name);
  g_free(v->summary);
  g_free(v->description);
  if (v->type == OWL_VARIABLE_STRING) {
    g_value_unset(&v->gval_default);
  }
  g_closure_unref(v->get_fn);
  g_closure_unref(v->set_fn);
  g_closure_unref(v->validate_fn);
  g_closure_unref(v->get_tostring_fn);
  g_closure_unref(v->set_fromstring_fn);
}
 
void owl_variable_cleanup_initializer(owl_variable_init_params *v) {
  g_free(v->name);
  g_free(v->summary);
  g_free(v->description);
  if (v->type == OWL_VARIABLE_STRING)
    g_free(v->pval_default);
 }

void owl_variable_delete(owl_variable *v)
{
  owl_variable_cleanup(v);
  g_free(v);
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

int owl_variable_get_type(const owl_variable *v)
{
  return v->type;
}

/* functions for getting and setting variable values */

static CALLER_OWN  char *owl_variable_invoke_tostring(const owl_variable *v,
						     const GValue *value) 
{
  GValue values[] = { {0}, {0}};
  GValue *value_box = values+1;
  GValue tostring_box = {0};
  char *ret = NULL;
  gboolean need_to_free = false;

  g_value_init(values, G_TYPE_POINTER);
  g_value_set_pointer(values, (gpointer)v);
  g_value_init(&tostring_box, G_TYPE_STRING);
  if(value) {
    g_value_init(value_box, G_VALUE_TYPE(value));
    g_value_copy(value, value_box);
    need_to_free = true;
  } else {
    g_value_init(value_box, owl_variable_gtype_map[v->type]);
    g_closure_invoke(v->get_fn, value_box, 1, values, NULL);
  }
  g_closure_invoke(v->get_tostring_fn,&tostring_box, 2, values, NULL);

  ret = g_value_dup_string(&tostring_box);
  g_value_unset(&tostring_box);
  if(need_to_free) {
    g_value_unset(value_box);
  }

  return ret;
}

/* returns 0 on success, prints a status msg if msg is true */
int owl_variable_set_fromstring(owl_variable *v, const char *value, int msg) {
  char *tostring;
  GValue values[] = {{0},{0},{0}};
  GValue *value_box = values+1;
  GValue return_box = {0};
  int set_successfully = -1;
  if (!v->set_fromstring_fn) {
    if (msg) owl_function_error("Variable %s is read-only", owl_variable_get_name(v));
    return -1;   
  }
  g_value_init(value_box, G_TYPE_STRING);
  g_value_init(values, G_TYPE_POINTER);
  g_value_set_pointer(values, v);
  g_value_init(&return_box, G_TYPE_INT);
  g_value_set_static_string(value_box, value);
  g_closure_invoke(v->set_fromstring_fn, &return_box, 2, values, NULL);
  set_successfully = g_value_get_int(&return_box);
  if (0 != set_successfully) {
    if (msg) owl_function_error("Unable to set %s (must be %s)", owl_variable_get_name(v),
                                owl_variable_get_validsettings(v));
  }
  if (msg && (0 != set_successfully)) {
    tostring = owl_variable_invoke_tostring(v, NULL);
    if (tostring) {
      owl_function_makemsg("%s = '%s'", owl_variable_get_name(v), tostring);
    } else {
      owl_function_makemsg("%s = <null>", owl_variable_get_name(v));
    }
    g_free(tostring);
  }
  g_value_unset(value_box);
  return set_successfully;
}

static int owl_variable_invoke_setter(owl_variable *v, const GValue *value) {
  GValue values[] = {{0},{0}};
  GValue return_box = {0};
  int ret = -1;
  g_value_init(&return_box, G_TYPE_INT);
  g_value_init(values, G_TYPE_POINTER);
  g_value_set_pointer(values, v);
  g_value_init(values+1, G_VALUE_TYPE(value));
  g_value_copy(value, values+1);
  g_closure_invoke(v->set_fn, &return_box, 2, values, NULL);
  ret = g_value_get_int(&return_box);
  /*  g_value_unset(&return_box); */
  g_value_unset(values+1);
  return ret;
}
 
int owl_variable_set_string(owl_variable *v, const char *newval)
{
  int ret = -1;
  GValue value_box = {0};
  if (v->type != OWL_VARIABLE_STRING) return -1;
  g_value_init(&value_box, G_TYPE_STRING);
  g_value_set_static_string(&value_box, newval);
  ret = owl_variable_invoke_setter(v, &value_box);
  g_value_unset(&value_box);
  return ret;
}

int owl_variable_set_int(owl_variable *v, int newval)
{
  int ret = -1;
  GValue value_box = {0};
  if (v->type != OWL_VARIABLE_INT && v->type != OWL_VARIABLE_BOOL) return -1;
  g_value_init(&value_box, G_TYPE_INT);
  g_value_set_int(&value_box, newval);
  ret = owl_variable_invoke_setter(v, &value_box);
  /* g_value_unset(&value_box); */
  return ret;
}

int owl_variable_set_bool(owl_variable *v, gboolean newval) {
  int ret = -1;
  GValue value_box = {0};
  if (v->type != OWL_VARIABLE_BOOL) return -1;
  g_value_init(&value_box, G_TYPE_BOOLEAN);
  g_value_set_int(&value_box, newval);
  ret = owl_variable_invoke_setter(v, &value_box);
  /* g_value_unset(&value_box); */
  return ret;
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
  return owl_variable_invoke_tostring(v, NULL);
}

CALLER_OWN char *owl_variable_get_default_tostring(const owl_variable *v)
{
  char *ret = NULL;
  GValue default_value_box = {0};
  GValue variable_box = {0};
  g_value_init(&variable_box, G_TYPE_POINTER);
  g_value_init(&default_value_box, owl_variable_gtype_map[v->type]);
  g_value_set_pointer(&variable_box, (gpointer)v);
  g_closure_invoke(v->get_default_fn, &default_value_box, 1, 
		   &variable_box, NULL);  
  ret = owl_variable_invoke_tostring(v, &default_value_box);
  g_value_unset(&default_value_box);
  return ret;
}

owl_variable *owl_variable_get_var(const owl_vardict *d, const char *name)
{
  return owl_dict_find_element(d, name);
}

const GValue *owl_variable_get(const owl_variable *v, GValue *val)
{
  GValue variable_box = {0};
  g_value_init(&variable_box, G_TYPE_POINTER);
  g_value_set_pointer(&variable_box,(gpointer)v);
  g_closure_invoke(v->get_fn, val, 1, &variable_box, NULL);  
  return val;
}

const char *owl_variable_get_string(const owl_variable *v)
{
  GValue value = {0};
  if (owl_variable_get_type(v) != OWL_VARIABLE_STRING) {
    owl_function_error("Variable '%s' is not a string.", owl_variable_get_name(v));
    return NULL;
  }
  g_value_init(&value, G_TYPE_STRING);
  /* not a leak, since we don't own the underlying string */
  return g_value_get_string(owl_variable_get(v,&value));
}

int owl_variable_get_int(const owl_variable *v)
{
  GValue value = {0};
  if (owl_variable_get_type(v) != OWL_VARIABLE_INT) {
    owl_function_error("Variable '%s' is an int.", owl_variable_get_name(v));
    return -1;
  }
  g_value_init(&value, G_TYPE_INT);
  return g_value_get_int(owl_variable_get(v,&value));
}

int owl_variable_get_bool(const owl_variable *v)
{
  GValue value = {0};
  if (owl_variable_get_type(v) != OWL_VARIABLE_BOOL) {
    owl_function_error("Variable '%s' is a boolean.", owl_variable_get_name(v));
    return -1;
  }
  g_value_init(&value, G_TYPE_BOOLEAN);
  return g_value_get_boolean(owl_variable_get(v,&value));
}

void owl_variable_describe(const owl_variable *v, owl_fmtext *fm)
{
  char *tostring = owl_variable_get_default_tostring(v);
  char *default_buf;

  if (tostring)
    default_buf = g_strdup_printf("'%s'", tostring);
  else
    default_buf = g_strdup("<null>");
  owl_fmtext_appendf_normal(fm, OWL_TABSTR "%-20s - %s (default: %s)\n",
                            owl_variable_get_name(v),
                            owl_variable_get_summary(v), default_buf);
  g_free(default_buf);
  g_free(tostring);
}

void owl_variable_get_help(const owl_variable *v, owl_fmtext *fm) {
  char *tostring;

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

  tostring = owl_variable_get_default_tostring(v);
  owl_fmtext_append_normal(fm, "Default:        ");
  owl_fmtext_append_normal(fm, (tostring ? tostring : "<null>"));
  owl_fmtext_append_normal(fm, "\n\n");
  g_free(tostring);

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

const char *owl_variable_string_get_default(const owl_variable *v, void *dummy) {
  return g_value_get_string(&(v->val));
}

const char *owl_variable_string_get_default_default(const owl_variable *v, void *dummy) {
  return g_value_get_string(&(v->gval_default));
}


const int owl_variable_int_get_default(const owl_variable *v, void *dummy) {
  return g_value_get_int(&(v->val));
}

const int owl_variable_int_get_default_default(const owl_variable *v, void *dummy) {
  return g_value_get_int(&(v->gval_default));
}


const gboolean owl_variable_bool_get_default(const owl_variable *v, void *dummy) {
  return g_value_get_boolean(&(v->val));
}

const gboolean owl_variable_bool_get_default_default(const owl_variable *v, void *dummy) {
  return g_value_get_boolean(&(v->gval_default));
}


void owl_variable_delete_default(owl_variable *v, void *dummy)
{
  g_value_unset(&(v->val));
}

/* default functions for booleans */

int owl_variable_bool_validate_default(const owl_variable *v, const gboolean newval, void *dummy) {
  return (newval == 1) || (newval == 0);
}

static int owl_variable_invoke_validator(owl_variable *v, const GValue *newval)
{
  GValue values[] = {{0},{0}};
  GValue ret = {0};
  g_value_init(&ret, G_TYPE_INT);
  g_value_init(values, G_TYPE_POINTER);
  g_value_set_pointer(values, v);
  g_value_init(values+1, G_VALUE_TYPE(newval));
  g_value_copy(newval, values+1);
  g_closure_invoke(v->validate_fn, &ret, 2, values, NULL);
  g_value_unset(values+1);
  return g_value_get_int(&ret);
}

int owl_variable_bool_set_default(owl_variable *v, const bool newval, void *dummy) {
  GValue value = {0};
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, newval);
  if (v->validate_fn) {
    if (!owl_variable_invoke_validator(v,&value)) {
      return(-1);
    }
  }
  g_value_set_boolean(&(v->val), newval);
  return(0);
}

int owl_variable_bool_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) {
  gboolean i;
  GValue value = {0};
  if (!strcmp(newval, "on")) {
    i=1;
  } else if (!strcmp(newval, "off")) {
    i=0;
  } else {
    return(-1);
  }
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, i);
  return owl_variable_invoke_setter(v,&value);
}

CALLER_OWN char *owl_variable_bool_get_tostring_default(const owl_variable *v, const bool val, void *dummy)
{
  if (val == 0) {
    return g_strdup("off");
  } else if (val == 1) {
    return g_strdup("on");
  } else {
    return g_strdup("<invalid>");
  }
}

/* default functions for integers */

int owl_variable_int_validate_default(const owl_variable *v, const int newval, void *dummy)
{
  return (1);
}

int owl_variable_int_set_default(owl_variable *v, const int newval, void *dummy) {
  GValue value = {0};
  g_value_init(&value, G_TYPE_INT);
  g_value_set_int(&value, newval);
  if (v->validate_fn) {
    if (!owl_variable_invoke_validator(v,&value)) {
      return(-1);
    }
  }
  g_value_set_int(&(v->val), newval);
  return(0);
}

int owl_variable_int_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) {
  int i;
  char *ep;
  GValue value = {0};
  i = strtol(newval, &ep, 10);
  if (*ep || ep==newval) return(-1);
  g_value_init(&value, G_TYPE_INT);
  g_value_set_int(&value, i);
  return owl_variable_invoke_setter(v, &value);
}

CALLER_OWN char *owl_variable_int_get_tostring_default(const owl_variable *v, const int val, void *dummy)
{
  return g_strdup_printf("%d", val);
}

/* default functions for enums (a variant of integers) */

int owl_variable_enum_validate(const owl_variable *v, const int newval, void *dummy) {  
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
  GValue box = {0};
  if (newval == NULL) return(-1);
  enums = g_strsplit_set(v->validsettings, ",", 0);
  for (i = 0; enums[i] != NULL; i++) {
    if (0==strcmp(newval, enums[i])) {
      val = i;
    }
  }
  g_strfreev(enums);
  if (val == -1) return(-1);
  g_value_init(&box, G_TYPE_INT);
  g_value_set_int(&box, val);
  return owl_variable_invoke_setter(v, &box);
}

CALLER_OWN char *owl_variable_enum_get_tostring(const owl_variable *v, const int val, void *dummy)
{
  char **enums;
  int nenums, i;
  char *tostring;

  enums = g_strsplit_set(v->validsettings, ",", 0);
  nenums = g_strv_length(enums);
  i = val;
  if (i<0 || i>=nenums) {
    g_strfreev(enums);
    return g_strdup_printf("<invalid:%d>", i);
  }
  tostring = g_strdup(enums[i]);
  g_strfreev(enums);
  return tostring;
}

/* default functions for stringeans */

int owl_variable_string_validate_default(const struct _owl_variable *v, const char *newval, void *dummy) {
  if (newval == NULL) return(0);
  else return (1);
}

int owl_variable_string_set_default(owl_variable *v, const char *newval, void *dummy) {
  GValue value = {0};
  g_value_init(&value, G_TYPE_STRING);
  g_value_set_static_string(&value, newval);
  if (v->validate_fn) {
    if (!owl_variable_invoke_validator(v,&value)) {
      return(-1);
    }
  }

  /* set_string dups the string for us */
  g_value_set_string(&(v->val), newval);
  return(0);
}

int owl_variable_string_set_fromstring_default(owl_variable *v, const char *newval, void *dummy) 
{
  GValue val = {0};
  int ret = -1;
  g_value_init(&val, G_TYPE_STRING);
  /* we don't need to dup the string because we don't own it and the setter function we invoke will DTRT */
  g_value_set_static_string(&val, newval);
  ret = owl_variable_invoke_setter(v, &val);
  g_value_unset(&val);
  return ret;
}

CALLER_OWN char *owl_variable_string_get_tostring_default(const owl_variable *v, const char *val, void *dummy)
{
  if(val) {
    return g_strdup(val);
  } else {
    return g_strdup("");
  }
}

