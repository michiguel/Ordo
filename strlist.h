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


#if !defined(H_STRLIST)
#define H_STRLIST
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"

struct STRNODE;

struct STRNODE {
	char *str;
	struct STRNODE *nxt;	
};

typedef struct STRNODE strnode_t;

struct STRLIST {
	strnode_t prehead;
	strnode_t *curr;
	strnode_t *last;
};

typedef struct STRLIST strlist_t;

//-------------------------------------------------

extern bool_t 		strlist_init (strlist_t *sl);
extern void 		strlist_done (strlist_t *sl);
extern bool_t 		strlist_push (strlist_t *sl, const char *s);
extern void 		strlist_rwnd (strlist_t *sl);
extern const char *	strlist_next (strlist_t *sl);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
