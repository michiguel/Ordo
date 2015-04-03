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


#if !defined(H_SUMMA)
#define H_SUMMA
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"

extern bool_t 	summations_calloc (struct summations *sm, player_t nplayers);

extern void 	summations_init (struct summations *sm);

extern void 	summations_done (struct summations *sm);

extern void		summations_update	
					( struct summations *sm
					, player_t topn
					, double *ratingof
					, double white_advantage
					, double drawrate_evenmatch
					);

extern void		summations_calc_sdev (struct summations *sm, player_t topn, double sim_n);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
