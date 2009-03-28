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

int owl_popwin_init(owl_popwin *pw)
{
  pw->active=0;
  pw->needsfirstrefresh=0;
  pw->lines=0;
  pw->cols=0;
  return(0);
}

int owl_popwin_up(owl_popwin *pw)
{
  int glines, gcols, startcol, startline;

  /* calculate the size of the popwin */
  glines=owl_global_get_lines(&g);
  gcols=owl_global_get_cols(&g);

  pw->lines = owl_util_min(glines,24)*3/4 + owl_util_max(glines-24,0)/2;
  startline = (glines-pw->lines)/2;

  pw->cols = owl_util_min(gcols,90)*15/16 + owl_util_max(gcols-90,0)/2;
  startcol = (gcols-pw->cols)/2;

  pw->borderwin=newwin(pw->lines, pw->cols, startline, startcol);
  pw->popwin=newwin(pw->lines-2, pw->cols-2, startline+1, startcol+1);
  pw->needsfirstrefresh=1;
  
  meta(pw->popwin,TRUE);
  nodelay(pw->popwin, 1);
  keypad(pw->popwin, TRUE);

  werase(pw->popwin);
  werase(pw->borderwin);
  if (owl_global_is_fancylines(&g)) {
    box(pw->borderwin, 0, 0);
  } else {
    box(pw->borderwin, '|', '-');
    wmove(pw->borderwin, 0, 0);
    waddch(pw->borderwin, '+');
    wmove(pw->borderwin, pw->lines-1, 0);
    waddch(pw->borderwin, '+');
    wmove(pw->borderwin, pw->lines-1, pw->cols-1);
    waddch(pw->borderwin, '+');
    wmove(pw->borderwin, 0, pw->cols-1);
    waddch(pw->borderwin, '+');
  }
    
  wnoutrefresh(pw->popwin);
  wnoutrefresh(pw->borderwin);
  owl_global_set_needrefresh(&g);
  pw->active=1;
  return(0);
}

int owl_popwin_display_text(owl_popwin *pw, char *text)
{
  waddstr(pw->popwin, text);
  wnoutrefresh(pw->popwin);
  touchwin(pw->borderwin);
  wnoutrefresh(pw->borderwin);
  owl_global_set_needrefresh(&g);
  return(0);
}	      

int owl_popwin_close(owl_popwin *pw)
{
  delwin(pw->popwin);
  delwin(pw->borderwin);
  pw->active=0;
  owl_global_set_needrefresh(&g);
  owl_mainwin_redisplay(owl_global_get_mainwin(&g));
  owl_function_full_redisplay(&g);
  return(0);
}

int owl_popwin_is_active(owl_popwin *pw)
{
  if (pw->active==1) return(1);
  return(0);
}

/* this will refresh the border as well as the text area */
int owl_popwin_refresh(owl_popwin *pw)
{
  touchwin(pw->borderwin);
  touchwin(pw->popwin);

  wnoutrefresh(pw->borderwin);
  wnoutrefresh(pw->popwin);
  owl_global_set_needrefresh(&g);
  return(0);
}

void owl_popwin_set_handler(owl_popwin *pw, void (*func)(int ch))
{
  pw->handler=func;
}

void owl_popwin_unset_handler(owl_popwin *pw)
{
  pw->handler=NULL;
}

WINDOW *owl_popwin_get_curswin(owl_popwin *pw)
{
  return(pw->popwin);
}

int owl_popwin_get_lines(owl_popwin *pw)
{
  return(pw->lines-2);
}

int owl_popwin_get_cols(owl_popwin *pw)
{
  return(pw->cols-2);
}

int owl_popwin_needs_first_refresh(owl_popwin *pw)
{
  if (pw->needsfirstrefresh) return(1);
  return(0);
}

void owl_popwin_no_needs_first_refresh(owl_popwin *pw)
{
  pw->needsfirstrefresh=0;
}
