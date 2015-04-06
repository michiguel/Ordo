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
extern void 	relpriors_init 		( bool_t quietmode
									, const struct PLAYERS *plyrs
									, const char *f_name
									, struct rel_prior_set *rps /*@out@*/
									, struct rel_prior_set *rps_backup /*@out@*/);

extern void		relpriors_done2		( struct rel_prior_set *rps /*@out@*/
									, struct rel_prior_set *rps_backup /*@out@*/);

extern void		relpriors_done1		( struct rel_prior_set *rps /*@out@*/);

extern bool_t 	relpriors_replicate	( struct rel_prior_set *rps, struct rel_prior_set *rps_dup);

//----------------------------------

extern player_t	Priored_n;

//----------------------------------

extern void 	priors_reset	( struct prior *p, player_t n);
extern void 	priors_load 	( bool_t quietmode
								, const char *fpriors_name
								, struct RATINGS *rat /*@out@*/
								, struct PLAYERS *plyrs /*@out@*/
								, struct prior *pr /*@out@*/);

extern void 	priors_copy		(const struct prior *p, player_t n, struct prior *q);
extern void 	priors_shuffle	(struct prior *p, player_t n);
extern void 	priors_show 	(const struct PLAYERS *plyrs, struct prior *p, player_t n);

extern bool_t 	has_a_prior		(struct prior *pr, player_t j);

extern void 	anchor_j 		( player_t j, double x
								, struct RATINGS *rat /*@out@*/
								, struct PLAYERS *plyrs /*@out@*/);

extern void 	init_manchors 	( bool_t quietmode
								, const char *fpins_name
								, struct RATINGS *rat /*@out@*/
								, struct PLAYERS *plyrs /*@out@*/);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
