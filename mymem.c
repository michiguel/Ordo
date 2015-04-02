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


#include <stdio.h>

#include <assert.h>
#include <stdlib.h>
#include "mymem.h"

#ifdef NDEBUG

	// release code here

#else

#include <string.h>

#define GARBAGE 0xA3

void * 
_Memnew(size_t x)
{
	void *p;
	assert(x > 0);
	p = malloc(x);
//printf("new --> %lu size=%lu\n", (long unsigned)p, x);
	if (p) memset(p, GARBAGE, x);
	return p;
}

void 
_Memrel(void *p)
{
	assert(p != NULL);
//printf("free--> %lu\n", (long unsigned)p);
	free(p);
//printf("freed\n");
}

#endif
