#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

/* fn is "char *foo(int argc, char **argv, char *buff)" */
#define OWLCMD_ARGS(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, fn, NULL, NULL, NULL, NULL, NULL }

/* fn is "void foo(void)" */
#define OWLCMD_VOID(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, fn, NULL, NULL, NULL, NULL }

/* fn is "void foo(int)" */
#define OWLCMD_INT(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, fn, NULL, NULL, NULL }

#define OWLCMD_ALIAS(name, actualname) \
        { name, OWL_CMD_ALIAS_SUMMARY_PREFIX actualname, "", "", OWL_CTX_ANY, \
          actualname, NULL, NULL, NULL, NULL, NULL, NULL }

/* fn is "char *foo(void *ctx, int argc, char **argv, char *buff)" */
#define OWLCMD_ARGS_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, ((char*(*)(void*,int,char**,char*))fn), NULL, NULL }

/* fn is "void foo(void)" */
#define OWLCMD_VOID_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, NULL, ((void(*)(void*))(fn)), NULL }

/* fn is "void foo(int)" */
#define OWLCMD_INT_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, NULL, NULL, ((void(*)(void*,int))fn) }


owl_cmd commands_to_init[]
  = {
  OWLCMD_ARGS("zlog", owl_command_zlog, OWL_CTX_ANY,
	      "send a login or logout notification",
	      "zlog in [tty]\nzlog out",
	      "zlog in will send a login notification, zlog out will send a\n"
	      "logout notification.  By default a login notification is sent\n"
	      "when owl is started and a logout notification is sent when owl\n"
	      "is exited.  This behavior can be changed with the 'startuplogin'\n"
	      "and 'shudownlogout' variables.  If a tty is specified for zlog in\n"
	      "then the owl variable 'tty' will be set to that string, causing\n"
	      "it to be used as the zephyr location tty.\n"),

  OWLCMD_VOID("quit", owl_command_quit, OWL_CTX_ANY,
	      "exit owl",
	      "",
	      "Exit owl and run any shutdown activities."),
  OWLCMD_ALIAS("exit", "quit"),
  OWLCMD_ALIAS("q",    "quit"),

  OWLCMD_ARGS("term", owl_command_term, OWL_CTX_ANY,
	      "control the terminal",
	      "term raise\n"
	      "term deiconify\n",
	      ""),

  OWLCMD_VOID("nop", owl_command_nop, OWL_CTX_ANY,
	      "do nothing",
	      "",
	      ""),
  
  OWLCMD_ARGS("start-command", owl_command_start_command, OWL_CTX_INTERACTIVE,
	      "prompts the user to enter a command",
	      "start-command [initial-value]",
	      "Initializes the command field to initial-value."),

  OWLCMD_ARGS("alias", owl_command_alias, OWL_CTX_ANY,
	      "creates a command alias",
	      "alias <new_command> <old_command>",
	      "Creates a command alias from new_command to old_command.\n"
	      "Any arguments passed to <new_command> will be appended to\n"
	      "<old_command> before it is executed.\n"),

  OWLCMD_ARGS("bindkey", owl_command_bindkey, OWL_CTX_ANY,
	      "creates a binding in a keymap",
	      "bindkey <keymap> <keyseq> command <command>",
	      "Binds a key sequence to a command within a keymap.\n"
	      "Use 'show keymaps' to see the existing keymaps.\n"
	      "Key sequences may be things like M-C-t or NPAGE.\n"),

  OWLCMD_ARGS("zwrite", owl_command_zwrite, OWL_CTX_INTERACTIVE,
	      "send a zephyr",
	      "zwrite [-n] [-C] [-c class] [-i instance] [-r realm] [-O opcde] [<user> ...] [-m <message...>]",
	      "Zwrite send a zephyr to the one or more users specified.\n\n"
	      "The following options are available:\n\n"
	      "-m    Specifies a message to send without prompting.\n"
	      "      Note that this does not yet log an outgoing message.\n"
	      "      This must be the last argument.\n\n"
	      "-n    Do not send a ping message.\n\n"
	      "-C    If the message is sent to more than one user include a\n"
	      "      \"cc:\" line in the text\n\n"
	      "-c class\n"
	      "      Send to the specified zephyr class\n\n"
	      "-i instance\n"
	      "      Send to the specified zephyr instance\n\n"
	      "-r realm\n"
	      "      Send to a foreign realm\n"
	      "-O opcode\n"
	      "      Send to the specified opcode\n"),

  OWLCMD_ARGS("aimwrite", owl_command_aimwrite, OWL_CTX_INTERACTIVE,
	      "send a zephyr",
	      "aimzwrite <user>",
	      "Send an aim message to a user.\n"),

  /*
  OWLCMD_ARGS("zcrypt", owl_command_zcrypt, OWL_CTX_INTERACTIVE,
	      "send an encrypted zephyr",
	      "zcrypt [-n] [-C] [-c class] [-i instance] [-r realm] [-O opcde] [-m <message...>]\n",
	      "Behaves like zwrite but uses encryption.  Not for use with\n"
	      "personal messages\n"),
  */
  
  OWLCMD_ARGS("reply", owl_command_reply,  OWL_CTX_INTERACTIVE,
	      "reply to the current message",
	      "reply [-e] [ sender | all | zaway ]",
	      "If -e is specified, the zwrite command line is presented to\n"
	      "allow editing.\n\n"
	      "If 'sender' is specified, reply to the sender.\n\n"
	      "If 'all' or no args are specified, reply publically to the\n"
	      "same class/instance for non-personal messages and to the\n"
	      "sender for personal messages.\n\n"
	      "If 'zaway' is specified, replies with a zaway message.\n\n"),

  OWLCMD_ARGS("set", owl_command_set, OWL_CTX_ANY,
	      "set a variable value",
	      "set [-q] <variable> [<value>]\n"
	      "set",
	      "Set the named variable to the specified value.  If no\n"
	      "arguments are used print the value of all variables.\n"
	      "If value is unspecified and the variable is a boolean, will set it to 'on'.\n"
	      "If -q is specified, is silent and doesn't print a message.\n"),

  OWLCMD_ARGS("unset", owl_command_unset, OWL_CTX_ANY,
	      "unset a boolean variable value",
	      "set [-q] <variable>\n"
	      "set",
	      "Set the named boolean variable to off.\n"
	      "If -q is specified, is silent and doesn't print a message.\n"),

  OWLCMD_ARGS("print", owl_command_print, OWL_CTX_ANY,
	      "print a variable value",
	      "print <variable>\n"
	      "print",
	      "Print the value of the named variable.  If no arugments\n"
	      "are used print the value of all variables.\n"),

  OWLCMD_ARGS("startup", owl_command_startup, OWL_CTX_ANY,
	      "run a command and set it to be run at every Owl startup",
	      "startup <commands> ...",
	      "Everything on the command line after the startup command\n"
	      "is executed as a normal owl command and is also placed in\n"
	      "a file so that the command is executed every time owl\n"
	      "is started"),

  OWLCMD_ARGS("unstartup", owl_command_unstartup, OWL_CTX_ANY,
	      "remove a command from the list of those to be run at Owl startup",
	      "unstartup <commands> ...",
	      ""),

  OWLCMD_VOID("version", owl_command_version, OWL_CTX_ANY,
	      "print the version of the running owl", "", ""),

  OWLCMD_ARGS("subscribe", owl_command_subscribe, OWL_CTX_ANY,
	      "subscribe to a zephyr class, instance, recipient",
	      "subscribe [-t] <class> <instance> [recipient]",
	      "Subscribe the specified class and instance.  If the recipient\n"
	      "is not listed on the command line it defaults\n"
	      "to * (the wildcard recipient).  If the -t option is present\n"
	      "the subscription will only be temporary, i.e., it will not\n"
	      "be written to the subscription file and will therefore not\n"
	      "be present the next time owl is started.\n"),
  OWLCMD_ALIAS("sub", "subscribe"),

  OWLCMD_ARGS("unsubscribe", owl_command_unsubscribe, OWL_CTX_ANY,
	      "unsubscribe from a zephyr class, instance, recipient",
	      "unsubscribe [-t] <class> <instance> [recipient]",
	      "Unsubscribe from the specified class and instance.  If the\n"
	      "recipient is not listed on the command line it defaults\n"
	      "to * (the wildcard recipient).  If the -t option is present\n"
	      "the unsubscription will only be temporary, i.e., it will not\n"
	      "be updated in the subscription file and will therefore not\n"
	      "be in effect the next time owl is started.\n"),
  OWLCMD_ALIAS("unsub", "unsubscribe"),

  OWLCMD_VOID("unsuball", owl_command_unsuball, OWL_CTX_ANY,
	      "unsubscribe from all zephyrs", "", ""),
  
  OWLCMD_VOID("getsubs", owl_command_getsubs, OWL_CTX_ANY,
	      "print all current subscriptions",
	      "getsubs",
	      "getsubs retrieves the current subscriptions from the server\n"
	      "and displays them.\n"),

  OWLCMD_ARGS("dump", owl_command_dump, OWL_CTX_ANY,
	      "dump messages to a file",
	      "dump <filename>",
	      "Dump messages in current view to the named file."),

  OWLCMD_ARGS("addbuddy", owl_command_addbuddy, OWL_CTX_INTERACTIVE,
	      "add a buddy to a buddylist",
	      "addbuddy aim <screenname>",
	      "Add the named buddy to your buddylist.  Eventually other protocols,\n"
	      "such as zephyr, will also be able to use this command.  For now the\n"
	      "only available protocol is 'aim', specified as the first argument."),

  OWLCMD_ARGS("delbuddy", owl_command_delbuddy, OWL_CTX_INTERACTIVE,
	      "delete a buddy from a buddylist",
	      "delbuddy aim <screenname>",
	      "Delete the named buddy to your buddylist.  Eventually other protocols,\n"
	      "such as zephyr, will also be able to use this command.  For now the\n"
	      "only available protocol is 'aim', specified as the first argument.\n"),

  OWLCMD_ARGS("smartzpunt", owl_command_smartzpunt, OWL_CTX_INTERACTIVE,
	      "creates a zpunt based on the current message",
	      "smartzpunt [-i | --instance]",
	      "Starts a zpunt command based on the current message's class\n"
	      "(and instance if -i is specified).\n"),

  OWLCMD_ARGS("zpunt", owl_command_zpunt, OWL_CTX_ANY,
	      "suppress a given zephyr triplet",
	      "zpunt <class> <instance> [recipient]\n"
	      "zpunt <instance>",
	      "The zpunt command will supress message to the specified\n"
	      "zephyr triplet.  In the second usage messages as supressed\n"
	      "for class MESSAGE and the named instance.\n\n"
	      "SEE ALSO:  zunpunt, show zpunts\n"),

  OWLCMD_ARGS("zunpunt", owl_command_zunpunt, OWL_CTX_ANY,
	      "undo a previous zpunt",
	      "zunpunt <class> <instance> [recipient]\n"
	      "zunpunt <instance>",
	      "The zunpunt command will allow messages that were previosly\n"
	      "suppressed to be received again.\n\n"
	      "SEE ALSO:  zpunt, show zpunts\n"),

  OWLCMD_VOID("info", owl_command_info, OWL_CTX_INTERACTIVE,
	      "display detailed information about the current message",
	      "", ""),
  
  OWLCMD_ARGS("help", owl_command_help, OWL_CTX_INTERACTIVE,
	      "display help on using owl",
	      "help [command]", ""),

  OWLCMD_ARGS("zlist", owl_command_zlist, OWL_CTX_INTERACTIVE,
	      "List users logged in",
	      "znol [-f file]",
	      "Print a znol-style listing of users logged in"),

  OWLCMD_ARGS("alist", owl_command_alist, OWL_CTX_INTERACTIVE,
	      "List AIM users logged in",
	      "alist",
	      "Print a listing of AIM users logged in"),

  OWLCMD_ARGS("blist", owl_command_blist, OWL_CTX_INTERACTIVE,
	      "List all buddies logged in",
	      "alist",
	      "Print a listing of buddies logged in, regardless of protocol."),

  OWLCMD_VOID("recv:shiftleft", owl_command_shift_left, OWL_CTX_INTERACTIVE,
	      "scrolls receive window to the left", "", ""),

  OWLCMD_VOID("recv:shiftright", owl_command_shift_right, OWL_CTX_INTERACTIVE,
	      "scrolls receive window to the left", "", ""),

  OWLCMD_VOID("recv:pagedown", owl_function_mainwin_pagedown, 
	      OWL_CTX_INTERACTIVE,
	      "scrolls down by a page", "", ""),

  OWLCMD_VOID("recv:pageup", owl_function_mainwin_pageup, OWL_CTX_INTERACTIVE,
	      "scrolls up by a page", "", ""),

  OWLCMD_INT ("recv:scroll", owl_function_page_curmsg, OWL_CTX_INTERACTIVE,
	      "scrolls current message up or down", 
	      "recv:scroll <numlines>", 
	      "Scrolls the current message up or down by <numlines>.\n"
	      "Scrolls up if <numlines> is negative, else scrolls down.\n"),

  OWLCMD_ARGS("next", owl_command_next, OWL_CTX_INTERACTIVE,
	      "move the pointer to the next message",
	      "recv:next [ --filter <name> ] [ --skip-deleted ] [ --last-if-none ]\n"
	      "          [ --smart-filter | --smart-filter-instance ]",
	      "Moves the pointer to the next message in the current view.\n"
	      "If --filter is specified, will only consider messages in\n"
	      "the filter <name>.\n"
	      "If --smart-filter or --smart-filter-instance is specified,\n"
	      "goes to the next message that is similar to the current message.\n"
	      "If --skip-deleted is specified, deleted messages will\n"
	      "be skipped.\n"
	      "If --last-if-none is specified, will stop at last message\n"
	      "in the view if no other suitable messages are found.\n"),
  OWLCMD_ALIAS("recv:next", "next"),

  OWLCMD_ARGS("prev", owl_command_prev, OWL_CTX_INTERACTIVE,
	      "move the pointer to the previous message",
	      "recv:prev [ --filter <name> ] [ --skip-deleted ] [ --first-if-none ]\n"
	      "          [ --smart-filter | --smart-filter-instance ]",
	      "Moves the pointer to the next message in the current view.\n"
	      "If --filter is specified, will only consider messages in\n"
	      "the filter <name>.\n"
	      "If --smart-filter or --smart-filter-instance is specified,\n"
	      "goes to the previous message that is similar to the current message.\n"
	      "If --skip-deleted is specified, deleted messages will\n"
	      "be skipped.\n"
	      "If --first-if-none is specified, will stop at first message\n"
	      "in the view if no other suitable messages are found.\n"),
  OWLCMD_ALIAS("recv:prev", "prev"),

  OWLCMD_ALIAS("recv:next-notdel", "recv:next --skip-deleted --last-if-none"),
  OWLCMD_ALIAS("next-notdel",      "recv:next --skip-deleted --last-if-none"),

  OWLCMD_ALIAS("recv:prev-notdel", "recv:prev --skip-deleted --first-if-none"),
  OWLCMD_ALIAS("prev-notdel",      "recv:prev --skip-deleted --first-if-none"),

  OWLCMD_ALIAS("recv:next-personal", "recv:next --filter personal"),

  OWLCMD_ALIAS("recv:prev-personal", "recv:prev --filter personal"),

  OWLCMD_VOID("first", owl_command_first, OWL_CTX_INTERACTIVE,
	      "move the pointer to the first message", "", ""),
  OWLCMD_ALIAS("recv:first", "first"),

  OWLCMD_VOID("last", owl_command_last, OWL_CTX_INTERACTIVE,
	      "move the pointer to the last message", "", 
	      "Moves the pointer to the last message in the view.\n"
	      "If we are already at the last message in the view,\n"
	      "blanks the screen and moves just past the end of the view\n"
	      "so that new messages will appear starting at the top\n"
	      "of the screen.\n"),
  OWLCMD_ALIAS("recv:last", "last"),

  OWLCMD_VOID("expunge", owl_command_expunge, OWL_CTX_INTERACTIVE,
	      "remove all messages marked for deletion", "", ""),

  OWLCMD_VOID("resize", owl_command_resize, OWL_CTX_ANY,
	      "resize the window to the current screen size", "", ""),

  OWLCMD_VOID("redisplay", owl_command_redisplay, OWL_CTX_ANY,
	      "redraw the entire window", "", ""),

  OWLCMD_VOID("suspend", owl_command_suspend, OWL_CTX_ANY,
	      "suspend owl", "", ""),

  OWLCMD_ARGS("echo", owl_command_echo, OWL_CTX_ANY,
	      "pops up a message in popup window",
	      "echo [args .. ]\n\n", ""),

  OWLCMD_ARGS("exec", owl_command_exec, OWL_CTX_ANY,
	      "run a command from the shell",
	      "exec [args .. ]", ""),

  OWLCMD_ARGS("aexec", owl_command_aexec, OWL_CTX_INTERACTIVE,
	      "run a command from the shell and display in an admin message",
	      "aexec [args .. ]", ""),

  OWLCMD_ARGS("pexec", owl_command_pexec, OWL_CTX_INTERACTIVE,
	      "run a command from the shell and display in a popup window",
	      "pexec [args .. ]", ""),

  OWLCMD_ARGS("perl", owl_command_perl, OWL_CTX_ANY,
	      "run a perl expression",
	      "perl [args .. ]", ""),

  OWLCMD_ARGS("aperl", owl_command_aperl, OWL_CTX_INTERACTIVE,
	      "run a perl expression and display in an admin message",
	      "aperl [args .. ]", ""),

  OWLCMD_ARGS("pperl", owl_command_pperl, OWL_CTX_INTERACTIVE,
	      "run a perl expression and display in a popup window",
	      "pperl [args .. ]", ""),

  OWLCMD_ARGS("multi", owl_command_multi, OWL_CTX_ANY,
	      "runs multiple ;-separated commands",
	      "multi <command1> ( ; <command2> )*\n",
	      "Runs multiple semicolon-separated commands in order.\n"
	      "Note quoting isn't supported here yet.\n"
	      "If you want to do something fancy, use perl.\n"),

  OWLCMD_ARGS("(", owl_command_multi, OWL_CTX_ANY,
	      "runs multiple ;-separated commands",
	      "'(' <command1> ( ; <command2> )* ')'\n",
	      "Runs multiple semicolon-separated commands in order.\n"
	      "You must have a space before the final ')'\n"
	      "Note quoting isn't supported here yet.\n"
	      "If you want to do something fancy, use perl.\n"),

  OWLCMD_VOID("pop-message", owl_command_pop_message, OWL_CTX_RECWIN,
	      "pops up a message in a window", "", ""),

  OWLCMD_VOID("openurl", owl_command_openurl, OWL_CTX_INTERACTIVE,
	      "opens up a URL from the current message",
	      "", 
	      "Uses the 'webbrowser' variable to determine\n"
	      "which browser to use.  Currently, 'netscape'\n"
	      "and 'galeon' are supported.\n"),

  OWLCMD_ARGS("zaway", owl_command_zaway, OWL_CTX_INTERACTIVE,
	      "running a command from the shell",
	      "zaway [ on | off | toggle ]\n"
	      "zaway <message>",
	      "Turn on or off the default zaway message.  If a message is\n"
	      "specified turn on zaway with that message\n"),

  OWLCMD_ARGS("load-subs", owl_command_loadsubs, OWL_CTX_ANY,
	      "load subscriptions from a file",
	      "load-subs <file>\n", ""),

  OWLCMD_ARGS("loadsubs", owl_command_loadsubs, OWL_CTX_ANY,
	      "load subscriptions from a file",
	      "loadsubs <file>\n", ""),

  OWLCMD_ARGS("loadloginsubs", owl_command_loadloginsubs, OWL_CTX_ANY,
	      "load login subscriptions from a file",
	      "loadloginsubs <file>\n",
	      "The file should contain a list of usernames, one per line."),

  OWLCMD_VOID("about", owl_command_about, OWL_CTX_INTERACTIVE,
	      "print information about owl", "", ""),

  OWLCMD_VOID("status", owl_command_status, OWL_CTX_ANY,
	      "print status information about the running owl", "", ""),
  
  OWLCMD_ARGS("zlocate", owl_command_zlocate, OWL_CTX_INTERACTIVE,
	      "locate a user",
	      "zlocate [-d] <user> ...", 
	      "Performs a zlocate on one ore more users and puts the result\n"
	      "int a popwin.  If -d is specified, does not authenticate\n"
	      "the lookup request.\n"),
  
  OWLCMD_ARGS("filter", owl_command_filter, OWL_CTX_ANY,
	      "create a message filter",
	      "filter <name> [ -c color ] [ <expression> ... ]",
	      "The filter command creates a filter with the specified name,\n"
	      "or if one already exists it is replaced.  Example filter\n"
	      "syntax would be:\n\n"
	      "     filter myfilter -c red ( class ^foobar$ ) or ( class ^quux$ and instance ^bar$ )\n\n"
	      "Valid matching fields are class, instance, recipient, sender,\n"
	      "opcode and realm. Valid operations are 'and', 'or' and 'not'.\n"
	      "Spaces must be present before and after parenthesis.  If the\n"
	      "optional color argument is used it specifies the color that\n"
	      "messages matching this filter should be displayed in.\n\n"

	      "SEE ALSO: view, viewclass, viewuser\n"),

  OWLCMD_ARGS("colorview", owl_command_colorview, OWL_CTX_INTERACTIVE,
	      "change the color on the current filter",
	      "colorview <color>",
	      "The color of messages in the current filter will be changed\n"
	      "to <color>.  Use the 'show colors' command for a list\n"
	      "of valid colors.\n\n"
	      "SEE ALSO: 'show colors'\n"),

  OWLCMD_ARGS("view", owl_command_view, OWL_CTX_INTERACTIVE,
	      "view messages matching a filter",
	      "view <filter>\n"
	      "view -d <expression>\n"
	      "view --home",
	      "In the first usage, The view command sets the current view to\n"
	      "the specified filter.   This causes only messages matching\n"
	      "that filter to be displayed.\n"
	      "\n"
	      "In the second usage a filter expression\n"
	      "and be specified directly\n"
	      "after -d.  Owl will build an internal filter based on this\n"
	      "filter and change the current view to use it.\n\n"
	      "If --home is specified, switches to the view specified by\n"
	      "the 'view_home' variable.\n\n"
	      "SEE ALSO: filter, viewclass, viewuser\n"),

  OWLCMD_ARGS("smartnarrow", owl_command_smartnarrow, OWL_CTX_INTERACTIVE,
	      "view only messages similar to the current message",
	      "smartnarrow [-i | --instance]",
	      "If the curmsg is a personal message narrow\n"
	      "   to the converstaion with that user.\n"
	      "If the curmsg is a class message, instance foo, recip *\n"
	      "   message, narrow to the class, inst.\n"
	      "If the curmsg is a class message then narrow\n"
	      "    to the class.\n"
	      "If the curmsg is a class message and '-i' is specied\n"
	      "    then narrow to the class, instance\n"),

  OWLCMD_ARGS("smartfilter", owl_command_smartfilter, OWL_CTX_INTERACTIVE,
	      "returns the name of a filter based on the current message",
	      "smartfilter [-i | --instance]",
	      "If the curmsg is a personal message, the filter is\n"
	      "   the converstaion with that user.\n"
	      "If the curmsg is a class message, instance foo, recip *\n"
	      "   message, the filter is the class, inst.\n"
	      "If the curmsg is a class message, the filter is that class.\n"
	      "If the curmsg is a class message and '-i' is specied\n"
	      "    the filter is that <class,instance> pair\n"),

  OWLCMD_ARGS("viewclass", owl_command_viewclass, OWL_CTX_INTERACTIVE,
	      "view messages matching a particular class",
	      "viewclass <class>",
	      "The viewclass command will automatically create a filter\n"
	      "matching the specified class and switch the current view\n"
	      "to it.\n\n"
	      "SEE ALSO: filter, view, viewuser\n"),
  OWLCMD_ALIAS("vc", "viewclass"),

  OWLCMD_ARGS("viewuser", owl_command_viewuser, OWL_CTX_INTERACTIVE,
	      "view messages matching a particular user",
	      "viewuser <user>",
	      "The viewuser command will automatically create a filter\n"
	      "matching the specified user and switch the current\n"
	      "view to it.\n\n"
	      "SEE ALSO: filter, view, viewclass\n"),
  OWLCMD_ALIAS("vu", "viewuser"),

  OWLCMD_ARGS("show", owl_command_show, OWL_CTX_INTERACTIVE,
	      "show information",
	      "show variables\n"
	      "show variable <variable>\n"
	      "show filters\n"
	      "show filter <filter>\n"
	      "show keymaps\n"
	      "show keymap <keymap>\n"
	      "show commands\n"
	      "show command <command>\n"
	      "show subs\n"
	      "show subscriptions\n"
	      "show zpunts\n"
	      "show colors\n"
	      "show terminal\n"
	      "show version\n"
	      "show status\n",

	      "Show colors will display a list of valid colors for the\n"
	      "     terminal."
	      "Show filters will list the names of all filters.\n"
	      "Show filter <filter> will show the definition of a particular\n"
	      "     filter.\n\n"
	      "Show zpunts will show the active zpunt filters.\n\n"
	      "Show keymaps will list the names of all keymaps.\n"
	      "Show keymap <keymap> will show the key bindings in a keymap.\n\n"
	      "Show commands will list the names of all keymaps.\n"
	      "Show command <command> will provide information about a command.\n\n"
	      "Show variables will list the names of all variables.\n\n"
	      "SEE ALSO: filter, view, alias, bindkey, help\n"),
  
  OWLCMD_ARGS("delete", owl_command_delete, OWL_CTX_INTERACTIVE,
	      "mark a message for deletion",
	      "delete [ -id msgid ] [ --no-move ]\n"
	      "delete view\n"
	      "delete trash",
	      "If no message id is specified the current message is marked\n"
	      "for deletion.  Otherwise the message with the given message\n"
	      "id is marked for deltion.\n"
	      "If '--no-move' is specified, don't move after deletion.\n"
	      "If 'trash' is specified, deletes all trash/auto messages\n"
	      "in the current view.\n"
	      "If 'view' is specified, deletes all messages in the\n"
	      "current view.\n"),
  OWLCMD_ALIAS("del", "delete"),

  OWLCMD_ARGS("undelete", owl_command_undelete, OWL_CTX_INTERACTIVE,
	      "unmark a message for deletion",
	      "undelete [ -id msgid ] [ --no-move ]\n"
	      "undelete view",
	      "If no message id is specified the current message is\n"
	      "unmarked for deletion.  Otherwise the message with the\n"
	      "given message id is marked for undeltion.\n"
	      "If '--no-move' is specified, don't move after deletion.\n"
	      "If 'view' is specified, undeletes all messages\n"
	      "in the current view.\n"),
  OWLCMD_ALIAS("undel", "undelete"),

  OWLCMD_VOID("beep", owl_command_beep, OWL_CTX_ANY,
	      "ring the terminal bell",
	      "beep",
	      "Beep will ring the terminal bell.\n"
	      "If the variable 'bell' has been\n"
	      "set to 'off' this command does nothing.\n"),

  OWLCMD_ARGS("debug", owl_command_debug, OWL_CTX_ANY,
	      "prints a message into the debug log",
	      "debug <message>", ""),

  OWLCMD_ARGS("getview", owl_command_getview, OWL_CTX_INTERACTIVE,
	      "returns the name of the filter for the current view",
	      "", ""),

  OWLCMD_ARGS("getvar", owl_command_getvar, OWL_CTX_INTERACTIVE,
	      "returns the value of a variable",
	      "getvar <varname>", ""),

  OWLCMD_ARGS("search", owl_command_search, OWL_CTX_INTERACTIVE,
	      "search messages for a particular string",
	      "search [-r] [<string>]",
	      "The search command will find messages that contain the\n"
	      "specified string and move the cursor there.  If no string\n"
	      "argument is supplied then the previous one is used.  By\n"
	      "default searches are done fowards, if -r is used the search\n"
	      "is performed backwards"),

  OWLCMD_ARGS("aimlogin", owl_command_aimlogin, OWL_CTX_ANY,
	      "login to an AIM account",
	      "aimlogin <screenname> <password>\n",
	      ""),

  OWLCMD_ARGS("aimlogout", owl_command_aimlogout, OWL_CTX_ANY,
	      "logout from AIM",
	      "aimlogout\n",
	      ""),


  /****************************************************************/
  /************************* EDIT-SPECIFIC ************************/
  /****************************************************************/

  OWLCMD_VOID_CTX("edit:move-next-word", owl_editwin_move_to_nextword, 
		  OWL_CTX_EDIT,
		  "moves cursor forward a word",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-prev-word", owl_editwin_move_to_previousword, 
		  OWL_CTX_EDIT,
		  "moves cursor backwards a word",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-to-buffer-start", owl_editwin_move_to_top,
		  OWL_CTX_EDIT,
		  "moves cursor to the top left (start) of the buffer",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-to-buffer-end", owl_editwin_move_to_end, 
		  OWL_CTX_EDIT,
		  "moves cursor to the bottom right (end) of the buffer",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-to-line-end", owl_editwin_move_to_line_end, 
		  OWL_CTX_EDIT,
		  "moves cursor to the end of the line",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-to-line-start", owl_editwin_move_to_line_start, 
		  OWL_CTX_EDIT,
		  "moves cursor to the beginning of the line",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-left", owl_editwin_key_left, 
		  OWL_CTX_EDIT,
		  "moves the cursor left by a character",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-right", owl_editwin_key_right,
		  OWL_CTX_EDIT,
		  "moves the cursor right by a character",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-next-word", owl_editwin_delete_nextword,
		  OWL_CTX_EDIT,
		  "deletes the word to the right of the cursor",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-prev-word", owl_editwin_delete_previousword,
		  OWL_CTX_EDIT,
		  "deletes the word to the left of the cursor",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-prev-char", owl_editwin_backspace,
		  OWL_CTX_EDIT,
		  "deletes the character to the left of the cursor",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-next-char", owl_editwin_delete_char, 
		  OWL_CTX_EDIT,
		  "deletes the character to the right of the cursor",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-to-line-end", owl_editwin_delete_to_endofline,
		  OWL_CTX_EDIT,
		  "deletes from the cursor to the end of the line",
		  "", ""),

  OWLCMD_VOID_CTX("edit:delete-all", owl_editwin_clear, 
		  OWL_CTX_EDIT,
		  "deletes all of the contents of the buffer",
		  "", ""),

  OWLCMD_VOID_CTX("edit:transpose-chars", owl_editwin_transpose_chars,
		  OWL_CTX_EDIT,
		  "Interchange characters around point, moving forward one character.",
		  "", ""),

  OWLCMD_VOID_CTX("edit:fill-paragraph", owl_editwin_fill_paragraph, 
		  OWL_CTX_EDIT,
		  "fills the current paragraph to line-wrap well",
		  "", ""),

  OWLCMD_VOID_CTX("edit:recenter", owl_editwin_recenter, 
		  OWL_CTX_EDIT,
		  "recenters the buffer",
		  "", ""),

  OWLCMD_ARGS_CTX("edit:insert-text", owl_command_edit_insert_text, 
		  OWL_CTX_EDIT,
		  "inserts text into the buffer",
		  "edit:insert-text <text>", ""),

  OWLCMD_VOID_CTX("edit:cancel", owl_command_edit_cancel, 
		  OWL_CTX_EDIT,
		  "cancels the current command",
		  "", ""),

  OWLCMD_VOID_CTX("edit:history-next", owl_command_edit_history_next, 
		  OWL_CTX_EDIT,
		  "replaces the text with the previous history",
		  "", ""),

  OWLCMD_VOID_CTX("edit:history-prev", owl_command_edit_history_prev, 
		  OWL_CTX_EDIT,
		  "replaces the text with the previous history",
		  "", ""),

  OWLCMD_VOID_CTX("editline:done", owl_command_editline_done, 
		  OWL_CTX_EDITLINE,
		  "completes the command (eg, executes command being composed)",
		  "", ""),

  OWLCMD_VOID_CTX("editmulti:move-up-line", owl_editwin_key_up, 
		  OWL_CTX_EDITMULTI,
		  "moves the cursor up one line",
		  "", ""),

  OWLCMD_VOID_CTX("editmulti:move-down-line", owl_editwin_key_down, 
		  OWL_CTX_EDITMULTI,
		  "moves the cursor down one line",
		  "", ""),

  OWLCMD_VOID_CTX("editmulti:done", owl_command_editmulti_done, 
		  OWL_CTX_EDITMULTI,
		  "completes the command (eg, sends message being composed)",
		  "", ""),

  OWLCMD_VOID_CTX("editmulti:done-or-delete", owl_command_editmulti_done_or_delete, 
		  OWL_CTX_EDITMULTI,
		  "completes the command, but only if at end of message",
		  "", 
		  "If only whitespace is to the right of the cursor,\n"
		  "runs 'editmulti:done'.\n"\
		  "Otherwise runs 'edit:delete-next-char'\n"),

  /****************************************************************/
  /********************** POPLESS-SPECIFIC ************************/
  /****************************************************************/

  OWLCMD_VOID_CTX("popless:scroll-down-page", owl_viewwin_pagedown, 
		  OWL_CTX_POPLESS,
		  "scrolls down one page",
		  "", ""),

  OWLCMD_VOID_CTX("popless:scroll-down-line", owl_viewwin_linedown, 
		  OWL_CTX_POPLESS,
		  "scrolls down one line",
		  "", ""),

  OWLCMD_VOID_CTX("popless:scroll-up-page", owl_viewwin_pageup, 
		  OWL_CTX_POPLESS,
		  "scrolls up one page",
		  "", ""),

  OWLCMD_VOID_CTX("popless:scroll-up-line", owl_viewwin_lineup, 
		  OWL_CTX_POPLESS,
		  "scrolls up one line",
		  "", ""),

  OWLCMD_VOID_CTX("popless:scroll-to-top", owl_viewwin_top, 
		  OWL_CTX_POPLESS,
		  "scrolls to the top of the buffer",
		  "", ""),

  OWLCMD_VOID_CTX("popless:scroll-to-bottom", owl_viewwin_bottom, 
		  OWL_CTX_POPLESS,
		  "scrolls to the bottom of the buffer",
		  "", ""),

  OWLCMD_INT_CTX ("popless:scroll-right", owl_viewwin_right, 
		  OWL_CTX_POPLESS,
		  "scrolls right in the buffer",
		  "popless:scroll-right <num-chars>", ""),

  OWLCMD_INT_CTX ("popless:scroll-left", owl_viewwin_left, 
		  OWL_CTX_POPLESS,
		  "scrolls left in the buffer",
		  "popless:scroll-left <num-chars>", ""),

  OWLCMD_VOID_CTX("popless:quit", owl_command_popless_quit, 
		  OWL_CTX_POPLESS,
		  "exits the popless window",
		  "", ""),

  /* This line MUST be last! */
  { NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

};

void owl_command_info() {
  owl_function_info();
}

void owl_command_nop() {
}

char *owl_command_help(int argc, char **argv, char *buff) {
  if (argc!=2) {
    owl_help();
    return NULL;
  }
  
  owl_function_help_for_command(argv[1]);
  return NULL;
}

char *owl_command_zlist(int argc, char **argv, char *buff) {
  int elapsed=0, timesort=0;
  char *file=NULL;

  argc--;
  argv++;
  while (argc) {
    if (!strcmp(argv[0], "-e")) {
      elapsed=1;
      argc--;
      argv++;
    } else if (!strcmp(argv[0], "-t")) {
      timesort=1;
      argc--;
      argv++;
    } else if (!strcmp(argv[0], "-f")) {
      if (argc==1) {
	owl_function_makemsg("zlist: -f needs an argument");
	return(NULL);
      }
      file=argv[1];
      argc-=2;
      argv+=2;
    } else {
      owl_function_makemsg("zlist: unknown argument");
      return(NULL);
    }
  }
  owl_function_buddylist(0, 1, file);
  return(NULL);
}

char *owl_command_alist() {
  owl_function_buddylist(1, 0, NULL);
  return(NULL);
}

char *owl_command_blist() {
  owl_function_buddylist(1, 1, NULL);
  return(NULL);
}

void owl_command_about() {
  owl_function_about();
}

void owl_command_version() {
  char buff[1024];

  sprintf(buff, "Owl version %s", OWL_VERSION_STRING);
  owl_function_makemsg(buff);
}

char *owl_command_addbuddy(int argc, char **argv, char *buff)
{
  if (!owl_global_is_aimloggedin(&g)) {
    owl_function_makemsg("addbuddy: You must be logged into aim to use this command.");
    return(NULL);
  }

  if (argc!=3) {
    owl_function_makemsg("usage: addbuddy <protocol> <buddyname>");
    return(NULL);
  }

  if (strcasecmp(argv[1], "aim")) {
    owl_function_makemsg("addbuddy: currently the only supported protocol is 'aim'");
    return(NULL);
  }

  owl_aim_addbuddy(argv[2]);
  owl_function_makemsg("%s added as AIM buddy for %s", argv[2], owl_global_get_aim_screenname(&g));

  return(NULL);
}

char *owl_command_delbuddy(int argc, char **argv, char *buff)
{
  if (!owl_global_is_aimloggedin(&g)) {
    owl_function_makemsg("delbuddy: You must be logged into aim to use this command.");
    return(NULL);
  }

  if (argc!=3) {
    owl_function_makemsg("usage: delbuddy <protocol> <buddyname>");
    return(NULL);
  }

  if (strcasecmp(argv[1], "aim")) {
    owl_function_makemsg("delbuddy: currently the only supported protocol is 'aim'");
    return(NULL);
  }

  owl_aim_delbuddy(argv[2]);
  owl_function_makemsg("%s deleted as AIM buddy for %s", argv[2], owl_global_get_aim_screenname(&g));

  return(NULL);
}

char *owl_command_startup(int argc, char **argv, char *buff)
{
  char *ptr;

  if (argc<2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return(NULL);
  }

  /* we'll be pedantic here, just in case this command gets another name
   *  (or is aliased) start after the first space on the line
   */
  ptr=strchr(buff, ' ');
  if (!ptr) {
    owl_function_makemsg("Parse error finding command for startup");
    return(NULL);
  }

  owl_function_command(ptr+1);
  owl_function_addstartup(ptr+1);

  return(NULL);
}

char *owl_command_unstartup(int argc, char **argv, char *buff)
{
  char *ptr;

  if (argc<2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return(NULL);
  }

  /* we'll be pedantic here, just in case this command gets another name
   *  (or is aliased) start after the first space on the line
   */
  ptr=strchr(buff, ' ');
  if (!ptr) {
    owl_function_makemsg("Parse error finding command for unstartup");
    return(NULL);
  }

  owl_function_delstartup(ptr+1);

  return(NULL);
}

char *owl_command_dump(int argc, char **argv, char *buff) {
  if (argc!=2) {
    owl_function_makemsg("usage: dump <filename>");
    return(NULL);
  }

  owl_function_dump(argv[1]);
  return(NULL);
}

char *owl_command_next(int argc, char **argv, char *buff) {
  char *filter=NULL;
  int skip_deleted=0, last_if_none=0;
  while (argc>1) {
    if (argc>=1 && !strcmp(argv[1], "--skip-deleted")) {
      skip_deleted=1;
      argc-=1; argv+=1; 
    } else if (argc>=1 && !strcmp(argv[1], "--last-if-none")) {
      last_if_none=1;
      argc-=1; argv+=1; 
    } else if (argc>=2 && !strcmp(argv[1], "--filter")) {
      filter = owl_strdup(argv[2]);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter")) {
      filter = owl_function_smartfilter(0);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter-instance")) {
      filter = owl_function_smartfilter(1);
      argc-=2; argv+=2; 
    } else {
      owl_function_makemsg("Invalid arguments to command 'next'.");
      return(NULL);
    }
  }
  owl_function_nextmsg_full(filter, skip_deleted, last_if_none);
  if (filter) owl_free(filter);
  return(NULL);
}

char *owl_command_prev(int argc, char **argv, char *buff) {
  char *filter=NULL;
  int skip_deleted=0, first_if_none=0;
  while (argc>1) {
    if (argc>=1 && !strcmp(argv[1], "--skip-deleted")) {
      skip_deleted=1;
      argc-=1; argv+=1; 
    } else if (argc>=1 && !strcmp(argv[1], "--first-if-none")) {
      first_if_none=1;
      argc-=1; argv+=1; 
    } else if (argc>=2 && !strcmp(argv[1], "--filter")) {
      filter = owl_strdup(argv[2]);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter")) {
      filter = owl_function_smartfilter(0);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter-instance")) {
      filter = owl_function_smartfilter(1);
      argc-=2; argv+=2;  
   } else {
      owl_function_makemsg("Invalid arguments to command 'prev'.");
      return(NULL);
    }
  }
  owl_function_prevmsg_full(filter, skip_deleted, first_if_none);
  if (filter) owl_free(filter);
  return(NULL);
}

char *owl_command_smartnarrow(int argc, char **argv, char *buff) {
  char *filtname = NULL;

  if (argc == 1) {
    filtname = owl_function_smartfilter(0);
  } else if (argc == 2 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--instance"))) {
    filtname = owl_function_smartfilter(1);
  } else {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);    
  }
  if (filtname) {
    owl_function_change_view(filtname);
    owl_free(filtname);
  }
  return NULL;
}

