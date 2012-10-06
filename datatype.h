#if !defined(H_DATATY)
#define H_DATATY
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stddef.h>
#include "ordolim.h"

typedef unsigned int  uint32_t;
typedef unsigned long long uint64_t;

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

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
