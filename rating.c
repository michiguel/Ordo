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

static double adjust_drawrate (double start_wadv, const double *ratingof, size_t N_enc, const struct ENC *enc, double beta);

// no globals
static void
ratings_restore (size_t n_players, const double *r_bk, double *r_of)
{
	size_t j;
	for (j = 0; j < n_players; j++) {
		r_of[j] = r_bk[j];
	}	
}

// no globals
static void
ratings_backup (size_t n_players, const double *r_of, double *r_bk)
{
	size_t j;
	for (j = 0; j < n_players; j++) {
		r_bk[j] = r_of[j];
	}	
}

// no globals
static double 
deviation (size_t n_players, const bool_t *flagged, const double *expected, const double *obtained, const int *playedby)
{
	double accum = 0;
	double diff;
	size_t j;

	for (accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			diff = expected[j] - obtained [j];
			accum += diff * diff / playedby[j];
		}
	}		
	return accum;
}

#if 0
static double
calc_excess		( size_t n_players
				, const bool_t *flagged
				, double general_average
				, double *ratingof)
{
	size_t 	j, notflagged;
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
#endif

// no globals
static double
adjust_rating 	( double delta
				, double kappa
				, size_t n_players
				, const bool_t *flagged
				, const bool_t *prefed
				, const double *expected 
				, const double *obtained 
				, int *playedby
				, double *ratingof
				, int anchored_n
)
{
	size_t 	j;
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
	|
	|	Uses globals:
	|	arrays:	Flagged, Prefed, Expected, Obtained, Playedby, Ratingof
	|	variables: N_players, General_average
	|	flags:	Multiple_anchors_present, Anchor_use
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
		d = (expected[j] - obtained[j]) / playedby[j];
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
adjust_rating_byanchor (int anchor, double general_average, size_t n_players, double *ratingof)
{
	size_t j;
	double excess = ratingof[anchor] - general_average;	
	for (j = 0; j < n_players; j++) {
			ratingof[j] -= excess;
	}	
}

static double
overallerrorE_fwadv (size_t N_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv)
{
	size_t e;
	int32_t w, b;
	double dp2, f;
	for (dp2 = 0, e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		f = xpect (ratingof[w] + wadv, ratingof[b], beta);
		dp2 +=	enc[e].W * (1.0 - f) * (1.0 - f)
		+		enc[e].D * (0.5 - f) * (0.5 - f)
		+		enc[e].L * (0.0 - f) * (0.0 - f)
		;
	}
	return dp2;
}

#define START_DELTA 100

static double
adjust_wadv (double start_wadv, const double *ratingof, size_t N_enc, const struct ENC *enc, double beta, double start_delta)
{
	double delta, wa, ei, ej, ek;

	delta = start_delta;
	wa = start_wadv;

	do {	

		ei = overallerrorE_fwadv (N_enc, enc, ratingof, beta, wa - delta);
		ej = overallerrorE_fwadv (N_enc, enc, ratingof, beta, wa + 0    );     
		ek = overallerrorE_fwadv (N_enc, enc, ratingof, beta, wa + delta);

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

static void ratings_copy (const double *r, size_t n, double *t) {size_t i;	for (i = 0; i < n; i++) {t[i] = r[i];}}

static double 
unfitness		( const struct ENC *enc
				, size_t		n_enc
				, size_t		n_players
				, const double *ratingof
				, const bool_t *flagged
				, double		white_adv
				, double		beta

				, double *		obtained
				, double *		expected
				, int *			playedby
)
{
		double dev;
		calc_expected (enc, n_enc, white_adv, n_players, ratingof, expected, beta);
		dev = deviation (n_players, flagged, expected, obtained, playedby);
		assert(!is_nan(dev));
		return dev;
}

static void
mobile_center_apply_excess (double excess, size_t n_players, const bool_t *flagged, const bool_t *prefed, double *ratingof)
{
	size_t j;
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
					, size_t		n_enc
					, size_t		n_players
					, const double *ratingof
					, const bool_t *flagged
					, const bool_t *prefed
					, double		white_adv
					, double		beta

					, double *		obtained
					, double *		expected
					, int *			playedby
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

#define MIN_DEVIA 0.000000001
#define MIN_RESOL 0.000001
#define START_RESOL 10.0
#define ACCEPTABLE_RESOL 0.001

static double absol(double x) {return x >= 0? x: -x;}

#include "fit1d.h"

struct UNFITPAR {
	const struct ENC *	enc;
	size_t				n_enc;
	size_t				n_players;
	const double *		ratingof;
	const bool_t *		flagged;
	const bool_t *		prefed;
	double				white_adv;
	double				beta;
	double *			obtained;
	double *			expected;
	int *				playedby;
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
					, size_t			n_enc
					, size_t			n_players
					, const double *	ratingof
					, const bool_t *	flagged
					, const bool_t *	prefed
					, double			white_adv
					, double			beta
					, double *			obtained
					, double *			expected
					, int *				playedby
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


#define PRECISIONERROR (1E-16)

size_t
calc_rating2 	( bool_t 		quiet
				, struct ENC *	enc
				, size_t		N_enc

				, size_t		N_players
				, double *		Obtained

				, int *			Playedby
				, double *		Ratingof
				, double *		Ratingbk
				, int *			Performance_type

				, bool_t *		Flagged
				, bool_t *		Prefed

				, double		*pWhite_advantage
				, double		General_average

				, bool_t		Multiple_anchors_present
				, bool_t		Anchor_use
				, int			Anchor
				, int			anchored_n
				
				, struct GAMES *g

				, const char *	Name[]
				, double		BETA
//
				, bool_t 		adjust_white_advantage

				, bool_t		adjust_draw_rate
				, double		*pDraw_date
				, double		*ratingtmp_buffer
)
{
	size_t	N_games = g->n;

	double 	*ratingtmp = ratingtmp_buffer;

	double 	olddev, curdev, outputdev;
	int 	i;
	int		rounds = 10000;
	double 	delta = 200.0;
	double 	kappa = 0.05;
	double 	denom = 2;
	int 	phase = 0;
	int 	n = 20;
	double 	resol;
	bool_t	doneonce = FALSE;

	int max_cycle;
	int cycle;

	double white_adv = *pWhite_advantage;
	double wa_previous = *pWhite_advantage;
	double wa_progress = START_DELTA;
	double min_devia = MIN_DEVIA;
	double min_resol = START_RESOL;

	double draw_rate = *pDraw_date;

	double *expected = NULL;
	size_t allocsize;

	allocsize = sizeof(double) * (N_players+1);
	expected = memnew(allocsize);
	if (expected == NULL) {
		fprintf(stderr, "Not enough memory to allocate all players\n");
		exit(EXIT_FAILURE);
	}

	max_cycle = adjust_white_advantage? 10: 1;

	for (cycle = 0; (cycle < max_cycle && wa_progress > 0.01) || min_resol > ACCEPTABLE_RESOL; cycle++) {

		bool_t done = FALSE;

		rounds = 10000;
		delta = 200.0;
		kappa = 0.05;
		denom = 2;
		phase = 0;
		n = 20;

		min_resol = cycle == 0? START_RESOL: (
					cycle == 1? START_RESOL/100: (
					cycle == 2? START_RESOL/10000: (
					MIN_RESOL
					)))
		;

		doneonce = FALSE;

		calc_obtained_playedby(enc, N_enc, N_players, Obtained, Playedby);
		calc_expected(enc, N_enc, white_adv, N_players, Ratingof, expected, BETA);

		olddev = curdev = deviation(N_players, Flagged, expected, Obtained, Playedby);

		if (!quiet) printf ("\nConvergence rating calculation (cycle #%d)\n\n", cycle+1);
		if (!quiet) printf ("%3s %4s %12s%14s\n", "phase", "iteration", "deviation","resolution");

		while (!done && n-->0) {
			bool_t failed = FALSE;
			double kk = 1.0;
			double cd, last_cd;
	
			last_cd = 100;

			for (i = 0; i < rounds && !done && !failed; i++) {

				ratings_backup(N_players, Ratingof, Ratingbk);
				olddev = curdev;

				resol = adjust_rating 	
					( delta
					, kappa*kk
					, N_players
					, Flagged
					, Prefed
					, expected 
					, Obtained 
					, Playedby
					, Ratingof
					, anchored_n
				);

				calc_expected(enc, N_enc, white_adv, N_players, Ratingof, expected, BETA);
				curdev = deviation(N_players, Flagged, expected, Obtained, Playedby);

				if (curdev >= olddev) {
					ratings_restore(N_players, Ratingbk, Ratingof);
					calc_expected(enc, N_enc, white_adv, N_players, Ratingof, expected, BETA);
					curdev = deviation(N_players, Flagged, expected, Obtained, Playedby);	
					assert (absol(curdev-olddev) < PRECISIONERROR || 
								!fprintf(stderr, "curdev=%.10e, olddev=%.10e, diff=%.10e\n", curdev, olddev, olddev-curdev));
					failed = TRUE;
				};	

				if (Multiple_anchors_present && anchored_n > 1) {

					cd = optimum_centerdelta	
						( last_cd
						, min_resol > 0.1? 0.1: min_resol
						, enc
						, N_enc
						, N_players
						, Ratingof
						, Flagged
						, Prefed
						, white_adv
						, BETA
						, Obtained
						, expected
						, Playedby
						, ratingtmp
						);

				} else if (Anchor_use && anchored_n == 1) {
					cd = 0;
				} else {
					cd = 0;
				}

				last_cd = cd;

				if (absol(cd) > MIN_RESOL) {
					mobile_center_apply_excess (cd, N_players, Flagged, Prefed, Ratingof);
					failed = FALSE;
				}

				if (!failed) { 
					calc_expected(enc, N_enc, white_adv, N_players, Ratingof, expected, BETA);
					curdev = deviation(N_players, Flagged, expected, Obtained, Playedby);	
	
					outputdev = 1000*sqrt(curdev/(double)N_games);
					done = outputdev < min_devia || (absol(resol)+absol(cd)) < min_resol;
					//kk *= 0.995;
					kk *= (1.0-1.0/200);
				}
			}

			delta /= denom;
			kappa *= denom;
			outputdev = 1000*sqrt(curdev/(double)N_games);

			if (!quiet) {
				printf ("%3d %7d %16.9f", phase, i, outputdev);
				printf ("%14.5f",resol);
				printf ("\n");
			}
			phase++;

		}

		if (!quiet) printf ("done\n");

		if (adjust_white_advantage) {
				white_adv = adjust_wadv (white_adv, Ratingof, N_enc, enc, BETA, doneonce? resol: START_DELTA);
				doneonce = TRUE;
				wa_progress = wa_previous > white_adv? wa_previous - white_adv: white_adv - wa_previous;
				wa_previous = white_adv;
				if (!quiet)
					printf ("Adjusted White Advantage = %.1f\n",white_adv);
		}

		if (adjust_draw_rate) {
				draw_rate = adjust_drawrate (white_adv, Ratingof, N_enc, enc, BETA);
				if (!quiet)
					printf ("Adjusted Draw Rate = %.1f %s\n\n", 100*draw_rate, "%");
		} 

		if (!quiet) 
			printf ("Post-Convergence rating estimation\n");

		N_enc = calc_encounters(ENCOUNTERS_FULL, g, Flagged, enc);
		calc_obtained_playedby(enc, N_enc, N_players, Obtained, Playedby);
		rate_super_players(quiet, enc, N_enc, Performance_type, (player_t)N_players, Ratingof, white_adv, Flagged, Name, draw_rate, BETA); //FIXME player_t
		N_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, g, Flagged, enc);;
		calc_obtained_playedby(enc, N_enc, N_players, Obtained, Playedby);

		if (!quiet) 
			printf ("done\n");

		// printf ("EXCESS =%lf\n", calc_excess	(N_players, Flagged, General_average, Ratingof));

		if (anchored_n == 1 && Anchor_use)
			adjust_rating_byanchor (Anchor, General_average, N_players, Ratingof);

	} //end while 

	*pWhite_advantage = white_adv;
	*pDraw_date = draw_rate;

	memrel(expected);
	return N_enc;
}


static double
overallerrorE_fdrawrate (size_t N_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv, double dr0)
{
	size_t e;
	int w, b;
	double dp2, f;
	double dexp;

	dp2 = 0;
	for (e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		f = xpect (ratingof[w] + wadv, ratingof[b], beta);

		dexp = draw_rate_fperf (f, dr0);

		dp2 +=
				enc[e].D                   * (1-dexp) * (1-dexp) +
				(enc[e].played - enc[e].D) *    dexp  *    dexp  ;
		;
	}

	return dp2;
}

#define DRAWRATE_RESOLUTION 0.0001

static double
adjust_drawrate (double start_wadv, const double *ratingof, size_t N_enc, const struct ENC *enc, double beta)
{
	double delta, wa, ei, ej, ek, dr;
	double lo,hi;

	delta = 0.5;
	wa = start_wadv;

	dr = 0.5;

	do {	

		ei = overallerrorE_fdrawrate (N_enc, enc, ratingof, beta, wa, dr - delta);
		ej = overallerrorE_fdrawrate (N_enc, enc, ratingof, beta, wa, dr + 0    );     
		ek = overallerrorE_fdrawrate (N_enc, enc, ratingof, beta, wa, dr + delta);

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


