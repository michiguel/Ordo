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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "mymem.h"
#include "rating.h"
#include "ratingb.h"
#include "encount.h"
#include "rtngcalc.h"
#include "relprior.h"

extern gamesnum_t
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

{
	double dr = *pDraw_rate;

	gamesnum_t ret;

	if (Prior_mode || ml) {

		ret = calc_rating_bayes2 
				( quiet
				, enc
				, N_enc
				, plyrs
				, rat
				, pWhite_advantage
				, General_average
				, Anchor_use && !Anchor_err_rel2avg
				, Anchor
				, priored_n
				, pGames
				, BETA
				, rps->n
				, PP
				, rps->x
				, some_prior_set
				, Wa_prior
				, Dr_prior
				, adjust_wadv
				, adjust_drate
				, &dr);

	} else {

		double *ratingtmp_memory;

		assert(plyrs->n > 0);
		if (NULL != (ratingtmp_memory = memnew (sizeof(double) * (size_t)plyrs->n))) {

			assert(ratings_sanity (plyrs->n, rat->ratingof)); //%%

			ret = calc_rating2 	
					( quiet
					, enc
					, N_enc
					, plyrs
					, rat
					, pWhite_advantage
					, General_average
					, Anchor_use && !Anchor_err_rel2avg
					, Anchor
					, pGames
					, BETA
					, adjust_wadv
					, adjust_drate
					, &dr
					, ratingtmp_memory);

			memrel(ratingtmp_memory);

		} else {
			fprintf(stderr, "Not enough memory available\n");
			exit(EXIT_FAILURE);
		}	
	}

	*pDraw_rate = dr;

	return ret;
}

