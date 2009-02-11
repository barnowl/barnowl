#include "owl.h"
#include <string.h>

static const char fileIdent[] = "$Id$";

void owl_help()
{
  owl_fmtext fm;
  char *varname;
  owl_list varnames;
  int i, numvarnames;

  owl_fmtext_init_null(&fm);
  owl_fmtext_append_bold
    (&fm, 
     "OWL HELP\n\n");

  owl_fmtext_append_normal
    (&fm, 
     "  If you're new to BarnOwl, the first thing you should type is\n\n"
     "    :show quickstart\n\n"
     "  For help on a specific command use 'help <command>'\n"
     "  For information on advanced keys, use 'show keymaps'.\n"
     "  For information on advanced commands, use 'show commands'.\n"
     "  For information on variables, use 'show variables'.\n\n");

  owl_fmtext_append_bold
    (&fm, 
     "  Basic Keys:\n"
     );
  owl_fmtext_append_normal
    (&fm, 
     "    n             Move to next non-deleted message\n"
     "    p             Move to previous non-deleted message\n"
     "    C-n , down    Move to next message\n"
     "    C-p , up      Move to previous message\n"
     "    < , >         Move to first, last message\n"
     "    right , left  Scroll screen left or right\n"
     "    C-v           Page down\n"
     "    M-v           Page up\n"
     "    i             Print more information about a message\n"
     "    P             Move to the next personal message\n"
     "    M-P           Move to the previous personal message\n"
     "\n"
     "    d             Mark message for deletion\n"
     "    u             Undelete a message marked for deletion\n"
     "    x             Expunge deleted messages\n"
     "    X             Expunge deleted messages and switch to home view\n"
     "    T             Mark all 'trash' messages for deletion\n"
     "    M-D           Mark all messages in current view for deletion\n"
     "    M-u           Unmark all messages in the current view for deletion\n"
     "\n"
     "    z             Start a zwrite command\n"
     "    a             Start an aimwrite command\n"
     "    r             Reply to the current message\n"
     "    R             Reply to sender\n"
     "    C-r           Reply but allow editing of reply line\n"
     "\n"
     "    M-n           View zephyrs in selected conversation\n"
     "    M-N           View zephyrs in selected converstaion of instance\n"
     "    M-p           View only personal zephyrs\n"
     "    V             Change to back to home view ('all' by default)\n"
     "    v             Start a view command\n"
     "    !             Invert the current view\n"
     "\n"
     "    l             Print a zephyr/AIM buddy listing\n"
     "    A             Toggle zaway\n"
     "    o             Toggle one-line display mode\n"
     "    w             Open a URL in the current message\n"
     "    C-l           Refresh the screen\n"
     "    C-z           Suspend Owl\n"
     "    h             Print this help message\n"
     "    : , M-x       Enter command mode\n"
     "\n"
     "    /             Foward search\n"
     "    ?             Reverse search\n"
     "\n\n"
     );
  owl_fmtext_append_bold
    (&fm, 
     "  Basic Commands:\n"
     );
  owl_fmtext_append_normal
    (&fm, 
     "    quit, exit    Exit owl\n"
     "    help          Get help about commands\n"
     "    show          Show information about owl (see detailed help)\n"
     "\n"
     "    zwrite        Send a zephyr\n"
     "    aimlogin      Login to AIM\n"
     "    aimwrite      Send an AIM message\n"
     "\n"
     "    addbuddy      Add a zephyr or AIM buddy\n"
     "    zaway         Turn zaway on or off, or set the message\n"
     "    zlocate       Locate a user\n"
     "    subscribe     Subscribe to a zephyr class or instance\n"
     "    unsubscribe   Unsubscribe to a zephyr class or instance\n"
     "    blist         Print a list of zephyr and AIM buddies logged in\n"
     "    search        Search for a text string\n"
     "\n"
     "    set           Set a variable (see list below)\n"
     "    print         Print a variable's value (variables listed below)\n"
     "    startup       Set a command to be run at every Owl startup\n"
     "    unstartup     Remove a command to be run at every Owl startup\n"
     "\n"
     "    getsubs       Print a list of current subscriptions\n"
     "    unsuball      Unsubscribe from all zephyr classes\n"
     "    load-subs     Load zephyr subscriptions from a file\n"
     "    zpunt         Supress messages from a zephyr triplet\n"
     "    zlog          Send a login or logout notification\n"
     "    zlist         Print a list of zephyr buddies logged in\n"
     "    alist         Print a list of AIM buddies logged in\n"
     "    info          Print detailed information about the current message\n"
     "    filter        Create a message filter\n"
     "    view          View messages matching a filter\n"
     "    viewuser      View messages to or from a particular user\n"
     "    viewclass     View messages to a particular class\n"
     "    expunge       Expunge messages marked for deletion\n"
     "    bindkey       Create a new key binding\n"
     "    alias         Create a command alias\n"
     "    dump          Dump messagelist as text to a file\n"
     "\n"
     "    about         Print information about owl\n"
     "    status        Print status information about the running owl\n"
     "    version       Print the version number of owl\n"
     "\n");
  
  /* help for variables */
  owl_fmtext_append_bold(&fm, 
			 "Variables:\n");
  owl_variable_dict_get_names(owl_global_get_vardict(&g), &varnames);
  numvarnames = owl_list_get_size(&varnames);
  for (i=0; i<numvarnames; i++) {
    varname = owl_list_get_element(&varnames, i);
    if (varname && varname[0]!='_') {
      owl_variable_describe(owl_global_get_vardict(&g), varname, &fm);
    }
  }
  owl_variable_dict_namelist_free(&varnames);

  owl_fmtext_append_normal(&fm, "\n");

  owl_function_popless_fmtext(&fm);

  owl_fmtext_free(&fm);
}
