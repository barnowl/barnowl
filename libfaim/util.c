/*
 *
 *
 *
 */

#define FAIM_INTERNAL
#include <aim.h>
#include <ctype.h>

faim_export faim_shortfunc int aimutil_putstr(u_char *dest, const char *src, int len)
{
	memcpy(dest, src, len);
	return len;
}

/*
 * Tokenizing functions.  Used to portably replace strtok/sep.
 *   -- DMP.
 *
 */
faim_export int aimutil_tokslen(char *toSearch, int index, char dl)
{
	int curCount = 1;
	char *next;
	char *last;
	int toReturn;

	last = toSearch;
	next = strchr(toSearch, dl);

	while(curCount < index && next != NULL) {
		curCount++;
		last = next + 1;
		next = strchr(last, dl);
	}

	if ((curCount < index) || (next == NULL))
		toReturn = strlen(toSearch) - (curCount - 1);
	else
		toReturn = next - toSearch - (curCount - 1);

	return toReturn;
}

faim_export int aimutil_itemcnt(char *toSearch, char dl)
{
	int curCount;
	char *next;

	curCount = 1;

	next = strchr(toSearch, dl);

	while(next != NULL) {
		curCount++;
		next = strchr(next + 1, dl);
	}

	return curCount;
}

faim_export char *aimutil_itemidx(char *toSearch, int index, char dl)
{
	int curCount;
	char *next;
	char *last;
	char *toReturn;

	curCount = 0;

	last = toSearch;
	next = strchr(toSearch, dl);

	while (curCount < index && next != NULL) {
		curCount++;
		last = next + 1;
		next = strchr(last, dl);
	}

	if (curCount < index) {
		toReturn = malloc(sizeof(char));
		*toReturn = '\0';
	}
	next = strchr(last, dl);

	if (curCount < index) {
		toReturn = malloc(sizeof(char));
		*toReturn = '\0';
	} else {
		if (next == NULL) {
			toReturn = malloc((strlen(last) + 1) * sizeof(char));
			strcpy(toReturn, last);
		} else {
			toReturn = malloc((next - last + 1) * sizeof(char));
			memcpy(toReturn, last, (next - last));
			toReturn[next - last] = '\0';
		}
	}
	return toReturn;
}

/*
* int snlen(const char *)
* 
* This takes a screen name and returns its length without
* spaces.  If there are no spaces in the SN, then the 
* return is equal to that of strlen().
*
*/
faim_export int aim_snlen(const char *sn)
{
	int i = 0;
	const char *curPtr = NULL;

	if (!sn)
		return 0;

	curPtr = sn;
	while ( (*curPtr) != (char) NULL) {
		if ((*curPtr) != ' ')
		i++;
		curPtr++;
	}

	return i;
}

/*
* int sncmp(const char *, const char *)
*
* This takes two screen names and compares them using the rules
* on screen names for AIM/AOL.  Mainly, this means case and space
* insensitivity (all case differences and spacing differences are
* ignored).
*
* Return: 0 if equal
*     non-0 if different
*
*/
faim_export int aim_sncmp(const char *sn1, const char *sn2)
{
	const char *curPtr1 = NULL, *curPtr2 = NULL;

	if (aim_snlen(sn1) != aim_snlen(sn2))
		return 1;

	curPtr1 = sn1;
	curPtr2 = sn2;
	while ( (*curPtr1 != (char) NULL) && (*curPtr2 != (char) NULL) ) {
		if ( (*curPtr1 == ' ') || (*curPtr2 == ' ') ) {
			if (*curPtr1 == ' ')
				curPtr1++;
			if (*curPtr2 == ' ')
				curPtr2++;
		} else {
			if ( toupper(*curPtr1) != toupper(*curPtr2))
				return 1;
			curPtr1++;
			curPtr2++;
		}
	}

	/* Should both be NULL */
	if (*curPtr1 != *curPtr2)
		return 1;

	return 0;
}

/* strsep Copyright (C) 1992, 1993 Free Software Foundation, Inc.
strsep is part of the GNU C Library.

The GNU C Library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The GNU C Library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with the GNU C Library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.  */

/* Minor changes by and1000 on 15/1/97 to make it go under Nemesis */

faim_export char *aim_strsep(char **pp, const char *delim)
{
	char *p, *q;

	if (!(p = *pp))
		return 0;

	if ((q = strpbrk (p, delim))) {
		*pp = q + 1;
		*q = '\0';
	} else
		*pp = 0;

	return p;
}
