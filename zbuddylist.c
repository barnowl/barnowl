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

static const char fileIdent[] = "$Id";

void owl_zbuddylist_create(owl_zbuddylist *zb)
{
  owl_list_create(&(zb->zusers));
}

int owl_zbuddylist_adduser(owl_zbuddylist *zb, char *name)
{
  int i, j;
  char *user;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    if (!strcasecmp(user, owl_list_get_element(&(zb->zusers), i))) {
      owl_free(user);
      return(-1);
    }
  }
  owl_list_append_element(&(zb->zusers), user);
  return(0);
}

int owl_zbuddylist_deluser(owl_zbuddylist *zb, char *name)
{
  int i, j;
  char *user, *ptr;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    ptr=owl_list_get_element(&(zb->zusers), i);
    if (!strcasecmp(user, ptr)) {
      owl_list_remove_element(&(zb->zusers), i);
      owl_free(ptr);
      owl_free(user);
      return(0);
    }
  }
  owl_free(user);
  return(-1);
}

int owl_zbuddylist_contains_user(owl_zbuddylist *zb, char *name)
{
  int i, j;
  char *user;

  user=long_zuser(name);

  j=owl_list_get_size(&(zb->zusers));
  for (i=0; i<j; i++) {
    if (!strcasecmp(user, owl_list_get_element(&(zb->zusers), i))) {
      owl_free(user);
      return(1);
    }
  }
  owl_free(user);
  return(0);
}
