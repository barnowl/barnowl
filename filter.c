#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

int owl_filter_init_fromstring(owl_filter *f, char *name, char *string)
{
  char **argv;
  int argc, out;

  argv=owl_parseline(string, &argc);
  out=owl_filter_init(f, name, argc, argv);
  owl_parsefree(argv, argc);
  return(out);
}

int owl_filter_init(owl_filter *f, char *name, int argc, char **argv)
{
  f->name=owl_strdup(name);
  f->fgcolor=OWL_COLOR_DEFAULT;
  f->bgcolor=OWL_COLOR_DEFAULT;
  f->cachedmsgid=-1;

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

  if(!(f->root = owl_filter_parse_expression(argc, argv, NULL)))
    return(-1);

  /* Now check for recursion. */
  if (owl_filter_is_toodeep(f)) {
    owl_function_error("Filter loop!");
    owl_filter_free(f);
    return(-1);
  }

  return(0);
}


/* A primitive expression is one without any toplevel ``and'' or ``or''s*/

static owl_filterelement * owl_filter_parse_primitive_expression(int argc, char **argv, int *next)
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
      owl_filterelement_create_re(fe, *argv, *(argv+1));
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
  owl_filterelement_free(fe);
  owl_free(fe);
  return NULL;
}

owl_filterelement * owl_filter_parse_expression(int argc, char **argv, int *next)
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
    owl_filterelement_free(op1);
    owl_free(op1);
  }
  return NULL;
}

char *owl_filter_get_name(owl_filter *f)
{
  return(f->name);
}

void owl_filter_set_fgcolor(owl_filter *f, int color)
{
  f->fgcolor=color;
}

int owl_filter_get_fgcolor(owl_filter *f)
{
  return(f->fgcolor);
}

void owl_filter_set_bgcolor(owl_filter *f, int color)
{
  f->bgcolor=color;
}

int owl_filter_get_bgcolor(owl_filter *f)
{
  return(f->bgcolor);
}

void owl_filter_set_cachedmsgid(owl_filter *f, int cachedmsgid)
{
  f->cachedmsgid=cachedmsgid;
}

int owl_filter_get_cachedmsgid(owl_filter *f)
{
  return(f->cachedmsgid);
}

/* return 1 if the message matches the given filter, otherwise
 * return 0.
 */
int owl_filter_message_match(owl_filter *f, owl_message *m)
{
  int ret;
  if(!f->root) return 0;
  ret = owl_filterelement_match(f->root, m);
  return ret;
}


char* owl_filter_print(owl_filter *f)
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
int owl_filter_equiv(owl_filter *a, owl_filter *b)
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


int owl_filter_is_toodeep(owl_filter *f)
{
  return owl_filterelement_is_toodeep(f, f->root);
}

void owl_filter_free(owl_filter *f)
{
  if(f->root) {
    owl_filterelement_free(f->root);
    owl_free(f->root);
  }
  if (f->name) owl_free(f->name);
}

/**************************************************************************/
/************************* REGRESSION TESTS *******************************/
/**************************************************************************/

#ifdef OWL_INCLUDE_REG_TESTS

int owl_filter_test_string(char * filt, owl_message *m, int shouldmatch) /* noproto */ {
  owl_filter f;
  int ok;
  int failed = 0;
  if(owl_filter_init_fromstring(&f, "test-filter", filt)) {
    printf("not ok can't parse %s\n", filt);
    failed = 1;
    goto out;
  }
  ok = owl_filter_message_match(&f, m);
  if((shouldmatch && !ok) || (!shouldmatch && ok)) {
    printf("not ok match %s (got %d, expected %d)\n", filt, ok, shouldmatch);
    failed = 1;
  }
 out:
  owl_filter_free(&f);
  if(!failed) {
    printf("ok %s %s\n", shouldmatch ? "matches" : "doesn't match", filt);
  }
  return failed;
}


#include "test.h"


int owl_filter_regtest(void) {
  int numfailed=0;
  owl_message m;
  owl_filter f1, f2, f3, f4, f5;

  owl_list_create(&(g.filterlist));
  owl_message_init(&m);
  owl_message_set_type_zephyr(&m);
  owl_message_set_direction_in(&m);
  owl_message_set_class(&m, "owl");
  owl_message_set_instance(&m, "tester");
  owl_message_set_sender(&m, "owl-user");
  owl_message_set_recipient(&m, "joe");
  owl_message_set_attribute(&m, "foo", "bar");

#define TEST_FILTER(f, e) numfailed += owl_filter_test_string(f, &m, e)


  TEST_FILTER("true", 1);
  TEST_FILTER("false", 0);
  TEST_FILTER("( true )", 1);
  TEST_FILTER("not false", 1);
  TEST_FILTER("( true ) or ( false )", 1);
  TEST_FILTER("true and false", 0);
  TEST_FILTER("( true or true ) or ( ( false ) )", 1);

  TEST_FILTER("class owl", 1);
  TEST_FILTER("class ^owl$", 1);
  TEST_FILTER("instance test", 1);
  TEST_FILTER("instance ^test$", 0);
  TEST_FILTER("instance ^tester$", 1);

  TEST_FILTER("foo bar", 1);
  TEST_FILTER("class owl and instance tester", 1);
  TEST_FILTER("type ^zephyr$ and direction ^in$ and ( class ^owl$ or instance ^owl$ )", 1);

  /* Order of operations and precedence */
  TEST_FILTER("not true or false", 0);
  TEST_FILTER("true or true and false", 0);
  TEST_FILTER("true and true and false or true", 1);
  TEST_FILTER("false and false or true", 1);
  TEST_FILTER("true and false or false", 0);

  owl_filter_init_fromstring(&f1, "f1", "class owl");
  owl_global_add_filter(&g, &f1);
  TEST_FILTER("filter f1", 1);

  /* Test recursion prevention */
  FAIL_UNLESS("self reference", owl_filter_init_fromstring(&f2, "test", "filter test"));

  /* mutual recursion */
  owl_filter_init_fromstring(&f3, "f3", "filter f4");
  owl_global_add_filter(&g, &f3);
  FAIL_UNLESS("mutual recursion",   owl_filter_init_fromstring(&f4, "f4", "filter f3"));

  /* support referencing a filter several times */
  FAIL_UNLESS("DAG", !owl_filter_init_fromstring(&f5, "dag", "filter f1 or filter f1"));

  return 0;
}


#endif /* OWL_INCLUDE_REG_TESTS */
