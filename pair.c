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

void owl_pair_create(owl_pair *p, void *key, void *value) {
  p->key=key;
  p->value=value;
}

void owl_pair_set_key(owl_pair *p, void *key) {
  p->key=key;
}

void owl_pair_set_value(owl_pair *p, void *value) {
  p->value=value;
}

void *owl_pair_get_key(owl_pair *p) {
  return(p->key);
}

void *owl_pair_get_value(owl_pair *p) {
  return(p->value);
}
