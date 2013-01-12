#include "owl.h"

CALLER_OWN owl_zwrite *owl_zwrite_new_from_line(const char *line)
{
  owl_zwrite *z = g_new(owl_zwrite, 1);
  if (owl_zwrite_create_from_line(z, line) != 0) {
    g_free(z);
    return NULL;
  }
  return z;
}

CALLER_OWN owl_zwrite *owl_zwrite_new(int argc, const char *const *argv)
{
  owl_zwrite *z = g_new(owl_zwrite, 1);
  if (owl_zwrite_create(z, argc, argv) != 0) {
    g_free(z);
    return NULL;
  }
  return z;
}

G_GNUC_WARN_UNUSED_RESULT int owl_zwrite_create_from_line(owl_zwrite *z, const char *line)
{
  int argc;
  char **argv;
  int ret;

  /* parse the command line for options */
  argv = owl_parseline(line, &argc);
  if (argc < 0) {
    owl_function_error("Unbalanced quotes in zwrite");
    return -1;
  }
  ret = owl_zwrite_create(z, argc, strs(argv));
  g_strfreev(argv);
  return ret;
}

G_GNUC_WARN_UNUSED_RESULT int owl_zwrite_create(owl_zwrite *z, int argc, const char *const *argv)
{
  int badargs = 0;
  char *msg = NULL;

  /* start with null entries */
  z->cmd=NULL;
  z->realm=NULL;
  z->class=NULL;
  z->inst=NULL;
  z->opcode=NULL;
  z->zsig=NULL;
  z->message=NULL;
  z->cc=0;
  z->noping=0;
  z->recips = g_ptr_array_new();
  z->zwriteline = owl_argv_quote(argc, argv);

  if (argc && *(argv[0])!='-') {
    z->cmd=g_strdup(argv[0]);
    argc--;
    argv++;
  }
  while (argc) {
    if (!strcmp(argv[0], "-c")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      z->class=owl_validate_utf8(argv[1]);
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-i")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      z->inst=owl_validate_utf8(argv[1]);
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-r")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      z->realm=owl_validate_utf8(argv[1]);
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-s")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      z->zsig=owl_validate_utf8(argv[1]);
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-O")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      z->opcode=owl_validate_utf8(argv[1]);
      argv+=2;
      argc-=2;
    } else if (!strcmp(argv[0], "-m")) {
      if (argc<2) {
	badargs=1;
	break;
      }
      /* we must already have users or a class or an instance */
      if (z->recips->len < 1 && (!z->class) && (!z->inst)) {
	badargs=1;
	break;
      }

      /* Once we have -m, gobble up everything else on the line */
      argv++;
      argc--;
      msg = g_strjoinv(" ", (char**)argv);
      break;
    } else if (!strcmp(argv[0], "-C")) {
      z->cc=1;
      argv++;
      argc--;
    } else if (!strcmp(argv[0], "-n")) {
      z->noping=1;
      argv++;
      argc--;
    } else {
      /* anything unattached is a recipient */
      g_ptr_array_add(z->recips, owl_validate_utf8(argv[0]));
      argv++;
      argc--;
    }
  }

  if (badargs) {
    owl_zwrite_cleanup(z);
    return(-1);
  }

  if (z->class == NULL &&
      z->inst == NULL &&
      z->recips->len == 0) {
    owl_function_error("You must specify a recipient for zwrite");
    owl_zwrite_cleanup(z);
    return(-1);
  }

  /* now deal with defaults */
  if (z->class==NULL) z->class=g_strdup("message");
  if (z->inst==NULL) z->inst=g_strdup("personal");
  if (z->realm==NULL) z->realm=g_strdup("");
  if (z->opcode==NULL) z->opcode=g_strdup("");
  /* z->message is allowed to stay NULL */

  if(msg) {
    owl_zwrite_set_message(z, msg);
    g_free(msg);
  }

  return(0);
}

void owl_zwrite_populate_zsig(owl_zwrite *z)
{
  /* get a zsig, if not given */
  if (z->zsig != NULL)
    return;

  z->zsig = owl_perlconfig_execute(owl_global_get_zsigfunc(&g));
}

void owl_zwrite_send_ping(const owl_zwrite *z)
{
  int i;
  char *to;

  if (z->noping) return;

  if (strcasecmp(z->class, "message")) {
    return;
  }

  for (i = 0; i < z->recips->len; i++) {
    to = owl_zwrite_get_recip_n_with_realm(z, i);
    if (owl_zwrite_recip_is_personal(to))
      send_ping(to, z->class, z->inst);
    g_free(to);
  }

}

/* Set the message with no post-processing*/
void owl_zwrite_set_message_raw(owl_zwrite *z, const char *msg)
{
  g_free(z->message);
  z->message = owl_validate_utf8(msg);
}

void owl_zwrite_set_message(owl_zwrite *z, const char *msg)
{
  int i;
  GString *message;
  char *tmp = NULL, *tmp2;

  g_free(z->message);

  if (z->cc && owl_zwrite_is_personal(z)) {
    message = g_string_new("CC: ");
    for (i = 0; i < z->recips->len; i++) {
      tmp = owl_zwrite_get_recip_n_with_realm(z, i);
      g_string_append_printf(message, "%s ", tmp);
      g_free(tmp);
      tmp = NULL;
    }
    tmp = owl_validate_utf8(msg);
    tmp2 = owl_text_expand_tabs(tmp);
    g_string_append_printf(message, "\n%s", tmp2);
    z->message = g_string_free(message, false);
    g_free(tmp);
    g_free(tmp2);
  } else {
    tmp=owl_validate_utf8(msg);
    z->message=owl_text_expand_tabs(tmp);
    g_free(tmp);
  }
}