char *owl_command_smartfilter(int argc, char **argv, char *buff) {
  char *filtname = NULL;

  if (argc == 1) {
    filtname = owl_function_smartfilter(0);
  } else if (argc == 2 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--instance"))) {
    filtname = owl_function_smartfilter(1);
  } else {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);    
  }
  return filtname;
}

void owl_command_expunge() {
  owl_function_expunge();
}

void owl_command_first() {
  owl_global_set_rightshift(&g, 0);
  owl_function_firstmsg();
}

void owl_command_last() {
  owl_function_lastmsg();
}

void owl_command_resize() {
  owl_function_resize();
}

void owl_command_redisplay() {
  owl_function_full_redisplay();
  owl_global_set_needrefresh(&g);
}

void owl_command_shift_right() {
  owl_function_shift_right();
}

void owl_command_shift_left() {
  owl_function_shift_left();
}

void owl_command_unsuball() {
  owl_function_unsuball();
}

char *owl_command_loadsubs(int argc, char **argv, char *buff) {
  if (argc == 2) {
    owl_function_loadsubs(argv[1]);
  } else if (argc == 1) {
    owl_function_loadsubs(NULL);
  } else {
    owl_function_makemsg("Wrong number of arguments for load-subs.");
    return(NULL);
  }
  return(NULL);
}


