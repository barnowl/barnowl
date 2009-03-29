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

#include "owl.h"
#include <stdlib.h>
#include <string.h>

static const char fileIdent[] = "$Id$";

int owl_messagelist_create(owl_messagelist *ml)
{
  owl_list_create(&(ml->list));
  return(0);
}

int owl_messagelist_get_size(owl_messagelist *ml)
{
  return(owl_list_get_size(&(ml->list)));
}

void *owl_messagelist_get_element(owl_messagelist *ml, int n)
{
  return(owl_list_get_element(&(ml->list), n));
}

owl_message *owl_messagelist_get_by_id(owl_messagelist *ml, int target_id)
{
  /* return the message with id == 'id'.  If it doesn't exist return NULL. */
  int first, last, mid, msg_id;
  owl_message *m;

  first = 0;
  last = owl_list_get_size(&(ml->list)) - 1;
  while (first <= last) {
    mid = (first + last) / 2;
    m = owl_list_get_element(&(ml->list), mid);
    msg_id = owl_message_get_id(m);
    if (msg_id == target_id) {
      return(m);
    } else if (msg_id < target_id) {
      first = mid + 1;
    } else {
      last = mid - 1;
    }
  }
  return(NULL);
}

int owl_messagelist_append_element(owl_messagelist *ml, void *element)
{
  return(owl_list_append_element(&(ml->list), element));
}

/* do we really still want this? */
int owl_messagelist_delete_element(owl_messagelist *ml, int n)
{
  /* mark a message as deleted */
  owl_message_mark_delete(owl_list_get_element(&(ml->list), n));
  return(0);
}

int owl_messagelist_undelete_element(owl_messagelist *ml, int n)
{
  /* mark a message as deleted */
  owl_message_unmark_delete(owl_list_get_element(&(ml->list), n));
  return(0);
}

int owl_messagelist_expunge(owl_messagelist *ml)
{
  /* expunge deleted messages */
  int i, j;
  owl_list newlist;
  owl_message *m;

  owl_list_create(&newlist);
  /*create a new list without messages marked as deleted */
  j=owl_list_get_size(&(ml->list));
  for (i=0; i<j; i++) {
    m=owl_list_get_element(&(ml->list), i);
    if (owl_message_is_delete(m)) {
      owl_message_free(m);
    } else {
      owl_list_append_element(&newlist, m);
    }
  }

  /* free the old list */
  owl_list_free_simple(&(ml->list));

  /* copy the new list to the old list */
  memcpy(&(ml->list), &newlist, sizeof(owl_list));

  return(0);
}

void owl_messagelist_invalidate_formats(owl_messagelist *ml)
{
  int i, j;
  owl_message *m;

  j=owl_list_get_size(&(ml->list));
  for (i=0; i<j; i++) {
    m=owl_list_get_element(&(ml->list), i);
    owl_message_invalidate_format(m);
  }
}
