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

#include "rating.h"
#include "encount.h"
#include "indiv.h"
#include "xpect.h"
#include "datatype.h"
#include "mymem.h"

#if !defined(NDEBUG)
static bool_t is_nan (double x) {if (x != x) return TRUE; else return FALSE;}
#endif

static double adjust_drawrate (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta);

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

// no globals
static double 
deviation (player_t n_players, const bool_t *flagged, const double *expected, const double *obtained, const gamesnum_t *playedby)
{
	double accum = 0;
	double diff;
	player_t j;

	for (accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			diff = expected[j] - obtained [j];
			accum += diff * diff / (double)playedby[j];
		}
	}		
	return accum;
}

#if 1
static double
calc_excess		( player_t n_players
				, const bool_t *flagged
				, double general_average
				, double *ratingof)
{
	player_t 	j, notflagged;
	double 	accum, average, excess;

	for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			notflagged++;
			accum += ratingof[j];
		}
	}
	average = accum /(double) notflagged;
	excess  = average - general_average;

	return excess;
}

static void
correct_excess	( player_t n_players
				, const bool_t *flagged
				, double excess
				, double *ratingof)
{
	player_t 	j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j]) ratingof[j] -= excess;
	}
	return;
}

#endif

// no globals
static double
adjust_rating 	( double delta
				, double kappa
				, player_t n_players
				, const bool_t *flagged
				, const bool_t *prefed
				, const double *expected 
				, const double *obtained 
				, gamesnum_t *playedby
				, double *ratingof
				, player_t anchored_n
)
{
	player_t 	j;
	double 	d;
	double 	y = 1.0;
	double 	ymax = 0;

	/*
	|	1) delta and 2) kappa control convergence speed:
	|	Delta is the standard increase/decrease for each player
	|	But, not every player gets that "delta" since it is modified by
	|	by multiplier "y". The bigger the difference between the expected 
	|	performance and the observed, the bigger the "y". However, this
	|	is controled so y won't be higher than 1.0. It will be asymptotic
	|	to 1.0, and the parameter that controls how fast this saturation is 
	|	reached is "kappa". Smaller kappas will allow to reach 1.0 faster.	
	*/

	for (j = 0; j < n_players; j++) {
		assert(flagged[j] == TRUE || flagged[j] == FALSE);
		assert(prefed [j] == TRUE || prefed [j] == FALSE);
		if (anchored_n > 1)	{
			if (	flagged[j]	// player previously removed
				|| 	prefed[j]	// already set, one of the multiple anchors
			) continue; 
		}

		// find multiplier "y"
		d = (expected[j] - obtained[j]) / (double)playedby[j];
		d = d < 0? -d: d;

		y = d / (kappa + d);
		if (y > ymax) ymax = y;

		// execute adjustment
		if (expected[j] > obtained [j]) {
			ratingof[j] -= delta * y;
		} else {
			ratingof[j] += delta * y;
		}
	}

#if 0
	// Normalization to a common reference (Global --> General_average)
	// The average could be normalized, or the rating of an anchor.
	// Skip in case of multiple anchors present

	if (!multiple_anchors_present) {
		if (anchor_use) {
			excess = ratingof[anchor] - general_average;
		} else {
			excess = calc_excess (n_players, flagged, general_average, ratingof);
		}
		for (j = 0; j < n_players; j++) {
			if (!flagged[j] && !prefed[j]) ratingof[j] -= excess;
		}	
	}	
#endif

	// Return maximum increase/decrease ==> "resolution"
	return ymax * delta;
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

static double
overallerrorE_fwadv (gamesnum_t n_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv)
{
	gamesnum_t e;
	player_t w, b;
	double dp2, f;
	for (dp2 = 0, e = 0; e < n_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		f = xpect (ratingof[w] + wadv, ratingof[b], beta);
		dp2 +=	(double)enc[e].W * (1.0 - f) * (1.0 - f)
		+		(double)enc[e].D * (0.5 - f) * (0.5 - f)
		+		(double)enc[e].L * (0.0 - f) * (0.0 - f)
		;
	}
	return dp2;
}

#define START_DELTA 100

static double
adjust_wadv (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta, double start_delta)
{
	double delta, wa, ei, ej, ek;

	delta = start_delta;
	wa = start_wadv;

	do {	

		ei = overallerrorE_fwadv (n_enc, enc, ratingof, beta, wa - delta);
		ej = overallerrorE_fwadv (n_enc, enc, ratingof, beta, wa + 0    );     
		ek = overallerrorE_fwadv (n_enc, enc, ratingof, beta, wa + delta);

		if (ei >= ej && ej <= ek) {
			delta = delta / 2;
		} else
		if (ej >= ei && ei <= ek) {
			wa -= delta;
		} else
		if (ei >= ek && ek <= ej) {
			wa += delta;
		}

	} while (
		delta > 0.01 
		&& -1000 < wa && wa < 1000
	);
	
	return wa;
}

//============ center adjustment begin

static void ratings_copy (const double *r, player_t n, double *t) {player_t i;	for (i = 0; i < n; i++) {t[i] = r[i];}}

static double 
unfitness		( const struct ENC *enc
				, gamesnum_t	n_enc
				, player_t		n_players
				, const double *ratingof
				, const bool_t *flagged
				, double		white_adv
				, double		beta
				, double *		obtained
				, double *		expected
				, gamesnum_t *	playedby
)
{
		double dev;
		calc_expected (enc, n_enc, white_adv, n_players, ratingof, expected, beta);
		dev = deviation (n_players, flagged, expected, obtained, playedby);
		assert(!is_nan(dev));
		return dev;
}

static void
mobile_center_apply_excess (double excess, player_t n_players, const bool_t *flagged, const bool_t *prefed, double *ratingof)
{
	player_t j;
	assert(!is_nan(excess));
	for (j = 0; j < n_players; j++) {
		if (!flagged[j] && !prefed[j]) {
			ratingof[j] += excess;
		}
	}	
	return;
}

static double
unfitness_fcenter 	( double excess
					, const struct ENC *enc
					, gamesnum_t	n_enc
					, player_t		n_players
					, const double *ratingof
					, const bool_t *flagged
					, const bool_t *prefed
					, double		white_adv
					, double		beta
					, double *		obtained
					, double *		expected
					, gamesnum_t *	playedby
					, double 	   *ratingtmp)
{
	double u;
	assert(!is_nan(excess));
	ratings_copy (ratingof, n_players, ratingtmp);
	mobile_center_apply_excess (excess, n_players, flagged, prefed, ratingtmp);
	u = unfitness	( enc
					, n_enc
					, n_players
					, ratingtmp
					, flagged
					, white_adv
					, beta
					, obtained
					, expected
					, playedby);
	assert(!is_nan(u));
	return u;
}

#define MIN_DEVIA 0.0000001
#define MIN_RESOL 0.000001
#define START_RESOL 10.0
#define ACCEPTABLE_RESOL MIN_RESOL
#define PRECISIONERROR (1E-16)

static double absol(double x) {return x >= 0? x: -x;}

#include "fit1d.h"

struct UNFITPAR {
	const struct ENC *	enc;
	gamesnum_t			n_enc;
	player_t			n_players;
	const double *		ratingof;
	const bool_t *		flagged;
	const bool_t *		prefed;
	double				white_adv;
	double				beta;
	double *			obtained;
	double *			expected;
	gamesnum_t *		playedby;
	double *			ratingtmp;
};


static double
unfitf (double x, const void *p)
{
	double r;
	const struct UNFITPAR *q = p;
	assert(!is_nan(x));
	r =	unfitness_fcenter 	( x
							, q->enc, q->n_enc, q->n_players, q->ratingof, q->flagged, q->prefed
							, q->white_adv, q->beta, q->obtained, q->expected, q->playedby, q->ratingtmp);
	assert(!is_nan(r));
	return r;
}

static double
optimum_centerdelta	( double 			start_delta
					, double 			resolution
					, const struct ENC *enc
					, gamesnum_t		n_enc
					, player_t			n_players
					, const double *	ratingof
					, const bool_t *	flagged
					, const bool_t *	prefed
					, double			white_adv
					, double			beta
					, double *			obtained
					, double *			expected
					, gamesnum_t *		playedby
					, double *			ratingtmp
					)
{
	double lo_d, hi_d;
	struct UNFITPAR p;
	p.enc 		= enc;
	p.n_enc		= n_enc;
	p.n_players	= n_players;
	p.ratingof	= ratingof;
	p.flagged	= flagged;
	p.prefed	= prefed;
	p.white_adv	= white_adv;
	p.beta		= beta;
	p.obtained	= obtained;
	p.expected	= expected;
	p.playedby	= playedby;
	p.ratingtmp = ratingtmp;

	lo_d = start_delta - 1000;
	hi_d = start_delta + 1000;

	assert(!is_nan(resolution));
	assert(!is_nan(lo_d));
	assert(!is_nan(hi_d));
	return quadfit1d (resolution, lo_d, hi_d, unfitf, &p);
}

//============ center adjustment end

#if 0
static double
ratings_rmsd(player_t n, double *a, double *b)
{
	player_t i;	
	double d, res;
	double acc = 0;
	for (i = 0; i < n; i ++) {
		d = a[i] - b[i];
		acc = d * d;
	}
	res = sqrt(acc/(double)n);
	return res;
}
#endif

gamesnum_t
calc_rating2 	( bool_t 			quiet
				, struct ENC *		enc
				, gamesnum_t		n_enc
				, struct PLAYERS 	*plyrs
				, struct RATINGS 	*rat
				, double			*pWhite_advantage
				, double			general_average
				, bool_t			anchor_use
				, player_t			anchor
				, struct GAMES 		*g
				, double			BETA
//
				, bool_t 			adjust_white_advantage
				, bool_t			adjust_draw_rate
				, double			*pDraw_date
				, double			*ratingtmp_buffer
)
{
	gamesnum_t	n_games = g->n;
	double 	*	ratingtmp = ratingtmp_buffer;
	double 		olddev, curdev, outputdev;
	int 		i;
	int			rounds = 10000;
	double 		delta = START_DELTA;
	double 		kappa = 0.05;
	double 		damp_delta = 2;
	double 		damp_kappa = 2;
	int 		phase = 0;
	int 		n = 20;
	double 		resol = START_RESOL; // big number at the beginning
	bool_t		doneonce = FALSE;
	int 		max_cycle;
	int 		cycle;
	double 		white_adv = *pWhite_advantage;
	double 		wa_previous = *pWhite_advantage;
	double 		wa_progress = START_DELTA;
	double 		min_devia = MIN_DEVIA;
	double 		min_resol = START_RESOL;
	double 		draw_rate = *pDraw_date;
	double *	expected = NULL;
	size_t 		allocsize;

// translation variables for refactoring
player_t		n_players 		= plyrs->n;
int *			Performance_type= plyrs->performance_type;
bool_t *		flagged 		= plyrs->flagged;
bool_t *		prefed  		= plyrs->prefed;
const char **	name 			= plyrs->name;
double *		obtained 		= rat->obtained;
gamesnum_t *	playedby 		= rat->playedby;
double *		ratingof 		= rat->ratingof;
double *		ratingbk 		= rat->ratingbk;
player_t		anchored_n 		= plyrs->anchored_n;
//double RAT[20000];
	allocsize = sizeof(double) * (size_t)(n_players+1);
	expected = memnew(allocsize);
	if (expected == NULL) {
		fprintf(stderr, "Not enough memory to allocate all players\n");
		exit(EXIT_FAILURE);
	}

	max_cycle = adjust_white_advantage? 10: 1;

	for (cycle = 0; (cycle < max_cycle && wa_progress > 0.01) || min_resol > ACCEPTABLE_RESOL; cycle++) {

		bool_t done = FALSE;

		#define KK_DAMP 1000
		rounds = 10000;
		delta = START_DELTA;
		kappa = 0.05;
		damp_delta = 1.1;
		damp_kappa = 1.0;
		phase = 0;
		n = 1000;

		min_resol = cycle == 0? START_RESOL: (
					cycle == 1? START_RESOL/100: (
					cycle == 2? START_RESOL/10000: (
					MIN_RESOL
					)))
		;

		doneonce = FALSE;

		calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);
		calc_expected(enc, n_enc, white_adv, n_players, ratingof, expected, BETA);

		olddev = curdev = deviation(n_players, flagged, expected, obtained, playedby);

		if (!quiet) printf ("\nConvergence rating calculation (cycle #%d)\n\n", cycle+1);
		if (!quiet) printf ("%3s %4s %12s%14s\n", "phase", "iteration", "deviation","resolution");
// ratings_backup(n_players, ratingof, RAT);
		while (!done && n-->0) {
			bool_t failed = FALSE;
			double kk = 1.0;
			double cd, last_cd;
	
			last_cd = 100;

			for (i = 0; i < rounds && !done && !failed; i++) {

				ratings_backup(n_players, ratingof, ratingbk);
				olddev = curdev;

				resol = adjust_rating 	
					( delta
					, kappa*kk
					, n_players
					, flagged
					, prefed
					, expected 
					, obtained 
					, playedby
					, ratingof
					, anchored_n
				);

				calc_expected(enc, n_enc, white_adv, n_players, ratingof, expected, BETA);
				curdev = deviation(n_players, flagged, expected, obtained, playedby);

				if (curdev >= olddev) {
					ratings_restore(n_players, ratingbk, ratingof);
					calc_expected(enc, n_enc, white_adv, n_players, ratingof, expected, BETA);
					curdev = deviation(n_players, flagged, expected, obtained, playedby);	
					assert (absol(curdev-olddev) < PRECISIONERROR || 
								!fprintf(stderr, "curdev=%.10e, olddev=%.10e, diff=%.10e\n", curdev, olddev, olddev-curdev));
					failed = TRUE;
				};	

				if (!failed) {

					if (anchored_n > 1) {

						cd = optimum_centerdelta	
							( last_cd
							, min_resol > 0.1? 0.1: min_resol
							, enc
							, n_enc
							, n_players
							, ratingof
							, flagged
							, prefed
							, white_adv
							, BETA
							, obtained
							, expected
							, playedby
							, ratingtmp
							);

					} else if (anchor_use && anchored_n == 1) {
						cd = 0;
					} else {
						cd = 0;
					}

					last_cd = cd;

					if (absol(cd) > MIN_RESOL) {
						mobile_center_apply_excess (cd, n_players, flagged, prefed, ratingof);
					}

					//

					calc_expected(enc, n_enc, white_adv, n_players, ratingof, expected, BETA);
					curdev = deviation(n_players, flagged, expected, obtained, playedby);	
	
					outputdev = 1000*sqrt(curdev/(double)n_games);
					done = outputdev < min_devia && (absol(resol)+absol(cd)) < min_resol;
					kk *= (1.0-1.0/KK_DAMP); //kk *= 0.995;
				}
			}

			delta /= damp_delta;
			kappa *= damp_kappa;
			outputdev = 1000*sqrt(curdev/(double)n_games);

			if (!quiet) {
				printf ("%3d %7d %16.9f", phase, i, outputdev);
				printf ("%14.5f",resol);
//	printf ("%10.5lf",ratings_rmsd(n_players, ratingof, RAT));
				printf ("\n");
			}
			phase++;

		}

		if (!quiet) printf ("done\n");

		if (adjust_white_advantage) {
				white_adv = adjust_wadv (white_adv, ratingof, n_enc, enc, BETA, doneonce? resol: START_DELTA);
				doneonce = TRUE;
				wa_progress = wa_previous > white_adv? wa_previous - white_adv: white_adv - wa_previous;
				wa_previous = white_adv;
				if (!quiet)
					printf ("Adjusted White Advantage = %.1f\n",white_adv);
		}

		if (adjust_draw_rate) {
				draw_rate = adjust_drawrate (white_adv, ratingof, n_enc, enc, BETA);
				if (!quiet)
					printf ("Adjusted Draw Rate = %.1f %s\n\n", 100*draw_rate, "%");
		} 

		if (!quiet) 
			printf ("Post-Convergence rating estimation\n");

		n_enc = calc_encounters(ENCOUNTERS_FULL, g, flagged, enc);
		calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);
		rate_super_players(quiet, enc, n_enc, Performance_type, n_players, ratingof, white_adv, flagged, name, draw_rate, BETA); 
		n_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, g, flagged, enc);;
		calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);

		if (!quiet) 
			printf ("done\n");

		// printf ("EXCESS =%lf\n", calc_excess	(n_players, flagged, general_average, ratingof));

		if (anchored_n == 1 && anchor_use)
			adjust_rating_byanchor (anchor, general_average, n_players, ratingof);

	} //end while 


	if (anchored_n == 0) {
		double excess = calc_excess (n_players, flagged, general_average, ratingof);
		correct_excess (n_players, flagged, excess, ratingof);
	}

	*pWhite_advantage = white_adv;
	*pDraw_date = draw_rate;

	memrel(expected);
	return n_enc;
}