char *owl_command_loadloginsubs(int argc, char **argv, char *buff) {
  if (argc == 2) {
    owl_function_loadloginsubs(argv[1]);
  } else if (argc == 1) {
    owl_function_loadloginsubs(NULL);
  } else {
    owl_function_makemsg("Wrong number of arguments for load-subs.");
    return(NULL);
  }
  return(NULL);
}

void owl_command_suspend() {
  owl_function_suspend();
}

char *owl_command_start_command(int argc, char **argv, char *buff) {
  buff = skiptokens(buff, 1);
  owl_function_start_command(buff);
  return NULL;
}

char *owl_command_zaway(int argc, char **argv, char *buff) {
  
  if ((argc==1) ||
      ((argc==2) && !strcmp(argv[1], "on"))) {
    owl_global_set_zaway_msg(&g, owl_global_get_zaway_msg_default(&g));
    owl_function_zaway_on();
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "off")) {
    owl_function_zaway_off();
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "toggle")) {
    owl_function_zaway_toggle();
    return NULL;
  }

  buff = skiptokens(buff, 1);
  owl_global_set_zaway_msg(&g, buff);
  owl_function_zaway_on();
  return NULL;
}


char *owl_command_set(int argc, char **argv, char *buff) {
  char *var, *val;
  int  silent=0;
  int requirebool=0;

  if (argc == 1) {
    owl_function_printallvars();
    return NULL;
  } 

  if (argc > 1 && !strcmp("-q",argv[1])) {
    silent = 1;
    argc--; argv++;
  }

  if (argc == 2) {
    var=argv[1];
    val="on";
    requirebool=1;
  } else if (argc == 3) {
    var=argv[1];
    val=argv[2];
  } else {
    owl_function_makemsg("Wrong number of arguments for set command");
    return NULL;
  }
  owl_variable_set_fromstring(owl_global_get_vardict(&g), var, val, !silent, requirebool);
  return NULL;
}

