#include <time.h>
#include "mytimer.h"

static clock_t Standard_clock = 0;

void timer_reset(void)
{
	Standard_clock = clock();
}

double timer_get(void)
{
	return ( (double)clock()-(double)Standard_clock)/(double)CLOCKS_PER_SEC;
}
