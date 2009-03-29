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
 * developers of the branched BarnOwl project, Copyright (c)
 * 2006-2009 The BarnOwl Developers. All rights reserved.
 */

#include <stdlib.h>
#include "owl.h"

static const char fileIdent[] = "$Id$";

void owl_view_create(owl_view *v, char *name, owl_filter *f, owl_style *s)
{
  v->name=owl_strdup(name);
  v->filter=f;
  v->style=s;
  owl_messagelist_create(&(v->ml));
  owl_view_recalculate(v);
}

char *owl_view_get_name(owl_view *v)
{
  return(v->name);
}

/* if the message matches the filter then add to view */
void owl_view_consider_message(owl_view *v, owl_message *m)
{
  if (owl_filter_message_match(v->filter, m)) {
    owl_messagelist_append_element(&(v->ml), m);
  }
}

/* remove all messages, add all the global messages that match the
 * filter.
 */
void owl_view_recalculate(owl_view *v)
{
  int i, j;
  owl_messagelist *gml;
  owl_messagelist *ml;
  owl_message *m;

  gml=owl_global_get_msglist(&g);
  ml=&(v->ml);

  /* nuke the old list */
  owl_list_free_simple((owl_list *) ml);
  owl_messagelist_create(&(v->ml));

  /* find all the messages we want */
  j=owl_messagelist_get_size(gml);
  for (i=0; i<j; i++) {
    m=owl_messagelist_get_element(gml, i);
    if (owl_filter_message_match(v->filter, m)) {
      owl_messagelist_append_element(ml, m);
    }
  }
}

void owl_view_new_filter(owl_view *v, owl_filter *f)
{
  v->filter=f;
  owl_view_recalculate(v);
}

void owl_view_set_style(owl_view *v, owl_style *s)
{
  v->style=s;
}

owl_style *owl_view_get_style(owl_view *v)
{
  return(v->style);
}

char *owl_view_get_style_name(owl_view *v) {
  return(owl_style_get_name(v->style));
}

owl_message *owl_view_get_element(owl_view *v, int index)
{
  return(owl_messagelist_get_element(&(v->ml), index));
}

void owl_view_delete_element(owl_view *v, int index)
{
  owl_messagelist_delete_element(&(v->ml), index);
}

void owl_view_undelete_element(owl_view *v, int index)
{
  owl_messagelist_undelete_element(&(v->ml), index);
}

int owl_view_get_size(owl_view *v)
{
  return(owl_messagelist_get_size(&(v->ml)));
}

/* Returns the position in the view with a message closest 
 * to the passed msgid. */
int owl_view_get_nearest_to_msgid(owl_view *v, int targetid)
{
  int first, last, mid = 0, max, bestdist, curid = 0;

  first = 0;
  last = max = owl_view_get_size(v) - 1;
  while (first <= last) {
    mid = (first + last) / 2;
    curid = owl_message_get_id(owl_view_get_element(v, mid));
    if (curid == targetid) {
      return(mid);
    } else if (curid < targetid) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  bestdist = abs(targetid-curid);
  if (curid < targetid && mid+1 < max) {
    curid = owl_message_get_id(owl_view_get_element(v, mid+1));
    mid = (bestdist < abs(targetid-curid)) ? mid : mid+1;
  }
  else if (curid > targetid && mid-1 >= 0) {
    curid = owl_message_get_id(owl_view_get_element(v, mid-1));
    mid = (bestdist < abs(targetid-curid)) ? mid : mid-1;
  }
  return mid;
}

int owl_view_get_nearest_to_saved(owl_view *v)
{
  int cachedid;

  cachedid=owl_filter_get_cachedmsgid(v->filter);
  if (cachedid<0) return(0);
  return (owl_view_get_nearest_to_msgid(v, cachedid));
}

/* saves the current message position in the filter so it can 
 * be restored later if we switch back to this filter. */
void owl_view_save_curmsgid(owl_view *v, int curid)
{
  owl_filter_set_cachedmsgid(v->filter, curid);
}

/* fmtext should already be initialized */
void owl_view_to_fmtext(owl_view *v, owl_fmtext *fm)
{
  owl_fmtext_append_normal(fm, "Name: ");
  owl_fmtext_append_normal(fm, v->name);
  owl_fmtext_append_normal(fm, "\n");

  owl_fmtext_append_normal(fm, "Filter: ");
  owl_fmtext_append_normal(fm, owl_filter_get_name(v->filter));
  owl_fmtext_append_normal(fm, "\n");

  owl_fmtext_append_normal(fm, "Style: ");
  owl_fmtext_append_normal(fm, owl_style_get_name(v->style));
  owl_fmtext_append_normal(fm, "\n");
}

char *owl_view_get_filtname(owl_view *v)
{
  return(owl_filter_get_name(v->filter));
}

void owl_view_free(owl_view *v)
{
  owl_list_free_simple((owl_list *) &(v->ml));
  if (v->name) owl_free(v->name);
}
