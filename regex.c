/* Copyright (c) 2002,2003,2004,2009 James M. Kretchmar
 *
 * This file is part of Owl.
 *
 * Owl is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Owl is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Owl.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ---------------------------------------------------------------
 * 
 * As of Owl version 2.1.12 there are patches contributed by
 * developers of the the branched BarnOwl project, Copyright (c)
 * 2006-2008 The BarnOwl Developers. All rights reserved.
 */

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
  char buff1[LINE], buff2[LINE];
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
    sprintf(buff2, "Error in regular expression: %s", buff1);
    owl_function_makemsg(buff2);
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
  if (re->string) owl_free(re->string);

  /* do we need to free the regular expression? */
}
