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

#define OWL_TIMER_DIRECTION_COUNTUP    1
#define OWL_TIMER_DIRECTION_COUNTDOWN  2


/* Create a "counting up" timer.  The counter starts running as soon
 * as this is called.  Use owl_timer_reset() to reset it.
 */
void owl_timer_create_countup(owl_timer *t)
{
  t->direction=OWL_TIMER_DIRECTION_COUNTUP;
  t->starttime=time(NULL);
}

/* create a "counting down" timer, which counts down from 'start'
 * seconds.  The counter starts running as soon as this is called.
 * Use owl_timer_reset to reset it.
 */
void owl_timer_create_countdown(owl_timer *t, int start)
{
  t->direction=OWL_TIMER_DIRECTION_COUNTDOWN;
  t->start=start;
  t->starttime=time(NULL);
}

/* Reset the timer.  For a "counting up" timer, it is reset to 0.  For
 * a "counting down" timer it is set to the value initially set with
 * owl_timer_create_countdown() or the last value set with
 * owl_timer_reset_newstart() */
void owl_timer_reset(owl_timer *t)
{
  t->starttime=time(NULL);
}

/* Only for a countdown timer.  Rest the timer, but this time (and on
 * future resets) start with 'start' seconds.
 */
void owl_timer_reset_newstart(owl_timer *t, int start)
{
  t->start=start;
  t->starttime=time(NULL);
}

/* Return the number of seconds elapsed or remaining.  If using a
 * countdown timer, a negative value is never reported.  Once the
 * timer gets to 0 it stays at 0 */
int owl_timer_get_time(owl_timer *t)
{
  time_t now;
  int rem;

  now=time(NULL);

  if (t->direction==OWL_TIMER_DIRECTION_COUNTUP) {
    return(now-t->starttime);
  } else if (t->direction==OWL_TIMER_DIRECTION_COUNTDOWN) {
    rem=t->start-(now-t->starttime);
    if (rem<0) return(0);
    return(rem);
  }

  /* never reached */
  return(0);
}

/* Only for countdown timer.  Return true if time has run out (the
 * timer is at 0 seconds or less
 */
int owl_timer_is_expired(owl_timer *t)
{
  if (owl_timer_get_time(t)==0) return(1);
  return(0);
}
