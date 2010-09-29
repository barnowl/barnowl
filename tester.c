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

extern void owl_perl_xs_init(pTHX);

int main(int argc, char **argv, char **env)
{
  FILE *rnull;
  FILE *wnull;
  char *perlerr;
  int status = 0;

  if (argc <= 1) {
    fprintf(stderr, "Usage: %s --builtin|TEST.t|-le CODE\n", argv[0]);
    return 1;
  }

  /* initialize a fake ncurses, detached from std{in,out} */
  wnull = fopen("/dev/null", "w");
  rnull = fopen("/dev/null", "r");
  newterm("xterm", wnull, rnull);
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

  FAIL_UNLESS("owl_util_substitute 1",
	      !strcmp("foo", owl_text_substitute("foo", "", "Y")));
  FAIL_UNLESS("owl_text_substitute 2",
	      !strcmp("fYZYZ", owl_text_substitute("foo", "o", "YZ")));
  FAIL_UNLESS("owl_text_substitute 3",
	      !strcmp("foo", owl_text_substitute("fYZYZ", "YZ", "o")));
  FAIL_UNLESS("owl_text_substitute 4",
	      !strcmp("/u/foo/meep", owl_text_substitute("~/meep", "~", "/u/foo")));

  FAIL_UNLESS("skiptokens 1", 
	      !strcmp("bar quux", skiptokens("foo bar quux", 1)));
  FAIL_UNLESS("skiptokens 2", 
	      !strcmp("meep", skiptokens("foo 'bar quux' meep", 2)));

  FAIL_UNLESS("expand_tabs 1",
              !strcmp("        hi", owl_text_expand_tabs("\thi")));

  FAIL_UNLESS("expand_tabs 2",
              !strcmp("        hi\nword    tab", owl_text_expand_tabs("\thi\nword\ttab")));

  FAIL_UNLESS("expand_tabs 3",
              !strcmp("                2 tabs", owl_text_expand_tabs("\t\t2 tabs")));
  FAIL_UNLESS("expand_tabs 4",
	      !strcmp("α       ααααααα!        ", owl_text_expand_tabs("α\tααααααα!\t")));
  FAIL_UNLESS("expand_tabs 5",
	      !strcmp("Ａ      ＡＡＡ!!        ", owl_text_expand_tabs("Ａ\tＡＡＡ!!\t")));

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

  /* if (numfailed) printf("*** WARNING: failures encountered with owl_util\n"); */
  printf("# END testing owl_util (%d failures)\n", numfailed);
  return(numfailed);
}

int owl_dict_regtest(void) {
  owl_dict d;
  owl_list l;
  int numfailed=0;
  char *av="aval", *bv="bval", *cv="cval", *dv="dval";

  printf("# BEGIN testing owl_dict\n");
  FAIL_UNLESS("create", 0==owl_dict_create(&d));
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
  FAIL_UNLESS("get_keys", 0==owl_dict_get_keys(&d, &l));
  FAIL_UNLESS("get_keys result size", 3==owl_list_get_size(&l));
  
  /* these assume the returned keys are sorted */
  FAIL_UNLESS("get_keys result val",0==strcmp("a",owl_list_get_element(&l,0)));
  FAIL_UNLESS("get_keys result val",0==strcmp("b",owl_list_get_element(&l,1)));
  FAIL_UNLESS("get_keys result val",0==strcmp("c",owl_list_get_element(&l,2)));

  owl_list_cleanup(&l, owl_free);
  owl_dict_cleanup(&d, NULL);

  /*  if (numfailed) printf("*** WARNING: failures encountered with owl_dict\n"); */
  printf("# END testing owl_dict (%d failures)\n", numfailed);
  return(numfailed);
}

