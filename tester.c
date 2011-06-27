#define OWL_PERL
#define WINDOW FAKE_WINDOW
#include "owl.h"
#undef WINDOW

#include <unistd.h>
#include <stdlib.h>

#undef instr
#include <curses.h>

owl_global g;

int numtests;

int owl_regtest(void);
int owl_util_regtest(void);
int owl_dict_regtest(void);
int owl_variable_regtest(void);
int owl_filter_regtest(void);
int owl_obarray_regtest(void);
int owl_editwin_regtest(void);
int owl_fmtext_regtest(void);
int owl_smartfilter_regtest(void);
int owl_history_regtest(void);

extern void owl_perl_xs_init(pTHX);

int main(int argc, char **argv, char **env)
{
  FILE *rnull;
  FILE *wnull;
  char *perlerr;
  int status = 0;
  SCREEN *screen;

  if (argc <= 1) {
    fprintf(stderr, "Usage: %s --builtin|TEST.t|-le CODE\n", argv[0]);
    return 1;
  }

  /* initialize a fake ncurses, detached from std{in,out} */
  wnull = fopen("/dev/null", "w");
  rnull = fopen("/dev/null", "r");
  screen = newterm("xterm", wnull, rnull);
  /* initialize global structures */
  owl_global_init(&g);

  perlerr = owl_perlconfig_initperl(NULL, &argc, &argv, &env);
  if (perlerr) {
    endwin();
    fprintf(stderr, "Internal perl error: %s\n", perlerr);
    status = 1;
    goto out;
  }

  owl_global_complete_setup(&g);
  owl_global_setup_default_filters(&g);

  owl_view_create(owl_global_get_current_view(&g), "main",
                  owl_global_get_filter(&g, "all"),
                  owl_global_get_style_by_name(&g, "default"));

  owl_function_firstmsg();

  ENTER;
  SAVETMPS;

  if (strcmp(argv[1], "--builtin") == 0) {
    status = owl_regtest();
  } else if (strcmp(argv[1], "-le") == 0 && argc > 2) {
    /*
     * 'prove' runs its harness perl with '-le CODE' to get some
     * information out.
     */
    moreswitches("l");
    eval_pv(argv[2], true);
  } else {
    sv_setpv(get_sv("0", false), argv[1]);
    sv_setpv(get_sv("main::test_prog", TRUE), argv[1]);

    eval_pv("do $main::test_prog; die($@) if($@)", true);
  }

  status = 0;

  FREETMPS;
  LEAVE;

 out:
  perl_destruct(owl_global_get_perlinterp(&g));
  perl_free(owl_global_get_perlinterp(&g));
  /* probably not necessary, but tear down the screen */
  endwin();
  delscreen(screen);
  fclose(rnull);
  fclose(wnull);
  return status;
}

int owl_regtest(void) {
  numtests = 0;
  int numfailures=0;
  /*
    printf("1..%d\n", OWL_UTIL_NTESTS+OWL_DICT_NTESTS+OWL_VARIABLE_NTESTS
    +OWL_FILTER_NTESTS+OWL_OBARRAY_NTESTS);
  */
  numfailures += owl_util_regtest();
  numfailures += owl_dict_regtest();
  numfailures += owl_variable_regtest();
  numfailures += owl_filter_regtest();
  numfailures += owl_editwin_regtest();
  numfailures += owl_fmtext_regtest();
  numfailures += owl_smartfilter_regtest();
  numfailures += owl_history_regtest();
  if (numfailures) {
      fprintf(stderr, "# *** WARNING: %d failures total\n", numfailures);
  }
  printf("1..%d\n", numtests);

  return(numfailures);
}

#define FAIL_UNLESS(desc,pred) do { int __pred = (pred);                \
    numtests++;                                                         \
    printf("%s %s", (__pred)?"ok":(numfailed++,"not ok"), desc);        \
    if(!(__pred)) printf("\t(%s:%d)", __FILE__, __LINE__); printf("%c", '\n'); } while(0)


