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
#include <string.h>

#include "mymem.h"
#include "mytypes.h"
#include "plyrs.h"


bool_t
players_name2idx (const struct PLAYERS *plyrs, const char *player_name, player_t *pi)
{
	player_t j;
	bool_t found;
	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_name);
		if (found) {
			*pi = j; 
		} 
	}
	return found;
}

//

void
players_purge (bool_t quiet, struct PLAYERS *pl)
{
	player_t n_players = pl->n;
	const int *performance_type = pl->performance_type;
	const char **name = pl->name;
	bool_t *flagged = pl->flagged;

	player_t j;
	assert(pl->perf_set);
	for (j = 0; j < n_players; j++) {
		if (flagged[j]) continue;
		if (performance_type[j] != PERF_NORMAL) {
			flagged[j]= TRUE;
			if (!quiet) printf ("purge --> %s\n", name[j]);
		} 
	}
}

void
players_set_priored_info (const struct prior *pr, const struct rel_prior_set *rps, struct PLAYERS *pl /*@out@*/)
{
	player_t 			i, j;
	player_t 			n_players = pl->n;
	struct relprior *	rp = rps->x;
	player_t 			rn = rps->n;

	// priors
	for (j = 0; j < n_players; j++) {
		pl->priored[j] = pr[j].isset;
	}

	// relative priors
	for (i = 0; i < rn; i++) {
		pl->priored[rp[i].player_a] = TRUE;
		pl->priored[rp[i].player_b] = TRUE;
	}
}


void
players_flags_reset (struct PLAYERS *pl)
{
	player_t 	j;
	player_t 	n;
	bool_t *	flagged;

	assert(pl);
	n = pl->n;
	flagged = pl->flagged;
	assert(flagged);
	for (j = 0; j < n; j++) {
		flagged[j] = FALSE;
	}	
}


player_t	
players_set_super (bool_t quiet, const struct ENCOUNTERS *ee, struct PLAYERS *pl)
{
	// encounters
	gamesnum_t N_enc = ee->n;
	const struct ENC *enc = ee->enc;

	// players
	player_t n_players = pl->n;
	int *perftype  = pl->performance_type;
	bool_t *ispriored = pl->priored; 
	const char **name    = pl->name;

	double 		*obt;
	gamesnum_t	*pla;
	gamesnum_t	e;
	player_t 	j;
	player_t 	w, b;
	player_t 	super = 0;

	obt = memnew (sizeof(double) * (size_t)n_players);
	pla = memnew (sizeof(gamesnum_t) * (size_t)n_players);
	if (NULL==obt || NULL==pla) {
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < n_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
	for (e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;

		assert(( w >= 0 && w < n_players) || !fprintf(stderr,"w=%ld np=%ld\n",(long)w,(long)n_players));
		assert(( b >= 0 && b < n_players) || !fprintf(stderr,"b=%ld np=%ld\n",(long)b,(long)n_players));

		obt[w] += enc[e].wscore;
		obt[b] += (double)enc[e].played - enc[e].wscore;

		pla[w] += enc[e].played;
		pla[b] += enc[e].played;

	}
	for (j = 0; j < n_players; j++) {
		//bool_t gotprior = has_a_prior(PP,j);
		bool_t gotprior = ispriored[j];
		perftype[j] = PERF_NORMAL;
		if (pla[j] == 0) {
			perftype[j] = PERF_NOGAMES;			
			if (!quiet) printf ("detected (player without games) --> %s\n", name[j]);
		} else {
			if (obt[j] < 0.001) {
				perftype[j] = gotprior? PERF_NORMAL: PERF_SUPERLOSER;			
				if (!quiet) printf ("detected (all-losses player) --> %s: seed rating present = %s\n", name[j], gotprior? "Yes":"No");
			}	
			if ((double)pla[j] - obt[j] < 0.001) {
				perftype[j] = gotprior? PERF_NORMAL: PERF_SUPERWINNER;
				if (!quiet) printf ("detected (all-wins player)   --> %s: seed rating present = %s\n", name[j], gotprior? "Yes":"No");
			}
		}
		if (perftype[j] != PERF_NORMAL) super++;
	}
	for (j = 0; j < n_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
	pl->perf_set = TRUE;

	memrel(obt);
	memrel(pla);
	return super;
}


void	
players_copy (const struct PLAYERS *source, struct PLAYERS *target)
{
	const struct PLAYERS *x = source;
	struct PLAYERS *y = target;
	player_t i;
	player_t n = x->n;

	y->n 			= x->n;
	y->size 		= x->size;
	y->anchored_n 	= x->anchored_n;
	y->perf_set 	= x->perf_set;
	for (i = 0; i < n; i++) {
		y->name[i] 				= x->name[i];
		y->flagged[i] 			= x->flagged[i];
		y->prefed[i] 			= x->prefed[i];
		y->priored[i] 			= x->priored[i];
		y->performance_type[i]	= x->performance_type[i]; 
	}
}

//------------------------- debug routines

#if !defined(NDEBUG)
bool_t
players_have_clear_flags (struct PLAYERS *pl)
{
	bool_t		found;
	player_t 	j;
	player_t 	n_players;
	bool_t *	flagged;

	assert(pl);
	n_players = pl->n;
	flagged = pl->flagged;
	assert(flagged);
	found = FALSE;
	for (j = 0; j < n_players; j++) {
		if (flagged[j]) {
			found = TRUE; 
			break;
		}
	}
	return !found;
}
#endif