int owl_variable_regtest(void) {
  owl_vardict vd;
  int numfailed=0;
  char buf[1024];
  const void *v;

  printf("# BEGIN testing owl_variable\n");
  FAIL_UNLESS("setup", 0==owl_variable_dict_setup(&vd));

  FAIL_UNLESS("get bool", 0==owl_variable_get_bool(&vd,"rxping"));
  FAIL_UNLESS("get bool (no such)", -1==owl_variable_get_bool(&vd,"mumble"));
  FAIL_UNLESS("get bool as string 1", 0==owl_variable_get_tostring(&vd,"rxping", buf, 1024));
  FAIL_UNLESS("get bool as string 2", 0==strcmp(buf,"off"));
  FAIL_UNLESS("set bool 1", 0==owl_variable_set_bool_on(&vd,"rxping"));
  FAIL_UNLESS("get bool 2", 1==owl_variable_get_bool(&vd,"rxping"));
  FAIL_UNLESS("set bool 3", 0==owl_variable_set_fromstring(&vd,"rxping","off",0,0));
  FAIL_UNLESS("get bool 4", 0==owl_variable_get_bool(&vd,"rxping"));
  FAIL_UNLESS("set bool 5", -1==owl_variable_set_fromstring(&vd,"rxping","xxx",0,0));
  FAIL_UNLESS("get bool 6", 0==owl_variable_get_bool(&vd,"rxping"));


  FAIL_UNLESS("get string", 0==strcmp("~/zlog/people", owl_variable_get_string(&vd,"logpath")));
  FAIL_UNLESS("set string 7", 0==owl_variable_set_string(&vd,"logpath","whee"));
  FAIL_UNLESS("get string", 0==strcmp("whee", owl_variable_get_string(&vd,"logpath")));

  FAIL_UNLESS("get int", 8==owl_variable_get_int(&vd,"typewinsize"));
  FAIL_UNLESS("get int (no such)", -1==owl_variable_get_int(&vd,"mmble"));
  FAIL_UNLESS("get int as string 1", 0==owl_variable_get_tostring(&vd,"typewinsize", buf, 1024));
  FAIL_UNLESS("get int as string 2", 0==strcmp(buf,"8"));
  FAIL_UNLESS("set int 1", 0==owl_variable_set_int(&vd,"typewinsize",12));
  FAIL_UNLESS("get int 2", 12==owl_variable_get_int(&vd,"typewinsize"));
  FAIL_UNLESS("set int 1b", -1==owl_variable_set_int(&vd,"typewinsize",-3));
  FAIL_UNLESS("get int 2b", 12==owl_variable_get_int(&vd,"typewinsize"));
  FAIL_UNLESS("set int 3", 0==owl_variable_set_fromstring(&vd,"typewinsize","9",0,0));
  FAIL_UNLESS("get int 4", 9==owl_variable_get_int(&vd,"typewinsize"));
  FAIL_UNLESS("set int 5", -1==owl_variable_set_fromstring(&vd,"typewinsize","xxx",0,0));
  FAIL_UNLESS("set int 6", -1==owl_variable_set_fromstring(&vd,"typewinsize","",0,0));
  FAIL_UNLESS("get int 7", 9==owl_variable_get_int(&vd,"typewinsize"));

  owl_variable_dict_newvar_string(&vd, "stringvar", "", "", "testval");
  FAIL_UNLESS("get new string var", NULL != (v = owl_variable_get(&vd, "stringvar", OWL_VARIABLE_STRING)));
  FAIL_UNLESS("get new string val", !strcmp("testval", owl_variable_get_string(&vd, "stringvar")));
  owl_variable_set_string(&vd, "stringvar", "new val");
  FAIL_UNLESS("update string val", !strcmp("new val", owl_variable_get_string(&vd, "stringvar")));

  owl_variable_dict_newvar_int(&vd, "intvar", "", "", 47);
  FAIL_UNLESS("get new int var", NULL != (v = owl_variable_get(&vd, "intvar", OWL_VARIABLE_INT)));
  FAIL_UNLESS("get new int val", 47 == owl_variable_get_int(&vd, "intvar"));
  owl_variable_set_int(&vd, "intvar", 17);
  FAIL_UNLESS("update bool val", 17 == owl_variable_get_int(&vd, "intvar"));

  owl_variable_dict_newvar_bool(&vd, "boolvar", "", "", 1);
  FAIL_UNLESS("get new bool var", NULL != (v = owl_variable_get(&vd, "boolvar", OWL_VARIABLE_BOOL)));
  FAIL_UNLESS("get new bool val", owl_variable_get_bool(&vd, "boolvar"));
  owl_variable_set_bool_off(&vd, "boolvar");
  FAIL_UNLESS("update string val", !owl_variable_get_bool(&vd, "boolvar"));

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

  owl_dict_create(&g.filters);
  g.filterlist = NULL;
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
  owl_free(str);

  owl_fmtext_clear(&fm2);
  owl_fmtext_truncate_lines(&fm1, 1, 5, &fm2);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("lines corrected truncated",
	      str && !strcmp(str, "1\n2\n3\n4\n"));
  owl_free(str);

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
  owl_free(str);

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
  owl_free(str);

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
  owl_free(str);

  owl_fmtext_clear(&fm2);
  owl_fmtext_expand_tabs(&fm1, &fm2, 1);
  str = owl_fmtext_print_plain(&fm2);
  FAIL_UNLESS("no tabs remaining", strchr(str, '\t') == NULL);
  FAIL_UNLESS("tabs corrected expanded",
              str && !strcmp(str, "12     1234567 1\n"
                                  "12345678       1"));
  owl_free(str);

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