int owl_util_regtest(void)
{
  int numfailed=0;

  printf("# BEGIN testing owl_util\n");

#define CHECK_STR_AND_FREE(desc, expected, expr)         \
    do {                                                 \
      char *__value = (expr);                            \
      FAIL_UNLESS((desc), !strcmp((expected), __value)); \
      g_free(__value);                                 \
    } while (0)

  CHECK_STR_AND_FREE("owl_text_substitute 2", "fYZYZ",
                     owl_text_substitute("foo", "o", "YZ"));
  CHECK_STR_AND_FREE("owl_text_substitute 3", "foo",
                     owl_text_substitute("fYZYZ", "YZ", "o"));
  CHECK_STR_AND_FREE("owl_text_substitute 4", "/u/foo/meep",
                     owl_text_substitute("~/meep", "~", "/u/foo"));

  FAIL_UNLESS("skiptokens 1", 
	      !strcmp("bar quux", skiptokens("foo bar quux", 1)));
  FAIL_UNLESS("skiptokens 2", 
	      !strcmp("meep", skiptokens("foo 'bar quux' meep", 2)));

  CHECK_STR_AND_FREE("expand_tabs 1", "        hi", owl_text_expand_tabs("\thi"));

  CHECK_STR_AND_FREE("expand_tabs 2", "        hi\nword    tab",
                     owl_text_expand_tabs("\thi\nword\ttab"));

  CHECK_STR_AND_FREE("expand_tabs 3", "                2 tabs",
                     owl_text_expand_tabs("\t\t2 tabs"));
  CHECK_STR_AND_FREE("expand_tabs 4", "α       ααααααα!        ",
                     owl_text_expand_tabs("α\tααααααα!\t"));
  CHECK_STR_AND_FREE("expand_tabs 5", "Ａ      ＡＡＡ!!        ",
                     owl_text_expand_tabs("Ａ\tＡＡＡ!!\t"));

  FAIL_UNLESS("skiptokens 1",
              !strcmp("world", skiptokens("hello world", 1)));

  FAIL_UNLESS("skiptokens 2",
              !strcmp("c d e", skiptokens("a   b c d e", 2)));

  FAIL_UNLESS("skiptokens 3",
              !strcmp("\"b\" c d e", skiptokens("a \"b\" c d e", 1)));

  FAIL_UNLESS("skiptokens 4",
              !strcmp("c d e", skiptokens("a \"b\" c d e", 2)));

  FAIL_UNLESS("skiptokens 5",
              !strcmp("c d e", skiptokens("a \"'\" c d e", 2)));

#define CHECK_QUOTING(desc, unquoted, quoted)		\
  do {							\
      int __argc;					\
      char *__quoted = owl_arg_quote(unquoted);		\
      char **__argv;					\
      FAIL_UNLESS(desc, !strcmp(quoted, __quoted));	\
      __argv = owl_parseline(__quoted, &__argc);	\
      FAIL_UNLESS(desc " - arg count", __argc == 1);	\
      FAIL_UNLESS(desc " - null-terminated",		\
		  __argv[__argc] == NULL);		\
      FAIL_UNLESS(desc " - parsed",			\
		  !strcmp(__argv[0], unquoted));	\
      g_strfreev(__argv);               		\
      g_free(__quoted);				\
    } while (0)

  CHECK_QUOTING("boring text", "mango", "mango");
  CHECK_QUOTING("spaces", "mangos are tasty", "'mangos are tasty'");
  CHECK_QUOTING("single quotes", "mango's", "\"mango's\"");
  CHECK_QUOTING("double quotes", "he said \"mangos are tasty\"",
		"'he said \"mangos are tasty\"'");
  CHECK_QUOTING("both quotes",
		"he said \"mango's are tasty even when you put in "
		"a random apostrophe\"",
		"\"he said \"'\"'\"mango's are tasty even when you put in "
		"a random apostrophe\"'\"'\"\"");
  CHECK_QUOTING("quote monster", "'\"\"'\"'''\"",
		"\""
		"'"
		"\"'\"'\""
		"\"'\"'\""
		"'"
		"\"'\"'\""
		"'"
		"'"
		"'"
		"\"'\"'\""
		"\"");

  GString *g = g_string_new("");
  owl_string_appendf_quoted(g, "%q foo %q%q %s %", "hello", "world is", "can't");
  FAIL_UNLESS("owl_string_appendf",
              !strcmp(g->str, "hello foo 'world is'\"can't\" %s %"));
  g_string_free(g, true);

  /* if (numfailed) printf("*** WARNING: failures encountered with owl_util\n"); */
  printf("# END testing owl_util (%d failures)\n", numfailed);
  return(numfailed);
}

