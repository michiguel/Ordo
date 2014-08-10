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
#include <math.h>
#include <assert.h>

#include "ratingb.h"
#include "encount.h"
#include "indiv.h"

#if 1
#define CALCIND_SWSL
#endif

#if 1
	#define DOPRIOR
#endif

#define MIN_RESOLUTION 0.000001
#define PRIOR_SMALLEST_SIGMA 0.0000001
#define WADV_RECALC
#undef WADV_RECALC

//

// ================= Testing Bayes concept 

// no globals
static double
relative_anchors_unfitness_full(long int n_relative_anchors, const struct relprior *ra, const double *ratingof)
{
	int a, b, i;
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
relative_anchors_unfitness_j(double R, int j, double *ratingof, long int n_relative_anchors, struct relprior *ra)
{
	int a, b, i;
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
adjust_rating_byanchor (bool_t anchor_use, int anchor, double general_average, int n_players, double *ratingof, bool_t *flagged)
{
	double excess;
	int j;
	if (anchor_use) {
		excess  = ratingof[anchor] - general_average;	
		for (j = 0; j < n_players; j++) {
			if (!flagged[j]) ratingof[j] -= excess;
		}	
	}
}

// no globals
static void
ratings_restore (int n_players, const double *r_bk, double *r_of)
{
	int j;
	for (j = 0; j < n_players; j++) {
		r_of[j] = r_bk[j];
	}	
}

// no globals
static void
ratings_backup (int n_players, const double *r_of, double *r_bk)
{
	int j;
	for (j = 0; j < n_players; j++) {
		r_bk[j] = r_of[j];
	}	
}

//

// no globals
static bool_t
super_players_present(int n_players, int *performance_type)
{ 
	bool_t found = FALSE;
	int j;
	for (j = 0; j < n_players && !found; j++) {
		found = performance_type[j] == PERF_SUPERWINNER || performance_type[j] == PERF_SUPERLOSER; 
	}
	return found;
}

static double
adjust_wadv_bayes 
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double beta
);

// no globals
static void
derivative_vector_calc 	( double delta
						, int n_encounters
						, const struct ENC *enc
						, double beta
						, int n_players
						, double *ratingof
						, bool_t *flagged
						, bool_t *prefed
						, double white_advantage
		 				, const struct prior *pp
						, long int n_relative_anchors
						, struct relprior *ra
						, double probarray[MAXPLAYERS][4]
						, double *vector 
);

// no globals
static double
calc_bayes_unfitness_full	
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double beta
);

// no globals
static double
adjust_rating_bayes 
				( double delta
				, bool_t multiple_anchors_present
				, bool_t some_prior_set
				, bool_t anchor_use
				, int anchor
				, double general_average 
				, int n_players 
				, const struct prior *p
				, double white_advantage
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *change_vector
				, const bool_t *flagged
				, const bool_t *prefed
				, double *ratingof // out 
				, double *ratingbk // out 
);

