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

void owl_errqueue_init(owl_errqueue *eq)
{
  owl_list_create(&(eq->errlist));
}

void owl_errqueue_append_err(owl_errqueue *eq, char *msg)
{
  owl_list_append_element(&(eq->errlist), owl_strdup(msg));
}

/* fmtext should already be initialized */
void owl_errqueue_to_fmtext(owl_errqueue *eq, owl_fmtext *fm)
{
  int i, j;

  j=owl_list_get_size(&(eq->errlist));
  for (i=0; i<j; i++) {
    owl_fmtext_append_normal(fm, owl_list_get_element(&(eq->errlist), i));
    owl_fmtext_append_normal(fm, "\n");
  }
}
