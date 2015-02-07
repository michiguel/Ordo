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


void 
cegt_output	( const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, double				confidence_factor
			, const struct GAMESTATS *pgame_stats
			, const struct DEVIATION_ACC *s);

// Function provided to have all head to head information
void 
head2head_output( const struct GAMES 	*g
				, const struct PLAYERS 	*p
				, const struct RATINGS 	*r
				, struct ENCOUNTERS 	*e  // memory just provided for local calculations
				, double 				*sdev
				, long 					simulate
				, double				confidence_factor
				, const struct GAMESTATS *pgame_stats
				, const struct DEVIATION_ACC *s
				, const char *head2head_str);

void
all_report 	( const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, bool_t				hide_old_ver
			, double				confidence_factor
			, FILE 					*csvf
			, FILE 					*textf
			, double 				white_advantage
			, double 				drawrate_evenmatch
			, int					decimals);

void
errorsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out, double confidence_factor);

void
ctsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out);

void
report_loadpars(

 struct PLAYERS *p,
 size_t 		n_relative_anchors,
 struct relprior *raa

);
/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