static double
overallerrorE_fdrawrate (gamesnum_t n_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv, double dr0)
{
	gamesnum_t e;
	player_t w, b;
	double dp2, f;
	double dexp;

	dp2 = 0;
	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		f = xpect (ratingof[w] + wadv, ratingof[b], beta);

		dexp = draw_rate_fperf (f, dr0);

		dp2 +=
				(double)enc[e].D                   * (1-dexp) * (1-dexp) +
				(double)(enc[e].played - enc[e].D) *    dexp  *    dexp  ;
		;
	}

	return dp2;
}

#define DRAWRATE_RESOLUTION 0.0001

static double
adjust_drawrate (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta)
{
	double delta, wa, ei, ej, ek, dr;
	double lo,hi;

	delta = 0.5;
	wa = start_wadv;

	dr = 0.5;

	do {	

		ei = overallerrorE_fdrawrate (n_enc, enc, ratingof, beta, wa, dr - delta);
		ej = overallerrorE_fdrawrate (n_enc, enc, ratingof, beta, wa, dr + 0    );     
		ek = overallerrorE_fdrawrate (n_enc, enc, ratingof, beta, wa, dr + delta);

		if (ei >= ej && ej <= ek) {
			delta = delta / 2;
		} else
		if (ej >= ei && ei <= ek) {
			dr -= delta;
		} else
		if (ei >= ek && ek <= ej) {
			dr += delta;
		}

		hi = dr+delta > 1.0? 1.0: dr+delta;
		lo = dr-delta < 0.0? 0.0: dr-delta;
		dr = (hi+lo)/2;
		delta = (hi-lo)/2;

	} while (delta > DRAWRATE_RESOLUTION);
	
	return dr;
}

