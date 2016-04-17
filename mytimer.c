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

#include <time.h>
#include <stdio.h>
#include "mytimer.h"
#include "boolean.h"

bool_t TIMELOG = TRUE;

static clock_t Standard_clock = 0;

void timer_reset(void)
{
	Standard_clock = clock();
}

double timer_get(void)
{
	return ( (double)clock()-(double)Standard_clock)/(double)CLOCKS_PER_SEC;
}

void timelog (const char *s)
{
	if (TIMELOG) {printf ("%8.2lf | %s\n", timer_get(), s);}
}

void timelog_ld (const char *s, long ld)
{
	if (TIMELOG) {
		printf ("%8.2lf | %s%ld\n", timer_get(), s, ld);
	}
}



