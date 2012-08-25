#if !defined(H_PGNGET)
#define H_PGNGET
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"
#include "ordolim.h"
#include "datatype.h"

enum RESULTS {
	WHITE_WIN = 0,
	BLACK_WIN = 2,
	RESULT_DRAW = 1,
	DISCARD = 3
};

extern struct DATA DB;

extern bool_t pgn_getresults (const char *pgn, bool_t quiet);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
