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


#if !defined(H_RATB)
#define H_RATB
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"
#include "ordolim.h"

extern size_t
calc_rating_bayes2 	
			( bool_t 		quiet
			, struct ENC *	enc
			, size_t		N_enc

			, size_t		n_players
			, double *		obtained
			, int *			playedby
			, double *		ratingof
			, double *		ratingbk
			, int *			performance_type

			, bool_t *		flagged
			, bool_t *		prefed

			, double		*pwadv
			, double		general_average

			, bool_t		multiple_anchors_present
			, bool_t		anchor_use
			, int			anchor
				
			, struct GAMES *g

			, char *		name[]
			, double		beta

			// different from non bayes calc

			, double *			changing
			, size_t 			n_relative_anchors
			, struct prior *	pp
			, double 			probarray [MAXPLAYERS] [4]
			, struct relprior *	ra
			, bool_t 			some_prior_set
			, struct prior 		wa_prior
			, struct prior 		dr_prior

			, bool_t 			adjust_white_advantage

			, bool_t			adjust_draw_rate
			, double *			pDraw_date
);


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
