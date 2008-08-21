#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

extern owl_cmd commands_to_init[];

/**************************************************************************/
/***************************** COMMAND DICT *******************************/
/**************************************************************************/

int owl_cmddict_setup(owl_cmddict *cd) {
  if (0 != owl_cmddict_init(cd)) return(-1);
  if (0 != owl_cmddict_add_from_list(cd, commands_to_init)) return(-1);
  return(0);
}

int owl_cmddict_init(owl_cmddict *cd) {
  if (owl_dict_create(cd)) return(-1);
  return(0);
}

/* for bulk initialization at startup */
int owl_cmddict_add_from_list(owl_cmddict *cd, owl_cmd *cmds) {
  owl_cmd *cur, *cmd;
  for (cur = cmds; cur->name != NULL; cur++) {  
    cmd = owl_malloc(sizeof(owl_cmd));
    owl_cmd_create_from_template(cmd, cur);
    owl_dict_insert_element(cd, cmd->name, (void*)cmd, NULL);
  }
  return 0;
}

/* free the list with owl_cmddict_namelist_free */
void owl_cmddict_get_names(owl_cmddict *d, owl_list *l) {
  owl_dict_get_keys(d, l);
}

owl_cmd *owl_cmddict_find(owl_cmddict *d, char *name) {
  return (owl_cmd*)owl_dict_find_element(d, name);
}

void owl_cmddict_namelist_free(owl_list *l) {
  owl_list_free_all(l, owl_free);
}

/* creates a new command alias */
int owl_cmddict_add_alias(owl_cmddict *cd, char *alias_from, char *alias_to) {
  owl_cmd *cmd;
  cmd = owl_malloc(sizeof(owl_cmd));
  owl_cmd_create_alias(cmd, alias_from, alias_to);
  owl_dict_insert_element(cd, cmd->name, (void*)cmd, (void(*)(void*))owl_cmd_free);    
  return(0);
}

int owl_cmddict_add_cmd(owl_cmddict *cd, owl_cmd * cmd) {
  owl_cmd * newcmd = owl_malloc(sizeof(owl_cmd));
  if(owl_cmd_create_from_template(newcmd, cmd) < 0) {
    owl_free(newcmd);
    return -1;
  }
  owl_function_debugmsg("Add cmd %s", cmd->name);
  return owl_dict_insert_element(cd, newcmd->name, (void*)newcmd, (void(*)(void*))owl_cmd_free);
}

char *_owl_cmddict_execute(owl_cmddict *cd, owl_context *ctx, char **argv, int argc, char *buff) {
  char *retval = NULL;
  owl_cmd *cmd;

  if (!strcmp(argv[0], "")) {
  } else if (NULL != (cmd = (owl_cmd*)owl_dict_find_element(cd, argv[0]))) {
    retval = owl_cmd_execute(cmd, cd, ctx, argc, argv, buff);
  } else {
    owl_function_makemsg("Unknown command '%s'.", buff);
  }
  return retval;
}

char *owl_cmddict_execute(owl_cmddict *cd, owl_context *ctx, char *cmdbuff) {
  char **argv;
  int argc;
  char *tmpbuff;
  char *retval = NULL;

  tmpbuff=owl_strdup(cmdbuff);
  argv=owl_parseline(tmpbuff, &argc);
  if (argc < 0) {
    owl_free(tmpbuff);
    sepbar(NULL);
    owl_function_makemsg("Unbalanced quotes");
    return NULL;
  } 
  
  if (argc < 1) return(NULL);

  retval = _owl_cmddict_execute(cd, ctx, argv, argc, cmdbuff);

  owl_parsefree(argv, argc);
  owl_free(tmpbuff);
  sepbar(NULL);
  return retval;
}

char *owl_cmddict_execute_argv(owl_cmddict *cd, owl_context *ctx, char **argv, int argc) {
  char *buff, *ptr;
  int len = 0, i;
  char *retval = NULL;

  for(i = 0; i < argc; i++) {
    len += strlen(argv[i]) + 1;
  }

  ptr = buff = owl_malloc(len);

  for(i = 0; i < argc; i++) {
    strcpy(ptr, argv[i]);
    ptr += strlen(argv[i]);
    *(ptr++) = ' ';
  }
  *(ptr - 1) = 0;

  retval = _owl_cmddict_execute(cd, ctx, argv, argc, buff);

  owl_free(buff);
  return retval;
}

/*********************************************************************/
/***************************** COMMAND *******************************/
/*********************************************************************/

