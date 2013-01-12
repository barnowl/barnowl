#include "owl.h"

/*
 * TODO: Idea for allowing functions to be user-specified ---
 *      Have function have a context bitmask that says where it
 *      can be used, and have keymaps also have one, and compare
 *      the two when setting.
 *
 */

static int owl_keybinding_make_keys(owl_keybinding *kb, const char *keyseq);

/* sets up a new keybinding for a command */
CALLER_OWN owl_keybinding *owl_keybinding_new(const char *keyseq, const char *command, void (*function_fn)(void), const char *desc)
{
  owl_keybinding *kb = g_new(owl_keybinding, 1);

  owl_function_debugmsg("owl_keybinding_init: creating binding for <%s> with desc: <%s>", keyseq, desc);
  if (command && function_fn) {
    g_free(kb);
    return NULL;
  } else if (command && !function_fn) {
    kb->type = OWL_KEYBINDING_COMMAND;
  } else if (!command && function_fn) {
    kb->type = OWL_KEYBINDING_FUNCTION;
  } else {
    kb->type = OWL_KEYBINDING_NOOP;
  }

  if (owl_keybinding_make_keys(kb, keyseq) != 0) {
    g_free(kb);
    return NULL;
  }

  kb->command = g_strdup(command);
  kb->function_fn = function_fn;
  kb->desc = g_strdup(desc);
  return kb;
}

static int owl_keybinding_make_keys(owl_keybinding *kb, const char *keyseq)
{
  char **ktokens;
  int    nktokens, i;

  ktokens = g_strsplit_set(keyseq, " ", 0);
  nktokens = g_strv_length(ktokens);
  if (nktokens < 1 || nktokens > OWL_KEYMAP_MAXSTACK) {
    g_strfreev(ktokens);
    return(-1);
  }
  kb->keys = g_new(int, nktokens);
  for (i=0; i<nktokens; i++) {
    kb->keys[i] = owl_keypress_fromstring(ktokens[i]);
    if (kb->keys[i] == ERR) {
      g_strfreev(ktokens);
      g_free(kb->keys);
      return(-1);
    }
  }
  kb->len = nktokens;
  g_strfreev(ktokens);
  return(0);
}

/* Releases data associated with a keybinding, and the kb itself */
void owl_keybinding_delete(owl_keybinding *kb)
{
  g_free(kb->keys);
  g_free(kb->desc);
  g_free(kb->command);
  g_free(kb);
}

/* executes a keybinding */
void owl_keybinding_execute(const owl_keybinding *kb, int j)
{
  if (kb->type == OWL_KEYBINDING_COMMAND && kb->command) {
    owl_function_command_norv(kb->command);
  } else if (kb->type == OWL_KEYBINDING_FUNCTION && kb->function_fn) {
    kb->function_fn();
  }
}

CALLER_OWN char *owl_keybinding_stack_tostring(int *j, int len)
{
  GString *string;
  int  i;

  string = g_string_new("");
  for (i = 0; i < len; i++) {
    char *keypress = owl_keypress_tostring(j[i], 0);
    g_string_append(string, keypress ? keypress : "INVALID");
    g_free(keypress);
    if (i < len - 1) g_string_append_c(string, ' ');
  }
  return g_string_free(string, false);
}

CALLER_OWN char *owl_keybinding_tostring(const owl_keybinding *kb)
{
  return owl_keybinding_stack_tostring(kb->keys, kb->len);
}

const char *owl_keybinding_get_desc(const owl_keybinding *kb)
{
  return kb->desc;
}

/* returns 0 on no match, 1 on subset match, and 2 on complete match */
int owl_keybinding_match(const owl_keybinding *kb, const owl_keyhandler *kh)
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
int owl_keybinding_equal(const owl_keybinding *kb1, const owl_keybinding *kb2)
{
  int i;

  if(kb1->len != kb2->len) return 0;

  for(i = 0; i < kb1->len; i++) {
    if(kb1->keys[i] != kb2->keys[i])
      return 0;
  }

  return 1;
}
