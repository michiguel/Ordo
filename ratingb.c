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
#include <math.h>
#include <assert.h>

#include "ratingb.h"
#include "encount.h"
#include "indiv.h"
#include "xpect.h"
#include "mymem.h"

#define MIN_RESOLUTION           0.000001
#define MIN_DRAW_RATE_RESOLUTION 0.00001
#define PRIOR_SMALLEST_SIGMA     0.0000001

#if !defined(NDEBUG)
static bool_t is_nan (double x) {if (x != x) return TRUE; else return FALSE;}
#endif

// ================= Bayes concept 

// no globals
static double
relative_anchors_unfitness_full(player_t n_relative_anchors, const struct relprior *ra, const double *ratingof)
{
	player_t a, b;
	player_t i;
	double d, x;
	double accum = 0;
	for (i = 0; i < n_relative_anchors; i++) {
		a = ra[i].player_a;
		b = ra[i].player_b;
		d = ratingof[a] - ratingof[b];
		x = (d - ra[i].delta)/ra[i].sigma;
		accum += 0.5 * x * x;
	}

	return accum;
}

// no globals
static double
relative_anchors_unfitness_j(double R, player_t j, double *ratingof, player_t n_relative_anchors, struct relprior *ra)
{
	player_t a, b;
	player_t i;
	double d, x;
	double accum = 0;
	double rem;

	rem = ratingof[j];
	ratingof[j] = R;

	for (i = 0; i < n_relative_anchors; i++) {
		a = ra[i].player_a; 
		b = ra[i].player_b; 
		if (a == j || b == j) {
			d = ratingof[a] - ratingof[b];
			x = (d - ra[i].delta)/ra[i].sigma;
			accum += 0.5 * x * x;
		}
	}

	ratingof[j] = rem;
	return accum;
}


// no globals
static void
adjust_rating_byanchor (player_t anchor, double general_average, player_t n_players, double *ratingof)
{
	player_t j;
	double excess = ratingof[anchor] - general_average;	
	for (j = 0; j < n_players; j++) {
		ratingof[j] -= excess;
	}	
}

// no globals
static void
ratings_restore (player_t n_players, const double *r_bk, double *r_of)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		r_of[j] = r_bk[j];
	}	
}

// no globals
static void
ratings_backup (player_t n_players, const double *r_of, double *r_bk)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		r_bk[j] = r_of[j];
	}	
}

//

// no globals
static bool_t
super_players_present(player_t n_players, int *performance_type)
{ 
	bool_t found = FALSE;
	player_t j;
	for (j = 0; j < n_players && !found; j++) {
		found = performance_type[j] == PERF_SUPERWINNER || performance_type[j] == PERF_SUPERLOSER; 
	}
	return found;
}

static double
adjust_wadv_bayes 
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double deq
				, struct prior dr_prior
				, double beta
);


static double
adjust_drawrate_bayes 
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double deq
				, struct prior dr_prior
				, double beta
);


// no globals
static void
derivative_vector_calc 	( double delta
						, gamesnum_t n_encounters
						, const struct ENC *enc
						, double deq
						, double beta
						, player_t n_players
						, double *ratingof
						, bool_t *flagged
						, bool_t *prefed
						, double white_advantage
		 				, const struct prior *pp
						, player_t n_relative_anchors
						, struct relprior *ra
						, double *probarray
						, double *vector 
);

// no globals
static double
calc_bayes_unfitness_full	
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double deq
				, struct prior dr_prior
				, double beta
);

// no globals
static double
adjust_rating_bayes 
		( double 					delta
		, bool_t 					multiple_anchors_present
		, bool_t 					some_prior_has_been_set
		, bool_t 					anchor_use
		, player_t 					anchor
		, double 					general_average 
		, player_t 					n_players 
		, const struct prior *		p
		, double 					white_advantage
		, struct 					prior wa_prior
		, double 					deq
		, struct prior 				dr_prior
		, player_t 					n_relative_anchors
		, const struct relprior *	ra
		, const double *			change_vector
		, const bool_t *			flagged
		, const bool_t *			prefed
		, double *					ratingof // out 
		, double *					ratingbk // out 
)
;