int owl_dict_regtest(void) {
  owl_dict d;
  GPtrArray *l;
  int numfailed=0;
  char *av = g_strdup("aval"), *bv = g_strdup("bval"), *cv = g_strdup("cval"),
    *dv = g_strdup("dval");

  printf("# BEGIN testing owl_dict\n");
  owl_dict_create(&d);
  FAIL_UNLESS("insert b", 0==owl_dict_insert_element(&d, "b", bv, owl_dict_noop_delete));
  FAIL_UNLESS("insert d", 0==owl_dict_insert_element(&d, "d", dv, owl_dict_noop_delete));
  FAIL_UNLESS("insert a", 0==owl_dict_insert_element(&d, "a", av, owl_dict_noop_delete));
  FAIL_UNLESS("insert c", 0==owl_dict_insert_element(&d, "c", cv, owl_dict_noop_delete));
  FAIL_UNLESS("reinsert d (no replace)", -2==owl_dict_insert_element(&d, "d", dv, 0));
  FAIL_UNLESS("find a", av==owl_dict_find_element(&d, "a"));
  FAIL_UNLESS("find b", bv==owl_dict_find_element(&d, "b"));
  FAIL_UNLESS("find c", cv==owl_dict_find_element(&d, "c"));
  FAIL_UNLESS("find d", dv==owl_dict_find_element(&d, "d"));
  FAIL_UNLESS("find e (non-existent)", NULL==owl_dict_find_element(&d, "e"));
  FAIL_UNLESS("remove d", dv==owl_dict_remove_element(&d, "d"));
  FAIL_UNLESS("find d (post-removal)", NULL==owl_dict_find_element(&d, "d"));

  FAIL_UNLESS("get_size", 3==owl_dict_get_size(&d));
  l = owl_dict_get_keys(&d);
  FAIL_UNLESS("get_keys result size", 3 == l->len);
  
  /* these assume the returned keys are sorted */
  FAIL_UNLESS("get_keys result val", 0 == strcmp("a", l->pdata[0]));
  FAIL_UNLESS("get_keys result val", 0 == strcmp("b", l->pdata[1]));
  FAIL_UNLESS("get_keys result val", 0 == strcmp("c", l->pdata[2]));

  owl_ptr_array_free(l, g_free);
  owl_dict_cleanup(&d, NULL);

  g_free(av);
  g_free(bv);
  g_free(cv);
  g_free(dv);

  /*  if (numfailed) printf("*** WARNING: failures encountered with owl_dict\n"); */
  printf("# END testing owl_dict (%d failures)\n", numfailed);
  return(numfailed);
}

int owl_variable_regtest(void) {
  owl_vardict vd;
  owl_variable *var;
  int numfailed=0;
  char *value;
  const void *v;

  printf("# BEGIN testing owl_variable\n");
  FAIL_UNLESS("setup", 0==owl_variable_dict_setup(&vd));

  FAIL_UNLESS("get bool var", NULL != (var = owl_variable_get_var(&vd, "rxping")));
  FAIL_UNLESS("get bool", 0 == owl_variable_get_bool(var));
  FAIL_UNLESS("get bool (no such)", NULL == owl_variable_get_var(&vd, "mumble"));
  FAIL_UNLESS("get bool as string",
	      !strcmp((value = owl_variable_get_tostring(var)), "off"));
  g_free(value);
  FAIL_UNLESS("set bool 1", 0 == owl_variable_set_bool_on(var));
  FAIL_UNLESS("get bool 2", 1 == owl_variable_get_bool(var));
  FAIL_UNLESS("set bool 3", 0 == owl_variable_set_fromstring(var, "off", 0));
  FAIL_UNLESS("get bool 4", 0 == owl_variable_get_bool(var));
  FAIL_UNLESS("set bool 5", -1 == owl_variable_set_fromstring(var, "xxx", 0));
  FAIL_UNLESS("get bool 6", 0 == owl_variable_get_bool(var));


  FAIL_UNLESS("get string var", NULL != (var = owl_variable_get_var(&vd, "logpath")));
  FAIL_UNLESS("get string", 0 == strcmp("~/zlog/people", owl_variable_get_string(var)));
  FAIL_UNLESS("set string 7", 0 == owl_variable_set_string(var, "whee"));
  FAIL_UNLESS("get string", !strcmp("whee", owl_variable_get_string(var)));

  FAIL_UNLESS("get int var", NULL != (var = owl_variable_get_var(&vd, "typewinsize")));
  FAIL_UNLESS("get int", 8 == owl_variable_get_int(var));
  FAIL_UNLESS("get int (no such)", NULL == owl_variable_get_var(&vd, "mumble"));
  FAIL_UNLESS("get int as string",
	      !strcmp((value = owl_variable_get_tostring(var)), "8"));
  g_free(value);
  FAIL_UNLESS("set int 1", 0 == owl_variable_set_int(var, 12));
  FAIL_UNLESS("get int 2", 12 == owl_variable_get_int(var));
  FAIL_UNLESS("set int 1b", -1 == owl_variable_set_int(var, -3));
  FAIL_UNLESS("get int 2b", 12 == owl_variable_get_int(var));
  FAIL_UNLESS("set int 3", 0 == owl_variable_set_fromstring(var, "9", 0));
  FAIL_UNLESS("get int 4", 9 == owl_variable_get_int(var));
  FAIL_UNLESS("set int 5", -1 == owl_variable_set_fromstring(var, "xxx", 0));
  FAIL_UNLESS("set int 6", -1 == owl_variable_set_fromstring(var, "", 0));
  FAIL_UNLESS("get int 7", 9 == owl_variable_get_int(var));

  owl_variable_dict_newvar_string(&vd, "stringvar", "", "", "testval");
  FAIL_UNLESS("get new string var", NULL != (var = owl_variable_get_var(&vd, "stringvar")));
  FAIL_UNLESS("get new string var", NULL != (v = owl_variable_get(var)));
  FAIL_UNLESS("get new string val", !strcmp("testval", owl_variable_get_string(var)));
  owl_variable_set_string(var, "new val");
  FAIL_UNLESS("update string val", !strcmp("new val", owl_variable_get_string(var)));

  owl_variable_dict_newvar_int(&vd, "intvar", "", "", 47);
  FAIL_UNLESS("get new int var", NULL != (var = owl_variable_get_var(&vd, "intvar")));
  FAIL_UNLESS("get new int var", NULL != (v = owl_variable_get(var)));
  FAIL_UNLESS("get new int val", 47 == owl_variable_get_int(var));
  owl_variable_set_int(var, 17);
  FAIL_UNLESS("update int val", 17 == owl_variable_get_int(var));

  owl_variable_dict_newvar_bool(&vd, "boolvar", "", "", 1);
  FAIL_UNLESS("get new bool var", NULL != (var = owl_variable_get_var(&vd, "boolvar")));
  FAIL_UNLESS("get new bool var", NULL != (v = owl_variable_get(var)));
  FAIL_UNLESS("get new bool val", owl_variable_get_bool(var));
  owl_variable_set_bool_off(var);
  FAIL_UNLESS("update bool val", !owl_variable_get_bool(var));

  owl_variable_dict_newvar_string(&vd, "nullstringvar", "", "", NULL);
  FAIL_UNLESS("get new string (NULL) var", NULL != (var = owl_variable_get_var(&vd, "nullstringvar")));
  FAIL_UNLESS("get string (NULL)", NULL == (value = owl_variable_get_tostring(var)));
  g_free(value);
  var = owl_variable_get_var(&vd, "zsigproc");
  FAIL_UNLESS("get string (NULL) 2", NULL == (value = owl_variable_get_tostring(var)));
  g_free(value);

  owl_variable_dict_cleanup(&vd);

  /* if (numfailed) printf("*** WARNING: failures encountered with owl_variable\n"); */
  printf("# END testing owl_variable (%d failures)\n", numfailed);
  return(numfailed);
}