// no globals
int
calc_rating_bayes2 	(  
			bool_t 		quiet
			, struct ENC *	enc
			, int 			N_enc

			, int			n_players
			, double *		obtained
//				, double * 		expected
			, int *			playedby
			, double *		ratingof
			, double *		ratingbk
			, int *			performance_type

			, bool_t *		flagged
			, bool_t *		prefed

			, double		*pwadv
			, double		general_average

			, bool_t		multiple_anchors_present
			, bool_t		anchor_use
			, int			anchor
				
			, int			n_games
			, int *			score
			, int *			whiteplayer
			, int *			blackplayer

			, char *		name[]
			, double		beta

// different from non bayes calc

			, double *changing
			, long int n_relative_anchors
			, struct prior *pp
			, double probarray [MAXPLAYERS] [4]
			, struct relprior *ra
			, bool_t some_prior_set
			, struct prior wa_prior
			, bool_t adjust_white_advantage
)
{
	double 	olddev, curdev, outputdev;
	int 	i;
	int		rounds = 10000;

double rtng_76 = (-log(1.0/0.76-1.0))/beta;

	double 	delta = rtng_76; //should be proportional to the scale
	double 	denom = 3;
	int 	phase = 0;
	int 	n = 40;
	double 	resol = delta;
	double 	resol_prev = delta;

	double white_advantage = *pwadv;

	// initial deviation
	olddev = curdev = calc_bayes_unfitness_full	
							( N_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, beta);

	if (!quiet) printf ("Converging...\n\n");
	if (!quiet) printf ("%3s %4s %10s %10s\n", "phase", "iteration", "unfitness","resolution");

	while (n-->0) {

		for (i = 0; i < rounds; i++) {
			double kappa = 1.0;
			ratings_backup  (n_players, ratingof, ratingbk);
			olddev = curdev;

			// Calc "changing" vector
			derivative_vector_calc
						( delta
						, N_enc
						, enc
						, beta
						, n_players
						, ratingof
						, flagged
						, prefed
						, white_advantage
						, pp
						, n_relative_anchors
						, ra
						, probarray
						, changing );


			resol_prev = resol;
			resol = 

					// adjust_rating_bayes(delta*kappa,changing);
					adjust_rating_bayes 
						( delta*kappa
						, multiple_anchors_present
						, some_prior_set
						, anchor_use
						, anchor
						, general_average 
						, n_players 
						, pp
						, white_advantage
						, wa_prior
						, n_relative_anchors
						, ra
						, changing
						, flagged
						, prefed
						, ratingof // out 
						, ratingbk // out 
					);


			resol = (resol_prev + resol) / 2;

			curdev = calc_bayes_unfitness_full	
							( N_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, beta);

			if (curdev >= olddev) {
				ratings_restore (n_players, ratingbk, ratingof);
				curdev = olddev;
				assert (curdev == olddev);
				break;
			} else {
				ratings_backup  (n_players, ratingof, ratingbk);
				olddev = curdev;
			}	

			if (resol < MIN_RESOLUTION) break;
		}

		delta /=  denom;
		outputdev = curdev/n_games;

		if (!quiet) {
			printf ("%3d %7d %14.5f", phase, i, outputdev);
			printf ("%11.5f",resol);
			printf ("\n");
		}
		phase++;

		if (adjust_white_advantage 
//			&& wa_prior.set // if !wa_prior set, it will give no error based on the wa
							// but it will adjust it based on the results. This is useful
							// for -W
			) {
			white_advantage = adjust_wadv_bayes 
							( N_enc
							, enc
							, n_players
							, pp
							, white_advantage
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, resol
							, beta);

			*pwadv = white_advantage;
		}

		if (resol < MIN_RESOLUTION) break;

	}

	if (!quiet) printf ("done\n\n");

	printf ("White_advantage = %lf\n\n", white_advantage);

#ifdef CALCIND_SWSL
	if (!quiet && super_players_present(n_players, performance_type)) printf ("Post-Convergence rating estimation for all-wins / all-losses players\n\n");

	N_enc = calc_encounters(ENCOUNTERS_FULL, n_games, score, flagged, whiteplayer, blackplayer, enc);
	calc_obtained_playedby(enc, N_enc, n_players, obtained, playedby);

//	assert(Performance_type_set); //FIXME

	rate_super_players(quiet, enc, N_enc, performance_type, n_players, ratingof, white_advantage, flagged, name, beta);

	N_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, n_games, score, flagged, whiteplayer, blackplayer, enc);
	calc_obtained_playedby(enc, N_enc, n_players, obtained, playedby);

#endif

	if (!multiple_anchors_present && !some_prior_set)
		adjust_rating_byanchor (anchor_use, anchor, general_average, n_players, ratingof, flagged);

	return N_enc;
}


// no globals
static double
adjust_wadv_bayes 
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
				, double beta
)
{
	double delta, wa, ei, ej, ek;

	delta = resol;
	wa = start_wadv;

	do {	

		ei = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa - delta //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, beta);

		ej = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa         //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, beta);

		ek = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa + delta //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof
							, beta);

		if (ei >= ej && ej <= ek) {
			delta = delta / 4;
		} else
		if (ej >= ei && ei <= ek) {
			wa -= delta;
		} else
		if (ei >= ek && ek <= ej) {
			wa += delta;
		}

	} while (delta > resol/10 && -1000 < wa && wa < 1000);
	
	return wa;
}


