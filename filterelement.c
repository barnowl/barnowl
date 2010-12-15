#include "owl.h"

static const char * owl_filterelement_get_field(const owl_message *m, const char * field)
{
  const char *match;
  if (!strcasecmp(field, "class")) {
    match=owl_message_get_class(m);
  } else if (!strcasecmp(field, "instance")) {
    match=owl_message_get_instance(m);
  } else if (!strcasecmp(field, "sender")) {
    match=owl_message_get_sender(m);
  } else if (!strcasecmp(field, "recipient")) {
    match=owl_message_get_recipient(m);
  } else if (!strcasecmp(field, "body")) {
    match=owl_message_get_body(m);
  } else if (!strcasecmp(field, "opcode")) {
    match=owl_message_get_opcode(m);
  } else if (!strcasecmp(field, "realm")) {
    match=owl_message_get_realm(m);
  } else if (!strcasecmp(field, "type")) {
    match=owl_message_get_type(m);
  } else if (!strcasecmp(field, "hostname")) {
    match=owl_message_get_hostname(m);
  } else if (!strcasecmp(field, "deleted")) {
    match=owl_message_is_delete(m) ? "true" : "false";
  } else if (!strcasecmp(field, "direction")) {
    if (owl_message_is_direction_out(m)) {
      match="out";
    } else if (owl_message_is_direction_in(m)) {
      match="in";
    } else if (owl_message_is_direction_none(m)) {
      match="none";
    } else {
      match="";
    }
  } else if (!strcasecmp(field, "login")) {
    if (owl_message_is_login(m)) {
      match="login";
    } else if (owl_message_is_logout(m)) {
      match="logout";
    } else {
      match="none";
    }
  } else {
    match = owl_message_get_attribute_value(m,field);
    if(match == NULL) match = "";
  }

  return match;
}

static int owl_filterelement_match_false(const owl_filterelement *fe, const owl_message *m)
{
  return 0;
}

static int owl_filterelement_match_true(const owl_filterelement *fe, const owl_message *m)
{
  return 1;
}

static int owl_filterelement_match_re(const owl_filterelement *fe, const owl_message *m)
{
  const char * val = owl_filterelement_get_field(m, fe->field);
  return !owl_regex_compare(&(fe->re), val, NULL, NULL);
}

static int owl_filterelement_match_filter(const owl_filterelement *fe, const owl_message *m)
{
  const owl_filter *subfilter;
  subfilter=owl_global_get_filter(&g, fe->field);
  if (!subfilter) {
    /* the filter does not exist, maybe because it was deleted.
     * Default to not matching
     */
    return 0;
  } 
  return owl_filter_message_match(subfilter, m);
}

static int owl_filterelement_match_perl(const owl_filterelement *fe, const owl_message *m)
{
  const char *subname;
  char *perlrv;
  int   tf=0;

  subname = fe->field;
  if (!owl_perlconfig_is_function(subname)) {
    return 0;
  }
  perlrv = owl_perlconfig_call_with_message(subname, m);
  if (perlrv) {
    if (0 == strcmp(perlrv, "1")) {
      tf=1;
    }
    owl_free(perlrv);
  }
  return tf;
}

static int owl_filterelement_match_group(const owl_filterelement *fe, const owl_message *m)
{
  return owl_filterelement_match(fe->left, m);
}

/* XXX: Our boolea operators short-circuit here. The original owl did
   not. Do we care?
*/

static int owl_filterelement_match_and(const owl_filterelement *fe, const owl_message *m)
{
  return owl_filterelement_match(fe->left, m) &&
    owl_filterelement_match(fe->right, m);
}

static int owl_filterelement_match_or(const owl_filterelement *fe, const owl_message *m)
{
  return owl_filterelement_match(fe->left, m) ||
    owl_filterelement_match(fe->right, m);
}

static int owl_filterelement_match_not(const owl_filterelement *fe, const owl_message *m)
{
  return !owl_filterelement_match(fe->left, m);
}

/* Print methods */

static void owl_filterelement_print_true(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, "true");
}

static void owl_filterelement_print_false(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, "false");
}

static void owl_filterelement_print_re(const owl_filterelement *fe, GString *buf)
{
  const char *re;
  g_string_append(buf, fe->field);
  g_string_append(buf, " ");

  re = owl_regex_get_string(&(fe->re));
  owl_string_append_quoted_arg(buf, re);
}

static void owl_filterelement_print_filter(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, "filter ");
  owl_string_append_quoted_arg(buf, fe->field);
}

static void owl_filterelement_print_perl(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, "perl ");
  owl_string_append_quoted_arg(buf, fe->field);
}

static void owl_filterelement_print_group(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, "( ");
  owl_filterelement_print(fe->left, buf) ;
  g_string_append(buf, " )");
}

static void owl_filterelement_print_or(const owl_filterelement *fe, GString *buf)
{
  owl_filterelement_print(fe->left, buf);
  g_string_append(buf, " or ");
  owl_filterelement_print(fe->right, buf);
}

static void owl_filterelement_print_and(const owl_filterelement *fe, GString *buf)
{
  owl_filterelement_print(fe->left, buf);
  g_string_append(buf, " and ");
  owl_filterelement_print(fe->right, buf);
}

