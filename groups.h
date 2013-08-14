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
	group_t 		*next;
	group_t 		*prev;
	group_t 		*combined;
	participant_t 	*pstart;
	participant_t 	*plast;
	connection_t	*cstart; // beat to
	connection_t	*clast;
	connection_t	*lstart; // lost to
	connection_t	*llast;
	int				id;
	bool_t			isolated;
};

struct CONNECT {
	connection_t 	*next;
	node_t			*node;
};

struct NODE {
	group_t 		*group;
};

struct PARTICIPANT {
	participant_t 	*next;
	char 			*name;
	int				id;
};

struct GROUP_BUFFER {
	group_t		list[MAXPLAYERS];
	group_t		*tail;
	group_t		*prehead;
	int			n;
} group_buffer;

struct PARTICIPANT_BUFFER {
	participant_t		list[MAXPLAYERS];
	int					n;
} participant_buffer;

struct CONNECT_BUFFER {
	connection_t		list[MAXPLAYERS];
	int					n;
} connection_buffer;

//

extern void scan_encounters(const struct ENC Encounter[], int N_encounters, int N_players);
extern void	convert_to_groups(FILE *f, int N_plyers, char **name);
extern void	sieve_encounters(const struct ENC *enc, int N_enc, struct ENC *enca, int *N_enca, struct ENC *encb, int *N_encb);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
