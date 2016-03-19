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


#if !defined(H_ENC)
#define H_ENC
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

enum SELECTIVITY {
	ENCOUNTERS_FULL = 0,
	ENCOUNTERS_NOFLAGGED = 1
};

extern bool_t 	encounters_init (gamesnum_t n, struct ENCOUNTERS *e);
extern void 	encounters_done (struct ENCOUNTERS *e);
extern void		encounters_copy (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt);
extern bool_t	encounters_replicate (const struct ENCOUNTERS *src, struct ENCOUNTERS *tgt);

extern void
encounters_calculate
				( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENCOUNTERS	*e
);

// no globals
extern void
calc_obtained_playedby 	( const struct ENC *enc
						, gamesnum_t N_enc
						, player_t n_players
						, double *obtained
						, gamesnum_t *playedby);

// no globals
extern void
calc_expected 	( const struct ENC *enc
				, gamesnum_t N_enc
				, double white_advantage
				, player_t n_players
				, const double *ratingof
				, double *expected
				, double beta);

// no globals
extern void
calc_output_info
			 	( const struct ENC *enc
				, gamesnum_t N_enc
				, const double *ratingof
				, player_t n_players
				, double *sdev
				, struct OUT_INFO *oi
				);


#if !defined(NDEBUG)
extern bool_t
ratings_sanity (player_t n_players, const double *ratingof);

extern bool_t
playedby_sanity (player_t n_players, const gamesnum_t *pb, const bool_t *flagged);
#endif

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
