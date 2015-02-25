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


#if !defined(H_RPMAN)
#define H_RPMAN
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"
#include "ordolim.h"

typedef struct relprior rpunit_t;

struct relprior_block;

struct relprior_block {
	rpunit_t 				u[MAX_RPBLOCK];
	size_t 					n;
	size_t					sz;
	struct relprior_block *	next;
};

typedef struct relprior_block rpblock_t;

struct rpmanager {
	rpblock_t *	first;
	rpblock_t *	last;
	rpblock_t *	p;
	size_t 		i;
};

extern bool_t		rpman_init (struct rpmanager *rm);
extern void			rpman_done (struct rpmanager *rm);
extern size_t		rpman_count(struct rpmanager *rm);
extern void			rpman_parkstart(struct rpmanager *rm);
extern rpunit_t *	rpman_pointnext_unit(struct rpmanager *rm);
extern bool_t 		rpman_add_unit(struct rpmanager *rm, rpunit_t *u);
extern rpunit_t *	rpman_to_newarray (struct rpmanager *rm, player_t *psz);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
