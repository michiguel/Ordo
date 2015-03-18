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
calc_rating ( bool_t quiet
			, bool_t ml
			, struct ENC *enc, gamesnum_t N_enc
			, double *pWhite_advantage
			, bool_t adjust_wadv
			, bool_t adjust_drate
			, double *pDraw_rate
			, struct rel_prior_set *rps
			, struct PLAYERS *plyrs
			, struct RATINGS *rat
			, struct GAMES *pGames
			//
			, bool_t	Prior_mode
			, double	General_average
			, bool_t	Anchor_use
			, bool_t	Anchor_err_rel2avg
			, player_t 	Anchor
			, int 		priored_n
			, bool_t 	some_prior_set
			, double	BETA
			, struct prior *PP
			, struct prior Wa_prior
			, struct prior Dr_prior
)
;


/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
