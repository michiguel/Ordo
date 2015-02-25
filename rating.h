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


#if !defined(H_RATNG)
#define H_RATNG
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

gamesnum_t
calc_rating2 	( bool_t 		quiet
				, struct ENC *	enc
				, gamesnum_t	N_enc
				, struct PLAYERS *plyrs
				, struct RATINGS *rat

				, double		*pWhite_advantage
				, double		General_average

				, bool_t		Anchor_use
				, int			Anchor
				
				, struct GAMES *g

				, double		BETA
//
				, bool_t 		adjust_white_advantage

				, bool_t		adjust_draw_rate
				, double		*pDraw_date
				, double		*ratingtmp_buffer
);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
