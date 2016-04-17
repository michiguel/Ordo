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
#include "fit1d.h"

#include "mytimer.h"

#define START_DELTA           100
#define MIN_DEVIA             0.0000001
#define MIN_RESOL             0.000001
#define START_RESOL        10.0
#define ACCEPTABLE_RESOL      MIN_RESOL
#define PRECISIONERROR        (1E-16)
#define DRAWRATE_RESOLUTION   0.0000001

#if !defined(NDEBUG)
static bool_t is_nan (double x) {if (x != x) return TRUE; else return FALSE;}
#endif

static double adjust_drawrate (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta);

static void
ratings_copyto (player_t n_players, const double *r_fr, double *r_to)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		r_to[j] = r_fr[j];
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
	assert(!is_nan(accum));
	return accum;
}

// no globals
static double 
unfitness		( const struct ENC *enc
				, gamesnum_t		n_enc
				, player_t			n_players
				, const double *	ratingof
				, const bool_t *	flagged
				, double			white_adv
				, double			beta
				, const double *	obtained
				, const gamesnum_t *playedby
				, double *			expected /*@out@*/
)
{
		double dev;
		calc_expected (enc, n_enc, white_adv, n_players, ratingof, expected, beta);
		dev = deviation (n_players, flagged, expected, obtained, playedby);
		assert(!is_nan(dev));
		return dev;
}

static double
calc_excess		( player_t n_players
				, const bool_t *flagged
				, double general_average
				, const double *ratingof)
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


// no globals
static double
adjust_rating 	( double delta
				, double kappa
				, player_t n_players
				, player_t anchored_n
				, const bool_t *flagged
				, const bool_t *prefed
				, const double *expected 
				, const double *obtained 
				, const gamesnum_t *playedby
				, double *ratingof /*@out@*/
)
{
	player_t 	j;
	double 	d;
	double 	y = 1.0;
	double 	ymax = 0;

	assert (kappa > 0);
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
		if (flagged[j])
			continue;
		if (anchored_n > 1)	{
			if (	flagged[j]	// player previously removed
				|| 	prefed[j]	// already set, one of the multiple anchors
			) continue; 
		}

		assert(playedby[j] > 0);
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
		assert(!is_nan(ratingof[j]));
	}

	// Return maximum increase/decrease ==> "resolution"
	return ymax * delta;
}

// no globals
static void
adjust_rating_byanchor (player_t anchor, double general_average, player_t n_players, double *ratingof /*@out@*/)
{
	player_t j;
	double excess = ratingof[anchor] - general_average;	
	for (j = 0; j < n_players; j++) {
			ratingof[j] -= excess;
	}	
}

//========================= WHITE ADVANTAGE FUNCTIONS ===========================

static void
white_cal_obt_tot 	( gamesnum_t n_enc
					, const struct ENC *enc	
					, const double *ratingof
					, double beta
					, double wadv
					, double *pcal 		/*@out@*/
					, double *pobt		/*@out@*/
					, gamesnum_t *ptot 	/*@out@*/
					)
{
	gamesnum_t e, t, total=0;
	player_t w, b;
	double f;
	double W,D;
	double cal, obt; // calculated, obtained

	for (cal = 0, obt = 0, e = 0; e < n_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		W = (double)enc[e].W;
		D = (double)enc[e].D;
		t = enc[e].W + enc[e].D + enc[e].L;

		f = xpect (ratingof[w] + wadv, ratingof[b], beta);
		obt += W + D/2;
		cal += f * (double)t;
		total += t;
	}

	*pcal = cal;
	*pobt = obt;
	*ptot = total;

	return;
}

static double
overallerrorE_fwadv (gamesnum_t n_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv)
{
	gamesnum_t tot;
	double cal, obt; // calculated, obtained
	double dp2;

	white_cal_obt_tot (n_enc, enc, ratingof, beta, wadv, &cal, &obt, &tot);
	dp2 = (cal - obt) * (cal - obt);	
	return dp2/(double)tot;
}