// no globals
gamesnum_t
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
{
	gamesnum_t  n_games = g->n;
	double 		olddev, curdev, outputdev;
	int 		i;
	int			rounds = 10000;
	double 		rtng_76 = (-log(1.0/0.76-1.0))/beta;
	double 		delta = rtng_76; //should be proportional to the scale
	double 		denom = 3;
	int 		phase = 0;
	int 		n = 40;
	double 		resol = delta;
	double  	resol_dr = 0.1;
	double		deq = *pDraw_date;
	double 		white_advantage = *pwadv;
	double *	probarr;

	// translation variables for refactoring ------------------
	struct ENC *	enc 			= encount->enc;
	gamesnum_t		n_enc 			= encount->n;
	player_t		n_players 		= plyrs->n;
	int *			performance_type= plyrs->performance_type;
	bool_t *		flagged 		= plyrs->flagged;
	bool_t *		prefed  		= plyrs->prefed;
	const char **	name 			= plyrs->name;
	double *		obtained 		= rat->obtained;
	gamesnum_t *	playedby 		= rat->playedby;
	double *		ratingof 		= rat->ratingof;
	double *		ratingbk 		= rat->ratingbk;
	double *		changing 		= rat->changing;
	player_t		anchored_n 		= plyrs->anchored_n;
	bool_t			multiple_anchors_present = anchored_n > 1; 
	//----------------------------------------------------------

	probarr = memnew (sizeof(double) * (size_t)n_players * 4);

	if (NULL == probarr) {
		fprintf(stderr,"Not enough memory to initialize probability arrays\n");
		exit(EXIT_FAILURE);
	}

	assert(deq <= 1 && deq >= 0);

	// initial deviation
	olddev = curdev = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, deq
							, dr_prior
							, beta);

	if (!quiet) printf ("Converging...\n\n");
	if (!quiet) printf ("%3s %4s %10s %10s\n", "phase", "iteration", "unfitness","resolution");

	while (n-->0 && resol >= MIN_RESOLUTION) {

		for (i = 0; i < rounds && resol >= MIN_RESOLUTION; i++) {
			ratings_backup  (n_players, ratingof, ratingbk);
			olddev = curdev;

			// Calc "changing" vector
			derivative_vector_calc
						( delta
						, n_enc
						, enc
						, deq
						, beta
						, n_players
						, ratingof
						, flagged
						, prefed
						, white_advantage
						, pp
						, n_relative_anchors
						, ra
						, probarr
						, changing );

			resol = adjust_rating_bayes 
						( delta
						, multiple_anchors_present
						, priored_n > 0 // some_prior_set
						, anchor_use
						, anchor
						, general_average 
						, n_players 
						, pp
						, white_advantage
						, wa_prior
						, deq
						, dr_prior
						, n_relative_anchors
						, ra
						, changing
						, flagged
						, prefed
						, ratingof // out 
						, ratingbk // out 
					);

			curdev = calc_bayes_unfitness_full	
						( n_enc
						, enc
						, n_players
						, pp
						, white_advantage
						, wa_prior
						, n_relative_anchors
						, ra
						, ratingof
						, deq
						, dr_prior
						, beta);

			if (curdev < olddev) {
				ratings_backup  (n_players, ratingof, ratingbk);
				olddev = curdev;
			} else {
				ratings_restore (n_players, ratingbk, ratingof);
				curdev = olddev;
				break;
			}
		}

		delta /=  denom;
		outputdev = curdev/(double)n_games;

		if (!quiet) {
			printf ("%3d %7d %14.5f", phase, i, outputdev);
			printf ("%11.5f",resol);
			printf ("\n");
		}
		phase++;

		if (adjust_white_advantage) {
			white_advantage = adjust_wadv_bayes 
							( n_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, resol
							, deq
							, dr_prior
							, beta);

			*pwadv = white_advantage;
		}

		if (adjust_draw_rate) {
			double deqx;
			deqx = adjust_drawrate_bayes 
							( n_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, resol_dr
							, deq
							, dr_prior
							, beta);

			resol_dr = deqx > deq? deqx - deq: deq - deqx;
			deq = deqx;
		}
	}

	if (!quiet) {
		printf ("done\n");
		printf ("\nWhite Advantage = %.1f", white_advantage);
		printf ("\nDraw Rate (eq.) = %.1f %s\n\n", 100*deq, "%");
	}
	if (!quiet && super_players_present(n_players, performance_type)) 
		printf ("Post-Convergence rating estimation for all-wins / all-losses players\n\n");