static double
fxpect (double a, double b, double beta)
{
	return 1.0 / (1.0 + exp((b-a)*beta));
}

#define DRAWRATE_AT_EQUAL_STRENGTH 0.33
#define DRAWFACTOR (1/(2*(DRAWRATE_AT_EQUAL_STRENGTH))-0.5)

static void
fget_pWDL(double dr /*delta rating*/, double *pw, double *pd, double *pl, double beta)
{
	// Performance comprises wins and draws.
	// if f is expected performance from 0 to 1.0, then
	// f = pwin + pdraw/2
	// from that, dc is the fraction of points that come from draws, not wins, so
	// pdraw (probability of draw) = 2 * f * dc
	// calculation of dc is an empirical formula to fit average data from CCRL:
	// Draw rate of equal engines is near 0.33, and decays on uneven matches.

	double f;
	double pdra, pwin, plos;
// 	double dc; 
	bool_t switched;
	
	switched = dr < 0;

	if (switched) dr = -dr;

	f = fxpect (dr,0,beta);

#if 0
	dc = 0.5 / (0.5 + DRAWFACTOR * exp(dr*beta));
	pdra = 2 * f * dc;
	pwin = f - pdra/2;
	plos = 1 - pwin - pdra; 
#else
	pwin = f * f;
	plos = (1-f) * (1-f);
	pdra = 1 - pwin - plos;
#endif

	if (switched) {
		*pw = plos;
		*pd = pdra;
		*pl = pwin;
	} else {
		*pw = pwin;
		*pd = pdra;
		*pl = plos;
	}
	return;
}

static void fget_pWDL(double dr /*delta rating*/, double *pw, double *pd, double *pl, double beta);

static double
wdl_probabilities (int ww, int dd, int ll, double pw, double pd, double pl)
{
	return 	 	(ww > 0? ww * log(pw) : 0) 
			+ 	(dd > 0? dd * log(pd) : 0) 
			+ 	(ll > 0? ll * log(pl) : 0)
			;
}

// no globals
static double
prior_unfitness	( int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
)
{
	int j;
	double x;
	double accum = 0;
	for (j = 0; j < n_players; j++) {
		if (p[j].set) {
			x = (ratingof[j] - p[j].rating)/p[j].sigma;
			accum += 0.5 * x * x;
		}
	}

	if (wa_prior.set) {
		x = (wadv - wa_prior.rating)/wa_prior.sigma;
		accum += 0.5 * x * x;		
	}

	//FIXME this could be slow!
	accum += relative_anchors_unfitness_full(n_relative_anchors, ra, ratingof); //~~

	return accum;
}

// no globals
static double
calc_bayes_unfitness_full	
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double beta
)
{
	double pw, pd, pl, accum;
	int e,w,b, ww,dd,ll;

	for (accum = 0, e = 0; e < n_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		fget_pWDL(ratingof[w] + wadv - ratingof[b], &pw, &pd, &pl, beta);

		ww = enc[e].W;
		dd = enc[e].D;
		ll = enc[e].L;

		accum 	+= 	(ww > 0? ww * log(pw) : 0) 
				+ 	(dd > 0? dd * log(pd) : 0) 
				+ 	(ll > 0? ll * log(pl) : 0);
	}
	// Priors
	accum += -prior_unfitness
				( n_players
				, p
				, wadv
				, wa_prior
				, n_relative_anchors
				, ra
				, ratingof
				);

	return -accum;
}

//============================================

static double Probarray [MAXPLAYERS] [4];

// no globals
static double
get_extra_unfitness_j (double R, int j, const struct prior *p, double *ratingof, long int n_relative_anchors, struct relprior *ra)
{
	double x;
	double u = 0;
	if (p[j].set) {
		x = (R - p[j].rating)/p[j].sigma;
		u = 0.5 * x * x;
	} 

	//FIXME this could be slow!
	u += relative_anchors_unfitness_j(R, j, ratingof, n_relative_anchors, ra); //~~

	return u;
}

