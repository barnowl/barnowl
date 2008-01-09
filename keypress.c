#include <ctype.h>
#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

static struct _owl_keypress_specialmap {
  int   kj;
  char *ks;
} specialmap[] = {
#ifdef KEY_CODE_YES
   { KEY_CODE_YES, 	"CODE_YES" }, 
#endif
   { KEY_MIN, 		"MIN" }, 
   { KEY_BREAK, 	"BREAK" }, 
   { KEY_DOWN, 		"DOWN" }, 
   { KEY_UP, 		"UP" }, 
   { KEY_LEFT, 		"LEFT" }, 
   { KEY_RIGHT, 	"RIGHT" }, 
   { KEY_HOME, 		"HOME" }, 
   { KEY_BACKSPACE, 	"BACKSPACE" }, 
   { KEY_F0, 		"F0" }, 
   { KEY_F(1), 		"F1" }, 
   { KEY_F(2), 		"F2" }, 
   { KEY_F(3), 		"F3" }, 
   { KEY_F(4), 		"F4" }, 
   { KEY_F(5), 		"F5" }, 
   { KEY_F(6), 		"F6" }, 
   { KEY_F(7), 		"F7" }, 
   { KEY_F(8), 		"F8" }, 
   { KEY_F(9), 		"F9" }, 
   { KEY_F(10),		"F10" }, 
   { KEY_F(11),		"F11" }, 
   { KEY_F(12),		"F12" }, 
   { KEY_DL, 		"DL" }, 
   { KEY_IL, 		"IL" }, 
   { KEY_DC, 		"DC" }, 
   { KEY_IC, 		"IC" }, 
   { KEY_EIC, 		"EIC" }, 
   { KEY_CLEAR, 	"CLEAR" }, 
   { KEY_EOS, 		"EOS" }, 
   { KEY_EOL, 		"EOL" }, 
   { KEY_SF, 		"SF" }, 
   { KEY_SR, 		"SR" }, 
   { KEY_NPAGE, 	"NPAGE" }, 
   { KEY_PPAGE, 	"PPAGE" }, 
   { KEY_STAB, 		"STAB" }, 
   { KEY_CTAB, 		"CTAB" }, 
   { KEY_CATAB, 	"CATAB" }, 
   { KEY_ENTER, 	"ENTER" }, 
   { KEY_SRESET, 	"SRESET" }, 
   { KEY_RESET, 	"RESET" }, 
   { KEY_PRINT, 	"PRINT" }, 
   { KEY_LL, 		"LL" }, 
   { KEY_A1, 		"A1" }, 
   { KEY_A3, 		"A3" }, 
   { KEY_B2, 		"B2" }, 
   { KEY_C1, 		"C1" }, 
   { KEY_C3, 		"C3" }, 
   { KEY_BTAB, 		"BTAB" }, 
   { KEY_BEG, 		"BEG" }, 
   { KEY_CANCEL, 	"CANCEL" }, 
   { KEY_CLOSE, 	"CLOSE" }, 
   { KEY_COMMAND, 	"COMMAND" }, 
   { KEY_COPY, 	        "COPY" }, 
   { KEY_CREATE, 	"CREATE" }, 
   { KEY_END, 	        "END" }, 
   { KEY_EXIT, 	        "EXIT" }, 
   { KEY_FIND, 	        "FIND" }, 
   { KEY_HELP, 	        "HELP" }, 
   { KEY_MARK, 	        "MARK" }, 
   { KEY_MESSAGE, 	"MESSAGE" }, 
   { KEY_MOVE, 	        "MOVE" }, 
   { KEY_NEXT, 	        "NEXT" }, 
   { KEY_OPEN, 	        "OPEN" }, 
   { KEY_OPTIONS, 	"OPTIONS" }, 
   { KEY_PREVIOUS, 	"PREVIOUS" }, 
   { KEY_REDO, 	        "REDO" }, 
   { KEY_REFERENCE, 	"REFERENCE" }, 
   { KEY_REFRESH, 	"REFRESH" }, 
   { KEY_REPLACE, 	"REPLACE" }, 
   { KEY_RESTART, 	"RESTART" }, 
   { KEY_RESUME, 	"RESUME" }, 
   { KEY_SAVE, 	        "SAVE" }, 
   { KEY_SBEG, 	        "SBEG" }, 
   { KEY_SCANCEL, 	"SCANCEL" }, 
   { KEY_SCOMMAND, 	"SCOMMAND" }, 
   { KEY_SCOPY, 	"SCOPY" }, 
   { KEY_SCREATE, 	"SCREATE" }, 
   { KEY_SDC, 	        "SDC" }, 
   { KEY_SDL, 	        "SDL" }, 
   { KEY_SELECT, 	"SELECT" }, 
   { KEY_SEND, 	        "SEND" }, 
   { KEY_SEOL, 	        "SEOL" }, 
   { KEY_SEXIT, 	"SEXIT" }, 
   { KEY_SFIND, 	"SFIND" }, 
   { KEY_SHELP, 	"SHELP" }, 
   { KEY_SHOME, 	"SHOME" }, 
   { KEY_SIC, 	        "SIC" }, 
   { KEY_SLEFT, 	"SLEFT" }, 
   { KEY_SMESSAGE, 	"SMESSAGE" }, 
   { KEY_SMOVE, 	"SMOVE" }, 
   { KEY_SNEXT, 	"SNEXT" }, 
   { KEY_SOPTIONS, 	"SOPTIONS" }, 
   { KEY_SPREVIOUS, 	"SPREVIOUS" }, 
   { KEY_SPRINT, 	"SPRINT" }, 
   { KEY_SREDO, 	"SREDO" }, 
   { KEY_SREPLACE, 	"SREPLACE" }, 
   { KEY_SRIGHT, 	"SRIGHT" }, 
   { KEY_SRSUME, 	"SRSUME" }, 
   { KEY_SSAVE, 	"SSAVE" }, 
   { KEY_SSUSPEND, 	"SSUSPEND" }, 
   { KEY_SUNDO, 	"SUNDO" }, 
   { KEY_SUSPEND, 	"SUSPEND" }, 
   { KEY_UNDO, 	        "UNDO" }, 
   { KEY_MOUSE, 	"MOUSE" }, 
#ifdef KEY_RESIZE
   { KEY_RESIZE, 	"RESIZE" }, 
#endif
   { KEY_MAX, 	        "MAX" }, 
   { ' ', 	        "SPACE" }, 
   { 27, 	        "ESCAPE" }, 
   { 127, 	        "DELETE" }, 
   { '\r', 	        "CR" }, 
   { '\n', 	        "LF" }, 
   { 0,                 NULL }
};

