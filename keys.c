#include "owl.h"

static const char fileIdent[] = "$Id$";

#define BIND_CMD(kpress, command, desc) \
         owl_keymap_create_binding(km, kpress, command, NULL, desc);

#define BIND_FNV(kpress, fn, desc) \
         owl_keymap_create_binding(km, kpress, NULL, fn, desc);


/* sets up the default keymaps */
void owl_keys_setup_keymaps(owl_keyhandler *kh) {
  owl_keymap *km, *km_global, *km_editwin, *km_mainwin,
    *km_ew_multi, *km_ew_onel, *km_viewwin;

  
  /****************************************************************/
  /*************************** GLOBAL *****************************/
  /****************************************************************/

  km_global = km = owl_keyhandler_create_and_add_keymap(kh, "global",
       "System-wide default key bindings", 
       owl_keys_default_invalid, NULL, NULL);
  BIND_CMD("C-z",      "suspend",            "Suspend owl");

  /****************************************************************/
  /***************************** EDIT *****************************/
  /****************************************************************/

  km_editwin = km = owl_keyhandler_create_and_add_keymap(kh, "edit",
       "Text editing and command window", 
       owl_keys_editwin_default, NULL, owl_keys_editwin_postalways);
  owl_keymap_set_submap(km_editwin, km_global);
  /*
  BIND_CMD("F1",          "help",            "");
  BIND_CMD("HELP",        "help",            "");
  BIND_CMD("M-[ 2 8 ~",   "help",            "");
  */

  BIND_CMD("C-c",         "edit:cancel", "");
  BIND_CMD("C-g",         "edit:cancel", "");

  BIND_CMD("M-f",         "edit:move-next-word", "");
  BIND_CMD("M-O 3 C",     "edit:move-next-word", "");
  BIND_CMD("M-LEFT",      "edit:move-next-word", "");
  BIND_CMD("M-b",         "edit:move-prev-word", "");
  BIND_CMD("M-O 3 D",     "edit:move-prev-word", "");
  BIND_CMD("M-RIGHT",     "edit:move-prev-word", "");

  BIND_CMD("LEFT",        "edit:move-left", "");
  BIND_CMD("C-b",         "edit:move-left", "");
  BIND_CMD("RIGHT",       "edit:move-right", "");
  BIND_CMD("C-f",         "edit:move-right", "");

  BIND_CMD("M-<",         "edit:move-to-buffer-start", "");
  BIND_CMD("HOME",        "edit:move-to-buffer-start", "");
  BIND_CMD("M->",         "edit:move-to-buffer-end", "");
  BIND_CMD("END",         "edit:move-to-buffer-end", "");

  BIND_CMD("C-a",         "edit:move-to-line-start", "");
  BIND_CMD("C-e",         "edit:move-to-line-end", "");

  BIND_CMD("M-BACKSPACE", "edit:delete-prev-word", "");
  BIND_CMD("M-DELETE",    "edit:delete-prev-word", "");
  BIND_CMD("M-d",         "edit:delete-next-word", "");
  BIND_CMD("M-[ 3 ; 3 ~", "edit:delete-next-word", "");

  BIND_CMD("C-h",         "edit:delete-prev-char", "");
  BIND_CMD("BACKSPACE",   "edit:delete-prev-char", "");
  BIND_CMD("DC",          "edit:delete-prev-char", "");
  BIND_CMD("DELETE",      "edit:delete-prev-char", "");

  BIND_CMD("C-k",         "edit:delete-to-line-end", "");

  BIND_CMD("C-t",         "edit:transpose-chars", "");

  BIND_CMD("M-q",         "edit:fill-paragraph", "");

  BIND_CMD("C-l",         "( edit:recenter ; redisplay )", "");

  BIND_CMD("C-d",     "edit:delete-next-char", "");


  /****************************************************************/
  /**************************** EDITMULTI *************************/
  /****************************************************************/

  km_ew_multi = km = owl_keyhandler_create_and_add_keymap(kh, "editmulti",
       "Multi-line text editing", 
       owl_keys_editwin_default, NULL, owl_keys_editwin_postalways);
  owl_keymap_set_submap(km_ew_multi, km_editwin);

  BIND_CMD("UP",      "editmulti:move-up-line", "");
  BIND_CMD("C-p",     "editmulti:move-up-line", "");
  BIND_CMD("DOWN",    "editmulti:move-down-line", "");
  BIND_CMD("C-n",     "editmulti:move-down-line", "");

  /* This would be nice, but interferes with C-c to cancel */
  /*BIND_CMD("C-c C-c", "editmulti:done", "sends the zephyr");*/

  BIND_CMD("M-p",         "edit:history-prev", "");
  BIND_CMD("M-n",         "edit:history-next", "");

  /* note that changing "disable-ctrl-d" to "on" will change this to 
   * edit:delete-next-char */
  BIND_CMD("C-d",     "editmulti:done-or-delete", "sends the zephyr if at the end of the message");


  /****************************************************************/
  /**************************** EDITLINE **************************/
  /****************************************************************/

  km_ew_onel = km = owl_keyhandler_create_and_add_keymap(kh, "editline",
       "Single-line text editing", 
       owl_keys_editwin_default, NULL, owl_keys_editwin_postalways);
  owl_keymap_set_submap(km_ew_onel, km_editwin);

  BIND_CMD("C-u",         "edit:delete-all", "Clears the entire line");

  BIND_CMD("UP",          "edit:history-prev", "");
  BIND_CMD("C-p",         "edit:history-prev", "");
  BIND_CMD("M-p",         "edit:history-prev", "");

  BIND_CMD("DOWN",        "edit:history-next", "");
  BIND_CMD("C-n",         "edit:history-next", "");
  BIND_CMD("M-n",         "edit:history-next", "");

  BIND_CMD("LF",          "editline:done", "executes the command");
  BIND_CMD("CR",          "editline:done", "executes the command");

  
  /****************************************************************/
  /**************************** EDITRESPONSE **********************/
  /****************************************************************/

  km_ew_onel = km = owl_keyhandler_create_and_add_keymap(kh, "editresponse",
       "Single-line response to question", 
       owl_keys_editwin_default, NULL, owl_keys_editwin_postalways);
  owl_keymap_set_submap(km_ew_onel, km_editwin);

  BIND_CMD("C-u",         "edit:delete-all", "Clears the entire line");

  BIND_CMD("LF",          "editresponse:done", "executes the command");
  BIND_CMD("CR",          "editresponse:done", "executes the command");


  /****************************************************************/
  /**************************** POPLESS ***************************/
  /****************************************************************/

  km_viewwin = km = owl_keyhandler_create_and_add_keymap(kh, "popless",
       "Pop-up window (eg, help)", 
       owl_keys_default_invalid, NULL, owl_keys_popless_postalways);
  owl_keymap_set_submap(km_viewwin, km_global);

  BIND_CMD("SPACE",       "popless:scroll-down-page", "");
  BIND_CMD("NPAGE",       "popless:scroll-down-page", "");
  BIND_CMD("M-n",         "popless:scroll-down-page", "");

  BIND_CMD("b",           "popless:scroll-up-page", "");
  BIND_CMD("PPAGE",       "popless:scroll-up-page", "")
  BIND_CMD("M-p",         "popless:scroll-up-page", "");

  BIND_CMD("CR",          "popless:scroll-down-line", "");
  BIND_CMD("LF",          "popless:scroll-down-line", "");
  BIND_CMD("DOWN",        "popless:scroll-down-line", "");
  BIND_CMD("C-n",         "popless:scroll-down-line", "");

  BIND_CMD("UP",          "popless:scroll-up-line", "");
  BIND_CMD("C-h",         "popless:scroll-up-line", "");
  BIND_CMD("C-p",         "popless:scroll-up-line", "");
  BIND_CMD("DELETE",      "popless:scroll-up-line", "");
  BIND_CMD("BACKSPACE",   "popless:scroll-up-line", "");
  BIND_CMD("DC",          "popless:scroll-up-line", "");

  BIND_CMD("RIGHT",       "popless:scroll-right 10", "scrolls right");
  BIND_CMD("LEFT",        "popless:scroll-left  10", "scrolls left");

  BIND_CMD("HOME",        "popless:scroll-to-top", "");
  BIND_CMD("<",           "popless:scroll-to-top", "");
  BIND_CMD("M-<",         "popless:scroll-to-top", "");

  BIND_CMD("END",         "popless:scroll-to-bottom", "");
  BIND_CMD(">",           "popless:scroll-to-bottom", "");
  BIND_CMD("M->",         "popless:scroll-to-bottom", "");

  BIND_CMD("q",           "popless:quit", "");
  BIND_CMD("C-c",         "popless:quit", "");
  BIND_CMD("C-g",         "popless:quit", "");

  BIND_CMD("C-l",         "redisplay", "");


  /****************************************************************/
  /***************************** RECV *****************************/
  /****************************************************************/

  km_mainwin = km = owl_keyhandler_create_and_add_keymap(kh, "recv",
	"Main window / message list",
        owl_keys_default_invalid, owl_keys_recwin_prealways, NULL);
  owl_keymap_set_submap(km_mainwin, km_global);
  BIND_CMD("C-x C-c", "start-command quit", "");
  BIND_CMD("F1",      "help",           "");
  BIND_CMD("h",       "help",           "");
  BIND_CMD("HELP",    "help",           "");
  BIND_CMD("M-[ 2 8 ~",   "help",       "");

  BIND_CMD(":",       "start-command",  "start a new command");
  BIND_CMD("M-x",     "start-command",  "start a new command");

  BIND_CMD("x",       "expunge",        "");
  BIND_CMD("u",       "undelete",       "");
  BIND_CMD("M-u",     "undelete view",  "undelete all messages in view");
  BIND_CMD("d",       "delete",         "mark message for deletion");
  BIND_CMD("M-D",     "delete view",    "mark all messages in view for deletion");
  BIND_CMD("C-x k",   "smartzpunt -i",  "zpunt current <class,instance>");

  BIND_CMD("X",   "( expunge ; view --home )", "expunge deletions and switch to home view");

  BIND_CMD("v",   "start-command view ", "start a view command");
  BIND_CMD("V",   "view --home",      "change to the home view ('all' by default)");
  BIND_CMD("!",   "view -r",          "invert the current view filter");
  BIND_CMD("M-n", "smartnarrow",      "narrow to a view based on the current message");
  BIND_CMD("M-N", "smartnarrow -i",   "narrow to a view based on the current message, and consider instance pair");
  BIND_CMD("M-p", "view personal", "");
  
  BIND_CMD("/",   "start-command search ", "start a search command");
  BIND_CMD("?",   "start-command search -r ", "start a reverse search command");

  BIND_CMD("LEFT",   "recv:shiftleft", "");
  BIND_CMD("M-[ D",  "recv:shiftleft", "");
  BIND_CMD("RIGHT",  "recv:shiftright","");
  BIND_CMD("M-[ C",  "recv:shiftleft", "");
  BIND_CMD("DOWN",   "recv:next",      "");
  BIND_CMD("C-n",    "recv:next",      "");
  BIND_CMD("M-[ B",  "recv:next",      "");
  BIND_CMD("M-C-n",  "recv:next --smart-filter", "move to next message matching the current one"); 
  BIND_CMD("UP",     "recv:prev",      "");
  BIND_CMD("M-[ A",  "recv:prev",      "");
  BIND_CMD("n",      "recv:next-notdel", "");
  BIND_CMD("p",      "recv:prev-notdel", "");
  BIND_CMD("C-p",    "recv:prev",        "");
  BIND_CMD("M-C-p",  "recv:prev --smart-filter", "move to previous message matching the current one");
  BIND_CMD("P",      "recv:next-personal", "");
  BIND_CMD("M-P",    "recv:prev-personal", "");
  BIND_CMD("M-<",    "recv:first",     "");
  BIND_CMD("<",      "recv:first",     "");
  BIND_CMD("M->",    "recv:last",      "");
  BIND_CMD(">",      "recv:last",      "");
  BIND_CMD("C-v",    "recv:pagedown",  "");
  BIND_CMD("NPAGE",  "recv:pagedown",  "");
  BIND_CMD("M-v",    "recv:pageup",    "");
  BIND_CMD("PPAGE",  "recv:pageup",    "");

  BIND_CMD("SPACE",     "recv:scroll  10", "scroll message down a page");
  BIND_CMD("CR",        "recv:scroll   1", "scroll message down a line");
  BIND_CMD("LF",        "recv:scroll   1", "scroll message down a line");
  BIND_CMD("C-h"  ,     "recv:scroll  -1", "scroll message up a line");
  BIND_CMD("DELETE",    "recv:scroll  -1", "scroll message up a line");
  BIND_CMD("BACKSPACE", "recv:scroll  -1", "scroll message up a line");
  BIND_CMD("DC",        "recv:scroll  -1", "scroll message up a line");
  BIND_CMD("b",         "recv:scroll -10", "scroll message up a page");

  BIND_CMD("C-l",       "redisplay",       "");

  BIND_CMD("i",   "info",             "");
  BIND_CMD("l",   "blist",            "");
  BIND_CMD("B",   "alist",            "");
  BIND_CMD("M",   "pop-message",      "");
  BIND_CMD("T",   "delete trash",     "mark all 'trash' messages for deletion");

  BIND_CMD("o",   "toggle-oneline", "");

  BIND_CMD("A",   "away toggle",     "toggles away message on and off");

  BIND_CMD("z",   "start-command zwrite ", "start a zwrite command");
  BIND_CMD("a",   "start-command aimwrite ", "start an aimwrite command");
  BIND_CMD("r",   "reply",            "reply to the current message");
  BIND_CMD("R",   "reply sender",     "reply to sender of the current message");
  BIND_CMD("C-r", "reply -e",         "reply to the current message, but allow editing of recipient");
  BIND_CMD("M-r", "reply -e",         "reply to the current message, but allow editing of recipient");
  BIND_CMD("M-R", "reply -e sender",  "reply to sender of the current message, but allow editing of recipient");
		  
  BIND_CMD("w",   "openurl",          "open a URL using a webbrowser");

  BIND_CMD("W",   "start-command webzephyr ", "start a webzephyr command");

  BIND_CMD("C-c",  "",                "no effect in this mode");
  BIND_CMD("C-g",  "",                "no effect in this mode");


  /**********************/

  owl_function_activate_keymap("recv");

}


/****************************************************************/
/********************* Support Functions ************************/
/****************************************************************/

void owl_keys_recwin_prealways(owl_input j) {
  /* Clear the message line on subsequent key presses */
  owl_function_makemsg("");
}

void owl_keys_editwin_default(owl_input j) {
  owl_editwin *e;
  if (NULL != (e=owl_global_get_typwin(&g))) {
       owl_editwin_process_char(e, j);
  }
}

void owl_keys_editwin_postalways(owl_input j) {
  owl_editwin *e;
  if (NULL != (e=owl_global_get_typwin(&g))) {
    owl_editwin_post_process_char(e, j);
  }
  owl_global_set_needrefresh(&g);
}

void owl_keys_popless_postalways(owl_input j) {
  owl_viewwin *v = owl_global_get_viewwin(&g);
  owl_popwin *pw = owl_global_get_popwin(&g);

  if (pw && owl_popwin_is_active(pw) && v) {
    owl_viewwin_redisplay(v, 1);
  }  
}

void owl_keys_default_invalid(owl_input j) {
  if (j.ch==ERR) return;
  if (j.ch==410) return;
  owl_keyhandler_invalidkey(owl_global_get_keyhandler(&g));
}