static int owl_filter_test_string(const char *filt, const owl_message *m, int shouldmatch)
{
  owl_filter *f;
  int ok;
  int failed = 0;
  if ((f = owl_filter_new_fromstring("test-filter", filt)) == NULL) {
    printf("not ok can't parse %s\n", filt);
    failed = 1;
    goto out;
  }
  ok = owl_filter_message_match(f, m);
  if((shouldmatch && !ok) || (!shouldmatch && ok)) {
    printf("not ok match %s (got %d, expected %d)\n", filt, ok, shouldmatch);
    failed = 1;
  }
 out:
  owl_filter_delete(f);
  if(!failed) {
    printf("ok %s %s\n", shouldmatch ? "matches" : "doesn't match", filt);
  }
  return failed;
}

int owl_filter_regtest(void) {
  int numfailed=0;
  owl_message m;
  owl_filter *f1, *f2, *f3, *f4, *f5;

  owl_message_init(&m);
  owl_message_set_type_zephyr(&m);
  owl_message_set_direction_in(&m);
  owl_message_set_class(&m, "owl");
  owl_message_set_instance(&m, "tester");
  owl_message_set_sender(&m, "owl-user");
  owl_message_set_recipient(&m, "joe");
  owl_message_set_attribute(&m, "foo", "bar");

#define TEST_FILTER(f, e) do {                          \
    numtests++;                                         \
    numfailed += owl_filter_test_string(f, &m, e);      \
      } while(0)

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

  f1 = owl_filter_new_fromstring("f1", "class owl");
  owl_global_add_filter(&g, f1);
  TEST_FILTER("filter f1", 1);
  owl_global_remove_filter(&g, "f1");

  /* Test recursion prevention */
  FAIL_UNLESS("self reference", (f2 = owl_filter_new_fromstring("test", "filter test")) == NULL);
  owl_filter_delete(f2);

  /* mutual recursion */
  f3 = owl_filter_new_fromstring("f3", "filter f4");
  owl_global_add_filter(&g, f3);
  FAIL_UNLESS("mutual recursion", (f4 = owl_filter_new_fromstring("f4", "filter f3")) == NULL);
  owl_global_remove_filter(&g, "f3");
  owl_filter_delete(f4);

  /* support referencing a filter several times */
  FAIL_UNLESS("DAG", (f5 = owl_filter_new_fromstring("dag", "filter f1 or filter f1")) != NULL);
  owl_filter_delete(f5);

  return 0;
}

