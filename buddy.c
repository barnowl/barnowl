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

static const char fileIdent[] = "$Id$";

void owl_buddy_create(owl_buddy *b, int proto, char *name)
{
  b->proto=proto;
  b->name=owl_strdup(name);
  b->idlesince=0;
}

char *owl_buddy_get_name(owl_buddy *b)
{
  if (b->name) return(b->name);
  return("");
}

int owl_buddy_is_idle(owl_buddy *b)
{
  if (b->isidle) return(1);
  return(0);
}

void owl_buddy_set_idle(owl_buddy *b)
{
  b->isidle=1;
}

void owl_buddy_set_unidle(owl_buddy *b)
{
  b->isidle=0;
}

int owl_buddy_get_proto(owl_buddy *b)
{
  return(b->proto);
}

int owl_buddy_is_proto_aim(owl_buddy *b)
{
  if (b->proto==OWL_PROTOCOL_AIM) return(1);
  return(0);
}

/* Set the buddy to have been idle since 'diff' minutes ago
 */
void owl_buddy_set_idle_since(owl_buddy *b, int diff)
{
  time_t now;

  now=time(NULL);
  b->idlesince=now-(diff*60);
}

/* return the number of minutes the buddy has been idle
 */
int owl_buddy_get_idle_time(owl_buddy *b)
{
  time_t now;

  if (b->isidle) {
    now=time(NULL);
    return((now - b->idlesince)/60);
  }
  return(0);
}

void owl_buddy_free(owl_buddy *b)
{
  if (b->name) owl_free(b->name);
}
