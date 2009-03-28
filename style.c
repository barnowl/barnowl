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

void owl_style_create_internal(owl_style *s, char *name, void (*formatfunc) (owl_fmtext *fm, owl_message *m), char *description)
{
  s->type=OWL_STYLE_TYPE_INTERNAL;
  s->name=owl_strdup(name);
  if (description) {
    s->description=owl_strdup(description);
  } else {
    s->description=owl_sprintf("Owl internal style %s", name);
  }
  s->perlfuncname=NULL;
  s->formatfunc=formatfunc;
}

void owl_style_create_perl(owl_style *s, char *name, char *perlfuncname, char *description)
{
  s->type=OWL_STYLE_TYPE_PERL;
  s->name=owl_strdup(name);
  s->perlfuncname=owl_strdup(perlfuncname);
  if (description) {
    s->description=owl_strdup(description);
  } else {
    s->description=owl_sprintf("User-defined perl style that calls %s", 
			       perlfuncname);
  }
  s->formatfunc=NULL;
}

int owl_style_matches_name(owl_style *s, char *name)
{
  if (!strcmp(s->name, name)) return(1);
  return(0);
}

char *owl_style_get_name(owl_style *s)
{
  return(s->name);
}

char *owl_style_get_description(owl_style *s)
{
  return(s->description);
}

/* Use style 's' to format message 'm' into fmtext 'fm'.
 * 'fm' should already be be initialzed
 */
void owl_style_get_formattext(owl_style *s, owl_fmtext *fm, owl_message *m)
{
  if (s->type==OWL_STYLE_TYPE_INTERNAL) {
    (* s->formatfunc)(fm, m);
  } else if (s->type==OWL_STYLE_TYPE_PERL) {
    char *body, *indent;
    int curlen;

    /* run the perl function */
    body=owl_perlconfig_getmsg(m, 1, s->perlfuncname);
    if (!strcmp(body, "")) {
      owl_free(body);
      body=owl_strdup("<unformatted message>");
    }
    
    /* indent and ensure ends with a newline */
    indent=owl_malloc(strlen(body)+(owl_text_num_lines(body))*OWL_TAB+10);
    owl_text_indent(indent, body, OWL_TAB);
    curlen = strlen(indent);
    if (curlen==0 || indent[curlen-1] != '\n') {
      indent[curlen] = '\n';
      indent[curlen+1] = '\0';
    }

    /* fmtext_append.  This needs to change */
    owl_fmtext_append_ztext(fm, indent);
    
    owl_free(indent);
    owl_free(body);
  }
}

int owl_style_validate(owl_style *s) {
  if (!s) {
    return -1;
  } else if (s->type==OWL_STYLE_TYPE_INTERNAL) {
    return 0;
  } else if (s->type==OWL_STYLE_TYPE_PERL 
	     && s->perlfuncname 
	     && owl_perlconfig_is_function(s->perlfuncname)) {
    return 0;
  } else {
    return -1;
  }
}

void owl_style_free(owl_style *s)
{
  if (s->name) owl_free(s->name);
  if (s->description) owl_free(s->description);
  if (s->type==OWL_STYLE_TYPE_PERL && s->perlfuncname) {
    owl_free(s->perlfuncname);
  }
}
