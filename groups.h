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