char *owl_command_unset(int argc, char **argv, char *buff) {
  char *var, *val;
  int  silent=0;

  if (argc > 1 && !strcmp("-q",argv[1])) {
    silent = 1;
    argc--; argv++;
  }
  if (argc == 2) {
    var=argv[1];
    val="off";
  } else {
    owl_function_makemsg("Wrong number of arguments for unset command");
    return NULL;
  }
  owl_variable_set_fromstring(owl_global_get_vardict(&g), var, val, !silent, 1);
  return NULL;
}

char *owl_command_print(int argc, char **argv, char *buff) {
  char *var;
  char valbuff[1024];

  if (argc==1) {
    owl_function_printallvars();
    return NULL;
  } else if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments for print command");
    return NULL;
  }

  var=argv[1];
    
  if (0 == owl_variable_get_tostring(owl_global_get_vardict(&g), 
				     var, valbuff, 1024)) {
    owl_function_makemsg("%s = '%s'", var, valbuff);
  } else {
    owl_function_makemsg("Unknown variable '%s'.", var);
  }
  return NULL;
}


char *owl_command_exec(int argc, char **argv, char *buff) {
  return owl_function_exec(argc, argv, buff, 0);
}

char *owl_command_pexec(int argc, char **argv, char *buff) {
  return owl_function_exec(argc, argv, buff, 1);
}

