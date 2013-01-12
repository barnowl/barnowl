#include "owl.h"

/**************************************************************************/
/***************************** COMMAND DICT *******************************/
/**************************************************************************/

void owl_cmddict_setup(owl_cmddict *cd)
{
  owl_cmddict_init(cd);
  owl_cmd_add_defaults(cd);
}

void owl_cmddict_init(owl_cmddict *cd) {
  owl_dict_create(cd);
}

/* for bulk initialization at startup */
void owl_cmddict_add_from_list(owl_cmddict *cd, const owl_cmd *cmds)
{
  const owl_cmd *cur;
  for (cur = cmds; cur->name != NULL; cur++) {
    owl_cmddict_add_cmd(cd, cur);
  }
}

GPtrArray *owl_cmddict_get_names(const owl_cmddict *d) {
  return owl_dict_get_keys(d);
}

const owl_cmd *owl_cmddict_find(const owl_cmddict *d, const char *name) {
  return owl_dict_find_element(d, name);
}

/* creates a new command alias */
void owl_cmddict_add_alias(owl_cmddict *cd, const char *alias_from, const char *alias_to) {
  owl_cmd *cmd;
  cmd = g_new(owl_cmd, 1);
  owl_cmd_create_alias(cmd, alias_from, alias_to);
  owl_perlconfig_new_command(cmd->name);
  owl_dict_insert_element(cd, cmd->name, cmd, (void (*)(void *))owl_cmd_delete);
}

int owl_cmddict_add_cmd(owl_cmddict *cd, const owl_cmd * cmd) {
  owl_cmd * newcmd = g_new(owl_cmd, 1);
  if(owl_cmd_create_from_template(newcmd, cmd) < 0) {
    g_free(newcmd);
    return -1;
  }
  owl_perlconfig_new_command(cmd->name);
  return owl_dict_insert_element(cd, newcmd->name, newcmd, (void (*)(void *))owl_cmd_delete);
}

/* caller must free the return */
CALLER_OWN char *_owl_cmddict_execute(const owl_cmddict *cd, const owl_context *ctx, const char *const *argv, int argc, const char *buff)
{
  char *retval = NULL;
  const owl_cmd *cmd;

  if (!strcmp(argv[0], "")) {
  } else if (NULL != (cmd = owl_dict_find_element(cd, argv[0]))) {
    retval = owl_cmd_execute(cmd, cd, ctx, argc, argv, buff);
    /* redraw the sepbar; TODO: don't violate layering */
    owl_global_sepbar_dirty(&g);
  } else {
    owl_function_makemsg("Unknown command '%s'.", buff);
  }
  return retval;
}

/* caller must free the return */
CALLER_OWN char *owl_cmddict_execute(const owl_cmddict *cd, const owl_context *ctx, const char *cmdbuff)
{
  char **argv;
  int argc;
  char *retval = NULL;

  argv = owl_parseline(cmdbuff, &argc);
  if (argv == NULL) {
    owl_function_makemsg("Unbalanced quotes");
    return NULL;
  }

  if (argc < 1) {
    g_strfreev(argv);
    return NULL;
  }

  retval = _owl_cmddict_execute(cd, ctx, strs(argv), argc, cmdbuff);

  g_strfreev(argv);
  return retval;
}

/* caller must free the return */
CALLER_OWN char *owl_cmddict_execute_argv(const owl_cmddict *cd, const owl_context *ctx, const char *const *argv, int argc)
{
  char *buff;
  char *retval = NULL;

  buff = g_strjoinv(" ", (char**)argv);
  retval = _owl_cmddict_execute(cd, ctx, argv, argc, buff);
  g_free(buff);

  return retval;
}

/*********************************************************************/
/***************************** COMMAND *******************************/
/*********************************************************************/

/* sets up a new command based on a template, copying strings */
int owl_cmd_create_from_template(owl_cmd *cmd, const owl_cmd *templ) {
  *cmd = *templ;
  if (!templ->name) return(-1);
  cmd->name = g_strdup(templ->name);
  if (templ->summary)     cmd->summary     = g_strdup(templ->summary);
  if (templ->usage)       cmd->usage       = g_strdup(templ->usage);
  if (templ->description) cmd->description = g_strdup(templ->description);
  if (templ->cmd_aliased_to) cmd->cmd_aliased_to = g_strdup(templ->cmd_aliased_to);
  return(0);
}

void owl_cmd_create_alias(owl_cmd *cmd, const char *name, const char *aliased_to) {
  memset(cmd, 0, sizeof(owl_cmd));
  cmd->name = g_strdup(name);
  cmd->cmd_aliased_to = g_strdup(aliased_to);
  cmd->summary = g_strdup_printf("%s%s", OWL_CMD_ALIAS_SUMMARY_PREFIX, aliased_to);
}

void owl_cmd_cleanup(owl_cmd *cmd)
{
  g_free(cmd->name);
  g_free(cmd->summary);
  g_free(cmd->usage);
  g_free(cmd->description);
  g_free(cmd->cmd_aliased_to);
  if (cmd->cmd_perl) owl_perlconfig_cmd_cleanup(cmd);
}

void owl_cmd_delete(owl_cmd *cmd)
{
  owl_cmd_cleanup(cmd);
  g_free(cmd);
}