struct UNFITWADV {
	gamesnum_t n_enc;
	const struct ENC *enc;
	const double *ratingof;
	double beta;
};

static double
unfit_wadv (double x, const void *p)
{
	double r;
	const struct UNFITWADV *q = p;
	assert(!is_nan(x));
	r = overallerrorE_fwadv (q->n_enc, q->enc, q->ratingof, q->beta, x);
	assert(!is_nan(r));
	return r;
}

static double
adjust_wadv (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta, double start_delta)
{
	double delta, wa, ei, ej, ek;
	struct UNFITWADV p;

	delta = start_delta;
	wa = start_wadv;

	p.n_enc 	= n_enc;
	p.enc   	= enc;
	p.ratingof 	= ratingof;
	p.beta		= beta;

	do {	

		ei = unfit_wadv (wa - delta, &p);
		ej = unfit_wadv (wa + 0    , &p);     
		ek = unfit_wadv (wa + delta, &p);

		if (ei >= ej && ej <= ek) {
			wa = quadfit1d	(MIN_RESOL, wa - delta, wa + delta, unfit_wadv, &p);
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

	} while (
		delta > MIN_RESOL 
		&& -1000 < wa && wa < 1000
	);
	
	return wa;
}

//=================== DRAW RATE FUNCTIONS ===============================

static double
overallerrorE_fdrawrate (gamesnum_t n_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv, double dr0)
{
	gamesnum_t e;
	player_t w, b;
	double dp2, f;
	double W,D,L;
	double cal, obt; // calculated, obtained
	double dexp;

	for (cal = 0, obt = 0, e = 0; e < n_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		W = (double)enc[e].W;
		D = (double)enc[e].D;
		L = (double)enc[e].L;

		f = xpect (ratingof[w] + wadv, ratingof[b], beta);
		dexp = draw_rate_fperf (f, dr0);
		obt += D;
		cal += dexp * (W + D + L);
	}

	dp2 = (cal - obt) * (cal - obt);	

	return dp2;
}

struct UNFITDRAWRATE {
	gamesnum_t n_enc;
	const struct ENC *enc;
	const double *ratingof;
	double beta;
	double wadv;
};

static double
unfit_drawrate (double x, const void *p)
{
	double r;
	const struct UNFITDRAWRATE *q = p;
	assert(!is_nan(x));
	r = overallerrorE_fdrawrate (q->n_enc, q->enc, q->ratingof, q->beta, q->wadv, x);
	assert(!is_nan(r));
	return r;
}

static double
adjust_drawrate (double start_wadv, const double *ratingof, gamesnum_t n_enc, const struct ENC *enc, double beta)
{
	double delta, ei, ej, ek, dr;
	double lo,hi;
	struct UNFITDRAWRATE p;
	p.n_enc 	= n_enc;
	p.enc   	= enc;
	p.ratingof 	= ratingof;
	p.beta		= beta;
	p.wadv		= start_wadv;

	delta = 0.5;
	dr = 0.5;

	do {	
		ei = unfit_drawrate (dr - delta, &p);
		ej = unfit_drawrate (dr + 0    , &p);     
		ek = unfit_drawrate (dr + delta, &p);

		if (ei >= ej && ej <= ek) {
			dr = quadfit1d	(DRAWRATE_RESOLUTION, dr - delta, dr + delta, unfit_drawrate, &p);
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

		hi = dr+delta > 1.0? 1.0: dr+delta;
		lo = dr-delta < 0.0? 0.0: dr-delta;
		dr = (hi+lo)/2;
		delta = (hi-lo)/2;

	} while (delta > DRAWRATE_RESOLUTION);
	
	return dr;
}

//============ CENTER ADJUSTMENT FUNCTIONS ==================================



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
	ratings_copyto (n_players, ratingof, ratingtmp);
	mobile_center_apply_excess (excess, n_players, flagged, prefed, ratingtmp);
	u = unfitness	( enc
					, n_enc
					, n_players
					, ratingtmp
					, flagged
					, white_adv
					, beta
					, obtained
					, playedby
					, expected
					);
	assert(!is_nan(u));
	return u;
}

static double absol(double x) {return x >= 0? x: -x;}

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

//============ MAIN RATING FUNCTION =========================================

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

static double
get_outputdev (double curdev, gamesnum_t n_games)
{
	return 1000*sqrt(curdev/(double)n_games);
}

gamesnum_t
calc_rating_ordo 	
				( bool_t 			quiet
				, bool_t 			adjust_white_advantage
				, bool_t			adjust_draw_rate
				, bool_t			anchor_use

				, double			*ratingtmp_buffer

				, double			BETA
				, double			general_average
				, player_t			anchor

				, struct ENCOUNTERS *encount
				, struct PLAYERS 	*plyrs
				, struct GAMES 		*g
				, struct RATINGS 	*rat

				, double			*pWhite_advantage
				, double			*pDraw_date
)
{
	gamesnum_t	n_games = g->n;
	double 	*	ratingtmp = ratingtmp_buffer;
	double 		olddev, curdev;
	int 		i;
	int			rounds;
	double 		delta;
	double 		kappa;
	double 		damp_delta;
	double 		damp_kappa;
	double 		KK_DAMP;
	int 		phase;
	int 		n;
	double 		resol;
	int 		max_cycle;
	int 		cycle;

	double 		white_adv = *pWhite_advantage;
	double 		wa_previous = *pWhite_advantage;
	double 		wa_progress = START_DELTA;
	double 		min_devia = MIN_DEVIA;
	double 		draw_rate = *pDraw_date;
	double *	expected = NULL;

	// translation variables for refactoring ------------------
	struct ENC *	enc   			= encount->enc;
	gamesnum_t		n_enc 			= encount->n;
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
	//----------------------------------------------------------

	//double RAT[20000];

	assert(ratings_sanity (n_players, ratingof));
	if (NULL == (expected = memnew(sizeof(double) * (size_t)(n_players+1)))) {
		fprintf(stderr, "Not enough memory to allocate all players\n");
		exit(EXIT_FAILURE);
	}

	max_cycle = adjust_white_advantage? 4: 1;

	for (resol = START_RESOL, cycle = 0; 
		 resol > ACCEPTABLE_RESOL || (cycle < max_cycle && wa_progress > 0.01); 
		 cycle++) {

		bool_t done = FALSE;

		KK_DAMP = 1000;
		rounds = 10000;
		delta = START_DELTA;
		kappa = 0.05;
		damp_delta = 1.1;
		damp_kappa = 1.0;
		phase = 0;
		n = 1000;

		calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);
		assert(playedby_sanity (n_players, playedby, flagged));

		olddev = curdev = unfitness	( enc, n_enc, n_players, ratingof, flagged, white_adv, BETA, obtained, playedby, expected);

		if (!quiet) printf ("\nConvergence rating calculation (cycle #%d)\n\n", cycle+1);
		if (!quiet) printf ("%3s %4s %12s%14s\n", "phase", "iteration", "deviation","resolution");

		while (!done && n-->0) {
			bool_t failed = FALSE;
			double kk = 1.0;
			double cd;
			double last_cd = 100;

			assert(ratings_sanity (n_players, ratingof)); //%%
			// adjust white advantage and draw rate at the beginning
			if (adjust_white_advantage) {
					white_adv = adjust_wadv (white_adv, ratingof, n_enc, enc, BETA, resol);
					wa_progress = wa_previous > white_adv? wa_previous - white_adv: white_adv - wa_previous;
					wa_previous = white_adv;
			}
			if (adjust_draw_rate) {
					draw_rate = adjust_drawrate (white_adv, ratingof, n_enc, enc, BETA);
			} 

			for (i = 0; i < rounds && !done && !failed; i++) {

				cd = 0;

				ratings_copyto (n_players, ratingof, ratingbk); // backup
				olddev = curdev;

				assert(ratings_sanity (n_players, ratingof)); //%%
				assert(playedby_sanity (n_players, playedby, flagged)); //%%

				resol = adjust_rating 	
							( delta
							, kappa*kk
							, n_players
							, anchored_n
							, flagged
							, prefed
							, expected 
							, obtained 
							, playedby
							, ratingof
							);

				curdev = unfitness ( enc, n_enc, n_players, ratingof, flagged, white_adv, BETA, obtained, playedby, expected);
				failed = curdev >= olddev;

				if (failed) {
					ratings_copyto (n_players, ratingbk, ratingof); // restore
					curdev = unfitness ( enc, n_enc, n_players, ratingof, flagged, white_adv, BETA, obtained, playedby, expected);
					assert (i == 0 || absol(curdev-olddev) < PRECISIONERROR || 
								!fprintf(stderr, "i=%d, curdev=%.10e, olddev=%.10e, diff=%.10e\n", i, curdev, olddev, olddev-curdev));
				} else {
					cd = 0; // includes the case (anchor_use && anchored_n == 1)
					if (anchored_n > 1) {
						cd = optimum_centerdelta	
							( last_cd
							, MIN_RESOL
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
					} 
					last_cd = cd;

					if (absol(cd) > MIN_RESOL) {
						mobile_center_apply_excess (cd, n_players, flagged, prefed, ratingof);
					}

					curdev = unfitness ( enc, n_enc, n_players, ratingof, flagged, white_adv, BETA, obtained, playedby, expected);
					kk *= (1.0-1.0/KK_DAMP); //kk *= 0.995;
				}

				done = get_outputdev (curdev, n_games) < min_devia && (absol(resol)+absol(cd)) < MIN_RESOL;

			} // end rounds

			delta /= damp_delta;
			kappa *= damp_kappa;

			if (!quiet) {
				printf ("%3d %7d %16.9f", phase, i, get_outputdev (curdev, n_games));
				printf ("%14.5f", resol);
				//	printf ("%10.5lf",ratings_rmsd(n_players, ratingof, RAT));
				printf ("\n");
			}
			phase++;

		} // end n-->0

		if (!quiet) printf ("done\n");

		if (adjust_white_advantage) {
				white_adv = adjust_wadv (white_adv, ratingof, n_enc, enc, BETA, resol);
				wa_progress = wa_previous > white_adv? wa_previous - white_adv: white_adv - wa_previous;
				wa_previous = white_adv;
		}
		if (adjust_draw_rate) {
				draw_rate = adjust_drawrate (white_adv, ratingof, n_enc, enc, BETA);
		} 

		if (!quiet)	printf ("\nWhite Advantage = %.1f", white_adv);
		if (!quiet)	printf ("\nDraw Rate (eq.) = %.1f %s\n\n", 100*draw_rate, "%");

		if (anchored_n == 1 && anchor_use)
			adjust_rating_byanchor (anchor, general_average, n_players, ratingof);

	} //end cycles 


	if (anchored_n == 0) {
		double excess = calc_excess (n_players, flagged, general_average, ratingof);
		correct_excess (n_players, flagged, excess, ratingof);
	}

	timelog("Post-Convergence rating estimation...");

	encounters_calculate(ENCOUNTERS_FULL, g, flagged, encount);
	enc   = encount->enc;
	n_enc = encount->n;

	calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);

	timelog("rate_super_players...");

	rate_super_players(quiet, enc, n_enc, Performance_type, n_players, ratingof, white_adv, flagged, name, draw_rate, BETA); 

	encounters_calculate(ENCOUNTERS_NOFLAGGED, g, flagged, encount);
	enc   = encount->enc;
	n_enc = encount->n;

	calc_obtained_playedby(enc, n_enc, n_players, obtained, playedby);

	timelog("done with rating calculation.");

	*pWhite_advantage = white_adv;
	*pDraw_date = draw_rate;

	memrel(expected);
	return n_enc;
}

