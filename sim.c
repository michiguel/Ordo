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

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include "sim.h"
#include "encount.h"
#include "groups.h"
#include "plyrs.h"
#include "ra.h"
#include "relprior.h"
#include "randfast.h"
#include "pgnget.h"
#include "xpect.h"

// Prototypes

static void
simulate_scores ( const double 	*ratingof_results
				, double 		deq
				, double 		wadv
				, double 		beta
				, struct GAMES *g	// output
);

static void
calc_encounters__
				( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENCOUNTERS	*e
) 
{
	e->n = 
	calc_encounters	( selectivity
					, g
					, flagged
					, e->enc);
};

//----------------------------------------------------------------

void
get_a_simulated_run	( int 					limit

					, double 				General_average
					, bool_t 				quiet_mode
					, struct PLAYERS 		Players
					, struct RATINGS 		RA
					, struct ENCOUNTERS 	Encounters

					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result
					, double 				beta
					, struct GAMES 			Games	// output

					, struct prior *		PP
					, struct prior *		PP_store

					, struct rel_prior_set	RPset 
					, struct rel_prior_set	RPset_store 
)
{
	int failed_sim = 0;

	do {
		if (!quiet_mode && failed_sim > 0) 
			printf("--> Simulation: [Rejected]\n\n");

		players_flags_reset (&Players);
		simulate_scores ( RA.ratingof_results
						, drawrate_evenmatch_result
						, white_advantage_result
						, beta
						, &Games /*out*/);

		relpriors_copy (&RPset, &RPset_store); 
		priors_copy (PP, Players.n, PP_store);
		relpriors_shuffle (&RPset);
		priors_shuffle (PP, Players.n);

		// may improve convergence in pathological cases, it should not be needed.
		ratings_set (Players.n, General_average, Players.prefed, Players.flagged, RA.ratingof);
		ratings_set (Players.n, General_average, Players.prefed, Players.flagged, RA.ratingbk);
		assert(ratings_sanity (Players.n, RA.ratingof));
		assert(ratings_sanity (Players.n, RA.ratingbk));

		assert(players_have_clear_flags(&Players));
		calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

		players_set_priored_info (PP, &RPset, &Players);
		if (0 < players_set_super (quiet_mode, &Encounters, &Players)) {
			players_purge (quiet_mode, &Players);
			calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
		}

	} while (failed_sim++ < limit && group_is_problematic (&Encounters, &Players));

	if (!quiet_mode) printf("--> Simulation: [Accepted]\n");
}

/*=== simulation routines ==========================================*/

static int
rand_threeway_wscore(double pwin, double pdraw)
{	
	long z,x,y;
	z = (long)((unsigned)(pwin * (0xffff+1)));
	x = (long)((unsigned)((pwin+pdraw) * (0xffff+1)));
	y = randfast32() & 0xffff;

	if (y < z) {
		return WHITE_WIN;
	} else if (y < x) {
		return RESULT_DRAW;
	} else {
		return BLACK_WIN;		
	}
}


// no globals
static void
simulate_scores ( const double 	*ratingof_results
				, double 		deq
				, double 		wadv
				, double 		beta
				, struct GAMES *g	// output
)
{
	gamesnum_t n_games = g->n;
	struct gamei *gam = g->ga;

	gamesnum_t i;
	player_t w, b;
	const double *rating = ratingof_results;
	double pwin, pdraw, plos;
	assert(deq <= 1 && deq >= 0);

	for (i = 0; i < n_games; i++) {
		if (gam[i].score != DISCARD) {
			w = gam[i].whiteplayer;
			b = gam[i].blackplayer;
			get_pWDL(rating[w] + wadv - rating[b], &pwin, &pdraw, &plos, deq, beta);
			gam[i].score = rand_threeway_wscore(pwin,pdraw);
		}
	}
}

/*==================================================================*/

// This section is to save simulated results for debugging purposes

static const char *Result_string[4] = {"1-0","1/2-1/2","0-1","*"};

void
save_simulated(struct PLAYERS *pPlayers, struct GAMES *pGames, int num)
{
	gamesnum_t i;
	const char *name_w;
	const char *name_b;
	const char *result;
	char filename[256] = "";	
	FILE *fout;

	sprintf (filename, "simulated_%04d.pgn", num);

	printf ("\n--> filename=%s\n\n",filename);

	if (NULL != (fout = fopen (filename, "w"))) {

		for (i = 0; i < pGames->n; i++) {

			int32_t score_i = pGames->ga[i].score;
			player_t wp_i = pGames->ga[i].whiteplayer;
			player_t bp_i = pGames->ga[i].blackplayer;

			if (score_i == DISCARD) continue;
	
			name_w = pPlayers->name [wp_i];
			name_b = pPlayers->name [bp_i];		
			result = Result_string[score_i];

			fprintf(fout,"[White \"%s\"]\n",name_w);
			fprintf(fout,"[Black \"%s\"]\n",name_b);
			fprintf(fout,"[Result \"%s\"]\n",result);
			fprintf(fout,"%s\n\n",result);
		}

		fclose(fout);
	}
}

