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
#include <math.h>

#include "encount.h"
#include "mytypes.h"
#include "pgnget.h"
#include "xpect.h"

//Statics

static struct ENC encounter_merge (const struct ENC *a, const struct ENC *b);
static int shrink_ENC (struct ENC *enc, int N_enc);
static int compare_ENC (const void * a, const void * b);

//=======================================================================

static int compare_ENC (const void * a, const void * b)
{
	const struct ENC *ap = a;
	const struct ENC *bp = b;
	if (ap->wh == bp->wh && ap->bl == bp->bl) return 0;
	if (ap->wh == bp->wh) {
		if (ap->bl > bp->bl) return 1; else return -1;
	} else {	 
		if (ap->wh > bp->wh) return 1; else return -1;
	}
	return 0;	
}

// no globals
int
calc_encounters ( int selectivity
				, int n_games
				, const int *score 
				, const bool_t *flagged
				, const int *whiteplayer
				, const int *blackplayer
				, struct ENC *enc
)
{
	int i, e = 0;
	int ne;

	for (i = 0; i < n_games; i++) {

		if (score[i] >= DISCARD) continue;

		if (selectivity == ENCOUNTERS_NOFLAGGED) {
			if (flagged[whiteplayer[i]] || flagged[blackplayer[i]])
				continue;
		}

		enc[e].W = 0;
		enc[e].D = 0;
		enc[e].L = 0;
		switch (score[i]) {
			case WHITE_WIN: 	enc[e].wscore = 1.0; enc[e].W = 1; break;
			case RESULT_DRAW:	enc[e].wscore = 0.5; enc[e].D = 1; break;
			case BLACK_WIN:		enc[e].wscore = 0.0; enc[e].L = 1; break;
		}

		enc[e].wh = whiteplayer[i];
		enc[e].bl = blackplayer[i];
		enc[e].played = 1;
		e++;
	}
	ne = e;

	ne = shrink_ENC (enc, ne);
	qsort (enc, (size_t)ne, sizeof(struct ENC), compare_ENC);
	ne = shrink_ENC (enc, ne);

	return ne;
}

// no globals
void
calc_obtained_playedby (const struct ENC *enc, int N_enc, int n_players, double *obtained, int *playedby)
{
	int e, j, w, b;

	for (j = 0; j < n_players; j++) {
		obtained[j] = 0.0;	
		playedby[j] = 0;
	}	

	for (e = 0; e < N_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		obtained[w] += enc[e].wscore;
		obtained[b] += (double)enc[e].played - enc[e].wscore;

		playedby[w] += enc[e].played;
		playedby[b] += enc[e].played;
	}
}

// no globals
void
calc_expected (const struct ENC *enc, int N_enc, double white_advantage, int n_players, const double *ratingof, double *expected, double beta)
{
	int e, j, w, b;
	double f;
	double wperf,bperf;

	for (j = 0; j < n_players; j++) {
		expected[j] = 0.0;	
	}	

	for (e = 0; e < N_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		f = xpect (ratingof[w] + white_advantage, ratingof[b], beta);

		wperf = enc[e].played * f;
		bperf = enc[e].played - wperf;

		expected [w] += wperf; 
		expected [b] += bperf; 

	}
}

static struct ENC 
encounter_merge (const struct ENC *a, const struct ENC *b)
{
		struct ENC r;	
		assert(a->wh == b->wh);
		assert(a->bl == b->bl);
		r.wh = a->wh;
		r.bl = a->bl; 
		r.wscore = a->wscore + b->wscore;
		r.played = a->played + b->played;

		r.W = a->W + b->W;
		r.D = a->D + b->D;
		r.L = a->L + b->L;
		return r;
}

static int
shrink_ENC (struct ENC *enc, int N_enc)
{
	int e, g;

	if (N_enc == 0) return 0; 

	g = 0;
	for (e = 1; e < N_enc; e++) {
	
		if (enc[e].wh == enc[g].wh && enc[e].bl == enc[g].bl) {
			enc[g] = encounter_merge (&enc[g], &enc[e]);
		}
		else {
			g++;
			enc[g] = enc[e];
		}
	}
	g++;
	return g; // New N_encounters
}