int owl_editwin_regtest(void) {
  int numfailed = 0;
  const char *p;
  owl_editwin *oe;
  const char *autowrap_string = "we feel our owls should live "
                                "closer to our ponies.";

  printf("# BEGIN testing owl_editwin\n");

  oe = owl_editwin_new(NULL, 80, 80, OWL_EDITWIN_STYLE_MULTILINE, NULL);

  /* TODO: make the strings a little more lenient w.r.t trailing whitespace */

  /* check paragraph fill */
  owl_editwin_insert_string(oe, "blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah.\n\nblah");
  owl_editwin_move_to_top(oe);
  owl_editwin_fill_paragraph(oe);
  p = owl_editwin_get_text(oe);
  FAIL_UNLESS("text was correctly wrapped", p && !strcmp(p, "blah blah blah blah blah blah blah blah blah blah blah blah blah blah\n"
							    "blah blah blah.\n"
							    "\n"
							    "blah"));

  owl_editwin_unref(oe); oe = NULL;
  oe = owl_editwin_new(NULL, 80, 80, OWL_EDITWIN_STYLE_MULTILINE, NULL);

  /* check that lines ending with ". " correctly fill */
  owl_editwin_insert_string(oe, "blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah blah. \n\nblah");
  owl_editwin_move_to_top(oe);
  owl_editwin_fill_paragraph(oe);
  p = owl_editwin_get_text(oe);
  FAIL_UNLESS("text was correctly wrapped", p && !strcmp(p, "blah blah blah blah blah blah blah blah blah blah blah blah blah blah\n"
							    "blah blah blah. \n"
							    "\n"
							    "blah"));

  owl_editwin_unref(oe); oe = NULL;

  /* Test owl_editwin_move_to_beginning_of_line. */
  oe = owl_editwin_new(NULL, 80, 80, OWL_EDITWIN_STYLE_MULTILINE, NULL);
  owl_editwin_insert_string(oe, "\n");
  owl_editwin_insert_string(oe, "12345678\n");
  owl_editwin_insert_string(oe, "\n");
  owl_editwin_insert_string(oe, "abcdefg\n");
  owl_editwin_move_to_top(oe);
  FAIL_UNLESS("already at beginning of line",
	      owl_editwin_move_to_beginning_of_line(oe) == 0);
  owl_editwin_line_move(oe, 1);
  owl_editwin_point_move(oe, 5);
  FAIL_UNLESS("find beginning of line after empty first line",
	      owl_editwin_move_to_beginning_of_line(oe) == -5);
  owl_editwin_line_move(oe, 1);
  FAIL_UNLESS("find beginning empty middle line",
	      owl_editwin_move_to_beginning_of_line(oe) == 0);
  owl_editwin_line_move(oe, 1);
  owl_editwin_point_move(oe, 2);
  FAIL_UNLESS("find beginning of line after empty middle line",
	      owl_editwin_move_to_beginning_of_line(oe) == -2);
  owl_editwin_unref(oe); oe = NULL;

  /* Test automatic line-wrapping. */
  owl_global_set_edit_maxwrapcols(&g, 10);
  oe = owl_editwin_new(NULL, 80, 80, OWL_EDITWIN_STYLE_MULTILINE, NULL);
  for (p = autowrap_string; *p; p++) {
    owl_input j;
    j.ch = *p;
    j.uch = *p; /* Assuming ASCII. */
    owl_editwin_process_char(oe, j);
  }
  p = owl_editwin_get_text(oe);
  FAIL_UNLESS("text was automatically wrapped",
	      p && !strcmp(p, "we feel\n"
			   "our owls\n"
			   "should\n"
			   "live\n"
			   "closer to\n"
			   "our\n"
			   "ponies."));
  owl_editwin_unref(oe); oe = NULL;
  owl_global_set_edit_maxwrapcols(&g, 70);

  /* Test owl_editwin_current_column. */
  oe = owl_editwin_new(NULL, 80, 80, OWL_EDITWIN_STYLE_MULTILINE, NULL);
  FAIL_UNLESS("initial column zero", owl_editwin_current_column(oe) == 0);
  owl_editwin_insert_string(oe, "abcdef");
  FAIL_UNLESS("simple insert", owl_editwin_current_column(oe) == 6);
  owl_editwin_insert_string(oe, "\t");
  FAIL_UNLESS("insert tabs", owl_editwin_current_column(oe) == 8);
  owl_editwin_insert_string(oe, "123\n12\t3");
  FAIL_UNLESS("newline with junk", owl_editwin_current_column(oe) == 9);
  owl_editwin_move_to_beginning_of_line(oe);
  FAIL_UNLESS("beginning of line", owl_editwin_current_column(oe) == 0);
  owl_editwin_unref(oe); oe = NULL;

  printf("# END testing owl_editwin (%d failures)\n", numfailed);

  return numfailed;
}

