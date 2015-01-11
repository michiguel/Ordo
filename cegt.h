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


#if !defined(H_CEGT)
#define H_CEGT
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "mytypes.h"

struct CEGT {
	struct ENC 			*enc;
	size_t 				n_enc;
	long 				simulate;

	size_t				n_players;
	int					*sorted; /* sorted index by rating */
	double				*ratingof_results;
	double				*obtained_results;
	int					*playedby_results;
	double				*sdev; 
	bool_t				*flagged;
	const char			**name;

	double				confidence_factor;

	struct GAMESTATS 	*gstat;

	struct DEVIATION_ACC *sim;
 
};

bool_t 
output_cegt_style (const char *general_name, const char *rating_name, const char *programs_name, struct CEGT *p); 

bool_t 
output_report_individual (const char *outindiv_name, struct CEGT *p, int simulate);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
