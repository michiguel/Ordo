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
#include "mymem.h"

//Statics

static struct ENC encounter_merge (const struct ENC *a, const struct ENC *b);
static gamesnum_t shrink_ENC (struct ENC *enc, gamesnum_t N_enc);
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

static gamesnum_t
calc_encounters ( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENC *enc // out
);


//-----------------------------------------------------------------------

bool_t 
encounters_init (gamesnum_t n, struct ENCOUNTERS *e)
{
	struct ENC 	*p;

	assert (n > 0);

	if (NULL == (p = memnew (sizeof(struct ENC) * (size_t)n))) {
		e->n	 	= 0; 
		e->size 	= 0;
		e->enc		= NULL;
		return FALSE; // failed
	}

	e->n	 	= 0; /* empty for now */
	e->size 	= n;
	e->enc		= p;
	return TRUE;
}

void 
encounters_done (struct ENCOUNTERS *e)
{
	memrel(e->enc);
	e->n	 = 0;
	e->size	 = 0;
	e->enc	 = NULL;
} 


void
encounters_copy (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt)
{
	gamesnum_t i, n = src->n;
	struct ENC *s = src->enc;
	struct ENC *t = tgt->enc; 
	tgt->n = src->n;
	tgt->size = src->size;
	for (i = 0; i < n; i++) {
		t[i] = s[i];
	}
}

bool_t
encounters_replicate (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt)
{
	bool_t ok;
	ok = encounters_init (src->size, tgt);
	if (ok) {
		encounters_copy (src, tgt);
	}
	return ok;
}



void
encounters_calculate
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
}

// no globals
static gamesnum_t
calc_encounters ( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENC *enc // out
)
{
	gamesnum_t n_games = g->n;
	const struct gamei *gam = g->ga;
	gamesnum_t i;
	gamesnum_t e = 0;
	gamesnum_t ne;
	bool_t skip;

	assert(enc);

	for (i = 0; i < n_games; i++) {
		int32_t score_i = gam[i].score;
		player_t wp_i = gam[i].whiteplayer;
		player_t bp_i = gam[i].blackplayer;

		skip = score_i >= DISCARD || (selectivity == ENCOUNTERS_NOFLAGGED && (flagged[wp_i] || flagged[bp_i]));

		if (!skip)	{
			enc[e].wh = wp_i;
			enc[e].bl = bp_i;
			enc[e].played = 1;
			enc[e].W = 0;
			enc[e].D = 0;
			enc[e].L = 0;
			switch (score_i) {
				case WHITE_WIN: 	enc[e].wscore = 1.0; enc[e].W = 1; break;
				case RESULT_DRAW:	enc[e].wscore = 0.5; enc[e].D = 1; break;
				case BLACK_WIN:		enc[e].wscore = 0.0; enc[e].L = 1; break;
			}
			e++;
		}
	}
	ne = e;

	ne = shrink_ENC (enc, ne);
	if (ne > 0) {
		qsort (enc, (size_t)ne, sizeof(struct ENC), compare_ENC);
		ne = shrink_ENC (enc, ne);
	}
	return ne;
}


