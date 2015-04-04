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

static void
ratings_set_to	( double general_average	
				, const struct PLAYERS *pPlayers
				, struct RATINGS *pRA /*@out@*/
);

//----------------------------------------------------------------

void
get_a_simulated_run	( int 					limit
					, bool_t 				quiet_mode
					, double 				beta
					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result

					, const struct RATINGS 			*pRA
					, const struct prior 			*PP_ori			
					, const struct rel_prior_set	*pRPset_ori 	

					, struct ENCOUNTERS 	*pEncounters 	// output
					, struct PLAYERS 		*pPlayers 		// output
					, struct GAMES 			*pGames			// output
					, struct prior 			*PP				// output
					, struct rel_prior_set	*pRPset 		// output
)
{
	int failed_sim = 0;

	relpriors_copy (pRPset_ori, pRPset); 		// reload original
	priors_copy    (PP_ori, pPlayers->n, PP); 	// reload original

	do {
		if (!quiet_mode && failed_sim > 0) 
			printf("--> Simulation: [Rejected]\n\n");

		players_flags_reset (pPlayers);
		simulate_scores ( pRA->ratingof_results
						, drawrate_evenmatch_result
						, white_advantage_result
						, beta
						, pGames /*out*/);

		relpriors_copy    (pRPset_ori, pRPset); 	// reload original
		relpriors_shuffle (pRPset);					// simulate new
		priors_copy       (PP_ori, pPlayers->n, PP);// reload original
		priors_shuffle    (PP, pPlayers->n);		// simulate new

		assert(players_have_clear_flags(pPlayers));
		calc_encounters__(ENCOUNTERS_FULL, pGames, pPlayers->flagged, pEncounters);

		players_set_priored_info (PP, pRPset, pPlayers);
		if (0 < players_set_super (quiet_mode, pEncounters, pPlayers)) {
			players_purge (quiet_mode, pPlayers);
			calc_encounters__(ENCOUNTERS_NOFLAGGED, pGames, pPlayers->flagged, pEncounters);
		}

	} while (failed_sim++ < limit && group_is_problematic (pEncounters, pPlayers));

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

//========================================================================

#include "summations.h"
#include "rtngcalc.h"

static void
ratings_set_to	( double general_average	
				, const struct PLAYERS *pPlayers
				, struct RATINGS *pRA /*@out@*/
)
{
		// may improve convergence in pathological cases, it should not be needed.
		ratings_set (pPlayers->n, general_average, pPlayers->prefed, pPlayers->flagged, pRA->ratingof);
		ratings_set (pPlayers->n, general_average, pPlayers->prefed, pPlayers->flagged, pRA->ratingbk);
		assert(ratings_sanity (pPlayers->n, pRA->ratingof));
		assert(ratings_sanity (pPlayers->n, pRA->ratingbk));
}

void
simul		( long 						simulate
			, bool_t 					sim_updates
			, bool_t 					quiet_mode
			, bool_t					prior_mode
			, bool_t 					adjust_white_advantage
			, bool_t 					adjust_draw_rate
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

			, double 					drawrate_evenmatch_result
			, double 					white_advantage_result
			, struct summations *		p_sfe_io

			, struct rel_prior_set 		RPset_store
			, struct prior *			PP_store

)
{
	double 					white_advantage = white_advantage_result;
	double 					drawrate_evenmatch = drawrate_evenmatch_result;

	struct summations 		sfe = *p_sfe_io; 	// summations for errors

	struct ENCOUNTERS		Encounters = *encount;
	struct rel_prior_set 	RPset = *rps;
	struct PLAYERS 			Players = *plyrs;
	struct RATINGS 			RA = *rat;
	struct GAMES 			Games = *pGames;

	struct prior *			PP = pPrior;

	long 					z;
	double 					n = (double) (simulate);
	ptrdiff_t 				topn = (ptrdiff_t)Players.n;

	double 					fraction = 0.0;
	double 					asterisk = n/50.0;
	int 					astcount = 0;

	assert (simulate > 1);
	if (simulate <= 1) return;

	/* Simulation block, begin */

	if(!summations_calloc(&sfe, Players.n)) {
		fprintf(stderr, "Memory for simulations could not be allocated\n");
		exit(EXIT_FAILURE);
	}

	// original run
	sfe.wa_sum1 += white_advantage_result;
	sfe.wa_sum2 += white_advantage_result * white_advantage_result;				
	sfe.dr_sum1 += drawrate_evenmatch_result;
	sfe.dr_sum2 += drawrate_evenmatch_result * drawrate_evenmatch_result;

	if (sim_updates) {
		printf ("0   10   20   30   40   50   60   70   80   90   100 (%s)\n","%");
		printf ("|----|----|----|----|----|----|----|----|----|----|\n");
	}

	for (z = 0; z < simulate; z++) {
		if (!quiet_mode) {		
			printf ("\n==> Simulation:%ld/%ld\n", z+1, simulate);
		} 

		if (sim_updates) {
			fraction += 1.0;
			while (fraction > asterisk) {
				fraction -= asterisk;
				astcount++;
				printf ("*"); 
			}
			fflush(stdout);
		}

		// store originals
		relpriors_copy (&RPset, &RPset_store); 
		priors_copy (PP, Players.n, PP_store);

		get_a_simulated_run	( 100
							, quiet_mode
							, beta
							, drawrate_evenmatch_result
							, white_advantage_result
							, &RA	
							, PP_store			
							, &RPset_store		
							, &Encounters 	// output
							, &Players		// output
							, &Games		// output
							, PP			// output
							, &RPset	 	// output
							);


		#if defined(SAVE_SIMULATION)
		if (z+1 == SAVE_SIMULATION_N) {
			save_simulated(&Players, &Games, (int)(z+1)); 
		}
		#endif

		// may improve convergence in pathological cases, it should not be needed.
		ratings_set_to (general_average, &Players, &RA);

		Encounters.n = calc_rating 
						( quiet_mode
						, prior_mode 
						, adjust_white_advantage
						, adjust_draw_rate
						, anchor_use
						, anchor_err_rel2avg

						, general_average
						, anchor
						, priored_n
						, beta

						, &Encounters
						, &RPset
						, &Players
						, &RA
						, &Games

						, PP
						, wa_prior
						, dr_prior

						, &white_advantage
						, &drawrate_evenmatch
						);

		ratings_cleared_for_purged (&Players, &RA);

		// restore priors. They were shuffled in the simulation and used in the calculation.
		relpriors_copy (&RPset_store, &RPset);
		priors_copy (PP_store, Players.n, PP);

		if (anchor_err_rel2avg) {
			ratings_copy (Players.n, RA.ratingof, RA.ratingbk);	// ** save
			ratings_center_to_zero (Players.n, Players.flagged, RA.ratingof);
		}

		// update summations for errors
		summations_update (&sfe, topn, RA.ratingof, white_advantage, drawrate_evenmatch);

		if (anchor_err_rel2avg) {
			ratings_copy (Players.n, RA.ratingbk, RA.ratingof); // ** restore
		}

	} // for loop end

	if (sim_updates) {
		int x = 51-astcount;
		while (x-->0) {printf ("*"); fflush(stdout);}
		printf ("\n");
	}

	/* use summations to get sdev */
	summations_calc_sdev (&sfe, topn, n);

	/* Simulation block, end */

	*p_sfe_io = sfe;

} /* Simulation function, end */

