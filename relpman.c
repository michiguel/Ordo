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


#include <assert.h>
#include <stdio.h>
#include "mymem.h"

#include "mytypes.h"
#include "relpman.h"


//================================================

static void
rpblock_clear (rpblock_t *r)
{
	assert (r);
	r->n = 0;
	r->sz = MAX_RPBLOCK;
	r->next = NULL;
}

static rpblock_t *
rpblock_create(void)
{
	rpblock_t *q = memnew (sizeof(rpblock_t));
	if (q) rpblock_clear(q);
	return q;
}

static void
rpblock_destroy(rpblock_t *p)
{
	assert(p);
	rpblock_clear(p);
	memrel(p);
	return;
}

static bool_t
rpblock_add (rpblock_t *b, rpunit_t *u)
{
	rpblock_t *q;
	rpblock_t *t;

	assert(b);
	assert(u);

	q = b;
	assert(q->n <= q->sz);

	if (q->n == q->sz) {
		t = rpblock_create();
		if (t) {
				q->next = t;
				q = t;
				q->n = 0;
		} else {
			return FALSE;
		}
	}

	q->u[q->n++] = *u;

	return TRUE;
}

//------------------ Relative Priors Manager Functions ----------------------------

bool_t
rpman_init (struct rpmanager *rm)
{
	rpblock_t *q = rpblock_create();
	assert(rm);
	rm->first = q;
	rm->last = q;
	rm->p = NULL;
	rm->i = 0;
	return q != NULL;
}

void
rpman_done (struct rpmanager *rm)
{
	rpblock_t *p;
	rpblock_t *q;
	assert(rm);
	p = rm->first;
	rm->first = NULL;
	rm->last  = NULL;
	rm->p     = NULL;
	rm->i 	  = 0;
	while(p) {
		q = p->next;
		rpblock_destroy(p);
		p = q;
	}
	return;
}

size_t
rpman_count(struct rpmanager *rm)
{
	rpblock_t *p;
	size_t count = 0;
	assert(rm);
	for (p = rm->first; p != NULL; p = p->next) {
			count += p->n;
	}
	return count;
}

void
rpman_parkstart(struct rpmanager *rm)
{
	assert(rm);
	rm->i = 0;
	rm->p = rm->first;
}

rpunit_t *
rpman_pointnext_unit(struct rpmanager *rm)
{
	size_t i;
	rpblock_t *p;
	rpunit_t *pu;

	assert(rm);
	p = rm->p;
	i = rm->i;
	assert(i <= p->n);
	
	while (i == p->n) {
		p = p->next;
		if (NULL == p) {
			return NULL;
		}
		i = 0;
	}

	pu = &p->u[i++];

	rm->p = p;
	rm->i = i;

	return pu;
}


bool_t
rpman_add_unit(struct rpmanager *rm, rpunit_t *u)
{
	bool_t ok;
	rpblock_t *p;

	assert(rm);
	p = rm->last;
	assert(p);

	ok = rpblock_add (p, u);
	if (ok && NULL != p->next) rm->last = p->next; // next was created
	return ok;
}


rpunit_t *
rpman_to_newarray (struct rpmanager *rm, player_t *psz)
{
	rpunit_t *p_ret;
	rpunit_t *p;
	rpunit_t *pu;
	player_t sz = (player_t)rpman_count (rm);

	p = memnew(sizeof(rpunit_t) * (size_t) sz);
	p_ret = p;

	if (p == NULL) return p;

	rpman_parkstart (rm);
	while (NULL != (pu = rpman_pointnext_unit(rm))) {
		*p++ = *pu;
	}

	*psz = sz;
	return p_ret;
}