// no globals
void
calc_obtained_playedby (const struct ENC *enc, gamesnum_t N_enc, player_t n_players, double *obtained, gamesnum_t *playedby)
{
	player_t 	w, b;
	player_t 	j;
	gamesnum_t 	e;


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

#if !defined(NDEBUG)
static bool_t is_nan (double x) {if (x != x) return TRUE; else return FALSE;}
#endif

#if !defined(NDEBUG)
bool_t
ratings_sanity (player_t n_players, const double *ratingof)
{
	player_t 	j;
	bool_t found_nan = FALSE;
	for (j = 0; j < n_players && !found_nan; j++) {
		found_nan = is_nan(ratingof[j]);
	}
	if (found_nan) {
		for (j = 0; j < n_players; j++) {
			fprintf(stderr, "rating[%ld] = %lf\n",j,ratingof[j]);
		}
	}
	return !found_nan;
}


bool_t
playedby_sanity (player_t n_players, const gamesnum_t *pb, const bool_t *flagged)
{
	player_t 	j;
	bool_t found_bad = FALSE;
	for (j = 0; j < n_players && !found_bad; j++) {
		found_bad = !(pb[j] > 0) && !flagged[j];
	}
	if (found_bad) {
		for (j = 0; j < n_players; j++) {
			fprintf(stderr, "fl=%d playedby[%ld] = %ld\n",flagged[j],j,(long)pb[j]);
		}
	}
	return !found_bad;
}
#endif

// no globals
void
calc_expected 	( const struct ENC *enc
				, gamesnum_t N_enc
				, double white_advantage
				, player_t n_players
				, const double *ratingof
				, double *expected
				, double beta)
{
	player_t 	w, b;
	player_t 	j;
	gamesnum_t 	e;
	double wperf;

	assert(ratings_sanity (n_players, ratingof));

	for (j = 0; j < n_players; j++) {
		expected[j] = 0.0;	
	}	
	for (e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		assert(!is_nan ((double)enc[e].played) );
		assert(!is_nan (white_advantage)       );
		assert(!is_nan (ratingof[w])           );
		assert(!is_nan (ratingof[b])           || !printf("b=%ld\n",(long)b));
		wperf = (double)enc[e].played * xpect (ratingof[w] + white_advantage, ratingof[b], beta);
		assert(!is_nan(wperf));
		expected [b] += (double)enc[e].played - wperf; 
		expected [w] += wperf; 
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


static gamesnum_t
shrink_ENC (struct ENC *enc, gamesnum_t N_enc)
{
	gamesnum_t e;
	gamesnum_t g;

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

//------------------------------------------------------------

static int compare_plist (const void * a, const void * b)
{
	const player_t *ap = a;
	const player_t *bp = b;
	return *ap > *bp? 1: (*ap < *bp? -1: 0);	
}

static player_t
compact_plist (player_t *plist, player_t plist_n)
{
	player_t r, i;

	qsort (plist, (size_t)plist_n, sizeof(player_t), compare_plist);

	r = 0;
	i = 1;		
	while (i < plist_n) {

		if (plist[i] == plist[r]) {
			i++;
		} else {
			plist[r+1] = plist[i];
			r++;
			i++;
		}
	}

	return plist_n > 0? r+1: 0;
}


#include "mymem.h"
#include "string.h"

typedef struct ENCNODE encnode_t;
 
struct ENCNODE {
	const struct ENC *encounter;
	encnode_t *whitenext;
	encnode_t *blacknext;
};

// no globals
void
calc_output_info
			 	( const struct ENC *enc
				, gamesnum_t N_enc
				, const double *ratingof
				, player_t n_players
				, double *sdev
				, struct OUT_INFO *oi
				, player_t *sorted
				, player_t sorted_n
				)
{
	encnode_t * *pend;
	encnode_t 	*ep;
	gamesnum_t 	*p__;
	player_t 	*plist;
	player_t 	plist_n;

	player_t 	wh, bl;
	player_t 	j, o, q, z;
	gamesnum_t 	e, n;

	if (NULL == (pend  = memnew(sizeof(encnode_t *) * (size_t)n_players))) {
		fprintf(stderr, "No enough memory available, FILE %s, LINE %d\n", __FILE__, __LINE__); exit(EXIT_FAILURE);
	}
	if (NULL == (ep    = memnew(sizeof(encnode_t  ) * (size_t)N_enc    ))) {
		memrel(pend);
		fprintf(stderr, "No enough memory available, FILE %s, LINE %d\n", __FILE__, __LINE__); exit(EXIT_FAILURE);
	}
	if (NULL == (p__   = memnew(sizeof(gamesnum_t ) * (size_t)n_players))) {
		memrel(pend);
		memrel(ep);
		fprintf(stderr, "No enough memory available, FILE %s, LINE %d\n", __FILE__, __LINE__); exit(EXIT_FAILURE);
	}
	if (NULL == (plist = memnew(sizeof(player_t   ) * (size_t)n_players * 2))) {
		memrel(pend);
		memrel(ep);
		memrel(p__);
		fprintf(stderr, "No enough memory available, FILE %s, LINE %d\n", __FILE__, __LINE__); exit(EXIT_FAILURE);
	}

	for (j = 0; j < n_players; j++) {
		oi[j].W = 0;
		oi[j].D = 0;	
		oi[j].L = 0;	
		oi[j].opprat = 0;
		oi[j].opperr = 0;
		oi[j].n_opp = 0;
		oi[j].diversity = 0.0;
	}	
	for (e = 0; e < N_enc; e++) {
		n  = enc[e].W + enc[e].D + enc[e].L;
		wh = enc[e].wh;
		bl = enc[e].bl;
		oi[wh].W += enc[e].W;
		oi[wh].D += enc[e].D;
		oi[wh].L += enc[e].L;
		oi[bl].W += enc[e].L;
		oi[bl].D += enc[e].D;
		oi[bl].L += enc[e].W;
		oi[wh].opprat += ratingof[bl] * (double)n;
		oi[bl].opprat += ratingof[wh] * (double)n;

	}
	for (j = 0; j < n_players; j++) {
		n =	oi[j].W + oi[j].D + oi[j].L;
		oi[j].opprat /= (double)n;
	}

//

	for (j = 0; j < n_players; j++) {
		pend[j] = NULL;
	}

	for (e = 0; e < N_enc; e++) {
		wh = enc[e].wh;
		bl = enc[e].bl;
		ep[e].encounter = &enc[e];
		ep[e].whitenext = pend[wh];
		ep[e].blacknext = pend[bl];
		pend[wh] = &ep[e];
		pend[bl] = &ep[e];
	}

//	for (j = 0; j < n_players; j++) {
	for (z = 0; z < sorted_n; z++) {
		double sum = 0.0;
		player_t n_opp = 0;
		player_t opp;
		encnode_t *x;
		gamesnum_t n_games = 0;

		j = sorted[z];
		x = pend[j];

		memset (p__, 0, sizeof(p__[0])*(size_t)n_players);
		plist_n = 0;

		while (x) {
			enc = x->encounter;
			opp = enc->wh == j? enc->bl: enc->wh;
			p__[opp] += enc->W + enc->D + enc->L;
			assert(plist_n < (sorted_n * 2));
			plist[plist_n++] = opp;
			x = enc->wh == j? x->whitenext: x->blacknext;
		}

		plist_n = compact_plist (plist, plist_n);

		n_games = 0;
		n_opp = 0;
		for (q = 0; q < plist_n; q++) {
			o = plist[q];
			if (p__[o] > 0) {
				n_opp++;
				n_games += p__[o];
			}
		}

		sum = 0;
		for (q = 0; q < plist_n; q++) {
			o = plist[q];
			if (p__[o] > 0) {
				double pi = (double)p__[o]/(double)n_games;
				sum +=  -pi * log(pi);
			}
		}

		oi[j].n_opp = n_opp;
		oi[j].diversity = exp(sum);

		if (sdev) {
			sum = 0;
			for (q = 0; q < plist_n; q++) {
				o = plist[q];
				if (p__[o] > 0) {
					sum += (double)p__[o] * sdev[o];
				}
			}
			oi[j].opperr = sum/(double)n_games;
		}
	}

	memrel(pend);
	memrel(ep);
	memrel(p__);
	memrel(plist);
}

