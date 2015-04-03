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

#if !defined(H_SIM)
#define H_SIM
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

/*
extern void
get_a_simulated_run	( int 					limit

					, double 				General_average
					, bool_t 				quiet_mode
					, struct PLAYERS 		Players
					, struct RATINGS 		RA
					, struct ENCOUNTERS 	Encounters

					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result
					, double 				beta
					, struct GAMES 			Games	// output

					, struct prior *		PP
					, struct prior *		PP_store

					, struct rel_prior_set	RPset 
					, struct rel_prior_set	RPset_store 
);
*/

void
get_a_simulated_run	( int 					limit
					, bool_t 				quiet_mode

					, double 				General_average
					, double 				beta

					, struct ENCOUNTERS 	*pEncounters
					, struct rel_prior_set	*pRPset 
					, struct PLAYERS 		*pPlayers
					, struct RATINGS 		*pRA
					, struct GAMES 			*pGames	// output
					, struct prior *		PP

					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result

					, struct prior *		PP_store
					, struct rel_prior_set	*pRPset_store 
)
;

extern void
save_simulated(struct PLAYERS *pPlayers, struct GAMES *pGames, int num);

extern void
simul		( long 						simulate
			, bool_t 					sim_updates
			, bool_t 					quiet_mode
			, bool_t					prior_mode
			, bool_t 					adjust_white_advantage
			, bool_t 					adjust_draw_rate
			, bool_t					anchor_use
			, bool_t					anchor_err_rel2avg

			, double					general_average
			, player_t 					anchor
			, player_t					priored_n
			, double					beta

			, struct ENCOUNTERS	*		encount
			, struct rel_prior_set *	rps
			, struct PLAYERS *			plyrs
			, struct RATINGS *			rat
			, struct GAMES *			pGames

			, struct prior *			pPrior
			, struct prior 				wa_prior
			, struct prior 				dr_prior

			, double 					drawrate_evenmatch_result
			, double 					white_advantage_result
			, struct summations *		p_sfe_input

			, struct rel_prior_set 		RPset_store
			, struct prior *			PP_store

)
;
/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
