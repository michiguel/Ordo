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


#if !defined(H_DATATY)
#define H_DATATY
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stddef.h>
#include "ordolim.h"
#include "mytypes.h"

struct NAMEBLOCK {
	const char *p[MAXNAMESxBLOCK];
};

struct GAMEBLOCK {
	player_t	white	[MAXGAMESxBLOCK];
	player_t	black	[MAXGAMESxBLOCK];
	int32_t		score	[MAXGAMESxBLOCK];
};

struct NAMENODE {
	char *			buf;
	struct NAMENODE *nxt;
	size_t			idx;
};

typedef struct NAMENODE namenode_t;

struct DATA {	
	player_t	n_players;
	gamesnum_t	n_games;

	namenode_t 	labels_head;
	namenode_t 	*curr;

	size_t 		nm_filled;
	size_t 		nm_idx;
	size_t		nm_allocated;

	struct NAMEBLOCK *nm[MAXBLOCKS];

	size_t 		gb_filled;
	size_t 		gb_idx;
	size_t		gb_allocated;

	struct GAMEBLOCK *gb[MAXBLOCKS];
};


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
