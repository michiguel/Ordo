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

struct GAMEBLOCK {
	int 		white	[MAXGAMESxBLOCK];
	int 		black	[MAXGAMESxBLOCK];
	int 		score	[MAXGAMESxBLOCK];
};

struct DATA {	
	int 		n_players;
	int 		n_games;
	char		labels[LABELBUFFERSIZE];
	ptrdiff_t	labels_end_idx;
	ptrdiff_t	name	[MAXPLAYERS];

	int 		gb_filled;
	int 		gb_idx;
	int			gb_allocated;

	struct GAMEBLOCK *gb[MAXBLOCKS];
};


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