char *owl_command_aexec(int argc, char **argv, char *buff) {
  return owl_function_exec(argc, argv, buff, 2);
}

char *owl_command_perl(int argc, char **argv, char *buff) {
  return owl_function_perl(argc, argv, buff, 0);
}

char *owl_command_pperl(int argc, char **argv, char *buff) {
  return owl_function_perl(argc, argv, buff, 1);
}

char *owl_command_aperl(int argc, char **argv, char *buff) {
  return owl_function_perl(argc, argv, buff, 2);
}

char *owl_command_multi(int argc, char **argv, char *buff) {
  char *lastrv = NULL, *newbuff;
  char **commands;
  int  ncommands, i;
  if (argc < 2) {
    owl_function_makemsg("Invalid arguments to 'multi' command.");    
    return NULL;
  }
  newbuff = owl_strdup(buff);
  newbuff = skiptokens(newbuff, 1);
  if (!strcmp(argv[0], "(")) {
    for (i=strlen(newbuff)-1; i>=0; i--) {
      if (newbuff[i] == ')') {
	newbuff[i] = '\0';
	break;
      } else if (newbuff[i] != ' ') {
	owl_function_makemsg("Invalid arguments to 'multi' command.");    
	owl_free(newbuff);
	return NULL;
      }
    }
  }
  commands = atokenize(newbuff, ";", &ncommands);
  for (i=0; i<ncommands; i++) {
    if (lastrv) {
      owl_free(lastrv);
    }
    lastrv = owl_function_command(commands[i]);
  }
  atokenize_free(commands, ncommands);
  return lastrv;
}


