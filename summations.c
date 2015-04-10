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

#include <stddef.h>
#include <assert.h>
#include <math.h>

#include "summations.h"
#include "mymem.h"

//---------------------------------- statics

static ptrdiff_t
head2head_idx_sdev (ptrdiff_t x, ptrdiff_t y)
{	
	ptrdiff_t idx;
	if (y < x) 
		idx = (x*x-x)/2+y;					
	else
		idx = (y*y-y)/2+x;
	return idx;
}

static double
get_sdev (double s1, double s2, double n)
{
	double xx = n*s2 - s1 * s1;
	xx = sqrt(xx*xx); // removes problems with -0.00000000000;
	return sqrt( xx ) /n;
}

static void
summations_clear (struct summations *sm, player_t nplayers)
{
	ptrdiff_t np = (ptrdiff_t)nplayers;
	ptrdiff_t est = (ptrdiff_t)((np*np-np)/2); /* elements of simulation table */
	ptrdiff_t i, idx;
	
	sm->wa_sum1 = 0;
	sm->wa_sum2 = 0;                               
	sm->dr_sum1 = 0;
	sm->dr_sum2 = 0; 
	sm->wa_sdev = 0;                               
	sm->dr_sdev = 0;
		
	for (idx = 0; idx < est; idx++) {
		sm->relative[idx].sum1 = 0;
		sm->relative[idx].sum2 = 0;
		sm->relative[idx].sdev = 0;
	}

	for (i = 0; i < np; i++) {
		sm->sum1[i] = 0;
		sm->sum2[i] = 0;
		sm->sdev[i] = 0;
	}
}

//---------------------------------- extern

bool_t 
summations_calloc (struct summations *sm, player_t nplayers)
{
	ptrdiff_t np = (ptrdiff_t)nplayers;
	ptrdiff_t est = (ptrdiff_t)((np*np-np)/2); /* elements of simulation table */
	size_t allocsize = sizeof(struct DEVIATION_ACC) * (size_t)est;

	double					*a;
	double 					*b;
	double 					*c;
	struct DEVIATION_ACC	*d;

	size_t		sa = sizeof(double);
	size_t		sb = sizeof(double);
	size_t		sc = sizeof(double);
	size_t		sd = allocsize;

	assert (sm);
	assert(nplayers > 0);

	if (NULL == (a = memnew (sa * (size_t)nplayers))) {
		return FALSE;
	} else 
	if (NULL == (b = memnew (sb * (size_t)nplayers))) {
		memrel(a);
		return FALSE;
	} else 
	if (NULL == (c = memnew (sc * (size_t)nplayers))) {
		memrel(a);
		memrel(b);
		return FALSE;
	} else 
	if (NULL == (d = memnew (sd))) {
		memrel(a);
		memrel(b);
		memrel(c);
		return FALSE;
	} 

	sm->sum1 	 	= a; 
	sm->sum2 	 	= b; 
	sm->sdev	 	= c; 
	sm->relative 	= d; 

	summations_clear (sm, nplayers);

	return TRUE;
}

void 
summations_init (struct summations *sm)
{
	assert (sm);
	sm->relative = NULL;
	sm->sum1 = NULL;
	sm->sum2 = NULL;
	sm->sdev = NULL; 
	sm->wa_sum1 = 0;
	sm->wa_sum2 = 0;                               
	sm->dr_sum1 = 0;
	sm->dr_sum2 = 0; 
	sm->wa_sdev = 0;                               
	sm->dr_sdev = 0;
	return;
}

void 
summations_done (struct summations *sm)
{
	assert (sm);

	if (sm->sum1) 		memrel (sm->sum1);
	if (sm->sum2) 		memrel (sm->sum2);
	if (sm->sdev)	 	memrel (sm->sdev);
	if (sm->relative) 	memrel (sm->relative);

	sm->sum1 	 	= NULL; 
	sm->sum2 	 	= NULL; 
	sm->sdev	 	= NULL; 
	sm->relative 	= NULL; 

	return;
}

void
summations_update	( struct summations *sm
					, player_t topn
					, double *ratingof
					, double white_advantage
					, double drawrate_evenmatch
)
{
	player_t i, j;
	ptrdiff_t idx;
	double diff;

	sm->wa_sum1 += white_advantage;
	sm->wa_sum2 += white_advantage * white_advantage;				
	sm->dr_sum1 += drawrate_evenmatch;
	sm->dr_sum2 += drawrate_evenmatch * drawrate_evenmatch;	

	// update summations for errors
	for (i = 0; i < topn; i++) {
		sm->sum1[i] += ratingof[i];
		sm->sum2[i] += ratingof[i]*ratingof[i];
		for (j = 0; j < i; j++) {
			idx = head2head_idx_sdev ((ptrdiff_t)i, (ptrdiff_t)j);
			assert(idx < (ptrdiff_t)((topn*topn-topn)/2));
			diff = ratingof[i] - ratingof[j];	
			sm->relative[idx].sum1 += diff; 
			sm->relative[idx].sum2 += diff * diff;
		}
	}
}

void
summations_calc_sdev (struct summations *sm, player_t topn, double sim_n)
{
	player_t i;
	ptrdiff_t est = (ptrdiff_t)((topn*topn-topn)/2); /* elements of simulation table */

	for (i = 0; i < topn; i++) {
		sm->sdev[i] = get_sdev (sm->sum1[i], sm->sum2[i], sim_n);
	}
	for (i = 0; i < est; i++) {
		sm->relative[i].sdev = get_sdev (sm->relative[i].sum1, sm->relative[i].sum2, sim_n);
	}
	sm->wa_sdev = get_sdev (sm->wa_sum1, sm->wa_sum2, sim_n+1);
	sm->dr_sdev = get_sdev (sm->dr_sum1, sm->dr_sum2, sim_n+1);
}

