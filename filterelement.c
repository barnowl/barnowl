#include "owl.h"

static const char fileIdent[] = "$Id$";

#define OWL_FILTERELEMENT_NULL        0
#define OWL_FILTERELEMENT_TRUE        1
#define OWL_FILTERELEMENT_FALSE       2
#define OWL_FILTERELEMENT_OPENBRACE   3
#define OWL_FILTERELEMENT_CLOSEBRACE  4
#define OWL_FILTERELEMENT_AND         5
#define OWL_FILTERELEMENT_OR          6
#define OWL_FILTERELEMENT_NOT         7
#define OWL_FILTERELEMENT_RE          8


void owl_filterelement_create_null(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_NULL;
  fe->field=NULL;
}

void owl_filterelement_create_openbrace(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_OPENBRACE;
  fe->field=NULL;
}

void owl_filterelement_create_closebrace(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_CLOSEBRACE;
  fe->field=NULL;
}

void owl_filterelement_create_and(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_AND;
  fe->field=NULL;
}

void owl_filterelement_create_or(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_OR;
  fe->field=NULL;
}

void owl_filterelement_create_not(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_NOT;
  fe->field=NULL;
}

void owl_filterelement_create_true(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_TRUE;
  fe->field=NULL;
}

void owl_filterelement_create_false(owl_filterelement *fe) {
  fe->type=OWL_FILTERELEMENT_FALSE;
  fe->field=NULL;
}

void owl_filterelement_create_re(owl_filterelement *fe, char *field, char *re) {
  fe->type=OWL_FILTERELEMENT_RE;
  fe->field=owl_strdup(field);
  owl_regex_create(&(fe->re), re);
}

void owl_filterelement_free(owl_filterelement *fe) {
  if (fe->field) owl_free(fe->field);
}

int owl_filterelement_is_null(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_NULL) return(1);
  return(0);
}

int owl_filterelement_is_openbrace(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_OPENBRACE) return(1);
  return(0);
}

int owl_filterelement_is_closebrace(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_CLOSEBRACE) return(1);
  return(0);
}

int owl_filterelement_is_and(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_AND) return(1);
  return(0);
}

int owl_filterelement_is_or(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_OR) return(1);
  return(0);
}

int owl_filterelement_is_not(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_NOT) return(1);
  return(0);
}

int owl_filterelement_is_true(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_TRUE) return(1);
  return(0);
}

int owl_filterelement_is_false(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_FALSE) return(1);
  return(0);
}

int owl_filterelement_is_re(owl_filterelement *fe) {
  if (fe->type==OWL_FILTERELEMENT_RE) return(1);
  return(0);
}

owl_regex *owl_filterelement_get_re(owl_filterelement *fe) {
  return(&(fe->re));
}

char *owl_filterelement_get_field(owl_filterelement *fe) {
  return(fe->field);
}

int owl_filterelement_is_value(owl_filterelement *fe) {
  if ( (fe->type==OWL_FILTERELEMENT_TRUE) ||
       (fe->type==OWL_FILTERELEMENT_FALSE) ||
       (fe->type==OWL_FILTERELEMENT_RE) ) {
    return(1);
  }
  return(0);
}


char *owl_filterelement_to_string(owl_filterelement *fe) {
  /* return must be freed by caller */
  
  if (owl_filterelement_is_openbrace(fe)) {
    return(owl_strdup("( "));
  } else if (owl_filterelement_is_closebrace(fe)) {
    return(owl_strdup(") "));
  } else if (owl_filterelement_is_and(fe)) {
    return(owl_strdup("and "));
  } else if (owl_filterelement_is_or(fe)) {
    return(owl_strdup("or "));
  } else if (owl_filterelement_is_not(fe)) {
    return(owl_strdup("not "));
  } else if (owl_filterelement_is_true(fe)) {
    return(owl_strdup("true "));
  } else if (owl_filterelement_is_false(fe)) {
    return(owl_strdup("false "));
  } else if (owl_filterelement_is_re(fe)) {
    char *buff;
    buff=owl_malloc(LINE);
    sprintf(buff, "%s %s ", fe->field, owl_regex_get_string(&(fe->re)));
    return(buff);
  }
  return(owl_strdup(""));
}
