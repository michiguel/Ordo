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

#include "sysport.h"

extern mythread_mutex_t Smpcount;
extern mythread_mutex_t Groupmtx;
extern mythread_mutex_t Summamtx;
extern mythread_mutex_t Printmtx;

void
get_a_simulated_run	( int 					limit
					, bool_t 				quiet_mode
					, double 				beta
					, double 				drawrate_evenmatch_result
					, double 				white_advantage_result

					, const struct RATINGS 			*pRA
					, const struct prior 			*PP_ori			
					, const struct rel_prior_set	*pRPset_ori 	

					, struct ENCOUNTERS 	*pEncounters 	// output
					, struct PLAYERS 		*pPlayers 		// output
					, struct GAMES 			*pGames			// output
					, struct prior 			*PP				// output
					, struct rel_prior_set	*pRPset 		// output
)
;

extern void
save_simulated(struct PLAYERS *pPlayers, struct GAMES *pGames, int num);

extern void
simul
	( long 							simulate
	, bool_t 						sim_updates
	, bool_t 						quiet_mode
	, bool_t						prior_mode
	, bool_t 						adjust_white_advantage
	, bool_t 						adjust_draw_rate
	, bool_t						anchor_use
	, bool_t						anchor_err_rel2avg

	, double						general_average
	, player_t 						anchor
	, player_t						priored_n
	, double						beta

	, double 						drawrate_evenmatch_result
	, double 						white_advantage_result
	, const struct rel_prior_set *	rps
	, const struct prior *			pPrior
	, struct prior 					wa_prior
	, struct prior 					dr_prior

	, struct ENCOUNTERS	*			encount				// io, modified
	, struct PLAYERS *				plyrs				// io, modified
	, struct RATINGS *				rat					// io, modified
	, struct GAMES *				pGames				// io, modified

	, struct rel_prior_set 			RPset_work			// mem provided
	, struct prior *				PP_work				// mem provided

	, struct summations *			p_sfe_io 			// output
)
;

void
simul_smp
	( int							cpus
	, long 							simulate
	, bool_t 						sim_updates
	, bool_t 						quiet_mode
	, bool_t						prior_mode
	, bool_t 						adjust_white_advantage
	, bool_t 						adjust_draw_rate
	, bool_t						anchor_use
	, bool_t						anchor_err_rel2avg

	, double						general_average
	, player_t 						anchor
	, player_t						priored_n
	, double						beta

	, double 						drawrate_evenmatch_result
	, double 						white_advantage_result
	, const struct rel_prior_set *	rps
	, const struct prior *			pPrior
	, struct prior 					wa_prior
	, struct prior 					dr_prior

	, struct ENCOUNTERS	*			encount				// io, modified
	, struct PLAYERS *				plyrs				// io, modified
	, struct RATINGS *				rat					// io, modified
	, struct GAMES *				pGames				// io, modified

	, struct rel_prior_set 			RPset_work			// mem provided
	, struct prior *				PP_work				// mem provided

	, struct summations *			p_sfe_io 			// output
)
;
/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