//	n_enc = calc_encounters(ENCOUNTERS_FULL, g, flagged, enc);

	encounters_calculate(ENCOUNTERS_FULL, g, flagged, encount);
	enc   = encount->enc;
	n_enc = encount->n;

	calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);
	rate_super_players(quiet, enc, n_enc, performance_type, n_players, ratingof, white_advantage, flagged, name, deq, beta); 
//	n_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, g, flagged, enc);

	encounters_calculate(ENCOUNTERS_NOFLAGGED, g, flagged, encount);
	enc   = encount->enc;
	n_enc = encount->n;

	calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby); 

	if (anchored_n == 1 && priored_n == 0)
		adjust_rating_byanchor (anchor, general_average, n_players, ratingof);

	*pDraw_date = deq;
	*pwadv = white_advantage;

	memrel(probarr);

	return n_enc;
}


static double
wdl_probabilities (gamesnum_t ww, gamesnum_t dd, gamesnum_t ll, double pw, double pd, double pl)
{
	return 	 	(ww > 0? (double)ww * log(pw) : 0) 
			+ 	(dd > 0? (double)dd * log(pd) : 0) 
			+ 	(ll > 0? (double)ll * log(pl) : 0)
			;
}

// no globals
static double
prior_unfitness	( player_t n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double deq
				, struct prior dr_prior
)
{
	player_t j;
	double x;
	double accum = 0;
	for (j = 0; j < n_players; j++) {
		if (p[j].isset) {
			x = (ratingof[j] - p[j].value)/p[j].sigma;
			accum += 0.5 * x * x;
		}
	}

	if (wa_prior.isset) {
		x = (wadv - wa_prior.value)/wa_prior.sigma;
		accum += 0.5 * x * x;		
	}

	if (dr_prior.isset) {
		x = (deq - dr_prior.value)/dr_prior.sigma;
		accum += 0.5 * x * x;		
	}

	//FIXME this could be slow!
	accum += relative_anchors_unfitness_full (n_relative_anchors, ra, ratingof); 

	return accum;
}

// no globals
static double
calc_bayes_unfitness_full	
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double deq
				, struct prior dr_prior
				, double beta
)
{
	double pw, pd, pl, accum;
	player_t w, b;
	gamesnum_t ww,dd,ll;
	gamesnum_t e;

	assert(deq <= 1 && deq >= 0);

	for (accum = 0, e = 0; e < n_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		get_pWDL(ratingof[w] + wadv - ratingof[b], &pw, &pd, &pl, deq, beta);

		ww = enc[e].W;
		dd = enc[e].D;
		ll = enc[e].L;

		accum 	+= 	(ww > 0? (double)ww * log(pw) : 0) 
				+ 	(dd > 0? (double)dd * log(pd) : 0) 
				+ 	(ll > 0? (double)ll * log(pl) : 0)
		;
	}
	
	assert(!is_nan(accum));

	// Priors
	accum += -prior_unfitness
				( n_players
				, p
				, wadv
				, wa_prior
				, n_relative_anchors
				, ra
				, ratingof
				, deq
				, dr_prior
				);

	assert(!is_nan(accum));

	return -accum;
}

//============================================

// no globals
static double
get_extra_unfitness_j (double R, player_t j, const struct prior *p, double *ratingof, player_t n_relative_anchors, struct relprior *ra)
{
	double x;
	double u = 0;
	if (p[j].isset) {
		x = (R - p[j].value)/p[j].sigma;
		u = 0.5 * x * x;
	} 

	//FIXME this could be slow!
	u += relative_anchors_unfitness_j(R, j, ratingof, n_relative_anchors, ra); 

	return u;
}

