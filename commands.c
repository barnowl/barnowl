#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "owl.h"

/* fn is "char *foo(int argc, const char *const *argv, const char *buff)" */
#define OWLCMD_ARGS(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, fn, NULL, NULL, NULL, NULL, NULL, NULL }

/* fn is "void foo(void)" */
#define OWLCMD_VOID(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, fn, NULL, NULL, NULL, NULL, NULL }

/* fn is "void foo(int)" */
#define OWLCMD_INT(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, fn, NULL, NULL, NULL, NULL }

#define OWLCMD_ALIAS(name, actualname) \
        { name, OWL_CMD_ALIAS_SUMMARY_PREFIX actualname, "", "", OWL_CTX_ANY, \
          actualname, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

/* fn is "char *foo(void *ctx, int argc, const char *const *argv, const char *buff)" */
#define OWLCMD_ARGS_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, ((char*(*)(void*,int,const char*const *,const char*))fn), NULL, NULL, NULL }

/* fn is "void foo(void)" */
#define OWLCMD_VOID_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, NULL, ((void(*)(void*))(fn)), NULL, NULL }

/* fn is "void foo(int)" */
#define OWLCMD_INT_CTX(name, fn, ctx, summary, usage, description) \
        { name, summary, usage, description, ctx, \
          NULL, NULL, NULL, NULL, NULL, NULL, ((void(*)(void*,int))fn), NULL }


const owl_cmd commands_to_init[]
  = {
  OWLCMD_ARGS("zlog", owl_command_zlog, OWL_CTX_ANY,
	      "send a login or logout notification",
	      "zlog in [tty]\nzlog out",
	      "zlog in will send a login notification, zlog out will send a\n"
	      "logout notification.  By default a login notification is sent\n"
	      "when owl is started and a logout notification is sent when owl\n"
	      "is exited.  This behavior can be changed with the 'startuplogin'\n"
	      "and 'shutdownlogout' variables.  If a tty is specified for zlog in\n"
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
	      "(Note: There is a literal word \"command\" between <keyseq>\n"
	      " and <command>.)\n"
	      "Binds a key sequence to a command within a keymap.\n"
	      "Use 'show keymaps' to see the existing keymaps.\n"
	      "Key sequences may be things like M-C-t or NPAGE.\n\n"
	      "Ex.: bindkey recv C-b command zwrite -c barnowl\n"
              "Ex.: bindkey recv m command start-command zwrite -c my-class -i \n\n"
              "SEE ALSO: unbindkey, start-command"),

  OWLCMD_ARGS("unbindkey", owl_command_unbindkey, OWL_CTX_ANY,
	      "removes a binding in a keymap",
	      "bindkey <keymap> <keyseq>",
	      "Removes a binding of a key sequence within a keymap.\n"
	      "Use 'show keymaps' to see the existing keymaps.\n"
	      "Ex.: unbindkey recv H\n\n"
              "SEE ALSO: bindkey"),

  OWLCMD_ARGS("zwrite", owl_command_zwrite, OWL_CTX_INTERACTIVE,
	      "send a zephyr",
	      "zwrite [-n] [-C] [-c class] [-i instance] [-r realm] [-O opcode] [<user> ...] [-m <message...>]",
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
	      "send an AIM message",
	      "aimwrite <user> [-m <message...>]",
	      "Send an aim message to a user.\n\n" 
              "The following options are available:\n\n"
              "-m    Specifies a message to send without prompting.\n"),

  OWLCMD_ARGS("loopwrite", owl_command_loopwrite, OWL_CTX_INTERACTIVE,
	      "send a loopback message",
	      "loopwrite",
	      "Send a local message.\n"),

  OWLCMD_ARGS("zcrypt", owl_command_zwrite, OWL_CTX_INTERACTIVE,
	      "send an encrypted zephyr",
	      "zcrypt [-n] [-C] [-c class] [-i instance] [-r realm] [-O opcode] [-m <message...>]\n",
	      "Behaves like zwrite but uses encryption.  Not for use with\n"
	      "personal messages\n"),
  
  OWLCMD_ARGS("reply", owl_command_reply,  OWL_CTX_INTERACTIVE,
	      "reply to the current message",
	      "reply [-e] [ sender | all | zaway ]",
	      "If -e is specified, the zwrite command line is presented to\n"
	      "allow editing.\n\n"
	      "If 'sender' is specified, reply to the sender.\n\n"
	      "If 'all' or no args are specified, reply publicly to the\n"
	      "same class/instance for non-personal messages and to the\n"
	      "sender for personal messages.\n\n"
	      "If 'zaway' is specified, replies with a zaway message.\n\n"),

  OWLCMD_ARGS("set", owl_command_set, OWL_CTX_ANY,
	      "set a variable value",
	      "set [-q] [<variable>] [<value>]\n"
	      "set",
	      "Set the named variable to the specified value.  If no\n"
	      "arguments are given, print the value of all variables.\n"
	      "If the value is unspecified and the variable is a boolean, it will be\n"
	      "set to 'on'.  If -q is used, set is silent and does not print a\n"
	      "message.\n"),

  OWLCMD_ARGS("unset", owl_command_unset, OWL_CTX_ANY,
	      "unset a boolean variable value",
	      "unset [-q] <variable>\n"
	      "unset",
	      "Set the named boolean variable to off.\n"
	      "If -q is specified, is silent and doesn't print a message.\n"),

  OWLCMD_ARGS("print", owl_command_print, OWL_CTX_ANY,
	      "print a variable value",
	      "print <variable>\n"
	      "print",
	      "Print the value of the named variable.  If no arguments\n"
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
	      "subscribe [-t] <class> [instance [recipient]]",
	      "Subscribe to the specified class and instance.  If the\n"
	      "instance or recipient is not listed on the command\n"
	      "line they default to * (the wildcard recipient).\n"
	      "If the -t option is present the subscription will\n"
	      "only be temporary, i.e., it will not be written to\n"
	      "the subscription file and will therefore not be\n"
	      "present the next time owl is started.\n"),
  OWLCMD_ALIAS("sub", "subscribe"),

  OWLCMD_ARGS("unsubscribe", owl_command_unsubscribe, OWL_CTX_ANY,
	      "unsubscribe from a zephyr class, instance, recipient",
	      "unsubscribe [-t] <class> [instance [recipient]]",
	      "Unsubscribe from the specified class and instance.  If the\n"
	      "instance or recipient is not listed on the command\n"
	      "line they default to * (the wildcard recipient).\n"
	      "If the -t option is present the unsubscription will\n"
	      "only be temporary, i.e., it will not be updated in\n"
	      "the subscription file and will therefore not be\n"
	      "in effect the next time owl is started.\n"),
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

  OWLCMD_ARGS("source", owl_command_source, OWL_CTX_ANY,
	      "execute owl commands from a file",
	      "source <filename>",
	      "Execute the owl commands in <filename>.\n"),

  OWLCMD_ARGS("aim", owl_command_aim, OWL_CTX_INTERACTIVE,
	      "AIM specific commands",
	      "aim search <email>",
	      ""),

  OWLCMD_ARGS("addbuddy", owl_command_addbuddy, OWL_CTX_INTERACTIVE,
	      "add a buddy to a buddylist",
	      "addbuddy <protocol> <screenname>",
	      "Add the named buddy to your buddylist.  <protocol> can be aim or zephyr\n"),

  OWLCMD_ARGS("delbuddy", owl_command_delbuddy, OWL_CTX_INTERACTIVE,
	      "delete a buddy from a buddylist",
	      "delbuddy <protocol> <screenname>",
	      "Delete the named buddy from your buddylist.  <protocol> can be aim or zephyr\n"),

  OWLCMD_ARGS("join", owl_command_join, OWL_CTX_INTERACTIVE,
	      "join a chat group",
	      "join aim <groupname> [exchange]",
	      "Join the AIM chatroom with 'groupname'.\n"),

  OWLCMD_ARGS("smartzpunt", owl_command_smartzpunt, OWL_CTX_INTERACTIVE,
	      "creates a zpunt based on the current message",
	      "smartzpunt [-i | --instance]",
	      "Starts a zpunt command based on the current message's class\n"
	      "(and instance if -i is specified).\n"),

  OWLCMD_ARGS("zpunt", owl_command_zpunt, OWL_CTX_ANY,
	      "suppress a given zephyr triplet",
	      "zpunt <class> <instance> [recipient]\n"
	      "zpunt <instance>",
	      "The zpunt command will suppress messages to the specified\n"
	      "zephyr triplet.  In the second usage messages are suppressed\n"
	      "for class MESSAGE and the named instance.\n\n"
	      "SEE ALSO:  zunpunt, show zpunts\n"),

  OWLCMD_ARGS("zunpunt", owl_command_zunpunt, OWL_CTX_ANY,
	      "undo a previous zpunt",
	      "zunpunt <class> <instance> [recipient]\n"
	      "zunpunt <instance>",
	      "The zunpunt command will allow messages that were previously\n"
	      "suppressed to be received again.\n\n"
	      "SEE ALSO:  zpunt, show zpunts\n"),

  OWLCMD_ARGS("punt", owl_command_punt, OWL_CTX_ANY,
	      "suppress an arbitrary filter",
	      "punt <filter-text>",
	      "punt <filter-text (multiple words)>\n"
	      "The punt command will suppress messages to the specified\n"
	      "filter\n\n"
	      "SEE ALSO:  unpunt, zpunt, show zpunts\n"),

  OWLCMD_ARGS("unpunt", owl_command_unpunt, OWL_CTX_ANY,
	      "remove an entry from the punt list",
	      "zpunt <filter-text>\n"
	      "zpunt <filter-text>\n"
	      "zpunt <number>\n",
	      "The unpunt command will remove an entry from the puntlist.\n"
	      "The first two forms correspond to the first two forms of the :punt\n"
	      "command. The latter allows you to remove a specific entry from the\n"
	      "the list (see :show zpunts)\n\n"
	      "SEE ALSO:  punt, zpunt, zunpunt, show zpunts\n"),

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

  OWLCMD_VOID("alist", owl_command_alist, OWL_CTX_INTERACTIVE,
	      "List AIM users logged in",
	      "alist",
	      "Print a listing of AIM users logged in"),

  OWLCMD_VOID("blist", owl_command_blist, OWL_CTX_INTERACTIVE,
	      "List all buddies logged in",
	      "blist",
	      "Print a listing of buddies logged in, regardless of protocol."),

  OWLCMD_VOID("toggle-oneline", owl_command_toggleoneline, OWL_CTX_INTERACTIVE,
	      "Toggle the style between oneline and the default style",
	      "toggle-oneline",
	      ""),

  OWLCMD_ARGS("recv:getshift", owl_command_get_shift, OWL_CTX_INTERACTIVE,
	      "gets position of receive window scrolling", "", ""),

  OWLCMD_INT("recv:setshift", owl_command_set_shift, OWL_CTX_INTERACTIVE,
	      "scrolls receive window to specified position", "", ""),

  OWLCMD_VOID("recv:pagedown", owl_function_mainwin_pagedown, 
	      OWL_CTX_INTERACTIVE,
	      "scrolls down by a page", "", ""),

  OWLCMD_VOID("recv:pageup", owl_function_mainwin_pageup, OWL_CTX_INTERACTIVE,
	      "scrolls up by a page", "", ""),

  OWLCMD_VOID("recv:mark", owl_function_mark_message,
	      OWL_CTX_INTERACTIVE,
	      "mark the current message", "", ""),

  OWLCMD_VOID("recv:swapmark", owl_function_swap_cur_marked,
	      OWL_CTX_INTERACTIVE,
	      "swap the positions of the pointer and the mark", "", ""),

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

  OWLCMD_ARGS("zaway", owl_command_zaway, OWL_CTX_INTERACTIVE,
	      "Set, enable or disable zephyr away message",
	      "zaway [ on | off | toggle ]\n"
	      "zaway <message>",
	      "Turn on or off a zaway message.  If 'message' is\n"
	      "specified turn on zaway with that message, otherwise\n"
	      "use the default.\n"),

  OWLCMD_ARGS("aaway", owl_command_aaway, OWL_CTX_INTERACTIVE,
	      "Set, enable or disable AIM away message",
	      "aaway [ on | off | toggle ]\n"
	      "aaway <message>",
	      "Turn on or off the AIM away message.  If 'message' is\n"
	      "specified turn on aaway with that message, otherwise\n"
	      "use the default.\n"),

  OWLCMD_ARGS("away", owl_command_away, OWL_CTX_INTERACTIVE,
	      "Set, enable or disable both AIM and zephyr away messages",
	      "away [ on | off | toggle ]\n"
	      "away <message>",
	      "Turn on or off the AIM and zephyr away message.  If\n"
	      "'message' is specified turn them on with that message,\n"
	      "otherwise use the default.\n"
	      "\n"
	      "This command really just runs the 'aaway' and 'zaway'\n"
	      "commands together\n"
	      "\n"
	      "SEE ALSO: aaway, zaway"),

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
	      "filter <name> [ -c fgcolor ] [ -b bgcolor ] [ <expression> ... ]",
	      "The filter command creates a filter with the specified name,\n"
	      "or if one already exists it is replaced.  Example filter\n"
	      "syntax would be:\n\n"
	      "     filter myfilter -c red ( class ^foobar$ ) or ( class ^quux$ and instance ^bar$ )\n\n"
	      "Valid matching fields are:\n"
	      "    sender     -  sender\n"
	      "    recipient  -  recipient\n"
	      "    class      -  zephyr class name\n"
	      "    instance   -  zephyr instance name\n"
	      "    opcode     -  zephyr opcode\n"
	      "    realm      -  zephyr realm\n"
	      "    body       -  message body\n"
	      "    hostname   -  hostname of sending host\n"
	      "    type       -  message type (zephyr, aim, admin)\n"
	      "    direction  -  either 'in' 'out' or 'none'\n"
	      "    login      -  either 'login' 'logout' or 'none'\n"
	      "Also you may match on the validity of another filter:\n"
	      "    filter <filtername>\n"
	      "Also you may pass the message to a perl function returning 0 or 1,\n"
	      "where 1 indicates that the function matches the filter:\n"
	      "    perl <subname>\n"
	      "Valid operators are:\n"
	      "    and\n"
	      "    or\n"
              "    not\n"
              "And additionally you may use the static values:\n"
	      "    true\n"
	      "    false\n"
	      "Spaces must be present before and after parentheses.  If the\n"
	      "optional color arguments are used they specifies the colors that\n"
	      "messages matching this filter should be displayed in.\n\n"
	      "SEE ALSO: view, viewclass, viewuser\n"),

  OWLCMD_ARGS("colorview", owl_command_colorview, OWL_CTX_INTERACTIVE,
	      "change the colors on the current filter",
	      "colorview <fgcolor> [<bgcolor>]",
	      "The colors of messages in the current filter will be changed\n"
	      "to <fgcolor>,<bgcolor>.  Use the 'show colors' command for a list\n"
	      "of valid colors.\n\n"
	      "SEE ALSO: 'show colors'\n"),

  OWLCMD_ARGS("colorclass", owl_command_colorclass, OWL_CTX_INTERACTIVE,
	      "create a filter to color messages of the given class name",
	      "colorclass <class> <fgcolor> [<bgcolor>]",
	      "A filter will be created to color messages in <class>"
	      "in <fgcolor>,<bgcolor>.  Use the 'show colors' command for a list\n"
	      "of valid colors.\n\n"
	      "SEE ALSO: 'show colors'\n"),

  OWLCMD_ARGS("view", owl_command_view, OWL_CTX_INTERACTIVE,
	      "view messages matching a filter",
	      "view [<viewname>] [-f <filter> | --home | -r ] [-s <style>]\n"
	      "view <filter>\n"
	      "view -d <expression>\n"
	      "view --home",
	      "The view command sets information associated with a particular view,\n"
	      "such as view's filter or style.  In the first general usage listed\n"
	      "above <viewname> is the name of the view to be changed.  If not\n"
	      "specified the default view 'main' will be used.  A filter can be set\n"
	      "for the view by listing a named filter after the -f argument.  If\n"
	      "the --home argument is used the filter will be set to the filter named\n"
	      "by the\n 'view_home' variable.  The style can be set by listing the\n"
              "name style after the -s argument.\n"
	      "\n"
	      "The other usages listed above are abbreviated forms that simply set\n"
	      "the filter of the current view. The -d option allows you to write a\n"
              "filter expression that will be dynamically created by owl and then\n"
              "applied as the view's filter\n"
	      "SEE ALSO: filter, viewclass, viewuser\n"),

  OWLCMD_ARGS("smartnarrow", owl_command_smartnarrow, OWL_CTX_INTERACTIVE,
	      "view only messages similar to the current message",
	      "smartnarrow [-i | --instance]  [-r | --related]",
	      "If the curmsg is a personal message narrow\n"
	      "   to the conversation with that user.\n"
	      "If the curmsg is a <MESSAGE, foo, *>\n"
	      "   message, narrow to the instance.\n"
	      "If the curmsg is a class message, narrow\n"
	      "    to the class.\n"
	      "If the curmsg is a class message and '-i' is specified\n"
	      "    then narrow to the class and instance.\n"
	      "If '-r' or '--related' is specified, behave as though the\n"
              "    'narrow-related' variable was inverted."),

  OWLCMD_ARGS("smartfilter", owl_command_smartfilter, OWL_CTX_INTERACTIVE,
	      "returns the name of a filter based on the current message",
	      "smartfilter [-i | --instance]",
	      "If the curmsg is a personal message, the filter is\n"
	      "   the conversation with that user.\n"
	      "If the curmsg is a <MESSAGE, foo, *>\n"
	      "   message, the filter is to that instance.\n"
	      "If the curmsg is a class message, the filter is that class.\n"
	      "If the curmsg is a class message and '-i' is specified\n"
	      "    the filter is to that class and instance.\n"),

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
  OWLCMD_ALIAS("viewperson", "viewuser"),
  OWLCMD_ALIAS("vp", "viewuser"),

  OWLCMD_ARGS("show", owl_command_show, OWL_CTX_INTERACTIVE,
	      "show information",
	      "show colors\n"
	      "show commands\n"
	      "show command <command>\n"
	      "show errors\n"
	      "show filters\n"
	      "show filter <filter>\n"
	      "show keymaps\n"
	      "show keymap <keymap>\n"
	      "show license\n"
	      "show quickstart\n"
	      "show startup\n"
	      "show status\n"
	      "show styles\n"
	      "show subscriptions / show subs\n"
	      "show terminal\n"
	      "show timers\n"
	      "show variables\n"
	      "show variable <variable>\n"
	      "show version\n"
	      "show view [<view>]\n"
	      "show zpunts\n",

	      "Show colors will display a list of valid colors for the\n"
	      "     terminal."
	      "Show filters will list the names of all filters.\n"
	      "Show filter <filter> will show the definition of a particular\n"
	      "     filter.\n\n"
	      "Show startup will display the custom startup config\n\n"
	      "Show zpunts will show the active zpunt filters.\n\n"
	      "Show keymaps will list the names of all keymaps.\n"
	      "Show keymap <keymap> will show the key bindings in a keymap.\n\n"
	      "Show commands will list the names of all keymaps.\n"
	      "Show command <command> will provide information about a command.\n\n"
	      "Show styles will list the names of all styles available\n"
	      "for formatting messages.\n\n"
	      "Show variables will list the names of all variables.\n\n"
	      "Show errors will show a list of errors encountered by Owl.\n\n"
	      "SEE ALSO: filter, view, alias, bindkey, help\n"),
  
  OWLCMD_ARGS("delete", owl_command_delete, OWL_CTX_INTERACTIVE,
	      "mark a message for deletion",
	      "delete [ -id msgid ] [ --no-move ]\n"
	      "delete view\n"
	      "delete trash",
	      "If no message id is specified the current message is marked\n"
	      "for deletion.  Otherwise the message with the given message\n"
	      "id is marked for deletion.\n"
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
	      "given message id is unmarked for deletion.\n"
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

  OWLCMD_ARGS("getfilter", owl_command_getfilter, OWL_CTX_INTERACTIVE,
	      "returns the definition of a filter",
	      "getfilter <filtername>", ""),

  OWLCMD_ARGS("getstyle", owl_command_getstyle, OWL_CTX_INTERACTIVE,
	      "returns the name of the style for the current view",
	      "", ""),

  OWLCMD_ARGS("search", owl_command_search, OWL_CTX_INTERACTIVE,
	      "search messages for a particular string",
	      "search [-r] [<string>]",
	      "The search command will find messages that contain the\n"
	      "specified string and move the cursor there.  If no string\n"
	      "argument is supplied then the previous one is used.  By\n"
	      "default searches are done forwards; if -r is used the search\n"
	      "is performed backwards"),

  OWLCMD_ARGS("setsearch", owl_command_setsearch, OWL_CTX_INTERACTIVE,
	      "set the search highlight string without searching",
	      "setsearch <string>",
	      "The setsearch command highlights all occurrences of its\n"
          "argument and makes it the default argument for future\n"
          "search commands, but does not move the cursor.  With\n"
          "no argument, it makes search highlighting inactive."),

  OWLCMD_ARGS("aimlogin", owl_command_aimlogin, OWL_CTX_ANY,
	      "login to an AIM account",
	      "aimlogin <screenname> [<password>]\n",
	      ""),

  OWLCMD_ARGS("aimlogout", owl_command_aimlogout, OWL_CTX_ANY,
	      "logout from AIM",
	      "aimlogout\n",
	      ""),

  OWLCMD_ARGS("error", owl_command_error, OWL_CTX_ANY,
              "Display an error message",
              "error <message>",
              ""),

  OWLCMD_ARGS("message", owl_command_message, OWL_CTX_ANY,
              "Display an informative message",
              "message <message>",
              ""),

  OWLCMD_ARGS("add-cmd-history", owl_command_add_cmd_history, OWL_CTX_ANY,
              "Add a command to the history",
              "add-cmd-history <cmd>",
              ""),

  OWLCMD_ARGS("with-history", owl_command_with_history, OWL_CTX_ANY,
              "Run a command and store it into the history",
              "with-history <cmd>",
              ""),

  OWLCMD_VOID("yes", owl_command_yes, OWL_CTX_RECV,
              "Answer yes to a question",
              "yes",
              ""),

  OWLCMD_VOID("no", owl_command_no, OWL_CTX_RECV,
              "Answer no to a question",
              "no",
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
		  "replaces the text with the next history",
		  "", ""),

  OWLCMD_VOID_CTX("edit:history-prev", owl_command_edit_history_prev, 
		  OWL_CTX_EDIT,
		  "replaces the text with the previous history",
		  "", ""),

  OWLCMD_VOID_CTX("edit:set-mark", owl_editwin_set_mark,
		  OWL_CTX_EDIT,
		  "sets the mark",
		  "", ""),

  OWLCMD_VOID_CTX("edit:exchange-point-and-mark", owl_editwin_exchange_point_and_mark,
		  OWL_CTX_EDIT,
		  "exchanges the point and the mark",
		  "", ""),

  OWLCMD_VOID_CTX("edit:copy-region-as-kill", owl_editwin_copy_region_as_kill,
		  OWL_CTX_EDIT,
		  "copy the text between the point and the mark",
		  "", ""),

  OWLCMD_VOID_CTX("edit:kill-region", owl_editwin_kill_region,
		  OWL_CTX_EDIT,
		  "kill text between the point and the mark",
		  "", ""),

  OWLCMD_VOID_CTX("edit:yank", owl_editwin_yank,
		  OWL_CTX_EDIT,
		  "insert the current text from the kill buffer",
		  "", ""),

  OWLCMD_ALIAS   ("editline:done", "edit:done"),
  OWLCMD_ALIAS   ("editresponse:done", "edit:done"),

  OWLCMD_VOID_CTX("edit:move-up-line", owl_editwin_key_up, 
		  OWL_CTX_EDITMULTI,
		  "moves the cursor up one line",
		  "", ""),

  OWLCMD_VOID_CTX("edit:move-down-line", owl_editwin_key_down, 
		  OWL_CTX_EDITMULTI,
		  "moves the cursor down one line",
		  "", ""),

  OWLCMD_VOID_CTX("edit:done", owl_command_edit_done, 
		  OWL_CTX_EDIT,
		  "Finishes entering text in the editwin.",
		  "", ""),

  OWLCMD_VOID_CTX("edit:done-or-delete", owl_command_edit_done_or_delete, 
		  OWL_CTX_EDITMULTI,
		  "completes the command, but only if at end of message",
		  "", 
		  "If only whitespace is to the right of the cursor,\n"
		  "runs 'edit:done'.\n"\
		  "Otherwise runs 'edit:delete-next-char'\n"),

  OWLCMD_VOID_CTX("edit:forward-paragraph", owl_editwin_forward_paragraph,
                  OWL_CTX_EDITMULTI,
                  "Move forward to end of paragraph.",
                  "",
                  "Move the point to the end of the current paragraph"),

  OWLCMD_VOID_CTX("edit:backward-paragraph", owl_editwin_backward_paragraph,
                  OWL_CTX_EDITMULTI,
                  "Move backward to the start of paragraph.",
                  "",
                  "Move the point to the start of the current paragraph"),

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

  OWLCMD_ARGS_CTX("popless:start-command", owl_viewwin_start_command,
		  OWL_CTX_POPLESS,
		  "starts a command line in the popless",
		  "popless:start-command [initial-value]",
		  "Initializes the command field to initial-value"),

  OWLCMD_ARGS_CTX("popless:search", owl_viewwin_command_search, OWL_CTX_POPLESS,
		  "search lines for a particular string",
		  "popless:search [-r] [<string>]",
		  "The popless:search command will find lines that contain the\n"
		  "specified string and scroll the popwin there.  If no string\n"
		  "argument is supplied then the previous one is used.  By\n"
		  "default searches are done forwards; if -r is used the search\n"
		  "is performed backwards"),

  OWLCMD_ARGS_CTX("popless:start-search", owl_viewwin_command_start_search, OWL_CTX_POPLESS,
		  "starts a command line to search for particular string",
		  "popless:start-search [-r] [inital-value]",
		  "Initializes the command-line to search for initial-value. If\n"
		  "-r is used, the search will be performed backwards.\n\n"
                  "SEE ALSO: popless:search"),

  OWLCMD_ALIAS("webzephyr", "zwrite " OWL_WEBZEPHYR_PRINCIPAL " -c " OWL_WEBZEPHYR_CLASS " -i"),

  /* This line MUST be last! */
  { NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL }

};

void owl_command_info(void)
{
  owl_function_info();
}

void owl_command_nop(void)
{
}

char *owl_command_help(int argc, const char *const *argv, const char *buff)
{
  if (argc!=2) {
    owl_help();
    return NULL;
  }
  
  owl_function_help_for_command(argv[1]);
  return NULL;
}

char *owl_command_zlist(int argc, const char *const *argv, const char *buff)
{
  const char *file=NULL;

  argc--;
  argv++;
  while (argc) {
    if (!strcmp(argv[0], "-f")) {
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

void owl_command_alist(void)
{
  owl_function_buddylist(1, 0, NULL);
}

void owl_command_blist(void)
{
  owl_function_buddylist(1, 1, NULL);
}

void owl_command_toggleoneline(void)
{
  owl_function_toggleoneline();
}

void owl_command_about(void)
{
  owl_function_about();
}

void owl_command_version(void)
{
  owl_function_makemsg("BarnOwl version %s", OWL_VERSION_STRING);
}

char *owl_command_aim(int argc, const char *const *argv, const char *buff)
{
  if (argc<2) {
    owl_function_makemsg("not enough arguments to aim command");
    return(NULL);
  }

  if (!strcmp(argv[1], "search")) {
    if (argc!=3) {
      owl_function_makemsg("not enough arguments to aim search command");
      return(NULL);
    }
    owl_aim_search(argv[2]);
  } else {
    owl_function_makemsg("unknown subcommand '%s' for aim command", argv[1]);
    return(NULL);
  }
  return(NULL);
}

char *owl_command_addbuddy(int argc, const char *const *argv, const char *buff)
{
  if (argc!=3) {
    owl_function_makemsg("usage: addbuddy <protocol> <buddyname>");
    return(NULL);
  }

  if (!strcasecmp(argv[1], "aim")) {
    if (!owl_global_is_aimloggedin(&g)) {
      owl_function_makemsg("addbuddy: You must be logged into aim to use this command.");
      return(NULL);
    }
    /*
    owl_function_makemsg("This function is not yet operational.  Stay tuned.");
    return(NULL);
    */
    owl_aim_addbuddy(argv[2]);
    owl_function_makemsg("%s added as AIM buddy for %s", argv[2], owl_global_get_aim_screenname(&g));
  } else if (!strcasecmp(argv[1], "zephyr")) {
    owl_zephyr_addbuddy(argv[2]);
    owl_function_makemsg("%s added as zephyr buddy", argv[2]);
  } else {
    owl_function_makemsg("addbuddy: currently the only supported protocols are 'zephyr' and 'aim'");
  }

  return(NULL);
}

char *owl_command_delbuddy(int argc, const char *const *argv, const char *buff)
{
  if (argc!=3) {
    owl_function_makemsg("usage: delbuddy <protocol> <buddyname>");
    return(NULL);
  }

  if (!strcasecmp(argv[1], "aim")) {
    if (!owl_global_is_aimloggedin(&g)) {
      owl_function_makemsg("delbuddy: You must be logged into aim to use this command.");
      return(NULL);
    }
    owl_aim_delbuddy(argv[2]);
    owl_function_makemsg("%s deleted as AIM buddy for %s", argv[2], owl_global_get_aim_screenname(&g));
  } else if (!strcasecmp(argv[1], "zephyr")) {
    owl_zephyr_delbuddy(argv[2]);
    owl_function_makemsg("%s deleted as zephyr buddy", argv[2]);
  } else {
    owl_function_makemsg("delbuddy: currently the only supported protocols are 'zephyr' and 'aim'");
  }

  return(NULL);
}

char *owl_command_join(int argc, const char *const *argv, const char *buff)
{
  if (argc!=3 && argc!=4) {
    owl_function_makemsg("usage: join <protocol> <buddyname> [exchange]");
    return(NULL);
  }

  if (!strcasecmp(argv[1], "aim")) {
    if (!owl_global_is_aimloggedin(&g)) {
      owl_function_makemsg("join aim: You must be logged into aim to use this command.");
      return(NULL);
    }
    if (argc==3) {
      owl_aim_chat_join(argv[2], 4);
    } else {
      owl_aim_chat_join(argv[2], atoi(argv[3]));
    }
    /* owl_function_makemsg("%s deleted as AIM buddy for %s", argv[2], owl_global_get_aim_screenname(&g)); */
  } else {
    owl_function_makemsg("join: currently the only supported protocol is 'aim'");
  }
  return(NULL);
}

char *owl_command_startup(int argc, const char *const *argv, const char *buff)
{
  const char *ptr;

  if (argc<2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return(NULL);
  }

  ptr = skiptokens(buff, 1);

  owl_function_command(ptr);
  owl_function_addstartup(ptr);

  return(NULL);
}

char *owl_command_unstartup(int argc, const char *const *argv, const char *buff)
{
  const char *ptr;

  if (argc<2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return(NULL);
  }

  ptr = skiptokens(buff, 1);

  owl_function_delstartup(ptr);

  return(NULL);
}

char *owl_command_dump(int argc, const char *const *argv, const char *buff)
{
  char *filename;
  
  if (argc!=2) {
    owl_function_makemsg("usage: dump <filename>");
    return(NULL);
  }
  filename=owl_util_makepath(argv[1]);
  owl_function_dump(filename);
  owl_free(filename);
  return(NULL);
}

char *owl_command_source(int argc, const char *const *argv, const char *buff)
{
  if (argc!=2) {
    owl_function_makemsg("usage: source <filename>");
    return(NULL);
  }

  owl_function_source(argv[1]);
  return(NULL);
}

char *owl_command_next(int argc, const char *const *argv, const char *buff)
{
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
      filter = owl_function_smartfilter(0, 0);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter-instance")) {
      filter = owl_function_smartfilter(1, 0);
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

char *owl_command_prev(int argc, const char *const *argv, const char *buff)
{
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
      filter = owl_function_smartfilter(0, 0);
      argc-=2; argv+=2; 
    } else if (argc>=2 && !strcmp(argv[1], "--smart-filter-instance")) {
      filter = owl_function_smartfilter(1, 0);
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

char *owl_command_smartnarrow(int argc, const char *const *argv, const char *buff)
{
  char *filtname = NULL;

  char opt;
  int instance = 0, related = 0, i;
  const char **tmp_argv = owl_malloc(sizeof(char *) * argc);

  static const struct option options[] = {
    {"instance", 0, 0, 'i'},
    {"related",  0, 0, 'r'},
    {NULL,       0, 0, 0}};

  for (i = 0; i < argc; i++)
    tmp_argv[i] = argv[i];

  optind = 0;
  while ((opt = getopt_long(argc, (char **)tmp_argv, "ir", options, NULL)) != -1) {
    switch (opt) {
      case 'i':
        instance = 1;
        break;
      case 'r':
        related = 1;
        break;
      default:
        owl_function_makemsg("Wrong number of arguments for %s (%c)", argv[0], opt);
        goto done;
    }
  }

  filtname = owl_function_smartfilter(instance, related);

  if (filtname) {
    owl_function_change_currentview_filter(filtname);
    owl_free(filtname);
  }

done:
  owl_free(tmp_argv);

  return NULL;
}

char *owl_command_smartfilter(int argc, const char *const *argv, const char *buff)
{
  char *filtname = NULL;

  if (argc == 1) {
    filtname = owl_function_smartfilter(0, 0);
  } else if (argc == 2 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--instance"))) {
    filtname = owl_function_smartfilter(1, 0);
  } else {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);    
  }
  return filtname;
}

void owl_command_expunge(void)
{
  owl_function_expunge();
}

void owl_command_first(void)
{
  owl_global_set_rightshift(&g, 0);
  owl_function_firstmsg();
}

void owl_command_last(void)
{
  owl_function_lastmsg();
}

void owl_command_resize(void)
{
  owl_function_resize();
}

void owl_command_redisplay(void)
{
  owl_function_full_redisplay();
}

char *owl_command_get_shift(int argc, const char *const *argv, const char *buff)
{
  if(argc != 1)
  {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  return owl_sprintf("%d", owl_global_get_rightshift(&g));
}

void owl_command_set_shift(int shift)
{
  owl_global_set_rightshift(&g, shift);
}

void owl_command_unsuball(void)
{
  owl_function_unsuball();
}

char *owl_command_loadsubs(int argc, const char *const *argv, const char *buff)
{
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


char *owl_command_loadloginsubs(int argc, const char *const *argv, const char *buff)
{
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

void owl_command_suspend(void)
{
  owl_function_suspend();
}

char *owl_command_start_command(int argc, const char *const *argv, const char *buff)
{
  buff = skiptokens(buff, 1);
  owl_function_start_command(buff);
  return(NULL);
}

char *owl_command_zaway(int argc, const char *const *argv, const char *buff)
{
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


char *owl_command_aaway(int argc, const char *const *argv, const char *buff)
{
  if ((argc==1) ||
      ((argc==2) && !strcmp(argv[1], "on"))) {
    owl_global_set_aaway_msg(&g, owl_global_get_aaway_msg_default(&g));
    owl_function_aaway_on();
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "off")) {
    owl_function_aaway_off();
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "toggle")) {
    owl_function_aaway_toggle();
    return NULL;
  }

  buff = skiptokens(buff, 1);
  owl_global_set_aaway_msg(&g, buff);
  owl_function_aaway_on();
  return NULL;
}


char *owl_command_away(int argc, const char *const *argv, const char *buff)
{
  if ((argc==1) ||
      ((argc==2) && !strcmp(argv[1], "on"))) {
    owl_global_set_aaway_msg(&g, owl_global_get_aaway_msg_default(&g));
    owl_global_set_zaway_msg(&g, owl_global_get_zaway_msg_default(&g));
    owl_function_aaway_on();
    owl_function_zaway_on();
    owl_function_makemsg("Away messages set.");
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "off")) {
    owl_function_aaway_off();
    owl_function_zaway_off();
    return NULL;
  }

  if (argc==2 && !strcmp(argv[1], "toggle")) {
    /* if either one is on, turn it off, otherwise toggle both (turn
     *  them both on)
     */
    if (!owl_global_is_zaway(&g) && !owl_global_is_aaway(&g)) {
      owl_function_aaway_toggle();
      owl_function_zaway_toggle();
      owl_function_makemsg("Away messages set.");
    } else {
      if (owl_global_is_zaway(&g)) owl_function_zaway_toggle();
      if (owl_global_is_aaway(&g)) owl_function_aaway_toggle();
      owl_function_makemsg("Away messages off.");
    }
    return NULL;
  }

  buff = skiptokens(buff, 1);
  owl_global_set_aaway_msg(&g, buff);
  owl_global_set_zaway_msg(&g, buff);
  owl_function_aaway_on();
  owl_function_zaway_on();
  owl_function_makemsg("Away messages set.");
  return NULL;
}

char *owl_command_set(int argc, const char *const *argv, const char *buff)
{
  const char *var, *val;
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

char *owl_command_unset(int argc, const char *const *argv, const char *buff)
{
  const char *var, *val;
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

char *owl_command_print(int argc, const char *const *argv, const char *buff)
{
  const char *var;
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


char *owl_command_exec(int argc, const char *const *argv, const char *buff)
{
  return owl_function_exec(argc, argv, buff, OWL_OUTPUT_RETURN);
}

char *owl_command_pexec(int argc, const char *const *argv, const char *buff)
{
  return owl_function_exec(argc, argv, buff, OWL_OUTPUT_POPUP);
}

char *owl_command_aexec(int argc, const char *const *argv, const char *buff)
{
  return owl_function_exec(argc, argv, buff, OWL_OUTPUT_ADMINMSG);
}

char *owl_command_perl(int argc, const char *const *argv, const char *buff)
{
  return owl_function_perl(argc, argv, buff, OWL_OUTPUT_RETURN);
}

char *owl_command_pperl(int argc, const char *const *argv, const char *buff)
{
  return owl_function_perl(argc, argv, buff, OWL_OUTPUT_POPUP);
}

char *owl_command_aperl(int argc, const char *const *argv, const char *buff)
{
  return owl_function_perl(argc, argv, buff, OWL_OUTPUT_ADMINMSG);
}

char *owl_command_multi(int argc, const char *const *argv, const char *buff)
{
  char *lastrv = NULL, *newbuff;
  char **commands;
  int  i;
  if (argc < 2) {
    owl_function_makemsg("Invalid arguments to 'multi' command.");    
    return NULL;
  }
  newbuff = owl_strdup(skiptokens(buff, 1));
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
  commands = g_strsplit_set(newbuff, ";", 0);
  for (i = 0; commands[i] != NULL; i++) {
    if (lastrv) {
      owl_free(lastrv);
    }
    lastrv = owl_function_command(commands[i]);
  }
  owl_free(newbuff);
  g_strfreev(commands);
  return lastrv;
}


char *owl_command_alias(int argc, const char *const *argv, const char *buff)
{
  if (argc < 3) {
    owl_function_makemsg("Invalid arguments to 'alias' command.");
    return NULL;
  }
  buff = skiptokens(buff, 2);
  owl_function_command_alias(argv[1], buff);
  return (NULL);
}


char *owl_command_bindkey(int argc, const char *const *argv, const char *buff)
{
  owl_keymap *km;
  int ret;

  if (argc < 5 || strcmp(argv[3], "command")) {
    owl_function_makemsg("Usage: bindkey <keymap> <binding> command <cmd>");
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


char *owl_command_unbindkey(int argc, const char *const *argv, const char *buf)
{
  owl_keymap *km;
  int ret;

  if (argc < 3) {
    owl_function_makemsg("Usage: bindkey <keymap> <binding>");
    return NULL;
  }
  km = owl_keyhandler_get_keymap(owl_global_get_keyhandler(&g), argv[1]);
  if (!km) {
    owl_function_makemsg("No such keymap '%s'", argv[1]);
    return NULL;
  }
  ret = owl_keymap_remove_binding(km, argv[2]);
  if (ret == -1) {
    owl_function_makemsg("Unable to unbind '%s' in keymap '%s'.",
			 argv[2], argv[1]);
    return NULL;
  } else if (ret == -2) {
    owl_function_makemsg("No such binding '%s' in keymap '%s'.",
                         argv[2], argv[1]);
  }
  return NULL;
}


void owl_command_quit(void)
{
  owl_function_quit();
}

char *owl_command_debug(int argc, const char *const *argv, const char *buff)
{
  if (argc<2) {
    owl_function_makemsg("Need at least one argument to debug command");
    return(NULL);
  }

  if (!owl_global_is_debug_fast(&g)) {
    owl_function_makemsg("Debugging is not turned on");
    return(NULL);
  }

  owl_function_debugmsg("%s", argv[1]);
  return(NULL);
}

char *owl_command_term(int argc, const char *const *argv, const char *buff)
{
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

char *owl_command_zlog(int argc, const char *const *argv, const char *buff)
{
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

char *owl_command_subscribe(int argc, const char *const *argv, const char *buff)
{
  const char *class, *instance, *recip="";
  int temp=0;
  int ret=0;

  if (argc < 2) {
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
  if (argc < 1) {
    owl_function_makemsg("Not enough arguments to the subscribe command");
    return(NULL);
  }

  if (argc > 3) {
    owl_function_makemsg("Too many arguments to the subscribe command");
    return(NULL);
  }

  class = argv[0];

  if (argc == 1) {
    instance = "*";
  } else {
    instance = argv[1];
  }

  if (argc <= 2) {
    recip="";
  } else if (argc==3) {
    recip=argv[2];
  }

  ret = owl_function_subscribe(class, instance, recip);
  if (!temp && !ret) {
    owl_zephyr_addsub(NULL, class, instance, recip);
  }
  return(NULL);
}


char *owl_command_unsubscribe(int argc, const char *const *argv, const char *buff)
{
  const char *class, *instance, *recip="";
  int temp=0;

  if (argc < 2) {
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
  if (argc < 1) {
    owl_function_makemsg("Not enough arguments to the unsubscribe command");
    return(NULL);
  }

  if (argc > 3) {
    owl_function_makemsg("Too many arguments to the unsubscribe command");
    return(NULL);
  }

  class = argv[0];

  if (argc == 1) {
    instance = "*";
  } else {
    instance = argv[1];
  }

  if (argc <= 2) {
    recip="";
  } else if (argc==3) {
    recip=argv[2];
  }

  owl_function_unsubscribe(class, instance, recip);
  if (!temp) {
    owl_zephyr_delsub(NULL, class, instance, recip);
  }
  return(NULL);
}

char *owl_command_echo(int argc, const char *const *argv, const char *buff)
{
  buff = skiptokens(buff, 1);
  owl_function_popless_text(buff);
  return NULL;
}

void owl_command_getsubs(void)
{
  owl_function_getsubs();
}

void owl_command_status(void)
{
  owl_function_status();
}

char *owl_command_zwrite(int argc, const char *const *argv, const char *buff)
{
  owl_zwrite *z;

  if (!owl_global_is_havezephyr(&g)) {
    owl_function_makemsg("Zephyr is not available");
    return(NULL);
  }
  /* check for a zwrite -m */
  z = owl_zwrite_new(buff);
  if (!z) {
    owl_function_error("Error in zwrite arguments");
    return NULL;
  }

  if (owl_zwrite_is_message_set(z)) {
    owl_function_zwrite(z, NULL);
    owl_zwrite_delete(z);
    return NULL;
  }

  if (argc < 2) {
    owl_zwrite_delete(z);
    owl_function_makemsg("Not enough arguments to the zwrite command.");
  } else {
    owl_function_zwrite_setup(z);
  }
  return(NULL);
}

char *owl_command_aimwrite(int argc, const char *const *argv, const char *buff)
{
  char *newbuff, *recip;
  const char *const *myargv;
  int i, j, myargc;
  owl_message *m;
  
  if (!owl_global_is_aimloggedin(&g)) {
    owl_function_makemsg("You are not logged in to AIM.");
    return(NULL);
  }

  if (argc < 2) {
    owl_function_makemsg("Not enough arguments to the aimwrite command.");
    return(NULL);
  }

  myargv=argv;
  if (argc<0) {
    owl_function_error("Unbalanced quotes in aimwrite");
    return(NULL);
  }
  myargc=argc;
  if (myargc && *(myargv[0])!='-') {
    myargc--;
    myargv++;
  }
  while (myargc) {
    if (!strcmp(myargv[0], "-m")) {
      if (myargc<2) {
	break;
      }

      /* Once we have -m, gobble up everything else on the line */
      myargv++;
      myargc--;
      newbuff=owl_strdup("");
      while (myargc) {
	newbuff=owl_realloc(newbuff, strlen(newbuff)+strlen(myargv[0])+5);
	strcat(newbuff, myargv[0]);
	strcat(newbuff, " ");
	myargc--;
	myargv++;
      }
      if (strlen(newbuff) >= 1)
	newbuff[strlen(newbuff) - 1] = '\0'; /* remove last space */

      recip=owl_strdup(argv[1]);
      owl_aim_send_im(recip, newbuff);
      m=owl_function_make_outgoing_aim(newbuff, recip);
      if (m) { 
          owl_global_messagequeue_addmsg(&g, m);
      } else {
          owl_function_error("Could not create outgoing AIM message");
      }

      owl_free(recip);
      owl_free(newbuff);
      return(NULL);
    } else {
      /* we don't care */
      myargv++;
      myargc--;
    }
  }

  /* squish arguments together to make one screenname w/o spaces for now */
  newbuff=owl_malloc(strlen(buff)+5);
  sprintf(newbuff, "%s ", argv[0]);
  j=argc-1;
  for (i=0; i<j; i++) {
    strcat(newbuff, argv[i+1]);
  }
    
  owl_function_aimwrite_setup(newbuff);
  owl_free(newbuff);
  return(NULL);
}

char *owl_command_loopwrite(int argc, const char *const *argv, const char *buff)
{
  owl_function_loopwrite_setup();
  return(NULL);
}

char *owl_command_reply(int argc, const char *const *argv, const char *buff)
{
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
    const owl_message *m;
    const owl_view    *v;
    v = owl_global_get_current_view(&g);    
    m = owl_view_get_element(v, owl_global_get_curmsg(&g));
    if (m) owl_zephyr_zaway(m);
  } else {
    owl_function_makemsg("Invalid arguments to the reply command.");
  }
  return NULL;
}

char *owl_command_filter(int argc, const char *const *argv, const char *buff)
{
  owl_function_create_filter(argc, argv);
  return NULL;
}

char *owl_command_zlocate(int argc, const char *const *argv, const char *buff)
{
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


/* Backwards compatability has made this kind of complicated:
 * view [<viewname>] [-f <filter> | -d <expression> | --home | -r ] [-s <style>]
 * view <filter>
 * view -d <expression>
 * view --home
 */
char *owl_command_view(int argc, const char *const *argv, const char *buff)
{
  /* First take the 'view --home' and 'view -r' cases */
  if (argc == 2) {
    if (!strcmp(argv[1], "--home")) {
      owl_function_change_currentview_filter(owl_global_get_view_home(&g));
      return(NULL);
    } else if (!strcmp(argv[1], "-r")) {
      char *foo;
      foo=owl_function_create_negative_filter(owl_view_get_filtname(owl_global_get_current_view(&g)));
      owl_function_change_currentview_filter(foo);
      owl_free(foo);
      return(NULL);
    }
  }

  /* Now look for 'view <filter>' */
  if (argc==2) {
    owl_function_change_currentview_filter(argv[1]);
    return(NULL);
  }

  /* Now get 'view -d <expression>' */
  if (argc>=3 && !strcmp(argv[1], "-d")) {
    const char **myargv;
    int i;

    /* Allocate one more than argc for the trailing NULL. */
    myargv = g_new0(const char*, argc+1);
    myargv[0]="";
    myargv[1]="owl-dynamic";
    for (i=2; i<argc; i++) {
      myargv[i]=argv[i];
    }
    owl_function_create_filter(argc, myargv);
    owl_function_change_currentview_filter("owl-dynamic");
    owl_free(myargv);
    return NULL;
  }

  /* Finally handle the general case */
  if (argc<3) {
    owl_function_makemsg("Too few arguments to the view command.");
    return(NULL);
  }
  argc--;
  argv++;
  if (strcmp(argv[0], "-f") &&
      strcmp(argv[0], "-d") &&
      strcmp(argv[0], "--home") &&
      strcmp(argv[0], "-s") &&
      strcmp(argv[0], "-r")) {
    if (strcmp(argv[0], "main")) {
      owl_function_makemsg("No view named '%s'", argv[0]);
      return(NULL);
    }
    argc--;
    argv++;
  }
  while (argc) {
    if (!strcmp(argv[0], "-f")) {
      if (argc<2) {
	owl_function_makemsg("Too few argments to the view command");
	return(NULL);
      }
      owl_function_change_currentview_filter(argv[1]);
      argc-=2;
      argv+=2;
    } else if (!strcmp(argv[0], "--home")) {
      owl_function_change_currentview_filter(owl_global_get_view_home(&g));
      argc--;
      argv++;
    } else if (!strcmp(argv[0], "-s")) {
      if (argc<2) {
	owl_function_makemsg("Too few argments to the view command");
	return(NULL);
      }
      owl_function_change_style(owl_global_get_current_view(&g), argv[1]);
      argc-=2;
      argv+=2;
    } else {
      owl_function_makemsg("Too few argments to the view command");
      return(NULL);
    }
    
  }
  return(NULL);
}

char *owl_command_show(int argc, const char *const *argv, const char *buff)
{
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
  } else if (!strcmp(argv[1], "view")) {
    if (argc==3) {
      owl_function_show_view(argv[2]);
    } else {
      owl_function_show_view(NULL);
    }
  } else if (!strcmp(argv[1], "colors")) {
    owl_function_show_colors();
  } else if (!strcmp(argv[1], "styles")) {
    owl_function_show_styles();
  } else if (!strcmp(argv[1], "timers")) {
    owl_function_show_timers();
  } else if (!strcmp(argv[1], "subs") || !strcmp(argv[1], "subscriptions")) {
    owl_function_getsubs();
  } else if (!strcmp(argv[1], "terminal") || !strcmp(argv[1], "term")) {
    owl_function_show_term();
  } else if (!strcmp(argv[1], "version")) {
    owl_function_about();
  } else if (!strcmp(argv[1], "status")) {
    owl_function_status();
  } else if (!strcmp(argv[1], "license")) {
    owl_function_show_license();
  } else if (!strcmp(argv[1], "quickstart")) {
    owl_function_show_quickstart();
  } else if (!strcmp(argv[1], "startup")) {
    const char *filename;
    
    filename=owl_global_get_startupfile(&g);
    owl_function_popless_file(filename);
  } else if (!strcmp(argv[1], "errors")) {
    owl_function_showerrs();
  } else {
    owl_function_makemsg("Unknown subcommand for 'show' command (see 'help show' for allowed args)");
    return NULL;
  }
  return NULL;
}

char *owl_command_viewclass(int argc, const char *const *argv, const char *buff)
{
  char *filtname;
  if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments to viewclass command");
    return NULL;
  }
  filtname = owl_function_classinstfilt(argv[1], NULL, owl_global_is_narrow_related(&g));
  if (filtname) {
    owl_function_change_currentview_filter(filtname);
    owl_free(filtname);
  }
  return NULL;
}

char *owl_command_viewuser(int argc, const char *const *argv, const char *buff)
{
  char *filtname;
  char *longuser;
  if (argc!=2) {
    owl_function_makemsg("Wrong number of arguments to viewuser command");
    return NULL;
  }
  longuser = long_zuser(argv[1]);
  filtname = owl_function_zuserfilt(longuser);
  owl_free(longuser);
  if (filtname) {
    owl_function_change_currentview_filter(filtname);
    owl_free(filtname);
  }
  return NULL;
}


void owl_command_pop_message(void)
{
  owl_function_curmsg_to_popwin();
}

char *owl_command_delete(int argc, const char *const *argv, const char *buff)
{
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

char *owl_command_undelete(int argc, const char *const *argv, const char *buff)
{
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

void owl_command_beep(void)
{
  owl_function_beep();
}

char *owl_command_colorview(int argc, const char *const *argv, const char *buff)
{
  if (argc < 2 || argc > 3) {
    owl_function_makemsg("Wrong number of arguments to colorview command");
    return NULL;
  }
  owl_function_color_current_filter(argv[1], (argc == 3 ? argv[2] : NULL));
  return NULL;
}

char *owl_command_colorclass(int argc, const char *const *argv, const char *buff)
{
  char *filtname;
  
  if (argc < 3 || argc > 4) {
    owl_function_makemsg("Wrong number of arguments to colorclass command");
    return NULL;
  }

  filtname=owl_function_classinstfilt(argv[1], NULL, owl_global_is_narrow_related(&g));
  if (filtname) {
    (void) owl_function_color_filter(filtname, argv[2], (argc == 4 ? argv[3] : NULL));
    owl_free(filtname);
  }
  return NULL;
}

char *owl_command_zpunt(int argc, const char *const *argv, const char *buff)
{
  owl_command_zpunt_and_zunpunt(argc, argv, 0);
  return NULL;
}

char *owl_command_zunpunt(int argc, const char *const *argv, const char *buff)
{
  owl_command_zpunt_and_zunpunt(argc, argv, 1);
  return NULL;
}

void owl_command_zpunt_and_zunpunt(int argc, const char *const *argv, int type)
{
  /* if type==0 then zpunt
   * if type==1 then zunpunt
   */
  const char *class, *inst, *recip;

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

char *owl_command_smartzpunt(int argc, const char *const *argv, const char *buff)
{
  if (argc == 1) {
    owl_function_smartzpunt(0);
  } else if (argc == 2 && (!strcmp(argv[1], "-i") || !strcmp(argv[1], "--instance"))) {
    owl_function_smartzpunt(1);
  } else {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);    
  }
  return NULL;
}

char *owl_command_punt(int argc, const char *const *argv, const char *buff)
{
  owl_command_punt_unpunt(argc, argv, buff, 0);
  return NULL;
}

char *owl_command_unpunt(int argc, const char *const *argv, const char *buff)
{
  owl_command_punt_unpunt(argc, argv, buff, 1);
  return NULL;
}

void owl_command_punt_unpunt(int argc, const char *const * argv, const char *buff, int unpunt)
{
  owl_list * fl;
  owl_filter * f;
  char * text;
  int i;

  fl = owl_global_get_puntlist(&g);
  if(argc == 1) {
    owl_function_show_zpunts();
  }

  if(argc == 2) {
    /* Handle :unpunt <number> */
    if(unpunt && (i=atoi(argv[1])) !=0) {
      i--;      /* Accept 1-based indexing */
      if(i < owl_list_get_size(fl)) {
        f = owl_list_get_element(fl, i);
        owl_list_remove_element(fl, i);
        owl_filter_delete(f);
        return;
      } else {
        owl_function_error("No such filter number: %d", i+1);
      }
    }
    text = owl_sprintf("filter %s", argv[1]);
    owl_function_punt(text, unpunt);
    owl_free(text);
  } else {
    owl_function_punt(skiptokens(buff, 1), unpunt);
  }
}


char *owl_command_getview(int argc, const char *const *argv, const char *buff)
{
  const char *filtname;
  if (argc != 1) {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  filtname = owl_view_get_filtname(owl_global_get_current_view(&g));
  if (filtname) return owl_strdup(filtname);
  return NULL;
}

char *owl_command_getvar(int argc, const char *const *argv, const char *buff)
{
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

char *owl_command_getfilter(int argc, const char *const *argv, const char *buf)
{
  const owl_filter *f;
  if (argc != 2) {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  f = owl_global_get_filter(&g, argv[1]);
  if (!f) {
    return NULL;
  }
  return owl_filter_print(f);
}

char *owl_command_search(int argc, const char *const *argv, const char *buff)
{
  int direction;
  const char *buffstart;

  direction=OWL_DIRECTION_DOWNWARDS;
  buffstart=skiptokens(buff, 1);
  if (argc>1 && !strcmp(argv[1], "-r")) {
    direction=OWL_DIRECTION_UPWARDS;
    buffstart=skiptokens(buff, 2);
  }
    
  if (argc==1 || (argc==2 && !strcmp(argv[1], "-r"))) {
    /* When continuing a search, don't consider the current message. */
    owl_function_search_helper(false, direction);
  } else {
    owl_function_set_search(buffstart);
    owl_function_search_helper(true, direction);
  }
  
  return(NULL);
}

char *owl_command_setsearch(int argc, const char *const *argv, const char *buff)
{
  const char *buffstart;

  buffstart=skiptokens(buff, 1);
  owl_function_set_search(*buffstart ? buffstart : NULL);
  
  return(NULL);
}

char *owl_command_aimlogin(int argc, const char *const *argv, const char *buff)
{
  if ((argc<2) || (argc>3)) {
    owl_function_makemsg("Wrong number of arguments to aimlogin command");
    return(NULL);
  }

  /* if we get two arguments, ask for the password */
  if (argc==2) {
    owl_editwin *e = owl_function_start_password("AIM Password: ");
    owl_editwin_set_cbdata(e, owl_strdup(argv[1]), owl_free);
    owl_editwin_set_callback(e, owl_callback_aimlogin);
    return(NULL);
  } else {
    owl_function_aimlogin(argv[1], argv[2]);
  }

  /* this is a test */
  return(NULL);
}

char *owl_command_aimlogout(int argc, const char *const *argv, const char *buff)
{
  /* clear the buddylist */
  owl_buddylist_clear(owl_global_get_buddylist(&g));

  owl_aim_logout();
  return(NULL);
}

char *owl_command_getstyle(int argc, const char *const *argv, const char *buff)
{
  const char *stylename;
  if (argc != 1) {
    owl_function_makemsg("Wrong number of arguments for %s", argv[0]);
    return NULL;
  }
  stylename = owl_view_get_style_name(owl_global_get_current_view(&g));
  if (stylename) return owl_strdup(stylename);
  return NULL;
}

char *owl_command_error(int argc, const char *const *argv, const char *buff)
{
    buff = skiptokens(buff, 1);
    owl_function_error("%s", buff);
    return NULL;
}

char *owl_command_message(int argc, const char *const *argv, const char *buff)
{
    buff = skiptokens(buff, 1);
    owl_function_makemsg("%s", buff);
    return NULL;
}

char *owl_command_add_cmd_history(int argc, const char *const *argv, const char *buff)
{
  owl_history *hist;
  const char *ptr;

  if (argc < 2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return NULL;
  }

  ptr = skiptokens(buff, 1);
  hist = owl_global_get_cmd_history(&g);
  owl_history_store(hist, ptr);
  owl_history_reset(hist);
  /* owl_function_makemsg("History '%s' stored successfully", ptr+1); */
  return NULL;
}

char *owl_command_with_history(int argc, const char *const *argv, const char *buff)
{
  owl_history *hist;
  const char *ptr;

  if (argc < 2) {
    owl_function_makemsg("usage: %s <commands> ...", argv[0]);
    return NULL;
  }

  ptr = skiptokens(buff, 1);
  if (!ptr) {
    owl_function_makemsg("Parse error finding command");
    return NULL;
  }

  hist = owl_global_get_cmd_history(&g);
  owl_history_store(hist, ptr);
  owl_history_reset(hist);
  return owl_function_command(ptr);
}

void owl_command_yes(void)
{
  owl_message *m;
  const owl_view *v;
  const char *cmd;

  v = owl_global_get_current_view(&g);

  /* bail if there's no current message */
  if (owl_view_get_size(v) < 1) {
    owl_function_error("No current message.");
    return;
  }

  m = owl_view_get_element(v, owl_global_get_curmsg(&g));
  if(!owl_message_is_question(m)) {
    owl_function_error("That message isn't a question.");
    return;
  }
  if(owl_message_is_answered(m)) {
    owl_function_error("You already answered that question.");
    return;
  }
  cmd = owl_message_get_attribute_value(m, "yescommand");
  if(!cmd) {
    owl_function_error("No 'yes' command!");
    return;
  }

  owl_function_command_norv(cmd);
  owl_message_set_isanswered(m);
  return;
}

void owl_command_no(void)
{
  owl_message *m;
  const owl_view *v;
  const char *cmd;

  v = owl_global_get_current_view(&g);

  /* bail if there's no current message */
  if (owl_view_get_size(v) < 1) {
    owl_function_error("No current message.");
    return;
  }

  m = owl_view_get_element(v, owl_global_get_curmsg(&g));
  if(!owl_message_is_question(m)) {
    owl_function_error("That message isn't a question.");
    return;
  }
  if(owl_message_is_answered(m)) {
    owl_function_error("You already answered that question.");
    return;
  }
  cmd = owl_message_get_attribute_value(m, "nocommand");
  if(!cmd) {
    owl_function_error("No 'no' command!");
    return;
  }

  owl_function_command_norv(cmd);
  owl_message_set_isanswered(m);
  return;
}

/*********************************************************************/
/************************** EDIT SPECIFIC ****************************/
/*********************************************************************/

void owl_command_edit_cancel(owl_editwin *e)
{
  owl_history *hist;

  owl_function_makemsg("Command cancelled.");

  hist = owl_editwin_get_history(e);
  if (hist) {
    owl_history_store(hist, owl_editwin_get_text(e));
    owl_history_reset(hist);
  }

  owl_global_pop_context(&g);
}

void owl_command_edit_history_prev(owl_editwin *e)
{
  owl_history *hist;
  const char *ptr;

  hist=owl_editwin_get_history(e);
  if (!hist)
    return;
  if (!owl_history_is_touched(hist)) {
    owl_history_store(hist, owl_editwin_get_text(e));
    owl_history_set_partial(hist);
  }
  ptr=owl_history_get_prev(hist);
  if (ptr) {
    owl_editwin_clear(e);
    owl_editwin_insert_string(e, ptr);
  } else {
    owl_function_beep();
  }
}

void owl_command_edit_history_next(owl_editwin *e)
{
  owl_history *hist;
  const char *ptr;

  hist=owl_editwin_get_history(e);
  if (!hist)
    return;
  ptr=owl_history_get_next(hist);
  if (ptr) {
    owl_editwin_clear(e);
    owl_editwin_insert_string(e, ptr);
  } else {
    owl_function_beep();
  }
}

char *owl_command_edit_insert_text(owl_editwin *e, int argc, const char *const *argv, const char *buff)
{
  buff = skiptokens(buff, 1);
  owl_editwin_insert_string(e, buff);
  return NULL;
}

void owl_command_edit_done(owl_editwin *e)
{
  owl_history *hist=owl_editwin_get_history(e);

  if (hist) {
    owl_history_store(hist, owl_editwin_get_text(e));
    owl_history_reset(hist);
  }

  /* Take a reference to the editwin, so that it survives the pop
   * context. TODO: We should perhaps refcount or otherwise protect
   * the context so that, even if a command pops a context, the
   * context itself will last until the command returns. */
  owl_editwin_ref(e);
  owl_global_pop_context(&g);

  owl_editwin_do_callback(e);
  owl_editwin_unref(e);
}

void owl_command_edit_done_or_delete(owl_editwin *e)
{
  if (owl_editwin_is_at_end(e)) {
    owl_command_edit_done(e);
  } else {
    owl_editwin_delete_char(e);
  }
}


/*********************************************************************/
/*********************** POPLESS SPECIFIC ****************************/
/*********************************************************************/

void owl_command_popless_quit(owl_viewwin *vw)
{
  owl_popwin *pw;
  pw = owl_global_get_popwin(&g);
  owl_global_set_popwin(&g, NULL);
  /* Kind of a hack, but you can only have one active viewwin right
   * now anyway. */
  if (vw == owl_global_get_viewwin(&g))
    owl_global_set_viewwin(&g, NULL);
  owl_viewwin_delete(vw);
  owl_popwin_delete(pw);
  owl_global_pop_context(&g);
}