int owl_fmtext_regtest(void) {
  int numfailed = 0;
  int start, end;
  owl_fmtext fm1;
  owl_fmtext fm2;
  owl_regex re;
  char *str;

  printf("# BEGIN testing owl_fmtext\n");

  owl_fmtext_init_null(&fm1);
  owl_fmtext_init_null(&fm2);

  /* Verify text gets correctly appended. */
  owl_fmtext_append_normal(&fm1, "1234567898");
  owl_fmtext_append_fmtext(&fm2, &fm1);
  FAIL_UNLESS("string lengths correct",
              owl_fmtext_num_bytes(&fm2) == strlen(owl_fmtext_get_text(&fm2)));

  /* Test owl_fmtext_num_lines. */
  owl_fmtext_clear(&fm1);
  FAIL_UNLESS("empty line correct", owl_fmtext_num_lines(&fm1) == 0);
  owl_fmtext_append_normal(&fm1, "12345\n67898");
  FAIL_UNLESS("trailing chars correct", owl_fmtext_num_lines(&fm1) == 2);
  owl_fmtext_append_normal(&fm1, "\n");
  FAIL_UNLESS("trailing newline correct", owl_fmtext_num_lines(&fm1) == 2);
  owl_fmtext_append_bold(&fm1, "");
  FAIL_UNLESS("trailing attributes correct", owl_fmtext_num_lines(&fm1) == 2);

  /* Test owl_fmtext_truncate_lines */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "0\n1\n2\n3\n4\n");

  owl_fmtext_clear(&fm2);
  owl_fmtext_truncate_lines(&fm1, 1, 3, &fm2);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("lines corrected truncated",
	      str && !strcmp(str, "1\n2\n3\n"));
  g_free(str);

  owl_fmtext_clear(&fm2);
  owl_fmtext_truncate_lines(&fm1, 1, 5, &fm2);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("lines corrected truncated",
	      str && !strcmp(str, "1\n2\n3\n4\n"));
  g_free(str);

  /* Test owl_fmtext_truncate_cols. */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "123456789012345\n");
  owl_fmtext_append_normal(&fm1, "123456789\n");
  owl_fmtext_append_normal(&fm1, "1234567890\n");

  owl_fmtext_clear(&fm2);
  owl_fmtext_truncate_cols(&fm1, 4, 9, &fm2);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("columns correctly truncated",
              str && !strcmp(str, "567890"
                                  "56789\n"
                                  "567890"));
  g_free(str);

  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "12\t1234");
  owl_fmtext_append_bold(&fm1, "56\n");
  owl_fmtext_append_bold(&fm1, "12345678\t\n");

  owl_fmtext_clear(&fm2);
  owl_fmtext_truncate_cols(&fm1, 4, 13, &fm2);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("columns correctly truncated",
              str && !strcmp(str, "    123456"
                                  "5678      "));
  g_free(str);

  /* Test owl_fmtext_expand_tabs. */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "12\t1234");
  owl_fmtext_append_bold(&fm1, "567\t1\n12345678\t1");
  owl_fmtext_clear(&fm2);
  owl_fmtext_expand_tabs(&fm1, &fm2, 0);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("no tabs remaining", strchr(str, '\t') == NULL);
  FAIL_UNLESS("tabs corrected expanded",
              str && !strcmp(str, "12      1234567 1\n"
                                  "12345678        1"));
  g_free(str);

  owl_fmtext_clear(&fm2);
  owl_fmtext_expand_tabs(&fm1, &fm2, 1);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("no tabs remaining", strchr(str, '\t') == NULL);
  FAIL_UNLESS("tabs corrected expanded",
              str && !strcmp(str, "12     1234567 1\n"
                                  "12345678       1"));
  g_free(str);

  /* Test owl_fmtext_search. */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "123123123123");
  owl_regex_create(&re, "12");
  {
    int count = 0, offset;
    offset = owl_fmtext_search(&fm1, &re, 0);
    while (offset >= 0) {
      FAIL_UNLESS("search matches",
		  !strncmp("12", owl_fmtext_get_text(&fm1) + offset, 2));
      count++;
      offset = owl_fmtext_search(&fm1, &re, offset+1);
    }
    FAIL_UNLESS("exactly four matches", count == 4);
  }
  owl_regex_cleanup(&re);

  /* Test owl_fmtext_line_number. */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "123\n456\n");
  owl_fmtext_append_bold(&fm1, "");
  FAIL_UNLESS("lines start at 0", 0 == owl_fmtext_line_number(&fm1, 0));
  FAIL_UNLESS("trailing formatting characters part of false line",
	      2 == owl_fmtext_line_number(&fm1, owl_fmtext_num_bytes(&fm1)));
  owl_regex_create_quoted(&re, "456");
  FAIL_UNLESS("correctly find second line (line 1)",
	      1 == owl_fmtext_line_number(&fm1, owl_fmtext_search(&fm1, &re, 0)));
  owl_regex_cleanup(&re);

  /* Test owl_fmtext_line_extents. */
  owl_fmtext_clear(&fm1);
  owl_fmtext_append_normal(&fm1, "123\n456\n789");
  owl_fmtext_line_extents(&fm1, 1, &start, &end);
  FAIL_UNLESS("line contents",
	      !strncmp("456\n", owl_fmtext_get_text(&fm1)+start, end-start));
  owl_fmtext_line_extents(&fm1, 2, &start, &end);
  FAIL_UNLESS("point to end of buffer", end == owl_fmtext_num_bytes(&fm1));

  owl_fmtext_cleanup(&fm1);
  owl_fmtext_cleanup(&fm2);

  printf("# END testing owl_fmtext (%d failures)\n", numfailed);

  return numfailed;
}

