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


#if !defined(H_INDIV)
#define H_INDIV
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

//static double calc_ind_rating(double cume_score, double *rtng, double *weig, int r);

//static double calc_ind_rating_superplayer (int perf_type, double x_estimated, double *rtng, double *weig, int r);

extern void
rate_super_players 	( bool_t quiet
					, struct ENC *enc
					, gamesnum_t N_enc
					, int *performance_type
					, player_t n_players
					, double *ratingof
					, double white_advantage
					, bool_t *flagged
					, const char *Name[]
					, double deq
					, double beta
);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
