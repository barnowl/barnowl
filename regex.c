#include <string.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_regex_init(owl_regex *re)
{
  re->negate=0;
  re->string=NULL;
}

int owl_regex_create(owl_regex *re, char *string)
{
  int ret;
  char buff1[LINE];
  char *ptr;
  
  re->string=owl_strdup(string);

  ptr=string;
  re->negate=0;
  if (string[0]=='!') {
    ptr++;
    re->negate=1;
  }

  /* set the regex */
  ret=regcomp(&(re->re), ptr, REG_EXTENDED|REG_ICASE);
  if (ret) {
    regerror(ret, NULL, buff1, LINE);
    owl_function_makemsg("Error in regular expression: %s", buff1);
    owl_free(re->string);
    re->string=NULL;
    return(-1);
  }

  return(0);
}

int owl_regex_create_quoted(owl_regex *re, char *string)
{
  char *quoted;
  
  quoted=owl_text_quote(string, OWL_REGEX_QUOTECHARS, OWL_REGEX_QUOTEWITH);
  owl_regex_create(re, quoted);
  owl_free(quoted);
  return(0);
}

int owl_regex_compare(owl_regex *re, char *string)
{
  int out, ret;

  /* if the regex is not set we match */
  if (!owl_regex_is_set(re)) {
    return(0);
  }
  
  ret=regexec(&(re->re), string, 0, NULL, 0);
  out=ret;
  if (re->negate) {
    out=!out;
  }
  return(out);
}

int owl_regex_is_set(owl_regex *re)
{
  if (re->string) return(1);
  return(0);
}

char *owl_regex_get_string(owl_regex *re)
{
  return(re->string);
}

void owl_regex_copy(owl_regex *a, owl_regex *b)
{
  b->negate=a->negate;
  b->string=owl_strdup(a->string);
  memcpy(&(b->re), &(a->re), sizeof(regex_t));
}

void owl_regex_free(owl_regex *re)
{
    if (re->string) {
        owl_free(re->string);
        regfree(&(re->re));
    }
}
