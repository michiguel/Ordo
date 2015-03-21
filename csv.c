/*
	Ordo is program for calculating ratings of engine or chess players
    Copyright 2013 Miguel A. Ballicora

    This file is part of Ordo.

    Ordo is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ordo is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ordo.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "boolean.h"
#include "csv.h"
#include "mymem.h"

#define MAXSIZETOKEN MAXSIZE_CSVLINE

static bool_t issep(int c) { return c == ',';}
static bool_t isquote(int c) { return c == '"';}
static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}

//===================================================================

static void
rm_blank_tails(char *s)
{
	char *t = s + strlen(s);
	while (t-- > s && isspace(*t))
		*t = '\0';
}

static char *
csv_gettoken(char *p, char *s, size_t max)
{
	char *s_ori;
	s_ori = s;

	if (*p == '\0') return NULL; // no more tokens

	p = skipblanks(p);

	if (issep(*p)) {
		s[0] = '\0'; //return empty string
		return ++p;
	}

	if (isquote(*p)) {
		p++;
		while (*p != '\0' && s < max + s_ori && !isquote(*p)) {*s++ = *p++;}
		*s = '\0';
		if (isquote(*p++)) {
			p = skipblanks(p);
			if (issep(*p)) p++;
		}
		return p;
	}

	while (*p != '\0' && s < max + s_ori && !issep(*p)) {*s++ = *p++;}
	*s = '\0';

	rm_blank_tails(s_ori);

	if (issep(*p)) p++;
	
	return p;
}


bool_t
csv_line_init(csv_line_t *c, char *p)
{
	char *s;
	bool_t success;

	assert(c != NULL);

	c->n = 0;	
	c->s[0] = NULL;
	c->mem = memnew(MAXSIZE_CSVLINE);

	if (NULL != c->mem) {

		s = c->mem;
		p = skipblanks(p);
		success = (NULL != (p = csv_gettoken(p, s, MAXSIZETOKEN)));

		while (success) {
			p = skipblanks(p);
			c->s[c->n++] = s;
			s += strlen(s) + 1;
			success = (NULL != (p = csv_gettoken(p, s, MAXSIZETOKEN)));
		}
	}
	return c->mem != NULL;
}

void
csv_line_done(csv_line_t *c)
{
	assert(c != NULL);
	assert(c->mem != NULL);

	if (c->mem)	memrel(c->mem);
	c->n = 0;
	c->mem = NULL;
	c->s[0] = NULL;
}