int owl_cmd_is_context_valid(const owl_cmd *cmd, const owl_context *ctx) {
  if (owl_context_matches(ctx, cmd->validctx)) return 1;
  else return 0;
}

/* caller must free the result */
CALLER_OWN char *owl_cmd_execute(const owl_cmd *cmd, const owl_cmddict *cd, const owl_context *ctx, int argc, const char *const *argv, const char *cmdbuff)
{
  static int alias_recurse_depth = 0;
  int ival=0;
  const char *cmdbuffargs;
  char *newcmd, *rv=NULL;

  if (argc < 1) return(NULL);

  /* Recurse if this is an alias */
  if (cmd->cmd_aliased_to) {
    if (alias_recurse_depth++ > 50) {
      owl_function_makemsg("Alias loop detected for '%s'.", cmdbuff);
    } else {
      cmdbuffargs = skiptokens(cmdbuff, 1);
      newcmd = g_strdup_printf("%s %s", cmd->cmd_aliased_to, cmdbuffargs);
      rv = owl_function_command(newcmd);
      g_free(newcmd);
    }
    alias_recurse_depth--;
    return rv;
  }

  /* Do validation and conversions */
  if (cmd->cmd_ctxargs_fn || cmd->cmd_ctxv_fn || cmd->cmd_ctxi_fn) {
    if (!owl_cmd_is_context_valid(cmd, ctx)) {
      owl_function_makemsg("Invalid context for command '%s'.", cmdbuff);
      return NULL;
    }
  }

  if ((argc != 1) && (cmd->cmd_v_fn || cmd->cmd_ctxv_fn)) {
    owl_function_makemsg("Wrong number of arguments for %s command.", argv[0]);
    return NULL;
  }

  if (cmd->cmd_i_fn || cmd->cmd_ctxi_fn) {
      char *ep;
      if (argc != 2) {
	owl_function_makemsg("Wrong number of arguments for %s command.", argv[0]);
	return NULL;
      }
      ival = strtol(argv[1], &ep, 10);
      if (*ep || ep==argv[1]) {
	owl_function_makemsg("Invalid argument '%s' for %s command.", argv[1], argv[0]);
	return(NULL);
      }
  }

  if (cmd->cmd_args_fn) {
    return cmd->cmd_args_fn(argc, argv, cmdbuff);
  } else if (cmd->cmd_v_fn) {
    cmd->cmd_v_fn();
  } else if (cmd->cmd_i_fn) {
    cmd->cmd_i_fn(ival);
  } else if (cmd->cmd_ctxargs_fn) {
    return cmd->cmd_ctxargs_fn(owl_context_get_data(ctx), argc, argv, cmdbuff);
  } else if (cmd->cmd_ctxv_fn) {
    cmd->cmd_ctxv_fn(owl_context_get_data(ctx));
  } else if (cmd->cmd_ctxi_fn) {
    cmd->cmd_ctxi_fn(owl_context_get_data(ctx), ival);
  } else if (cmd->cmd_perl) {
    return owl_perlconfig_perlcmd(cmd, argc, argv);
  }

  return NULL;
}

/* returns a reference */
const char *owl_cmd_get_summary(const owl_cmd *cmd) {
  return cmd->summary;
}

/* returns a summary line describing this keymap.  the caller must free. */
CALLER_OWN char *owl_cmd_describe(const owl_cmd *cmd)
{
  if (!cmd || !cmd->name || !cmd->summary) return NULL;
  return g_strdup_printf("%-25s - %s", cmd->name, cmd->summary);
}



void owl_cmd_get_help(const owl_cmddict *d, const char *name, owl_fmtext *fm) {
  const char *s;
  char *indent;
  owl_cmd *cmd;

  if (!name || (cmd = owl_dict_find_element(d, name)) == NULL) {
    owl_fmtext_append_bold(fm, "OWL HELP\n\n");
    owl_fmtext_append_normal(fm, "No such command...\n");
    return;
  }

  owl_fmtext_append_bold(fm, "OWL HELP\n\n");
  owl_fmtext_append_bold(fm, "NAME\n\n");
  owl_fmtext_append_normal(fm, OWL_TABSTR);
  owl_fmtext_append_normal(fm, cmd->name);

  if (cmd->summary && *cmd->summary) {
    owl_fmtext_append_normal(fm, " - ");
    owl_fmtext_append_normal(fm, cmd->summary);
  }
  owl_fmtext_append_normal(fm, "\n");

  if (cmd->usage && *cmd->usage) {
    s = cmd->usage;
    indent = owl_text_indent(s, OWL_TAB, true);
    owl_fmtext_append_bold(fm, "\nSYNOPSIS\n");
    owl_fmtext_append_normal(fm, indent);
    owl_fmtext_append_normal(fm, "\n");
    g_free(indent);
  } else {
    owl_fmtext_append_bold(fm, "\nSYNOPSIS\n");
    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_normal(fm, cmd->name);
    owl_fmtext_append_normal(fm, "\n");
  }

  if (cmd->description && *cmd->description) {
    s = cmd->description;
    indent = owl_text_indent(s, OWL_TAB, true);
    owl_fmtext_append_bold(fm, "\nDESCRIPTION\n");
    owl_fmtext_append_normal(fm, indent);
    owl_fmtext_append_normal(fm, "\n");
    g_free(indent);
  }

  owl_fmtext_append_normal(fm, "\n\n");
}
