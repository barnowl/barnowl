#include <ctype.h>
#include <string.h>
#include "owl.h"


/*
 * TODO: Idea for allowing functions to be user-specified --- 
 *      Have function have a context bitmask that says where it 
 *      can be used, and have keymaps also have one, and compare
 *      the two when setting.
 *      
 */

/* sets up a new keybinding for a command */
int owl_keybinding_init(owl_keybinding *kb, char *keyseq, char *command, void (*function_fn)(void), char *desc) {
  char **ktokens;
  int    nktokens, i;
  
  owl_function_debugmsg("creating binding for <%s> with desc: <%s>\n", keyseq, desc);
  if (command && !function_fn) {
    kb->type = OWL_KEYBINDING_COMMAND;
  } else if (!command && function_fn) {
    kb->type = OWL_KEYBINDING_FUNCTION;
  } else {
    return(-1);
  }

  ktokens = atokenize(keyseq, " ", &nktokens);
  if (!ktokens) return(-1);
  if (nktokens > OWL_KEYMAP_MAXSTACK) {
    atokenize_free(ktokens, nktokens);
    return(-1);
  }
  kb->j = owl_malloc((nktokens+1)*sizeof(int));
  for (i=0; i<nktokens; i++) {
    kb->j[i] = owl_keypress_fromstring(ktokens[i]);
    if (kb->j[i] == ERR) { 
      atokenize_free(ktokens, nktokens);
      owl_free(kb->j);
      return(-1);
    }
  }
  kb->j[i] = 0;

  if (command) kb->command = owl_strdup(command);
  kb->function_fn = function_fn;
  if (desc) kb->desc = owl_strdup(desc);
  else kb->desc = NULL;
  return(0);
}

/* Releases data associated with a keybinding */
void owl_keybinding_free(owl_keybinding *kb) {
  if (kb->j) owl_free(kb->j);
  if (kb->desc) owl_free(kb->desc);
  if (kb->command) owl_free(kb->command);
}

/* Releases data associated with a keybinding, and the kb itself */
void owl_keybinding_free_all(owl_keybinding *kb) {
  owl_keybinding_free(kb);
  owl_free(kb);
}

/* executes a keybinding */
void owl_keybinding_execute(owl_keybinding *kb, int j) {
  if (kb->type == OWL_KEYBINDING_COMMAND && kb->command) {
    owl_function_command_norv(kb->command);
  } else if (kb->type == OWL_KEYBINDING_FUNCTION && kb->function_fn) {
    kb->function_fn();
  }
}

/* returns 0 on success */
int owl_keybinding_stack_tostring(int *j, char *buff, int bufflen) {
  char *pos = buff;
  int   rem = bufflen;
  int   i, n;

  for (i=0; j[i]; i++) {
    owl_keypress_tostring(j[i], 0, pos, rem-1);
    if (j[i+1]) strcat(pos, " ");
    n = strlen(pos);
    pos += n;
    rem -= n;
  }
  return 0;
}

/* returns 0 on success */
int owl_keybinding_tostring(owl_keybinding *kb, char *buff, int bufflen) {
  return owl_keybinding_stack_tostring(kb->j, buff, bufflen);
}

char *owl_keybinding_get_desc(owl_keybinding *kb) {
  return kb->desc;
}

/* returns 0 on no match, 1 on subset match, and 2 on complete match */
int owl_keybinding_match(owl_keybinding *kb, int *kpstack) {
  int *kbstack = kb->j;
  
  while (*kbstack && *kpstack) {
    if (*kbstack != *kpstack) {
      return 0;
    }
    kbstack++; kpstack++;
  }
  if (!*kpstack && !*kbstack) {
    return 2;
  } else if (!*kpstack) {
    return 1;
  } else {
    return 0;
  }
}

/* returns 1 if keypress sequence is the same */
int owl_keybinding_equal(owl_keybinding *kb1, owl_keybinding *kb2) {
  int *j1 = kb1->j;
  int *j2 = kb2->j;
  while (*j1 && *j2) {
    if (*(j1++) != *(j2++))  return(0);
  }
  if (*j1 != *j2) return(0);
  return(1);
}
