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


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "indiv.h"
#include "ordolim.h"
#include "xpect.h"
#include "mymem.h"

//===============================================================

#if 1
static double
ind_expected (double x, double *rtng, double *weig, int n, double beta)
{
	int i;
	double cume = 0;
	for (i = 0; i < n; i++) {
		cume += weig[i] * xpect (x, rtng[i], beta);
	}
	return cume;
}

static double 
adjust_x (double x, double xp, double sc, double delta, double kappa)
{
	double y;
	double	d;
	d = xp - sc;
	
	d = d < 0? -d: d;
	y = d / (kappa+d);
	if (xp > sc) {
		x -= delta * y;
	} else {
		x += delta * y;
	}
	return x;	
}


static double
calc_ind_rating(double cume_score, double *rtng, double *weig, int r, double beta)
{
	int 	i;
	double 	olddev, curdev;
	int		rounds = 10000;
	double 	delta = 200.0;
	double 	kappa = 0.05;
	double 	denom = 2;
	int 	phase = 0;
	int 	n = 20;

	double  D,sc,oldx;

	double x = 2000;
	double xp;

	D = cume_score - ind_expected(x,rtng,weig,r,beta) ;
	curdev = D*D;
	olddev = curdev;

	while (n-->0) {
		double kk = 1.0;
		for (i = 0; i < rounds; i++) {
			oldx = x;
			olddev = curdev;

			sc = cume_score;
			xp = ind_expected(x,rtng,weig,r,beta);
			x  = adjust_x (x, xp, sc, delta, kappa*kk);
			xp = ind_expected(x,rtng,weig,r,beta);
			D = xp - sc;
			curdev = D*D;

			if (curdev >= olddev) {
				x = oldx;
#if 0
				D = cume_score - ind_expected(x,rtng,weig,r,beta) ;
				curdev = D*D;	
				assert (curdev == olddev);
#else
				curdev = olddev;
#endif	

				break;
			};	

			if (curdev < 0.000001) break;
			kk *= 0.995;
		}

		delta /= denom;
		kappa *= denom;

		phase++;

		if (curdev < 0.000001) break;
	}

	return x;
}
#endif

//=========================================


static double
prob2absolute_result (int perftype, double myrating, double *rtng, double *weig, int n, double deq, double beta)
{
	int i;
	double p, cume;
	double pwin, pdraw, ploss;
	assert(n);
	assert(perftype == PERF_SUPERWINNER || perftype == PERF_SUPERLOSER);
	assert(deq <= 1 && deq >= 0);

	cume = 1.0;
	if (PERF_SUPERWINNER == perftype) {
		for (i = 0; i < n; i++) {
			get_pWDL(myrating - rtng[i], &pwin, &pdraw, &ploss, deq, beta);
			p = pwin;
			cume *= exp(weig[i] * log (p)); // p ^ weight
		}	
	} else {
		for (i = 0; i < n; i++) {
			get_pWDL(myrating - rtng[i], &pwin, &pdraw, &ploss, deq, beta);
			p = ploss;
			cume *= exp(weig[i] * log (p)); // p ^ weight
		}
	}
	return cume;
}


static double
calc_ind_rating_superplayer (int perf_type, double x_estimated, double *rtng, double *weig, int r, double deq, double beta)
{
	int 	i;
	double 	old_unfit, cur_unfit;
	int		rounds = 2000;
	double 	delta = 200.0;
	double 	denom = 2;
	double fdelta;
	double  D, oldx;
	double x = x_estimated;

	assert(r);

	if (perf_type == PERF_SUPERLOSER) 
		D = - 0.5 + prob2absolute_result(perf_type, x, rtng, weig, r, deq, beta);		
	else
		D = + 0.5 - prob2absolute_result(perf_type, x, rtng, weig, r, deq, beta);

	cur_unfit = D * D;
	old_unfit = cur_unfit;

	fdelta = D < 0? -delta:  delta;	

	for (i = 0; i < rounds; i++) {

		oldx = x;
		old_unfit = cur_unfit;

		x += fdelta;

		if (perf_type == PERF_SUPERLOSER) 
			D = - 0.5 + prob2absolute_result(perf_type, x, rtng, weig, r, deq, beta);		
		else
			D = + 0.5 - prob2absolute_result(perf_type, x, rtng, weig, r, deq, beta);

		cur_unfit = D * D;
		fdelta = D < 0? -delta: delta;


		if (cur_unfit >= old_unfit) {
			x = oldx;
			cur_unfit = old_unfit;
			delta /= denom;

		} else {

		}	

		if (cur_unfit < 0.0000000001) break;
	}

	return x;
}

