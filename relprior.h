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


#if !defined(H_RELP)
#define H_RELP
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"

extern void		relpriors_shuffle	(struct rel_prior_set *rps /*@out@*/);
extern void		relpriors_copy		(const struct rel_prior_set *r, struct rel_prior_set *s /*@out@*/);
extern void 	relpriors_show		(const struct PLAYERS *plyrs, const struct rel_prior_set *rps);
extern void 	relpriors_load		(bool_t quietmode, const struct PLAYERS *plyrs, const char *f_name, struct rel_prior_set *rps);

//extern bool_t 	set_relprior 		(const struct PLAYERS *plyrs, const char *player_a, const char *player_b
//									, double x, double sigma, struct rel_prior_set *rps /*@out@*/);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