static void owl_filterelement_print_not(const owl_filterelement *fe, GString *buf)
{
  g_string_append(buf, " not ");
  owl_filterelement_print(fe->left, buf);
}

/* Constructors */

void owl_filterelement_create(owl_filterelement *fe) {
  fe->field = NULL;
  fe->left = fe->right = NULL;
  fe->match_message = NULL;
  fe->print_elt = NULL;
  owl_regex_init(&(fe->re));
}


void owl_filterelement_create_true(owl_filterelement *fe)
{
  owl_filterelement_create(fe);
  fe->match_message = owl_filterelement_match_true;
  fe->print_elt = owl_filterelement_print_true;
}

void owl_filterelement_create_false(owl_filterelement *fe)
{
  owl_filterelement_create(fe);
  fe->match_message = owl_filterelement_match_false;
  fe->print_elt = owl_filterelement_print_false;
}

int owl_filterelement_create_re(owl_filterelement *fe, const char *field, const char *re)
{
  owl_filterelement_create(fe);
  fe->field=owl_strdup(field);
  if(owl_regex_create(&(fe->re), re)) {
    owl_free(fe->field);
    fe->field = NULL;
    return (-1);
  }
  fe->match_message = owl_filterelement_match_re;
  fe->print_elt = owl_filterelement_print_re;
  return 0;
}

void owl_filterelement_create_filter(owl_filterelement *fe, const char *name)
{
  owl_filterelement_create(fe);
  fe->field=owl_strdup(name);
  fe->match_message = owl_filterelement_match_filter;
  fe->print_elt = owl_filterelement_print_filter;
}

void owl_filterelement_create_perl(owl_filterelement *fe, const char *name)
{
  owl_filterelement_create(fe);
  fe->field=owl_strdup(name);
  fe->match_message = owl_filterelement_match_perl;
  fe->print_elt = owl_filterelement_print_perl;
}

void owl_filterelement_create_group(owl_filterelement *fe, owl_filterelement *in)
{
  owl_filterelement_create(fe);
  fe->left = in;
  fe->match_message = owl_filterelement_match_group;
  fe->print_elt = owl_filterelement_print_group;
}

void owl_filterelement_create_not(owl_filterelement *fe, owl_filterelement *in)
{
  owl_filterelement_create(fe);
  fe->left = in;
  fe->match_message = owl_filterelement_match_not;
  fe->print_elt = owl_filterelement_print_not;
}

void owl_filterelement_create_and(owl_filterelement *fe, owl_filterelement *lhs, owl_filterelement *rhs)
{
  owl_filterelement_create(fe);
  fe->left = lhs;
  fe->right = rhs;
  fe->match_message = owl_filterelement_match_and;
  fe->print_elt = owl_filterelement_print_and;
}

void owl_filterelement_create_or(owl_filterelement *fe, owl_filterelement *lhs, owl_filterelement *rhs)
{
  owl_filterelement_create(fe);
  fe->left = lhs;
  fe->right = rhs;
  fe->match_message = owl_filterelement_match_or;
  fe->print_elt = owl_filterelement_print_or;
}

int owl_filterelement_match(const owl_filterelement *fe, const owl_message *m)
{
  if(!fe) return 0;
  if(!fe->match_message) return 0;
  return fe->match_message(fe, m);
}

static int fe_visiting = 0;
static int fe_visited  = 1;

int owl_filterelement_is_toodeep(const owl_filter *f, const owl_filterelement *fe)
{
  int rv;
  owl_dict filters;
  owl_dict_create(&filters);

  owl_dict_insert_element(&filters, f->name, &fe_visiting, owl_dict_noop_delete);

  rv = _owl_filterelement_is_toodeep(fe, &filters);

  owl_dict_cleanup(&filters, NULL);
  return rv;
}

int _owl_filterelement_is_toodeep(const owl_filterelement *fe, owl_dict *seen)
{
  int rv = 0;
  const owl_filter *f;

  if(fe->match_message == owl_filterelement_match_filter) {
    int *nval = owl_dict_find_element(seen, fe->field);
    if(nval == &fe_visiting) {
      return 1;
    } else if (nval == NULL) {
      f = owl_global_get_filter(&g, fe->field);
      owl_dict_insert_element(seen, fe->field, &fe_visiting, owl_dict_noop_delete);
      if(f) rv = _owl_filterelement_is_toodeep(f->root, seen);
      owl_dict_insert_element(seen, fe->field, &fe_visited, owl_dict_noop_delete);
    }
  } else {
    if(fe->left)
      rv = rv || _owl_filterelement_is_toodeep(fe->left, seen);
    if(fe->right)
      rv = rv || _owl_filterelement_is_toodeep(fe->right, seen);
  }
  return rv;
}

void owl_filterelement_cleanup(owl_filterelement *fe)
{
  if (fe->field) owl_free(fe->field);
  if (fe->left) {
    owl_filterelement_cleanup(fe->left);
    owl_free(fe->left);
  }
  if (fe->right) {
    owl_filterelement_cleanup(fe->right);
    owl_free(fe->right);
  }
  owl_regex_cleanup(&(fe->re));
}

void owl_filterelement_print(const owl_filterelement *fe, GString *buf)
{
  if(!fe || !fe->print_elt) return;
  fe->print_elt(fe, buf);
}
