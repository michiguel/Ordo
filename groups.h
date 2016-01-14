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

extern void 	scan_encounters	( const struct ENC Encounter[]
								, gamesnum_t N_encounters
								, player_t N_players);

extern player_t	convert_to_groups(FILE *f, player_t N_plyers, const char **name, const struct PLAYERS *players);

extern void		sieve_encounters( const struct ENC *enc
								, gamesnum_t N_enc
								, struct ENC *enca
								, gamesnum_t *N_enca
								, struct ENC *encb
								, gamesnum_t *N_encb);

extern bool_t 	supporting_encmem_init (gamesnum_t nenc);

extern void 	supporting_encmem_done (void);

extern bool_t 	supporting_groupmem_init (player_t nplayers, gamesnum_t nenc);

extern void 	supporting_groupmem_done (void);

extern bool_t	groups_process 	( bool_t quiet
								, const struct ENCOUNTERS *encounters
								, const struct PLAYERS *players
								, FILE *groupf);

extern bool_t	groups_process_to_count ( const struct ENCOUNTERS *encounters
										, const struct PLAYERS *players
										, player_t *n);

extern bool_t	group_is_problematic	( const struct ENCOUNTERS *encounters
										, const struct PLAYERS *players);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