static int owl_smartfilter_test_equals(const char *filtname, const char *expected) {
  owl_filter *f = NULL;
  char *filtstr = NULL;
  int failed = 0;
  f = owl_global_get_filter(&g, filtname);
  if (f == NULL) {
    printf("not ok filter missing: %s\n", filtname);
    failed = 1;
    goto out;
  }

  /* TODO: Come up with a better way to test this. */
  filtstr = owl_filter_print(f);
  if (strcmp(expected, filtstr)) {
    printf("not ok filter incorrect: |%s| instead of |%s|\n",
	   filtstr, expected);
    failed = 1;
    goto out;
  }

 out:
  g_free(filtstr);
  return failed;
}

static int owl_classinstfilt_test(const char *c, const char *i, int related, const char *expected) {
  char *filtname = NULL;
  int failed = 0;

  filtname = owl_function_classinstfilt(c, i, related);
  if (filtname == NULL) {
    printf("not ok null filtname: %s %s %s\n", c, i ? i : "(null)",
	   related ? "related" : "not related");
    failed = 1;
    goto out;
  }
  if (owl_smartfilter_test_equals(filtname, expected)) {
    failed = 1;
    goto out;
  }
 out:
  if (!failed) {
    printf("ok %s\n", filtname);
  }
  if (filtname)
    owl_global_remove_filter(&g, filtname);
  g_free(filtname);
  return failed;
}

static int owl_zuserfilt_test(const char *longuser, const char *expected) {
  char *filtname = NULL;
  int failed = 0;

  filtname = owl_function_zuserfilt(longuser);
  if (filtname == NULL) {
    printf("not ok null filtname: %s\n", longuser);
    failed = 1;
    goto out;
  }
  if (owl_smartfilter_test_equals(filtname, expected)) {
    failed = 1;
    goto out;
  }
 out:
  if (!failed) {
    printf("ok %s\n", filtname);
  }
  if (filtname)
    owl_global_remove_filter(&g, filtname);
  g_free(filtname);
  return failed;
}


int owl_smartfilter_regtest(void) {
  int numfailed = 0;

  printf("# BEGIN testing owl_smartfilter\n");

  /* Check classinst making. */

#define TEST_CLASSINSTFILT(c, i, r, e) do {		\
    numtests++;						\
    numfailed += owl_classinstfilt_test(c, i, r, e);	\
  } while (0)
  TEST_CLASSINSTFILT("message", NULL, false,
		     "class ^message$\n");
  TEST_CLASSINSTFILT("message", NULL, true,
		     "class ^(un)*message(\\.d)*$\n");
  TEST_CLASSINSTFILT("message", "personal", false,
		     "class ^message$ and instance ^personal$\n");
  TEST_CLASSINSTFILT("message", "personal", true,
		     "class ^(un)*message(\\.d)*$ and ( instance ^(un)*personal(\\.d)*$ )\n");

  TEST_CLASSINSTFILT("message", "evil\tinstance", false,
		     "class ^message$ and instance '^evil\tinstance$'\n");
  TEST_CLASSINSTFILT("message", "evil instance", false,
		     "class ^message$ and instance '^evil instance$'\n");
  TEST_CLASSINSTFILT("message", "evil'instance", false,
		     "class ^message$ and instance \"^evil'instance$\"\n");
  TEST_CLASSINSTFILT("message", "evil\"instance", false,
		     "class ^message$ and instance '^evil\"instance$'\n");
  TEST_CLASSINSTFILT("message", "evil$instance", false,
		     "class ^message$ and instance ^evil\\$instance$\n");

