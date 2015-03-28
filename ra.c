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

#include "ra.h"


void
ratings_starting_point (player_t n, double rat0, struct RATINGS *rat)
{
	player_t i;
	for (i = 0; i < n; i++) {
		rat->ratingof[i] = rat0;
	}
	for (i = 0; i < n; i++) {
		rat->ratingbk[i] = rat0;
	}
}


void
ratings_set (player_t n_players, double general_average, const bool_t *prefed, const bool_t *flagged, double *rating)
{
	player_t i;
	for (i = 0; i < n_players; i++) {
		if (!prefed[i] && !flagged[i])
			rating[i] = general_average;
	}
}

void
ratings_copy (player_t n, const double *r, double *t)
{
	player_t i;
	for (i = 0; i < n; i++) {
		t[i] = r[i];
	}
}

void
ratings_results	( bool_t anchor_err_rel2avg
				, bool_t anchor_use 
				, player_t anchor
				, double general_average				
				, struct PLAYERS *plyrs
				, struct RATINGS *rat
)
{
	double excess;

	player_t j;
	ratings_cleared_for_purged (plyrs, rat);
	for (j = 0; j < plyrs->n; j++) {
		rat->ratingof_results[j] = rat->ratingof[j];
		rat->obtained_results[j] = rat->obtained[j];
		rat->playedby_results[j] = rat->playedby[j];
	}

	// shifting ratings to fix the anchor.
	// Only done if the error is relative to the average.
	// Otherwise, it is taken care in the rating calculation already.
	// If Anchor_err_rel2avg is set, shifting in the calculation (later) is deactivated.
	excess = 0.0;
	if (anchor_err_rel2avg && anchor_use) {
		excess = general_average - rat->ratingof_results[anchor];		
		for (j = 0; j < plyrs->n; j++) {
			rat->ratingof_results[j] += excess;
		}
	}
}

void
ratings_cleared_for_purged (const struct PLAYERS *p, struct RATINGS *r /*@out@*/)
{
	player_t j;
	player_t n = p->n;
	for (j = 0; j < n; j++) {
		if (p->flagged[j]) {
			r->ratingof[j] = 0;
		}
	}	
}

static void
ratings_apply_excess_correction (double excess, player_t n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

void
ratings_center_to_zero (player_t n_players, const bool_t *flagged, double *ratingof)
{
	player_t 	j, notflagged;
	double 	excess, average;
	double 	accum = 0;

	// general average
	for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			notflagged++;
			accum += ratingof[j];
		}
	}
	if (notflagged > 0) {
		average = accum / (double) notflagged;
		excess  = average;
		// Correct the excess
		ratings_apply_excess_correction(excess, n_players, flagged, ratingof);
	}
}
