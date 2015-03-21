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

extern gamesnum_t
calc_rating_bayes
			( bool_t 				quiet
			, bool_t 				adjust_white_advantage
			, bool_t				adjust_draw_rate
			, bool_t				anchor_use

			, double				beta
			, double				general_average
			, player_t				anchor
			, player_t				priored_n
			, player_t 				n_relative_anchors

			, struct ENCOUNTERS *	encount
			, struct PLAYERS *		plyrs
			, struct GAMES *		g
			, struct RATINGS *		rat

			, struct prior *		pp
			, struct relprior *		ra
			, struct prior 			wa_prior
			, struct prior			dr_prior

			, double *				pwadv
			, double *				pDraw_date
)
;


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
