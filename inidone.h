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


#if !defined(H_INIDONE)
#define H_INIDONE
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"

extern bool_t 	ratings_init (player_t n, struct RATINGS *r); 

extern void 	ratings_done (struct RATINGS *r);

extern bool_t 	games_init (gamesnum_t n, struct GAMES *g);

extern void 	games_done (struct GAMES *g);

extern bool_t 	encounters_init (gamesnum_t n, struct ENCOUNTERS *e);

extern void 	encounters_done (struct ENCOUNTERS *e);

extern bool_t 	players_init (player_t n, struct PLAYERS *x);

extern void 	players_done (struct PLAYERS *x);

extern bool_t 	supporting_auxmem_init 	
						( player_t nplayers
						, double **pSum1
						, double **pSum2
						, double **pSdev
						, struct prior **pPP
						, struct prior **pPP_store
						);
extern void 	supporting_auxmem_done 	
						( double **pSum1
						, double **pSum2
						, double **pSdev
						, struct prior **pPP
						, struct prior **pPP_store);

extern bool_t 	summations_init (struct summations *sm, player_t nplayers);

extern void 	summations_done (struct summations *sm);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