char *owl_command_alias(int argc, char **argv, char *buff) {
  if (argc < 3) {
    owl_function_makemsg("Invalid arguments to 'alias' command.");
    return NULL;
  }
  buff = skiptokens(buff, 2);
  owl_function_command_alias(argv[1], buff);
  return (NULL);
}


char *owl_command_bindkey(int argc, char **argv, char *buff) {
  owl_keymap *km;
  int ret;

  if (argc < 5 || strcmp(argv[3], "command")) {
    owl_function_makemsg("Usage: bindkey <keymap> <binding> command <cmd>", argv[3], argc);
    return NULL;
  }
  km = owl_keyhandler_get_keymap(owl_global_get_keyhandler(&g), argv[1]);
  if (!km) {
    owl_function_makemsg("No such keymap '%s'", argv[1]);
    return NULL;
  }
  buff = skiptokens(buff, 4);
  ret = owl_keymap_create_binding(km, argv[2], buff, NULL, "*user*");
  if (ret!=0) {
    owl_function_makemsg("Unable to bind '%s' in keymap '%s' to '%s'.",
			 argv[2], argv[1], buff);
    return NULL;
  }
  return NULL;
}

void owl_command_quit() {
  owl_function_quit();
}

char *owl_command_debug(int argc, char **argv, char *buff) {
  if (argc<2) {
    owl_function_makemsg("Need at least one argument to debug command");
    return(NULL);
  }

  if (!owl_global_is_debug_fast(&g)) {
    owl_function_makemsg("Debugging is not turned on");
    return(NULL);
  }

  owl_function_debugmsg(argv[1]);
  return(NULL);
}

char *owl_command_term(int argc, char **argv, char *buff) {
  if (argc<2) {
    owl_function_makemsg("Need at least one argument to the term command");
    return(NULL);
  }

  if (!strcmp(argv[1], "raise")) {
    owl_function_xterm_raise();
  } else if (!strcmp(argv[1], "deiconify")) {
    owl_function_xterm_deiconify();
  } else {
    owl_function_makemsg("Unknown terminal subcommand");
  }
  return(NULL);
}

char *owl_command_zlog(int argc, char **argv, char *buff) {
  if ((argc<2) || (argc>3)) {
    owl_function_makemsg("Wrong number of arguments for zlog command");
    return(NULL);
  }

  if (!strcmp(argv[1], "in")) {
    if (argc>2) {
      owl_global_set_tty(&g, argv[2]);
    }
    owl_zephyr_zlog_in();
  } else if (!strcmp(argv[1], "out")) {
    if (argc!=2) {
      owl_function_makemsg("Wrong number of arguments for zlog command");
      return(NULL);
    }
    owl_zephyr_zlog_out();
  } else {
    owl_function_makemsg("Invalid subcommand for zlog");
  }
  return(NULL);
}


void owl_command_zlog_out(void) {
  owl_zephyr_zlog_out();
}


char *owl_command_subscribe(int argc, char **argv, char *buff) {
  char *recip="";
  int temp=0;
  
  if (argc<3) {
    owl_function_makemsg("Not enough arguments to the subscribe command");
    return(NULL);
  }
  argc--;
  argv++;

  if (!strcmp(argv[0], "-t")) {
    temp=1;
    argc--;
    argv++;
  }
  if (argc<2) {
    owl_function_makemsg("Not enough arguments to the subscribe command");
    return(NULL);
  }

  if (argc>3) {
    owl_function_makemsg("Too many arguments to the subscribe command");
    return(NULL);
  }

  if (argc==2) {
    recip="";
  } else if (argc==3) {
    recip=argv[2];
  }

  owl_function_subscribe(argv[0], argv[1], recip);
  if (!temp) {
    owl_zephyr_addsub(NULL, argv[0], argv[1], recip);
  }
  return(NULL);
}