#define OWL_CTRL(key) ((key)&037)
/* OWL_META is definied in owl.h */

/* returns 0 on success */
int owl_keypress_tostring(int j, int esc, char *buff, int bufflen)
{
  char kb[64], kb2[2];
  struct _owl_keypress_specialmap *sm;

  *kb = '\0';
  for (sm = specialmap; sm->kj!=0; sm++) {
    if (j == OWL_META(sm->kj) || (esc && j == sm->kj)) {
      strcat(kb, "M-");
      strcat(kb, sm->ks);
      break;
    } else if (j == sm->kj) {
      strcat(kb, sm->ks);
      break;
    }
  }
  if (!*kb) {
    if (j & OWL_META(0)) {
      strcat(kb, "M-");
      j &= ~OWL_META(0);
    }
    if ((OWL_CTRL(j) == j)) {
      strcat(kb, "C-");
      j |= 0x40;
      if (isupper(j)) j = tolower(j);

    }
    if (isascii(j)) {
      kb2[0] = j;
      kb2[1] = 0;
      strcat(kb, kb2);    
    }
    
  }  
  if (!*kb) {
    /* not a valid key */
    strncpy(buff, "INVALID", bufflen);
    return(-1);
  }
  strncpy(buff, kb, bufflen);
  return(0);
}


/* returns ERR on failure, else a keycode */
int owl_keypress_fromstring(char *kb)
{
  struct _owl_keypress_specialmap *sm;
  int ismeta=0, isctrl=0;
  int j = ERR;

  while (*kb && kb[1] == '-' && (kb[0] == 'C' || kb[0] == 'M')) {
    if ((kb[0] == 'C') && (kb[1] == '-')) {
      isctrl = 1;
      kb+=2;
    }
    if ((kb[0] == 'M') && (kb[1] == '-')) {
      ismeta = 1;
      kb+=2;
    }
  }
  if (isascii(kb[0]) && !kb[1]) {
    j = kb[0];
  } else {
    for (sm = specialmap; sm->kj!=0; sm++) {
      if (!strcmp(sm->ks, kb)) {
	j = sm->kj;
      }
    }
  }
  if (j==ERR) return(ERR);
  if (isctrl) {
    j = OWL_CTRL(j);
  }
  if (ismeta) {
    j = OWL_META(j);
  }
  return(j);
}

