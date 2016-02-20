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
#include <string.h>
#include <assert.h>

#include "strlist.h"

bool_t
strlist_init (strlist_t *sl)
{
	strnode_t *ph = &(sl->prehead);
	ph->str = NULL;
	ph->nxt = NULL;
	sl->curr = NULL;
	sl->last = ph;
	return TRUE;
}

static strnode_t *
newnode(void)
{
	strnode_t *p = malloc(sizeof(strnode_t));
	return p;
}

static char *
string_dup (const char *s)
{
	char *p;
	char *q;
	size_t len = strlen(s);
	p = malloc (len + 1);
	if (p == NULL) return NULL;
	q = p;
	while (*s) *p++ = *s++;
	*p = '\0';
	return q;
}

static void
string_free (char *s)
{
	free(s);
}


bool_t
strlist_push (strlist_t *sl, const char *s)
{
	strnode_t *nn;
	char *ns;
	bool_t ok;

	if (NULL != (nn = newnode())) {
		if (NULL != (ns = string_dup (s))) {
			ok = TRUE;
		} else {
			free(nn);
			ok = FALSE;
		}
	} else {
		ok = FALSE;
	}

	if (ok) {
		nn->str = ns;
		nn->nxt = NULL;
		sl->last->nxt = nn;
		sl->last = nn;
	}

	return ok;
}

static void
freenode(strnode_t *n)
{
	if (n) {
		if (n->str) string_free(n->str);
		free(n);
	}
}

void
strlist_done (strlist_t *sl)
{
	strnode_t *ph;
	strnode_t *f;

	assert(sl);

	ph = &(sl->prehead);
	f = ph->nxt;
	while (f) {
		ph->nxt = f->nxt;
		freenode(f);
		f = ph->nxt;
	}
	sl->curr = NULL;
	sl->last = ph;
}


void
strlist_rwnd (strlist_t *sl)
{
	if (sl)	sl->curr = sl->prehead.nxt;
}


const char *
strlist_next (strlist_t *sl)
{
	char *rstr = NULL;
	if (NULL != sl && NULL != sl->curr) {
		rstr = sl->curr->str;
		sl->curr = sl->curr->nxt;
	}
	return rstr;
}

