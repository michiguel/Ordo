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
extern bool_t	ratings_replicate (const struct RATINGS *src, struct RATINGS *tgt);

extern bool_t 	games_init (gamesnum_t n, struct GAMES *g);
extern void 	games_done (struct GAMES *g);
extern bool_t	games_replicate (const struct GAMES *src, struct GAMES *tgt);

extern bool_t 	encounters_init (gamesnum_t n, struct ENCOUNTERS *e);
extern void 	encounters_done (struct ENCOUNTERS *e);
extern void		encounters_copy (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt);
extern bool_t	encounters_replicate (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt);


extern bool_t 	players_init (player_t n, struct PLAYERS *x);
extern void 	players_done (struct PLAYERS *x);
extern bool_t 	players_replicate (const struct PLAYERS *src, struct PLAYERS *tgt);

extern bool_t 	supporting_auxmem_init 	
						( player_t nplayers
						, struct prior **pPP
						, struct prior **pPP_store
						);

extern void 	supporting_auxmem_done 	
						( struct prior **pPP
						, struct prior **pPP_store);


extern struct prior *priorlist_init 	(player_t nplayers);

extern void 	priorlist_done 	(struct prior **pPP);

extern bool_t	priorlist_replicate ( player_t nplayers
									, struct prior *PP
									, struct prior **pQQ);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
