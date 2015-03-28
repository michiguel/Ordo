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

#if !defined(H_RA)
#define H_RA
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

extern void	ratings_starting_point (player_t n, double rat0, struct RATINGS *rat /*@out@*/);
extern void	ratings_set (player_t n_players, double general_average, const bool_t *prefed, const bool_t *flagged, double *rating);
extern void	ratings_copy (player_t n, const double *r, double *t);
extern void ratings_results	
				( bool_t anchor_err_rel2avg
				, bool_t anchor_use 
				, player_t anchor
				, double general_average				
				, struct PLAYERS *plyrs
				, struct RATINGS *rat
);
extern void	ratings_cleared_for_purged (const struct PLAYERS *p, struct RATINGS *r /*@out@*/);
extern void ratings_center_to_zero (player_t n_players, const bool_t *flagged, double *ratingof);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
