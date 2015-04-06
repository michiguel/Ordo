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


#if !defined(H_PLYRS)
#define H_PLYRS
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"

extern bool_t 	players_name2idx (const struct PLAYERS *plyrs, const char *player_name, player_t *pi);
extern void		players_purge (bool_t quiet, struct PLAYERS *pl);
extern void		players_set_priored_info (const struct prior *pr, const struct rel_prior_set *rps, struct PLAYERS *pl /*@out@*/);
extern void		players_flags_reset (struct PLAYERS *pl);
extern player_t	players_set_super (bool_t quiet, const struct ENCOUNTERS *ee, struct PLAYERS *pl);
extern void 	players_copy (const struct PLAYERS *source, struct PLAYERS *target);

#if !defined(NDEBUG)
extern bool_t	players_have_clear_flags (struct PLAYERS *pl);
#endif

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
