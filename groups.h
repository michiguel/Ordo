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


#if !defined(H_GROUPS)
#define H_GROUPS
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stdio.h>

#include "boolean.h"
#include "ordolim.h"
#include "mytypes.h"

typedef struct GROUP 		group_t;
typedef struct CONNECT 		connection_t;
typedef struct NODE 		node_t;
typedef struct PARTICIPANT 	participant_t;

struct GROUP {
	group_t *		next;
	group_t *		prev;
	group_t *		combined;
	participant_t *	pstart;
	participant_t *	plast;
	connection_t *	cstart; // beat to
	connection_t *	clast;
	connection_t *	lstart; // lost to
	connection_t *	llast;
	player_t		id;
	bool_t			isolated;
};

struct CONNECT {
	connection_t *	next;
	node_t	*		node;
};

struct NODE {
	group_t *		group;
};

struct PARTICIPANT {
	participant_t *	next;
	const char	*	name;
	player_t		id;
};

struct GROUP_BUFFER {
	group_t	*		list; //buffer
	group_t	*		tail;
	group_t	*		prehead;
	player_t		n;
	player_t		max;
};

struct PARTICIPANT_BUFFER {
	participant_t *	list; //buffer
	player_t		n;
	player_t		max;
};

struct CONNECT_BUFFER {
	connection_t *	list; //buffer
	gamesnum_t		n;
	gamesnum_t		max;
};

struct GROUPCELL {
	group_t * 	group;
	player_t 	count;
};

typedef struct GROUPCELL groupcell_t;

struct GROUPVAR {
	player_t		nplayers;
	player_t	*	groupbelong;
	player_t *		getnewid;
	groupcell_t	*	groupfinallist;
	player_t		groupfinallist_n;
	node_t	*		node;
	player_t *		gchain;

	group_t **		tofindgroup;

	struct GROUP_BUFFER 		groupbuffer;
	struct PARTICIPANT_BUFFER	participantbuffer;
	struct CONNECT_BUFFER		connectionbuffer;

};

typedef struct GROUPVAR group_var_t;



extern player_t			groupvar_build(group_var_t *gv, player_t N_plyers
											, const char **name, const struct PLAYERS *players, const struct ENCOUNTERS *encounters);
extern bool_t 			groupvar_init (group_var_t *gv, player_t nplayers, gamesnum_t nenc);
extern void 			groupvar_done (group_var_t *gv);

extern group_var_t *	GV_make	(const struct ENCOUNTERS *encounters, const struct PLAYERS *players);
extern group_var_t *	GV_kill (group_var_t *gv);
extern void				GV_out (group_var_t *gv, FILE *f);
extern void				GV_sieve (group_var_t *gv, const struct ENCOUNTERS *encounters, gamesnum_t * pN_intra, gamesnum_t * pN_inter);
extern player_t			GV_counter (group_var_t *gv);
extern void				GV_groupid (group_var_t *gv, player_t *groupid_out);

extern player_t 		GV_non_empty_groups_pop (group_var_t *gv, const struct PLAYERS *players);

extern bool_t			well_connected (const struct ENCOUNTERS *pEncounters, const struct PLAYERS *pPlayers);

extern void 			timer_reset(void);
extern double 			timer_get(void);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