char *owl_command_unsubscribe(int argc, char **argv, char *buff) {
  char *recip="";
  int temp=0;

  if (argc<3) {
    owl_function_makemsg("Not enough arguments to the unsubscribe command");
    return(NULL);
  }
  argc--;
  argv++;

  if (!strcmp(argv[0], "-t")) {
    temp=1;
    argc--;
    argv++;
  }
  if (argc<2) {
    owl_function_makemsg("Not enough arguments to the subscribe command");
    return(NULL);
  }

  if (argc>3) {
    owl_function_makemsg("Too many arguments to the unsubscribe command");
    return(NULL);
  }

  if (argc==2) {
    recip="";
  } else if (argc==3) {
    recip=argv[2];
  }

  owl_function_unsubscribe(argv[0], argv[1], recip);
  if (!temp) {
    owl_zephyr_delsub(NULL, argv[0], argv[1], recip);
  }
  return(NULL);
}

char *owl_command_echo(int argc, char **argv, char *buff) {
  buff = skiptokens(buff, 1);
  owl_function_popless_text(buff);
  return NULL;
}

void owl_command_getsubs() {
  owl_function_getsubs();
}

void owl_command_status() {
  owl_function_status();
}

char *owl_command_zwrite(int argc, char **argv, char *buff) {
  char *tmpbuff, *pos, *cmd, *msg;

  /* check for a zwrite -m */
  for (pos = buff; *pos; pos = skiptokens(pos, 1)) {
    if (!strncmp(pos, "-m ", 3)) {
      cmd = owl_strdup(buff);
      msg = cmd+(pos-buff);
      *msg = '\0';
      msg += 3;
      owl_zwrite_create_and_send_from_line(cmd, msg);
      owl_free(cmd);
      return NULL;
    }
  }

  if (argc < 2) {
    owl_function_makemsg("Not enough arguments to the zwrite command.");
  } else {
    tmpbuff = owl_strdup(buff);
    owl_function_zwrite_setup(tmpbuff);
    owl_global_set_buffercommand(&g, tmpbuff);
    owl_free(tmpbuff);
  }
  return NULL;
}

char *owl_command_aimwrite(int argc, char **argv, char *buff) {
  char *tmpbuff;

  if (!owl_global_is_aimloggedin(&g)) {
    owl_function_makemsg("You are not logged in to AIM.");
    return(NULL);
  }

  if (argc < 2) {
    owl_function_makemsg("Not enough arguments to the aimwrite command.");
    return(NULL);
  }

  tmpbuff=owl_strdup(buff);
  owl_function_aimwrite_setup(tmpbuff);
  owl_global_set_buffercommand(&g, tmpbuff);
  owl_free(tmpbuff);
  return(NULL);
}

char *owl_command_zcrypt(int argc, char **argv, char *buff) {
  char *tmpbuff, *pos, *cmd, *msg;
  
  /* check for a zwrite -m */
  for (pos = buff; *pos; pos = skiptokens(pos, 1)) {
    if (!strncmp(pos, "-m ", 3)) {
      cmd = owl_strdup(buff);
      msg = cmd+(pos-buff);
      *msg = '\0';
      msg += 3;
      owl_zwrite_create_and_send_from_line(cmd, msg);
      owl_free(cmd);
      return NULL;
    }
  }

  if (argc < 2) {
    owl_function_makemsg("Not enough arguments to the zcrypt command.");
  } else {
    tmpbuff = owl_strdup(buff);
    owl_function_zwrite_setup(tmpbuff);
    owl_global_set_buffercommand(&g, tmpbuff);
    owl_free(tmpbuff);
  }
  return(NULL);
}

char *owl_command_reply(int argc, char **argv, char *buff) {
  int edit=0;
  
  if (argc>=2 && !strcmp("-e", argv[1])) {
    edit=1;
    argv++;
    argc--;
  }

  if ((argc==1) || (argc==2 && !strcmp(argv[1], "all"))) {    
    owl_function_reply(0, !edit);
  } else if (argc==2 && !strcmp(argv[1], "sender")) {
    owl_function_reply(1, !edit);
  } else if (argc==2 && !strcmp(argv[1], "zaway")) {
    owl_message *m;
    owl_view    *v;
    v = owl_global_get_current_view(&g);    
    m = owl_view_get_element(v, owl_global_get_curmsg(&g));
    if (m) owl_zephyr_zaway(m);
  } else {
    owl_function_makemsg("Invalid arguments to the reply command.");
  }
  return NULL;
}

char *owl_command_filter(int argc, char **argv, char *buff) {
  owl_function_create_filter(argc, argv);
  return NULL;
}

char *owl_command_zlocate(int argc, char **argv, char *buff) {
  int auth;
  
  if (argc<2) {
    owl_function_makemsg("Too few arguments for zlocate command");
    return NULL;
  }

  auth=1;
  if (!strcmp(argv[1], "-d")) {
    if (argc>2) {
      auth=0;
      argc--;
      argv++;
    } else {
      owl_function_makemsg("Missing arguments for zlocate command");
      return NULL;
    }
  }

  argc--;
  argv++;
  owl_function_zlocate(argc, argv, auth);
  return NULL;
}

char *owl_command_view(int argc, char **argv, char *buff) {
  if (argc<2) {
    owl_function_makemsg("Wrong number of arguments to view command");
    return NULL;
  }

  if (argc == 2 && !strcmp(argv[1], "--home")) {
    owl_function_change_view(owl_global_get_view_home(&g));
    return NULL;
  }

  /* is it a dynamic filter? */
  if (!strcmp(argv[1], "-d")) {
    char **myargv;
    int i;

    myargv=owl_malloc((argc*sizeof(char *))+50);
    myargv[0]="";
    myargv[1]="owl-dynamic";
    for (i=2; i<argc; i++) {
      myargv[i]=argv[i];
    }
    owl_function_create_filter(argc, myargv);
    owl_function_change_view("owl-dynamic");
    owl_free(myargv);
    return NULL;
  }

  /* otherwise it's a normal view command */
  if (argc>2) {
    owl_function_makemsg("Wrong number of arguments to view command");
    return NULL;
  }
  owl_function_change_view(argv[1]);
  return NULL;
}


char *owl_command_show(int argc, char **argv, char *buff) {

  if (argc<2) {
    owl_function_help_for_command("show");
    return NULL;
  }

  if (!strcmp(argv[1], "filter") || !strcmp(argv[1], "filters")) {
    if (argc==2) {
      owl_function_show_filters();
    } else {
      owl_function_show_filter(argv[2]);
    }
  } else if (argc==2 
	     && (!strcmp(argv[1], "zpunts") || !strcmp(argv[1], "zpunted"))) {
    owl_function_show_zpunts();
  } else if (!strcmp(argv[1], "command") || !strcmp(argv[1], "commands")) {
    if (argc==2) {
      owl_function_show_commands();
    } else {
      owl_function_show_command(argv[2]);
    }
  } else if (!strcmp(argv[1], "variable") || !strcmp(argv[1], "variables")) {
    if (argc==2) {
      owl_function_show_variables();
    } else {
      owl_function_show_variable(argv[2]);
    }
  } else if (!strcmp(argv[1], "keymap") || !strcmp(argv[1], "keymaps")) {
    if (argc==2) {
      owl_function_show_keymaps();
    } else {
      owl_function_show_keymap(argv[2]);
    }
  } else if (!strcmp(argv[1], "colors")) {
    owl_function_show_colors();
  } else if (!strcmp(argv[1], "subs") || !strcmp(argv[1], "subscriptions")) {
    owl_function_getsubs();
  } else if (!strcmp(argv[1], "terminal") || !strcmp(argv[1], "term")) {
    owl_function_show_term();
  } else if (!strcmp(argv[1], "version")) {
    owl_function_about();
  } else if (!strcmp(argv[1], "status")) {
    owl_function_status();
  } else {
    owl_function_makemsg("Unknown subcommand for 'show' command (see 'help show' for allowed args)");
    return NULL;
  }
  return NULL;
}

char *owl_command_viewclass(int argc, char **argv, char *buff) {
  char *filtname;
  if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments to viewclass command");
    return NULL;
  }
  filtname = owl_function_classinstfilt(argv[1], NULL);
  owl_function_change_view(filtname);
  owl_free(filtname);
  return NULL;
}

char *owl_command_viewuser(int argc, char **argv, char *buff) {
  char *filtname;
  if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments to viewuser command");
    return NULL;
  }
  filtname=owl_function_zuserfilt(argv[1]);
  owl_function_change_view(filtname);
  owl_free(filtname);
  return NULL;
}


void owl_command_pop_message(void) {
  owl_function_curmsg_to_popwin();
}

void owl_command_openurl(void) {
  owl_function_openurl();
}

