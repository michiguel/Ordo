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
calc_rating ( bool_t 					quiet
			, bool_t 					ml
			, bool_t 					adjust_wadv
			, bool_t 					adjust_drate
			, bool_t					prior_mode
			, bool_t					anchor_use
			, bool_t					anchor_err_rel2avg
			, bool_t 					some_prior_set

			, struct ENC *				enc
			, gamesnum_t 				n_enc
			, double *					pWhite_advantage
			, double *					pDraw_rate
			, struct rel_prior_set *	rps
			, struct PLAYERS *			plyrs
			, struct RATINGS *			rat
			, struct GAMES *			pGames
			//
			, double					general_average
			, player_t 					anchor
			, player_t					priored_n
			, double					beta
			, struct prior *			pPrior
			, struct prior 				wa_prior
			, struct prior 				dr_prior
)

{
	double dr = *pDraw_rate;

	gamesnum_t ret;

	if (prior_mode || ml) {

		ret = calc_rating_bayes 
				( quiet
				, enc
				, n_enc
				, plyrs
				, rat
				, pWhite_advantage
				, general_average
				, anchor_use && !anchor_err_rel2avg
				, anchor
				, priored_n
				, pGames
				, beta
				, rps->n
				, pPrior
				, rps->x
				, some_prior_set
				, wa_prior
				, dr_prior
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
					, n_enc
					, plyrs
					, rat
					, pWhite_advantage
					, general_average
					, anchor_use && !anchor_err_rel2avg
					, anchor
					, pGames
					, beta
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