// no globals
static void
probarray_reset(int n_players, double probarray[MAXPLAYERS][4])
{
	int j, k;
	for (j = 0; j < n_players; j++) {
		for (k = 0; k < 4; k++) {
			probarray[j][k] = 0;
		}	
	}
}

// no globals
static void
probarray_build(int n_enc, const struct ENC *enc, double inputdelta, double beta, double *ratingof, double white_advantage)
{
	double pw, pd, pl, delta;
	double p;
	int e,w,b;

	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh;	b = enc[e].bl;

		delta = 0;
		fget_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [1] -= p;			
		Probarray [b] [1] -= p;	

		delta = +inputdelta;
		fget_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [2] -= p;			
		Probarray [b] [0] -= p;	

		delta = -inputdelta;
		fget_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl, beta);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [0] -= p;			
		Probarray [b] [2] -= p;	

	}
}

// no globals
static double
derivative_single (int j, double delta, double *ratingof, const struct prior *pp,
 long int n_relative_anchors, struct relprior *ra, double probarray[MAXPLAYERS][4])
{
	double decrem, increm, center;
	double change;

	decrem = probarray [j] [0] + get_extra_unfitness_j (ratingof[j] - delta, j, pp, ratingof, n_relative_anchors, ra);
	center = probarray [j] [1] + get_extra_unfitness_j (ratingof[j]        , j, pp, ratingof, n_relative_anchors, ra);
	increm = probarray [j] [2] + get_extra_unfitness_j (ratingof[j] + delta, j, pp, ratingof, n_relative_anchors, ra);

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
						, int n_encounters
						, const struct ENC *enc
						, double beta
						, int n_players
						, double *ratingof
						, bool_t *flagged
						, bool_t *prefed
						, double white_advantage
		 				, const struct prior *pp
						, long int n_relative_anchors
						, struct relprior *ra
						, double probarray[MAXPLAYERS][4]
						, double *vector 
)
{
	int j;
	probarray_reset(n_players, probarray);
	probarray_build(n_encounters, enc, delta, beta, ratingof, white_advantage);

	for (j = 0; j < n_players; j++) {
		if (flagged[j] || prefed[j]) {
			vector[j] = 0.0;
		} else {
			vector[j] = derivative_single (j, delta, ratingof, pp, n_relative_anchors, ra, probarray);
		}
	}	
}

static double fitexcess ( int n_players
						, const struct prior *p
						, double wadv
						, struct prior wa_prior
						, long int n_relative_anchors
						, const struct relprior *ra
						, double *ratingof
						, double *ratingbk
						, const bool_t *flagged);

static void ratings_apply_excess_correction(double excess, int n_players, const bool_t *flagged, double *ratingof /*out*/);

static double absol(double x) {return x < 0? -x: x;}

// no globals
static double
adjust_rating_bayes 
				( double delta
				, bool_t multiple_anchors_present
				, bool_t some_prior_set
				, bool_t anchor_use
				, int anchor
				, double general_average 
				, int n_players 
				, const struct prior *p
				, double white_advantage
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *change_vector
				, const bool_t *flagged
				, const bool_t *prefed
				, double *ratingof // out 
				, double *ratingbk // out 
)
{
	int 	j, notflagged;
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

	} else if (some_prior_set) {
 
		excess = fitexcess
					( n_players
					, p
					, white_advantage
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);

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
		average = accum / notflagged;
		excess  = average - general_average;
	}

	// Correct the excess
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);

	// Return maximum increase/decrease ==> "resolution"
	return ymax * delta;
}

// no globals
static void
ratings_apply_excess_correction(double excess, int n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	int j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

// no globals
static double
ufex
				( double excess
				, int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, double *ratingof
				, double *ratingbk
				, const bool_t *flagged
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
				);

	ratings_restore (n_players, ratingbk, ratingof);
	return u;
}

// no globals
static double
fitexcess 		( int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, double *ratingof
				, double *ratingbk
				, const bool_t *flagged
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
					, flagged);

		uc = ufex 	( c
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);

		ut = ufex 	( t
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);


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