// no globals
static void
probarray_reset(player_t n_players, double *probarray)
{
	player_t j, k;
	for (j = 0; j < n_players; j++) {
		for (k = 0; k < 4; k++) {
			probarray[(j<<2)|k] = 0;
		}	
	}
}

// no globals
static void
probarray_build	( gamesnum_t n_enc
				, const struct ENC *enc
				, double inputdelta
				, double deq
				, double beta
				, double *ratingof
				, double white_advantage
				, double *probarray)
{
	double pw, pd, pl, delta;
	double p;
	player_t w,b;
	gamesnum_t e;
	assert(deq <= 1 && deq >= 0);

	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh;	b = enc[e].bl;

		delta = 0;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, deq, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		probarray [(w<<2)|1] -= p;			
		probarray [(b<<2)|1] -= p;	

		delta = +inputdelta;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, deq, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		probarray [(w<<2)|2] -= p;			
		probarray [(b<<2)|0] -= p;	

		delta = -inputdelta;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, deq, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		probarray [(w<<2)|0] -= p;			
		probarray [(b<<2)|2] -= p;	

	}
}

// no globals
static double
derivative_single 	( player_t j
					, double delta
					, double *ratingof
					, const struct prior *pp
					, player_t n_relative_anchors
					, struct relprior *ra
					, double *probarray)
{
	double decrem, increm, center;
	double change;

	decrem = probarray [(j<<2)|0] + get_extra_unfitness_j (ratingof[j] - delta, j, pp, ratingof, n_relative_anchors, ra);
	center = probarray [(j<<2)|1] + get_extra_unfitness_j (ratingof[j]        , j, pp, ratingof, n_relative_anchors, ra);
	increm = probarray [(j<<2)|2] + get_extra_unfitness_j (ratingof[j] + delta, j, pp, ratingof, n_relative_anchors, ra);

	if (center < decrem && center < increm) {
		change = decrem > increm? 0.5: -0.5; 
	} else {
		change = decrem > increm? 1: -1; 
	}
	return change;
}

// no globals
static void
derivative_vector_calc 	( double delta
						, gamesnum_t n_encounters
						, const struct ENC *enc
						, double deq
						, double beta
						, player_t n_players
						, double *ratingof
						, bool_t *flagged
						, bool_t *prefed
						, double white_advantage
		 				, const struct prior *pp
						, player_t n_relative_anchors
						, struct relprior *ra
						, double *probarray
						, double *vector 
)
{
	player_t j;
	probarray_reset(n_players, probarray);
	probarray_build(n_encounters, enc, delta, deq, beta, ratingof, white_advantage, probarray);

	for (j = 0; j < n_players; j++) {
		if (flagged[j] || prefed[j]) {
			vector[j] = 0.0;
		} else {
			vector[j] = derivative_single (j, delta, ratingof, pp, n_relative_anchors, ra, probarray);
		}
	}	
}

static double fitexcess ( player_t n_players
						, const struct prior *p
						, double wadv
						, struct prior wa_prior
						, player_t n_relative_anchors
						, const struct relprior *ra
						, double *ratingof
						, double *ratingbk
						, const bool_t *flagged				
						, double deq
						, struct prior dr_prior
);


static void ratings_apply_excess_correction(double excess, player_t n_players, const bool_t *flagged, double *ratingof /*out*/);

static double absol(double x) {return x < 0? -x: x;}

