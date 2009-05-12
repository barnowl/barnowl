#include <ctype.h>
#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

/*
 * TODO: Idea for allowing functions to be user-specified --- 
 *      Have function have a context bitmask that says where it 
 *      can be used, and have keymaps also have one, and compare
 *      the two when setting.
 *      
 */

/* sets up a new keybinding for a command */
int owl_keybinding_init(owl_keybinding *kb, char *keyseq, char *command, void (*function_fn)(void), char *desc)
{
  char **ktokens;
  int    nktokens, i;
  
  owl_function_debugmsg("owl_keybinding_init: creating binding for <%s> with desc: <%s>", keyseq, desc);
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
  kb->keys = owl_malloc(nktokens*sizeof(int));
  for (i=0; i<nktokens; i++) {
    kb->keys[i] = owl_keypress_fromstring(ktokens[i]);
    if (kb->keys[i] == ERR) { 
      atokenize_free(ktokens, nktokens);
      owl_free(kb->keys);
      return(-1);
    }
  }
  kb->len = nktokens;

  atokenize_free(ktokens, nktokens);

  if (command) kb->command = owl_strdup(command);
  kb->function_fn = function_fn;
  if (desc) kb->desc = owl_strdup(desc);
  else kb->desc = NULL;
  return(0);
}

/* Releases data associated with a keybinding */
void owl_keybinding_free(owl_keybinding *kb)
{
  if (kb->keys) owl_free(kb->keys);
  if (kb->desc) owl_free(kb->desc);
  if (kb->command) owl_free(kb->command);
}

/* Releases data associated with a keybinding, and the kb itself */
void owl_keybinding_free_all(owl_keybinding *kb)
{
  owl_keybinding_free(kb);
  owl_free(kb);
}

/* executes a keybinding */
void owl_keybinding_execute(owl_keybinding *kb, int j)
{
  if (kb->type == OWL_KEYBINDING_COMMAND && kb->command) {
    owl_function_command_norv(kb->command);
  } else if (kb->type == OWL_KEYBINDING_FUNCTION && kb->function_fn) {
    kb->function_fn();
  }
}

/* returns 0 on success */
int owl_keybinding_stack_tostring(int *j, int len, char *buff, int bufflen)
{
  char *pos = buff;
  int   rem = bufflen;
  int   i, n;

  for (i=0; i < len; i++) {
    owl_keypress_tostring(j[i], 0, pos, rem-1);
    if (i < len - 1) strcat(pos, " ");
    n = strlen(pos);
    pos += n;
    rem -= n;
  }
  return 0;
}

/* returns 0 on success */
int owl_keybinding_tostring(owl_keybinding *kb, char *buff, int bufflen)
{
  return owl_keybinding_stack_tostring(kb->keys, kb->len, buff, bufflen);
}

char *owl_keybinding_get_desc(owl_keybinding *kb)
{
  return kb->desc;
}

/* returns 0 on no match, 1 on subset match, and 2 on complete match */
int owl_keybinding_match(owl_keybinding *kb, owl_keyhandler *kh)
{
  int i;
  for(i = 0; i <= kh->kpstackpos && i < kb->len; i++) {
    if(kb->keys[i] != kh->kpstack[i])
      return 0;
  }

  /* If we've made it to this point, then they match as far as they are. */
  if(kb->len == kh->kpstackpos + 1) {
    /* Equal length */
    return 2;
  } else if(kb->len > kh->kpstackpos + 1) {
    return 1;
  }

  return 0;
}

/* returns 1 if keypress sequence is the same */
int owl_keybinding_equal(owl_keybinding *kb1, owl_keybinding *kb2)
{
  int i;

  if(kb1->len != kb2->len) return 0;

  for(i = 0; i < kb1->len; i++) {
    if(kb1->keys[i] != kb2->keys[i])
      return 0;
  }

  return 1;
}
