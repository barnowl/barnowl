#include <string.h>
#include "owl.h"

owl_filter *owl_filter_new_fromstring(const char *name, const char *string)
{
  owl_filter *f;
  char **argv;
  int argc;

  argv = owl_parseline(string, &argc);
  f = owl_filter_new(name, argc, strs(argv));
  owl_parse_delete(argv, argc);

  return f;
}

owl_filter *owl_filter_new(const char *name, int argc, const char *const *argv)
{
  owl_filter *f;

  f = owl_malloc(sizeof(owl_filter));

  f->name=owl_strdup(name);
  f->fgcolor=OWL_COLOR_DEFAULT;
  f->bgcolor=OWL_COLOR_DEFAULT;

  /* first take arguments that have to come first */
  /* set the color */
  while ( argc>=2 && ( !strcmp(argv[0], "-c") ||
		       !strcmp(argv[0], "-b") ) ) {
    if (owl_util_string_to_color(argv[1])==OWL_COLOR_INVALID) {
      owl_function_error("The color '%s' is not available, using default.", argv[1]);
    } else {
      switch (argv[0][1]) {
      case 'c':
	f->fgcolor=owl_util_string_to_color(argv[1]);
	break;
      case 'b':
	f->bgcolor=owl_util_string_to_color(argv[1]);
	break;
      }
    }
    argc-=2;
    argv+=2;
  }

  if (!(f->root = owl_filter_parse_expression(argc, argv, NULL))) {
    owl_filter_delete(f);
    return NULL;
  }

  /* Now check for recursion. */
  if (owl_filter_is_toodeep(f)) {
    owl_function_error("Filter loop!");
    owl_filter_delete(f);
    return NULL;
  }

  return f;
}


/* A primitive expression is one without any toplevel ``and'' or ``or''s*/

static owl_filterelement * owl_filter_parse_primitive_expression(int argc, const char *const *argv, int *next)
{
  owl_filterelement *fe, *op;
  int i = 0, skip;

  if(!argc) return NULL;

  fe = owl_malloc(sizeof(owl_filterelement));
  owl_filterelement_create(fe);

  if(!strcasecmp(argv[i], "(")) {
    i++;
    op = owl_filter_parse_expression(argc-i, argv+i, &skip);
    if(!op) goto err;
    i += skip;
    if(i >= argc) goto err;
    if(strcasecmp(argv[i++], ")")) goto err;
    owl_filterelement_create_group(fe, op);
  } else if(!strcasecmp(argv[i], "not")) {
    i++;
    op = owl_filter_parse_primitive_expression(argc-i, argv+i, &skip);
    if(!op) goto err;
    i += skip;
    owl_filterelement_create_not(fe, op);
  } else if(!strcasecmp(argv[i], "true")) {
    i++;
    owl_filterelement_create_true(fe);
  } else if(!strcasecmp(argv[i], "false")) {
    i++;
    owl_filterelement_create_false(fe);
  } else {
    if(argc == 1) goto err;
    if(!strcasecmp(*argv, "filter")) {
      owl_filterelement_create_filter(fe, *(argv+1));
    } else if(!strcasecmp(*argv, "perl")) {
      owl_filterelement_create_perl(fe, *(argv+1));
    } else {
      if(owl_filterelement_create_re(fe, *argv, *(argv+1))) {
        goto err;
      }
    }
    i += 2;
  }

  if(next) {
    *next = i;
  } else if(i != argc) {
    goto err;
  }
  return fe;
err:
  owl_filterelement_cleanup(fe);
  owl_free(fe);
  return NULL;
}

owl_filterelement * owl_filter_parse_expression(int argc, const char *const *argv, int *next)
{
  int i = 0, skip;
  owl_filterelement * op1 = NULL, * op2 = NULL, *tmp;

  op1 = owl_filter_parse_primitive_expression(argc-i, argv+i, &skip);
  i += skip;
  if(!op1) goto err;

  while(i < argc) {
    if(strcasecmp(argv[i], "and") &&
       strcasecmp(argv[i], "or")) break;
    op2 = owl_filter_parse_primitive_expression(argc-i-1, argv+i+1, &skip);
    if(!op2) goto err;
    tmp = owl_malloc(sizeof(owl_filterelement));
    if(!strcasecmp(argv[i], "and")) {
      owl_filterelement_create_and(tmp, op1, op2);
    } else {
      owl_filterelement_create_or(tmp, op1, op2);
    }
    op1 = tmp;
    op2 = NULL;
    i += skip+1;
  }

  if(next) {
    *next = i;
  } else if(i != argc) {
    goto err;
  }
  return op1;
err:
  if(op1) {
    owl_filterelement_cleanup(op1);
    owl_free(op1);
  }
  return NULL;
}

const char *owl_filter_get_name(const owl_filter *f)
{
  return(f->name);
}

SV *owl_filter_to_sv(const owl_filter *f)
{
  return owl_new_sv(owl_filter_get_name(f));
}

void owl_filter_set_fgcolor(owl_filter *f, int color)
{
  f->fgcolor=color;
}

int owl_filter_get_fgcolor(const owl_filter *f)
{
  return(f->fgcolor);
}

void owl_filter_set_bgcolor(owl_filter *f, int color)
{
  f->bgcolor=color;
}

int owl_filter_get_bgcolor(const owl_filter *f)
{
  return(f->bgcolor);
}

/* return 1 if the message matches the given filter, otherwise
 * return 0.
 */
int owl_filter_message_match(const owl_filter *f, const owl_message *m)
{
  int ret;
  if(!f->root) return 0;
  ret = owl_filterelement_match(f->root, m);
  return ret;
}


char* owl_filter_print(const owl_filter *f)
{
  GString *out = g_string_new("");

  if (f->fgcolor!=OWL_COLOR_DEFAULT) {
    g_string_append(out, "-c ");
    if (f->fgcolor < 8) {
      g_string_append(out, owl_util_color_to_string(f->fgcolor));
    }
    else {
      g_string_append_printf(out, "%i",f->fgcolor);
    }
    g_string_append(out, " ");
  }
  if (f->bgcolor!=OWL_COLOR_DEFAULT) {
    g_string_append(out, "-b ");
    if (f->bgcolor < 8) {
      g_string_append(out, owl_util_color_to_string(f->bgcolor));
    }
    else {
      g_string_append_printf(out, "%i",f->fgcolor);
    }
    g_string_append(out, " ");
  }
  if(f->root) {
    owl_filterelement_print(f->root, out);
    g_string_append(out, "\n");
  }

  return g_string_free(out, 0);
}

/* Return 1 if the filters 'a' and 'b' are equivalent, 0 otherwise */
int owl_filter_equiv(const owl_filter *a, const owl_filter *b)
{
  char *buffa, *buffb;
  int ret;

  buffa = owl_filter_print(a);
  buffb = owl_filter_print(b);

  ret = !strcmp(buffa, buffb);
  ret = ret && !strcmp(owl_filter_get_name(a),
                       owl_filter_get_name(b));

  owl_free(buffa);
  owl_free(buffb);

  return ret;
}


int owl_filter_is_toodeep(const owl_filter *f)
{
  return owl_filterelement_is_toodeep(f, f->root);
}

void owl_filter_delete(owl_filter *f)
{
  if (f == NULL)
    return;
  if (f->root) {
    owl_filterelement_cleanup(f->root);
    owl_free(f->root);
  }
  if (f->name)
    owl_free(f->name);
  owl_free(f);
}