static double calc_ind_rating(double cume_score, double *rtng, double *weig, int r, double beta);
static double calc_ind_rating_superplayer (int perf_type, double x_estimated, double *rtng, double *weig, int r, double deq, double beta);

static void
rate_super_players_internal
					( bool_t quiet
					, struct ENC *enc
					, size_t N_enc
					, int *performance_type
					, player_t n_players
					, double *ratingof
					, double white_advantage
					, bool_t *flagged
					, const char *Name[]
					, double deq
					, double beta
					, struct ENC *myenc
					, double weig[]
					, double rtng[]
)
{
	size_t e;
	player_t j;
	size_t myenc_n = 0;

		for (j = 0; j < n_players; j++) {

			if (performance_type[j] != PERF_SUPERWINNER && performance_type[j] != PERF_SUPERLOSER) 
				continue;

			myenc_n = 0; // reset

			for (e = 0; e < N_enc; e++) {
				player_t w = enc[e].wh;
				player_t b = enc[e].bl;
				if (j == w) { 
					myenc[myenc_n++] = enc[e];
				} else
				if (j == b) { 
					myenc[myenc_n++] = enc[e];
				}
			}

			if (!quiet) {
				if (0 == myenc_n) {
					printf ("  no games   --> %s\n", Name[j]);
				} else
				if (performance_type[j] == PERF_SUPERWINNER) {
					printf ("  all wins   --> %s\n", Name[j]);
				} else
				if (performance_type[j] == PERF_SUPERLOSER) {
					printf ("  all losses --> %s\n", Name[j]);
				}
			}

			if (0 != myenc_n)
			{
				double	cume_score = 0; 
				double	cume_total = 0;
				int		r = 0;

				while (myenc_n-->0) {
					size_t n = myenc_n;
					if (myenc[n].wh == j) { 
						player_t opp = myenc[n].bl;
						weig[r	] = (double)myenc[n].played;
						rtng[r++] = ratingof[opp] - white_advantage;
						cume_score += myenc[n].wscore;
						cume_total += (double)myenc[n].played;
				 	} else 
					if (myenc[myenc_n].bl == j) { 
						player_t opp = myenc[n].wh;
						weig[r	] = (double)myenc[n].played;
						rtng[r++] = ratingof[opp] + white_advantage;
						cume_score += (double)myenc[n].played - myenc[n].wscore;
						cume_total += (double)myenc[n].played;
					} else {
						fprintf(stderr,"ERROR!! function rate_super_players()\n");
						exit(0);
						continue;
					} 
				}

				if (performance_type[j] == PERF_SUPERWINNER) {
					double ori_estimation = calc_ind_rating (cume_score-0.25, rtng, weig, r, beta); 
					assert(r);
					ratingof[j] = calc_ind_rating_superplayer (PERF_SUPERWINNER, ori_estimation, rtng, weig, r, deq, beta);
				}
				if (performance_type[j] == PERF_SUPERLOSER) {
					double ori_estimation = calc_ind_rating (cume_score+0.25, rtng, weig, r, beta); 
					assert(r);
					ratingof[j] = calc_ind_rating_superplayer (PERF_SUPERLOSER,  ori_estimation, rtng, weig, r, deq, beta);
				}
				flagged[j] = FALSE;
			} else {
				assert (performance_type[j] == PERF_NOGAMES);
				flagged[j] = TRUE;
			}

		}

	return;
}

//
void
rate_super_players	( bool_t quiet
					, struct ENC *enc
					, gamesnum_t N_enc
					, int *performance_type
					, player_t n_players
					, double *ratingof
					, double white_advantage
					, bool_t *flagged
					, const char *Name[]
					, double deq
					, double beta
)
{
	struct ENC 	*myenc = NULL;
	double 		*weig = NULL;
	double 		*rtng = NULL;
	bool_t		ok;
	size_t		np = (size_t) n_players;
	size_t		ne = (size_t) N_enc;

	if (NULL != (weig = memnew(sizeof(double) * ne))) {
		if (NULL != (rtng = memnew(sizeof(double) * ne))) {
			if (NULL != (myenc = memnew (sizeof(struct ENC) * ne))) {

				rate_super_players_internal
					( quiet
					, enc
					, ne
					, performance_type
					, (player_t) np
					, ratingof
					, white_advantage
					, flagged
					, Name
					, deq
					, beta
					, myenc
					, weig
					, rtng
					);

				memrel(myenc);
			}
			memrel(rtng);
		}
		memrel(weig);
	} 
	ok = myenc != NULL && rtng != NULL && weig != NULL;

	if (!ok) {
		fprintf(stderr,"not enough memory for allocation in rate_super_players.");
		exit(EXIT_FAILURE);
	}

	return;
}

