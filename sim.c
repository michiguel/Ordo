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
					, bool_t 				quiet_mode

					, double 				General_average
					, double 				beta

					, struct ENCOUNTERS 	*pEncounters
					, struct rel_prior_set	*pRPset 
					, struct PLAYERS 		*pPlayers
					, struct RATINGS 		*pRA
					, struct GAMES 			*pGames	// output
					, struct prior *		PP

					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result

					, struct prior *		PP_store
					, struct rel_prior_set	*pRPset_store 
)
{

struct ENCOUNTERS 		Encounters	= *pEncounters;
struct rel_prior_set	RPset 		= *pRPset;
struct PLAYERS 			Players		= *pPlayers;
struct RATINGS 			RA			= *pRA;
struct GAMES 			Games 		= *pGames;

struct rel_prior_set	RPset_store = *pRPset_store;

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

//========================================================================

#include "summations.h"
#include "rtngcalc.h"

void
simul		( bool_t 					quiet_mode
			, bool_t					prior_mode
			, bool_t 					adjust_white_advantage
			, bool_t 					adjust_draw_rate
			, bool_t					Anchor_use
			, bool_t					Anchor_err_rel2avg

			, double					General_average
			, player_t 					Anchor
			, player_t					priored_n
			, double					BETA

			, struct ENCOUNTERS	*		encount
			, struct rel_prior_set *	rps
			, struct PLAYERS *			plyrs
			, struct RATINGS *			rat
			, struct GAMES *			pGames

			, struct prior *			pPrior
			, struct prior 				Wa_prior
			, struct prior 				Dr_prior

			, double *					pWhite_advantage
			, double *					pDraw_rate

			, long 						Simulate
			, double 					drawrate_evenmatch_result
			, double 					white_advantage_result
			, struct summations *		p_sfe_input
			, bool_t 					sim_updates

			, struct rel_prior_set 		RPset_store
			, struct prior *			PP_store

)
{
struct summations 		sfe = *p_sfe_input; 	// summations for errors
double 					White_advantage = *pWhite_advantage;
double 					Drawrate_evenmatch = *pDraw_rate;

struct ENCOUNTERS		Encounters = *encount;
struct rel_prior_set 	RPset = *rps;
struct PLAYERS 			Players = *plyrs;
struct RATINGS 			RA = *rat;
struct GAMES 			Games = *pGames;

struct prior *			PP = pPrior;


	/* Simulation block, begin */
	if (Simulate > 1) {

		long z = Simulate;
		double n = (double) (Simulate);
		ptrdiff_t topn = (ptrdiff_t)Players.n;

		double fraction = 0.0;
		double asterisk = n/50.0;
		int astcount = 0;

		if(!summations_calloc(&sfe, Players.n)) {
			fprintf(stderr, "Memory for simulations could not be allocated\n");
			exit(EXIT_FAILURE);
		}

		// original run
		sfe.wa_sum1 += White_advantage;
		sfe.wa_sum2 += White_advantage * White_advantage;				
		sfe.dr_sum1 += Drawrate_evenmatch;
		sfe.dr_sum2 += Drawrate_evenmatch * Drawrate_evenmatch;

		if (sim_updates) {
			printf ("0   10   20   30   40   50   60   70   80   90   100 (%s)\n","%");
			printf ("|----|----|----|----|----|----|----|----|----|----|\n");
		}

		assert(z > 1);
		while (z-->0) {
			if (!quiet_mode) {		
				printf ("\n==> Simulation:%ld/%ld\n", Simulate-z, Simulate);
			} 

			if (sim_updates) {
				fraction += 1.0;
				while (fraction > asterisk) {
					fraction -= asterisk;
					astcount++;
					printf ("*"); fflush(stdout);
				}
			}

			get_a_simulated_run	( 100
								, quiet_mode
								, General_average
								, BETA
								, &Encounters // output
								, &RPset 
								, &Players
								, &RA
								, &Games	// output
								, PP
								, drawrate_evenmatch_result
								, white_advantage_result
								, PP_store
								, &RPset_store );

			#if defined(SAVE_SIMULATION)
			if ((Simulate-z) == SAVE_SIMULATION_N) {
				save_simulated(&Players, &Games, (int)(Simulate-z)); 
			}
			#endif

			Encounters.n = calc_rating 
							( quiet_mode
							, prior_mode //Forces_ML || Prior_mode
							, adjust_white_advantage
							, adjust_draw_rate
							, Anchor_use
							, Anchor_err_rel2avg

							, General_average
							, Anchor
							, priored_n
							, BETA

							, &Encounters
							, &RPset
							, &Players
							, &RA
							, &Games

							, PP
							, Wa_prior
							, Dr_prior

							, &White_advantage
							, &Drawrate_evenmatch
							);

			ratings_cleared_for_purged (&Players, &RA);

			relpriors_copy (&RPset_store, &RPset);
			priors_copy (PP_store, Players.n, PP);

			if (Anchor_err_rel2avg) {
				ratings_copy (Players.n, RA.ratingof, RA.ratingbk);	// ** save
				ratings_center_to_zero (Players.n, Players.flagged, RA.ratingof);
			}

			// update summations for errors
			summations_update (&sfe, topn, RA.ratingof, White_advantage, Drawrate_evenmatch);

			if (Anchor_err_rel2avg) {
				ratings_copy (Players.n, RA.ratingbk, RA.ratingof); // ** restore
			}

			if (sim_updates && z == 0) {
				int x = 51-astcount;
				while (x-->0) {printf ("*"); fflush(stdout);}
				printf ("\n");
			}

		} // while

		/* use summations to get sdev */
		summations_calc_sdev (&sfe, topn, n);

	}
	/* Simulation block, end */

} /* Simulation fuction, end */

