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


#if !defined(H_PGNGET)
#define H_PGNGET
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "ordolim.h"
#include "datatype.h"
#include "strlist.h"
#include "bitarray.h"

enum RESULTS {
	WHITE_WIN = 0,
	RESULT_DRAW = 1,
	BLACK_WIN = 2,
	DISCARD = 3
};

extern struct DATA *database_init_frompgn (strlist_t *sl, const char *synfile_name, bool_t quiet);
extern void 		database_done (struct DATA *p);

#include "mytypes.h"

extern void 		database_transform(const struct DATA *db, struct GAMES *g, struct PLAYERS *p, struct GAMESTATS *gs);
extern void 		database_ignore_draws (struct DATA *db);
extern const char *	database_getname (const struct DATA *db, player_t i);
extern void 		database_include_only (struct DATA *db, bitarray_t *pba);

extern void 		namelist_to_bitarray (bool_t quietmode, const char *finp_name, const struct DATA *d, bitarray_t *pba);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
