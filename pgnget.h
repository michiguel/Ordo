#if !defined(H_PGNGET)
#define H_PGNGET
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stddef.h>
#include "boolean.h"
#include "ordolim.h"

struct DATA {	
	int 		n_players;
	int 		n_games;
	char		labels[LABELBUFFERSIZE];
	ptrdiff_t	labels_end_idx;
	ptrdiff_t	name	[MAXPLAYERS];
	int 		white	[MAXGAMES];
	int 		black	[MAXGAMES];
	int 		score	[MAXGAMES];
};
extern struct DATA DB;

extern bool_t pgn_getresults (const char *pgn, bool_t quiet);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