/* sets up a new command based on a template, copying strings */
int owl_cmd_create_from_template(owl_cmd *cmd, owl_cmd *templ) {
  *cmd = *templ;
  if (!templ->name) return(-1);
  cmd->name = owl_strdup(templ->name);
  if (templ->summary)     cmd->summary     = owl_strdup(templ->summary);
  if (templ->usage)       cmd->usage       = owl_strdup(templ->usage);
  if (templ->description) cmd->description = owl_strdup(templ->description);
  if (templ->cmd_aliased_to) cmd->cmd_aliased_to = owl_strdup(templ->cmd_aliased_to);
  return(0);
}

int owl_cmd_create_alias(owl_cmd *cmd, char *name, char *aliased_to) {
  memset(cmd, 0, sizeof(owl_cmd));
  cmd->name = owl_strdup(name);
  cmd->cmd_aliased_to = owl_strdup(aliased_to);
  cmd->summary = owl_malloc(strlen(aliased_to)+strlen(OWL_CMD_ALIAS_SUMMARY_PREFIX)+2);
  strcpy(cmd->summary, OWL_CMD_ALIAS_SUMMARY_PREFIX);
  strcat(cmd->summary, aliased_to);
  return(0);
}

void owl_cmd_free(owl_cmd *cmd) {
  if (cmd->name) owl_free(cmd->name);
  if (cmd->summary) owl_free(cmd->summary);
  if (cmd->usage) owl_free(cmd->usage);
  if (cmd->description) owl_free(cmd->description);
  if (cmd->cmd_aliased_to) owl_free(cmd->cmd_aliased_to);
  if (cmd->cmd_perl) owl_perlconfig_cmd_free(cmd);
}

int owl_cmd_is_context_valid(owl_cmd *cmd, owl_context *ctx) { 
  if (owl_context_matches(ctx, cmd->validctx)) return 1;
  else return 0;
}

char *owl_cmd_execute(owl_cmd *cmd, owl_cmddict *cd, owl_context *ctx, int argc, char **argv, char *cmdbuff) {
  static int alias_recurse_depth = 0;
  int ival=0;
  char *cmdbuffargs, *newcmd, *rv=NULL;

  if (argc < 1) return(NULL);

  /* Recurse if this is an alias */
  if (cmd->cmd_aliased_to) {
    if (alias_recurse_depth++ > 50) {
      owl_function_makemsg("Alias loop detected for '%s'.", cmdbuff);
    } else {
      cmdbuffargs = skiptokens(cmdbuff, 1);
      newcmd = owl_malloc(strlen(cmd->cmd_aliased_to)+strlen(cmdbuffargs)+2);
      strcpy(newcmd, cmd->cmd_aliased_to);
      strcat(newcmd, " ");
      strcat(newcmd, cmdbuffargs);
      rv = owl_function_command(newcmd);
      owl_free(newcmd);
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
      char *ep = "x";
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
char *owl_cmd_get_summary(owl_cmd *cmd) {
  return cmd->summary;
}

/* returns a summary line describing this keymap.  the caller must free. */
char *owl_cmd_describe(owl_cmd *cmd) {
  char *s;
  int slen;
  if (!cmd || !cmd->name || !cmd->summary) return NULL;
  slen = strlen(cmd->name)+strlen(cmd->summary)+30;
  s = owl_malloc(slen);
  snprintf(s, slen-1, "%-25s - %s", cmd->name, cmd->summary);
  return s;
}



void owl_cmd_get_help(owl_cmddict *d, char *name, owl_fmtext *fm) {
  char *indent, *s;
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
    indent = owl_malloc(strlen(s)+(owl_text_num_lines(s)+3)*OWL_TAB+1);
    owl_text_indent(indent, s, OWL_TAB);
    owl_fmtext_append_bold(fm, "\nSYNOPSIS\n");
    owl_fmtext_append_normal(fm, indent);
    owl_fmtext_append_normal(fm, "\n");
    owl_free(indent);
  } else {
    owl_fmtext_append_bold(fm, "\nSYNOPSIS\n");
    owl_fmtext_append_normal(fm, OWL_TABSTR);
    owl_fmtext_append_normal(fm, cmd->name);
    owl_fmtext_append_normal(fm, "\n");
  }

  if (cmd->description && *cmd->description) {
    s = cmd->description;
    indent = owl_malloc(strlen(s)+(owl_text_num_lines(s)+3)*OWL_TAB+1);
    owl_text_indent(indent, s, OWL_TAB);
    owl_fmtext_append_bold(fm, "\nDESCRIPTION\n");
    owl_fmtext_append_normal(fm, indent);
    owl_fmtext_append_normal(fm, "\n");
    owl_free(indent);
  }

  owl_fmtext_append_normal(fm, "\n\n");  
}
