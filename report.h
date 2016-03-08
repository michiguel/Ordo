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


#if !defined(H_REPORT)
#define H_REPORT
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stdio.h>
#include <stdlib.h>
#include "mytypes.h"

#define MAX_prnt 15

void 
cegt_output	( bool_t 				quiet
			, const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, double				confidence_factor
			, const struct GAMESTATS *pgame_stats
			, const struct DEVIATION_ACC *s
			, struct output_qualifiers outqual
			, int decimals);

// Function provided to have all head to head information
void 
head2head_output( const struct GAMES 	*		g
				, const struct PLAYERS 	*		p
				, const struct RATINGS 	*		r
				, struct ENCOUNTERS 	*		e  // memory just provided for local calculations
				, double 				*		sdev
				, long 							simulate
				, double						confidence_factor
				, const struct GAMESTATS *		pgame_stats
				, const struct DEVIATION_ACC *	s
				, const char *					head2head_str
				, int 							decimals);


void
all_report 	( const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, const struct rel_prior_set *rps
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, bool_t				hide_old_ver
			, double				confidence_factor
			, FILE 					*csvf
			, FILE 					*textf
			, double 				white_advantage
			, double 				drawrate_evenmatch
			, int					decimals
			, int					decimals_score
			, struct output_qualifiers	outqual
			, double				wa_sdev				
			, double				dr_sdev
			, const struct DEVIATION_ACC *	s
			, bool_t 				csf_column
			, int					*inp_list
			);

void
errorsout	( const struct PLAYERS *p
			, const struct RATINGS *r
			, const struct DEVIATION_ACC *s
			, const char *out
			, double confidence_factor);

void
ctsout		( const struct PLAYERS *p
			, const struct RATINGS *r
			, const struct DEVIATION_ACC *s
			, const char *out);

void
look_at_predictions 
			( gamesnum_t n_enc
			, const struct ENC *enc
			, const double *ratingof
			, double beta
			, double wadv
			, double dr0);

void 
look_at_individual_deviation 
			( player_t 			n_players
			, const bool_t *	flagged
			, struct RATINGS *	rat
			, struct ENC *		enc
			, gamesnum_t		n_enc
			, double			white_adv
			, double			beta);

bool_t
str2list 
			( const char *inp_str
			, int max
			, int *n
			, int *t);

bool_t
report_columns_set (int i, int shft, const char *hdr);

bool_t
report_columns_set_str (const char *i_str, const char *s_str, const char *hdr);

void
report_columns_unset (int i);

bool_t
report_columns_init (void);

void
report_columns_done (void);

void
report_columns_load_settings (bool_t quietmode, const char *finp_name);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