char *owl_command_delete(int argc, char **argv, char *buff) {
  int move_after = 1;

  if (argc>1 && !strcmp(argv[1], "--no-move")) {
    move_after = 0;
    argc--; 
    argv++;
  }

  if (argc==1) {
    owl_function_deletecur(move_after);
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "view")) {
    owl_function_delete_curview_msgs(1);
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "trash")) {
    owl_function_delete_automsgs();
    return NULL;
  }

  if (argc==3 && (!strcmp(argv[1], "-id") || !strcmp(argv[1], "--id"))) {
    owl_function_delete_by_id(atoi(argv[2]), 1);
    return NULL;
  }

  owl_function_makemsg("Unknown arguments to delete command");
  return NULL;
}

char *owl_command_undelete(int argc, char **argv, char *buff) {
  int move_after = 1;

  if (argc>1 && !strcmp(argv[1], "--no-move")) {
    move_after = 0;
    argc--; 
    argv++;
  }

  if (argc==1) {
    owl_function_undeletecur(move_after);
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "view")) {
    owl_function_delete_curview_msgs(0);
    return NULL;
  }

  if (argc==3 && (!strcmp(argv[1], "-id") || !strcmp(argv[1], "--id"))) {
    owl_function_delete_by_id(atoi(argv[2]), 0);
    return NULL;
  }

  owl_function_makemsg("Unknown arguments to delete command");
  return NULL;
}

void owl_command_beep() {
  owl_function_beep();
}

char *owl_command_colorview(int argc, char **argv, char *buff) {
  if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments to colorview command");
    return NULL;
  }
  owl_function_color_current_filter(argv[1]);
  return NULL;
}

char *owl_command_zpunt(int argc, char **argv, char *buff) {
  owl_command_zpunt_and_zunpunt(argc, argv, 0);
  return NULL;
}

char *owl_command_zunpunt(int argc, char **argv, char *buff) {
  owl_command_zpunt_and_zunpunt(argc, argv, 1);
  return NULL;
}


void owl_command_zpunt_and_zunpunt(int argc, char **argv, int type) {
  /* if type==0 then zpunt
   * if type==1 then zunpunt
   */
  char *class, *inst, *recip;

  class="message";
  inst="";
  recip="*";

  if (argc==1) {
    /* show current punt filters */
    owl_function_show_zpunts();
    return;
  } else if (argc==2) {
    inst=argv[1];
  } else if (argc==3) {
    class=argv[1];
    inst=argv[2];
  } else if (argc==4) {
    class=argv[1];
    inst=argv[2];
    recip=argv[3];
  } else {
    owl_function_makemsg("Wrong number of arguments to the zpunt command");
    return;
  }

  owl_function_zpunt(class, inst, recip, type);
  if (type==0) {
    owl_function_makemsg("<%s, %s, %s> added to punt list.", class, inst, recip);
  } else if (type==1) {
    owl_function_makemsg("<%s, %s, %s> removed from punt list.", class, inst, recip);
  }
}

char *owl_command_smartzpunt(int argc, char **argv, char *buff) {
  if (argc == 1) {
    owl_function_smartzpunt(0);
  } else if (argc == 2 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--instance"))) {
    owl_function_smartzpunt(1);
  } else {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);    
  }
  return NULL;
}

char *owl_command_getview(int argc, char **argv, char *buff) {
  char *filtname;
  if (argc != 1) {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  filtname = owl_view_get_filtname(owl_global_get_current_view(&g));
  if (filtname) filtname = owl_strdup(filtname);
  return filtname;
}

char *owl_command_getvar(int argc, char **argv, char *buff) {
  char tmpbuff[1024];
  if (argc != 2) {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  if (owl_variable_get_tostring(owl_global_get_vardict(&g), 
				argv[1], tmpbuff, 1024)) {
    return NULL;
  }
  return owl_strdup(tmpbuff); 
}

char *owl_command_search(int argc, char **argv, char *buff) {
  int direction;
  char *buffstart;

  direction=OWL_DIRECTION_DOWNWARDS;
  buffstart=skiptokens(buff, 1);
  if (argc>1 && !strcmp(argv[1], "-r")) {
    direction=OWL_DIRECTION_UPWARDS;
    buffstart=skiptokens(buff, 2);
  }
    
  if (argc==1 || (argc==2 && !strcmp(argv[1], "-r"))) {
    owl_function_search_continue(direction);
  } else {
    owl_function_search_start(buffstart, direction);
  }
  
  return(NULL);
}

char *owl_command_aimlogin(int argc, char **argv, char *buff) {
  int ret;
  
  if (argc!=3) {
    owl_function_makemsg("Wrong number of arguments to aimlogin command");
    return(NULL);
  }

  /* clear the buddylist */
  owl_buddylist_clear(owl_global_get_buddylist(&g));

  /* try to login */
  ret=owl_aim_login(argv[1], argv[2]);
  if (!ret) {
    owl_function_makemsg("%s logged in.\n", argv[1]);
    return(NULL);
  }
  
  owl_function_makemsg("Warning: login for %s failed.\n");
  return(NULL);
}

char *owl_command_aimlogout(int argc, char **argv, char *buff) {
  /* clear the buddylist */
  owl_buddylist_clear(owl_global_get_buddylist(&g));

  owl_aim_logout();
  return(NULL);
}

/*********************************************************************/
/************************** EDIT SPECIFIC ****************************/
/*********************************************************************/

void owl_command_edit_cancel(owl_editwin *e) {
  owl_history *hist;

  owl_function_makemsg("Command cancelled.");

  hist=owl_editwin_get_history(e);
  owl_history_store(hist, owl_editwin_get_text(e));
  owl_history_reset(hist);

  owl_editwin_fullclear(e);
  owl_global_set_needrefresh(&g);
  wnoutrefresh(owl_editwin_get_curswin(e));
  owl_global_set_typwin_inactive(&g);
  owl_editwin_new_style(e, OWL_EDITWIN_STYLE_ONELINE, NULL);
}

void owl_command_edit_history_prev(owl_editwin *e) {
  owl_history *hist;
  char *ptr;

  hist=owl_editwin_get_history(e);
  if (!owl_history_is_touched(hist)) {
    owl_history_store(hist, owl_editwin_get_text(e));
    owl_history_set_partial(hist);
  }
  ptr=owl_history_get_prev(hist);
  if (ptr) {
    owl_editwin_clear(e);
    owl_editwin_insert_string(e, ptr);
    owl_editwin_redisplay(e, 0);
    owl_global_set_needrefresh(&g);
  } else {
    owl_function_beep();
  }
}

void owl_command_edit_history_next(owl_editwin *e) {
  owl_history *hist;
  char *ptr;

  hist=owl_editwin_get_history(e);
  ptr=owl_history_get_next(hist);
  if (ptr) {
    owl_editwin_clear(e);
    owl_editwin_insert_string(e, ptr);
    owl_editwin_redisplay(e, 0);
    owl_global_set_needrefresh(&g);
  } else {
    owl_function_beep();
  }
}

char *owl_command_edit_insert_text(owl_editwin *e, int argc, char **argv, char *buff) {
  buff = skiptokens(buff, 1);
  owl_editwin_insert_string(e, buff);
  owl_editwin_redisplay(e, 0);
  owl_global_set_needrefresh(&g);  
  return NULL;
}

void owl_command_editline_done(owl_editwin *e) {
  owl_history *hist=owl_editwin_get_history(e);
  char *rv, *cmd;

  owl_history_store(hist, owl_editwin_get_text(e));
  owl_history_reset(hist);
  owl_global_set_typwin_inactive(&g);
  cmd = owl_strdup(owl_editwin_get_text(e));
  owl_editwin_fullclear(e);
  rv = owl_function_command(cmd);
  owl_free(cmd);

  wnoutrefresh(owl_editwin_get_curswin(e));
  owl_global_set_needrefresh(&g);

  if (rv) {
    owl_function_makemsg("%s", rv);
    owl_free(rv);
  }
}

void owl_command_editmulti_done(owl_editwin *e) {
  owl_history *hist=owl_editwin_get_history(e);

  owl_history_store(hist, owl_editwin_get_text(e));
  owl_history_reset(hist);

  owl_function_run_buffercommand();
  owl_editwin_new_style(e, OWL_EDITWIN_STYLE_ONELINE, NULL);
  owl_editwin_fullclear(e);
  owl_global_set_typwin_inactive(&g);
  owl_global_set_needrefresh(&g);
  wnoutrefresh(owl_editwin_get_curswin(e));
}

void owl_command_editmulti_done_or_delete(owl_editwin *e) {
  if (owl_editwin_is_at_end(e)) {
    owl_command_editmulti_done(e);
  } else {
    owl_editwin_delete_char(e);
  }
}


/*********************************************************************/
/*********************** POPLESS SPECIFIC ****************************/
/*********************************************************************/

void owl_command_popless_quit(owl_viewwin *vw) {
  owl_popwin_close(owl_global_get_popwin(&g));
  owl_viewwin_free(vw);
  owl_global_set_needrefresh(&g);
}