#define TEST_ZUSERFILT(l, e) do {			\
    numtests++;						\
    numfailed += owl_zuserfilt_test(l, e);		\
  } while (0)
  TEST_ZUSERFILT("user",
		 "( type ^zephyr$ and filter personal and "
		 "( ( direction ^in$ and sender "
		 "^user$"
		 " ) or ( direction ^out$ and recipient "
		 "^user$"
		 " ) ) ) or ( ( class ^login$ ) and ( sender "
		 "^user$"
		 " ) )\n");
  TEST_ZUSERFILT("very evil\t.user",
		 "( type ^zephyr$ and filter personal and "
		 "( ( direction ^in$ and sender "
		 "'^very evil\t\\.user$'"
		 " ) or ( direction ^out$ and recipient "
		 "'^very evil\t\\.user$'"
		 " ) ) ) or ( ( class ^login$ ) and ( sender "
		 "'^very evil\t\\.user$'"
		 " ) )\n");

  printf("# END testing owl_smartfilter (%d failures)\n", numfailed);

  return numfailed;
}

int owl_history_regtest(void)
{
  int numfailed = 0;
  int i;
  owl_history h;

  printf("# BEGIN testing owl_history\n");
  owl_history_init(&h);

  /* Operations on empty history. */
  FAIL_UNLESS("prev NULL", owl_history_get_prev(&h) == NULL);
  FAIL_UNLESS("next NULL", owl_history_get_next(&h) == NULL);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));

  /* Insert a few records. */
  owl_history_store(&h, "a", false);
  owl_history_store(&h, "b", false);
  owl_history_store(&h, "c", false);
  owl_history_store(&h, "d", true);

  /* Walk up and down the history a bit. */
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("touched", owl_history_is_touched(&h));
  FAIL_UNLESS("next d", strcmp(owl_history_get_next(&h), "d") == 0);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));
  FAIL_UNLESS("next NULL", owl_history_get_next(&h) == NULL);
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("prev b", strcmp(owl_history_get_prev(&h), "b") == 0);
  FAIL_UNLESS("prev a", strcmp(owl_history_get_prev(&h), "a") == 0);
  FAIL_UNLESS("prev NULL", owl_history_get_prev(&h) == NULL);

  /* Now insert something. It should reset and blow away 'd'. */
  owl_history_store(&h, "e", false);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));
  FAIL_UNLESS("next NULL", owl_history_get_next(&h) == NULL);
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("touched", owl_history_is_touched(&h));
  FAIL_UNLESS("next e", strcmp(owl_history_get_next(&h), "e") == 0);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));

  /* Lines get de-duplicated on insert. */
  owl_history_store(&h, "e", false);
  owl_history_store(&h, "e", false);
  owl_history_store(&h, "e", false);
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("next e", strcmp(owl_history_get_next(&h), "e") == 0);

  /* But a partial is not deduplicated, as it'll go away soon. */
  owl_history_store(&h, "e", true);
  FAIL_UNLESS("prev e", strcmp(owl_history_get_prev(&h), "e") == 0);
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("next e", strcmp(owl_history_get_next(&h), "e") == 0);
  FAIL_UNLESS("next e", strcmp(owl_history_get_next(&h), "e") == 0);

  /* Reset moves to the front... */
  owl_history_store(&h, "f", true);
  FAIL_UNLESS("prev e", strcmp(owl_history_get_prev(&h), "e") == 0);
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  owl_history_reset(&h);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));
  /* ...and destroys any pending partial entry... */
  FAIL_UNLESS("prev c", strcmp(owl_history_get_prev(&h), "c") == 0);
  FAIL_UNLESS("prev b", strcmp(owl_history_get_prev(&h), "b") == 0);
  /* ...but not non-partial ones. */
  owl_history_reset(&h);
  FAIL_UNLESS("untouched", !owl_history_is_touched(&h));

  /* Finally, check we are bounded by OWL_HISTORYSIZE. */
  for (i = 0; i < OWL_HISTORYSIZE; i++) {
    char *string = g_strdup_printf("mango%d", i);
    owl_history_store(&h, string, false);
    g_free(string);
  }
  /* The OWL_HISTORYSIZE'th prev gets NULL. */
  for (i = OWL_HISTORYSIZE - 2; i >= 0; i--) {
    char *string = g_strdup_printf("mango%d", i);
    FAIL_UNLESS("prev mango_N", strcmp(owl_history_get_prev(&h), string) == 0);
    g_free(string);
  }
  FAIL_UNLESS("prev NULL", owl_history_get_prev(&h) == NULL);

  owl_history_cleanup(&h);

  printf("# END testing owl_history (%d failures)\n", numfailed);
  return numfailed;
}
