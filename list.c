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

static const char fileIdent[] = "$Id$";

#define INITSIZE 30
#define GROWAT 2
#define GROWBY 1.5

int owl_list_create(owl_list *l)
{
  l->size=0;
  l->list=(void **)owl_malloc(INITSIZE*sizeof(void *));
  l->avail=INITSIZE;
  if (l->list==NULL) return(-1);
  return(0);
}

int owl_list_get_size(owl_list *l)
{
  return(l->size);
}

void *owl_list_get_element(owl_list *l, int n)
{
  if (n>l->size-1) return(NULL);
  return(l->list[n]);
}

int owl_list_append_element(owl_list *l, void *element)
{
  void *ptr;
  
  if ((l->size+1) > (l->avail/GROWAT)) {
    ptr=owl_realloc(l->list, l->avail*GROWBY*sizeof(void *));
    if (ptr==NULL) return(-1);
    l->list=ptr;
    l->avail=l->avail*GROWBY;
  }

  l->list[l->size]=element;
  l->size++;
  return(0);
}

int owl_list_prepend_element(owl_list *l, void *element)
{
  void *ptr;
  int i;
 
  if ((l->size+1) > (l->avail/GROWAT)) {
    ptr=owl_realloc(l->list, l->avail*GROWBY*sizeof(void *));
    if (ptr==NULL) return(-1);
    l->list=ptr;
    l->avail=l->avail*GROWBY;
  }

  for (i=l->size; i>0; i--) {
    l->list[i]=l->list[i-1];
  }
  l->list[0]=element;
  l->size++;
  return(0);
}

int owl_list_remove_element(owl_list *l, int n)
{
  int i;

  if (n>l->size-1) return(-1);
  for (i=n; i<l->size-1; i++) {
    l->list[i]=l->list[i+1];
  }
  l->size--;
  return(0);
}

/* todo: might leak memory */
int owl_list_replace_element(owl_list *l, int n, void *element)
{
  if (n>l->size-1) return(-1);

  l->list[n]=element;
  return(0);
}

void owl_list_free_all(owl_list *l, void (*elefree)(void *))
{
  int i;

  for (i=0; i<l->size; i++) {
    (elefree)(l->list[i]);
  }
  owl_free(l->list);
}

void owl_list_free_simple(owl_list *l)
{
  if (l->list) owl_free(l->list);
}
