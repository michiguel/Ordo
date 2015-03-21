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


#if !defined(H_RTNGC)
#define H_RTNGC
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

gamesnum_t
calc_rating ( bool_t 					quiet
			, bool_t					prior_mode
			, bool_t 					adjust_wadv
			, bool_t 					adjust_drate
			, bool_t					anchor_use
			, bool_t					anchor_err_rel2avg

			, double					general_average
			, player_t 					anchor
			, player_t					priored_n
			, double					beta

			, struct ENCOUNTERS	*		encount
			, struct rel_prior_set *	rps
			, struct PLAYERS *			plyrs
			, struct RATINGS *			rat
			, struct GAMES *			pGames

			, struct prior *			pPrior
			, struct prior 				wa_prior
			, struct prior 				dr_prior

			, double *					pWhite_advantage
			, double *					pDraw_rate
)
;


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
