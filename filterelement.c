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
#define OWL_FILTERELEMENT_FILTER      9
#define OWL_FILTERELEMENT_PERL       10

void owl_filterelement_create_null(owl_filterelement *fe)
{
  fe->type=OWL_FILTERELEMENT_NULL;
  fe->field=NULL;
  fe->filtername=NULL;
}

void owl_filterelement_create_openbrace(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_OPENBRACE;
}

void owl_filterelement_create_closebrace(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_CLOSEBRACE;
}

void owl_filterelement_create_and(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_AND;
}

void owl_filterelement_create_or(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_OR;
}

void owl_filterelement_create_not(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_NOT;
}

void owl_filterelement_create_true(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_TRUE;
}

void owl_filterelement_create_false(owl_filterelement *fe)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_FALSE;
}

void owl_filterelement_create_re(owl_filterelement *fe, char *field, char *re)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_RE;
  fe->field=owl_strdup(field);
  owl_regex_create(&(fe->re), re);
}

void owl_filterelement_create_filter(owl_filterelement *fe, char *name)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_FILTER;
  fe->filtername=owl_strdup(name);
}

void owl_filterelement_create_perl(owl_filterelement *fe, char *name)
{
  owl_filterelement_create_null(fe);
  fe->type=OWL_FILTERELEMENT_PERL;
  fe->filtername=owl_strdup(name);
}

void owl_filterelement_free(owl_filterelement *fe)
{
  if (fe->field) owl_free(fe->field);
  if (fe->filtername) owl_free(fe->filtername);
}

int owl_filterelement_is_null(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_NULL) return(1);
  return(0);
}

int owl_filterelement_is_openbrace(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_OPENBRACE) return(1);
  return(0);
}

int owl_filterelement_is_closebrace(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_CLOSEBRACE) return(1);
  return(0);
}

int owl_filterelement_is_and(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_AND) return(1);
  return(0);
}

int owl_filterelement_is_or(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_OR) return(1);
  return(0);
}

int owl_filterelement_is_not(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_NOT) return(1);
  return(0);
}

int owl_filterelement_is_true(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_TRUE) return(1);
  return(0);
}

int owl_filterelement_is_false(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_FALSE) return(1);
  return(0);
}

int owl_filterelement_is_re(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_RE) return(1);
  return(0);
}

int owl_filterelement_is_perl(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_PERL) return(1);
  return(0);
}

owl_regex *owl_filterelement_get_re(owl_filterelement *fe)
{
  return(&(fe->re));
}

int owl_filterelement_is_filter(owl_filterelement *fe)
{
  if (fe->type==OWL_FILTERELEMENT_FILTER) return(1);
  return(0);
}

char *owl_filterelement_get_field(owl_filterelement *fe)
{
  if (fe->field) return(fe->field);
  return("unknown-field");
}

char *owl_filterelement_get_filtername(owl_filterelement *fe)
{
  if (fe->filtername) return(fe->filtername);
  return("unknown-filter");
}

int owl_filterelement_is_value(owl_filterelement *fe)
{
  if ( (fe->type==OWL_FILTERELEMENT_TRUE) ||
       (fe->type==OWL_FILTERELEMENT_FALSE) ||
       (fe->type==OWL_FILTERELEMENT_RE) ||
       (fe->type==OWL_FILTERELEMENT_PERL) ||
       (fe->type==OWL_FILTERELEMENT_FILTER)) {
    return(1);
  }
  return(0);
}

/* caller must free the return */
char *owl_filterelement_to_string(owl_filterelement *fe)
{
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
    return(owl_sprintf("%s %s ", fe->field, owl_regex_get_string(&(fe->re))));
  } else if (owl_filterelement_is_filter(fe)) {
    return(owl_sprintf("filter %s ", fe->filtername));
  } else if (owl_filterelement_is_perl(fe)) {
    return(owl_sprintf("perl %s ", fe->filtername));
  }

  return(owl_strdup("?"));
}