const char *owl_zwrite_get_message(const owl_zwrite *z)
{
  if (z->message) return(z->message);
  return("");
}

int owl_zwrite_is_message_set(const owl_zwrite *z)
{
  if (z->message) return(1);
  return(0);
}

int owl_zwrite_send_message(const owl_zwrite *z)
{
  int i, ret = 0;
  char *to = NULL;

  if (z->message==NULL) return(-1);

  if (z->recips->len > 0) {
    for (i = 0; i < z->recips->len; i++) {
      to = owl_zwrite_get_recip_n_with_realm(z, i);
      ret = send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
      /* Abort on the first error, to match the zwrite binary. */
      if (ret != 0)
        break;
      g_free(to);
      to = NULL;
    }
  } else {
    to = g_strdup_printf( "@%s", z->realm);
    ret = send_zephyr(z->opcode, z->zsig, z->class, z->inst, to, z->message);
  }
  g_free(to);
  return ret;
}

int owl_zwrite_create_and_send_from_line(const char *cmd, const char *msg)
{
  owl_zwrite z;
  int rv;
  rv = owl_zwrite_create_from_line(&z, cmd);
  if (rv != 0) return rv;
  if (!owl_zwrite_is_message_set(&z)) {
    owl_zwrite_set_message(&z, msg);
  }
  owl_zwrite_populate_zsig(&z);
  owl_zwrite_send_message(&z);
  owl_zwrite_cleanup(&z);
  return(0);
}

const char *owl_zwrite_get_class(const owl_zwrite *z)
{
  return(z->class);
}

const char *owl_zwrite_get_instance(const owl_zwrite *z)
{
  return(z->inst);
}

const char *owl_zwrite_get_opcode(const owl_zwrite *z)
{
  return(z->opcode);
}

void owl_zwrite_set_opcode(owl_zwrite *z, const char *opcode)
{
  g_free(z->opcode);
  z->opcode=owl_validate_utf8(opcode);
}

const char *owl_zwrite_get_realm(const owl_zwrite *z)
{
  return(z->realm);
}

const char *owl_zwrite_get_zsig(const owl_zwrite *z)
{
  if (z->zsig) return(z->zsig);
  return("");
}

void owl_zwrite_set_zsig(owl_zwrite *z, const char *zsig)
{
  g_free(z->zsig);
  z->zsig = g_strdup(zsig);
}

int owl_zwrite_get_numrecips(const owl_zwrite *z)
{
  return z->recips->len;
}

const char *owl_zwrite_get_recip_n(const owl_zwrite *z, int n)
{
  return z->recips->pdata[n];
}

/* Caller must free the result. */
CALLER_OWN char *owl_zwrite_get_recip_n_with_realm(const owl_zwrite *z, int n)
{
  if (z->realm[0]) {
    return g_strdup_printf("%s@%s", owl_zwrite_get_recip_n(z, n), z->realm);
  } else {
    return g_strdup(owl_zwrite_get_recip_n(z, n));
  }
}

bool owl_zwrite_recip_is_personal(const char *recipient)
{
  return recipient[0] && recipient[0] != '@';
}

bool owl_zwrite_is_personal(const owl_zwrite *z)
{
  /* return true if at least one of the recipients is personal */
  int i;

  for (i = 0; i < z->recips->len; i++)
    if (owl_zwrite_recip_is_personal(z->recips->pdata[i]))
      return true;
  return false;
}

void owl_zwrite_delete(owl_zwrite *z)
{
  owl_zwrite_cleanup(z);
  g_free(z);
}

void owl_zwrite_cleanup(owl_zwrite *z)
{
  owl_ptr_array_free(z->recips, g_free);
  g_free(z->cmd);
  g_free(z->zwriteline);
  g_free(z->class);
  g_free(z->inst);
  g_free(z->opcode);
  g_free(z->realm);
  g_free(z->message);
  g_free(z->zsig);
}

/*
 * Returns a zwrite line suitable for replying, specifically the
 * message field is stripped out. Result should be freed with
 * g_free.
 *
 * If not a CC, only the recip_index'th user will be replied to.
 */
CALLER_OWN char *owl_zwrite_get_replyline(const owl_zwrite *z, int recip_index)
{
  /* Match ordering in zwrite help. */
  GString *buf = g_string_new("");
  int i;

  /* Disturbingly, it is apparently possible to z->cmd to be null if
   * owl_zwrite_create_from_line got something starting with -. And we
   * can't kill it because this is exported to perl. */
  owl_string_append_quoted_arg(buf, z->cmd ? z->cmd : "zwrite");
  if (z->noping) {
    g_string_append(buf, " -n");
  }
  if (z->cc) {
    g_string_append(buf, " -C");
  }
  if (strcmp(z->class, "message")) {
    g_string_append(buf, " -c ");
    owl_string_append_quoted_arg(buf, z->class);
  }
  if (strcmp(z->inst, "personal")) {
    g_string_append(buf, " -i ");
    owl_string_append_quoted_arg(buf, z->inst);
  }
  if (z->realm && z->realm[0] != '\0') {
    g_string_append(buf, " -r ");
    owl_string_append_quoted_arg(buf, z->realm);
  }
  if (z->opcode && z->opcode[0] != '\0') {
    g_string_append(buf, " -O ");
    owl_string_append_quoted_arg(buf, z->opcode);
  }
  if (z->cc) {
    for (i = 0; i < z->recips->len; i++) {
      g_string_append_c(buf, ' ');
      owl_string_append_quoted_arg(buf, z->recips->pdata[i]);
    }
  } else if (recip_index < z->recips->len) {
    g_string_append_c(buf, ' ');
    owl_string_append_quoted_arg(buf, z->recips->pdata[recip_index]);
  }

  return g_string_free(buf, false);
}