// no globals
static double
adjust_rating_bayes 
		( double 					delta
		, bool_t 					multiple_anchors_present
		, bool_t 					some_prior_has_been_set
		, bool_t 					anchor_use
		, player_t 					anchor
		, double 					general_average 
		, player_t 					n_players 
		, const struct prior *		p
		, double 					white_advantage
		, struct 					prior wa_prior
		, double 					deq
		, struct prior 				dr_prior
		, player_t 					n_relative_anchors
		, const struct relprior *	ra
		, const double *			change_vector
		, const bool_t *			flagged
		, const bool_t *			prefed
		, double *					ratingof // out 
		, double *					ratingbk // out 
)
{
	player_t notflagged;
	player_t j;
	double 	excess, average;
	double 	y, ymax = 0;
	double 	accum = 0;

	for (j = 0; j < n_players; j++) {
		if (	flagged[j]	// player previously removed
			|| 	prefed[j]	// already fixed, one of the multiple anchors
		) continue; 

		// max
		y = absol(change_vector[j]);
		if (y > ymax) ymax = y;

		// execute adjustment
		ratingof[j] += delta * change_vector[j];	

	}	

	// Normalization to a common reference (Global --> General_average)
	// The average could be normalized, or the rating of an anchor.
	// Skip in case of multiple anchors present

	if (multiple_anchors_present) {

		excess = 0; // do nothing, was done before

	} else if (some_prior_has_been_set) {
 
		excess = fitexcess
					( n_players
					, p
					, white_advantage
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged
					, deq
					, dr_prior
				);

	} else if (anchor_use) {

		excess  = ratingof[anchor] - general_average;

	} else {

		// general average
		for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
			if (!flagged[j]) {
				notflagged++;
				accum += ratingof[j];
			}
		}
		average = accum / (double)notflagged;
		excess  = average - general_average;
	}

	// Correct the excess
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);

	// Return maximum increase/decrease ==> "resolution"
	return ymax * delta;
}

// no globals
static void
ratings_apply_excess_correction(double excess, player_t n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

// no globals
static double
ufex
				( double 				excess
				, player_t				n_players
				, const struct prior 	*p
				, double				wadv
				, struct prior 			wa_prior
				, player_t 				n_relative_anchors
				, const struct relprior *ra
				, double 				*ratingof
				, double 				*ratingbk
				, const bool_t 			*flagged
				, double				deq
				, struct prior 			dr_prior
)
{
	double u;
	ratings_backup  (n_players, ratingof, ratingbk);
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);

	u = prior_unfitness

				( n_players
				, p
				, wadv
				, wa_prior
				, n_relative_anchors
				, ra
				, ratingof
				, deq
				, dr_prior
				);

	ratings_restore (n_players, ratingbk, ratingof);
	return u;
}

// no globals
static double
fitexcess 		( player_t n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, double *ratingof
				, double *ratingbk
				, const bool_t *flagged
				, double deq
				, struct prior dr_prior
)
{
	double ub, ut, uc, newb, newt, newc;
	double c = 0;
	double t = c + 100;
	double b = c - 100;

	do {

		ub = ufex 	( b
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged
					, deq
					, dr_prior
			);

		uc = ufex 	( c
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged
					, deq
					, dr_prior
			);

		ut = ufex 	( t
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged
					, deq
					, dr_prior
			);

		if (uc <= ub && uc <= ut) {
			newb = (b+c)/4;
			newt = (t+c)/4; 
			newc = c;
		} else if (ub < ut) {
			newb = b - (c - b);
			newc = b;
			newt = c;
		} else {
			newb = c;
			newc = t;
			newt = t + ( t - c);
		}
		c = newc;
		t = newt;
		b = newb;
	} while ((t-b) > MIN_RESOLUTION);

	return c;
}

//========================== end bayesian concept

#include "fit1d.h"

struct UNFITNESS_WA_DR  
		{ gamesnum_t n_enc
		; const struct ENC *enc
		; player_t n_players
		; const struct prior *p
		; double wadv
		; struct prior wa_prior
		; player_t n_relative_anchors
		; const struct relprior *ra
		; const double *ratingof
		; double deq
		; struct prior dr_prior
		; double beta;
};

static double
unfit_wadv (double x, const void *p)
{
	double r;
	const struct UNFITNESS_WA_DR *q = p;
	assert(!is_nan(x));
	r = calc_bayes_unfitness_full	
							( q->n_enc
							, q->enc
							, q->n_players
							, q->p
							, x
							, q->wa_prior
							, q->n_relative_anchors
							, q->ra
							, q->ratingof
							, q->deq
							, q->dr_prior
							, q->beta);
	assert(!is_nan(r));
	return r;
}

static double
unfit_drra (double x, const void *p)
{
	double r;
	const struct UNFITNESS_WA_DR *q = p;
	assert(!is_nan(x));
	r = calc_bayes_unfitness_full	
							( q->n_enc
							, q->enc
							, q->n_players
							, q->p
							, q->wadv
							, q->wa_prior
							, q->n_relative_anchors
							, q->ra
							, q->ratingof
							, x
							, q->dr_prior
							, q->beta);
	assert(!is_nan(r));
	return r;
}

// no globals
static double
adjust_wadv_bayes 
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double deq
				, struct prior dr_prior
				, double beta
)
{
	double delta, wa, ei, ej, ek;
	struct UNFITNESS_WA_DR su;

	su.n_enc 				= n_enc;
	su.enc 					= enc;
	su.n_players			= n_players;
	su.p 					= p;
	su.wadv					= 0;
	su.wa_prior 			= wa_prior;
	su.n_relative_anchors 	= n_relative_anchors;
	su.ra 					= ra;
	su.ratingof 			= ratingof;
	su.deq 					= deq;
	su.dr_prior 			= dr_prior;
	su.beta 				= beta;

	assert(deq <= 1 && deq >= 0);

	delta = resol;
	wa = start_wadv;

	do {	

		ei = unfit_wadv (wa - delta, &su);
		ej = unfit_wadv (wa - 0    , &su);
		ek = unfit_wadv (wa + delta, &su);

		assert (!is_nan(ei));
		assert (!is_nan(ej));
		assert (!is_nan(ek));

		if (ei >= ej && ej <= ek) {
			//delta = delta / 4;
			wa = quadfit1d	(resol/10, wa - delta, wa + delta, unfit_wadv, &su);
			break;
		} else
		if (ej >= ei && ei <= ek) {
			wa -= delta;
			delta *= 1.5;
		} else
		if (ei >= ek && ek <= ej) {
			wa += delta;
			delta *= 1.5;
		}

	} while (delta > resol/10 && -1000 < wa && wa < 1000);
	
	return wa;
}

// no globals
static double
adjust_drawrate_bayes 
				( gamesnum_t n_enc
				, const struct ENC *enc
				, player_t n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, player_t n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double deq
				, struct prior dr_prior
				, double beta
)
{
	double delta, ei, ej, ek, dr;
	double di, dj, dk;
	struct UNFITNESS_WA_DR su;

	su.n_enc 				= n_enc;
	su.enc 					= enc;
	su.n_players			= n_players;
	su.p 					= p;
	su.wadv					= start_wadv;
	su.wa_prior 			= wa_prior;
	su.n_relative_anchors 	= n_relative_anchors;
	su.ra 					= ra;
	su.ratingof 			= ratingof;
	su.deq 					= deq;
	su.dr_prior 			= dr_prior;
	su.beta 				= beta;

	assert(deq <= 1 && deq >= 0);

	delta = resol > 0.0001? resol: 0.0001;
	dr = deq;
	do {	
		di = dr - delta;
		dj = dr        ;
		dk = dr + delta;

		// do not allow the boundaries to go over 1 or below 0
		if (dk >= 1) {
			// di: untouched
			dj = (1 + di)/2;
			dk = 1;
			dr = dj;
			delta = dj - di;			
		}

		if (di <= 0) {
			di = 0;
			dj = (0 + dk)/2;
			// dk: untouched
			dr = dj;
			delta = dj - di;			
		}

		assert(dk <= 1 && di >=0);

		ei = unfit_drra (di, &su);
		ej = unfit_drra (dj, &su);
		ek = unfit_drra (dk, &su);

		if (ei >= ej && ej <= ek) {
			//delta = delta / 4;
			dr = quadfit1d	(MIN_DRAW_RATE_RESOLUTION, di, dk, unfit_drra, &su);
			break;
		} else
		if (ej >= ei && ei <= ek) {
			dr -= delta;
			delta *= 1.1;
		} else
		if (ei >= ek && ek <= ej) {
			dr += delta;
			delta *= 1.1;
		}

	} while (
		delta > MIN_DRAW_RATE_RESOLUTION
	);

	return dr;
}

