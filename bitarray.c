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

#include <assert.h>

#include "mytypes.h"
#include "mymem.h"
#include "bitarray.h"

//------------- Bit array begin --------------------------------------- 

void
ba_put(struct BITARRAY *ba, player_t x)
{
	assert(ba);
	assert(ba->pod && ba->max);
	if (x < ba->max) {
		ba->pod[x/64] |= (uint64_t)1 << (x & 63);
	}
}

bool_t
ba_ison(struct BITARRAY *ba, player_t x)
{
	uint64_t y;
	bool_t ret;
	assert(ba);
	assert(ba->pod && ba->max);
	assert(x < ba->max);
	y = (uint64_t)1 & (ba->pod[x/64] >> (x & 63));	
	ret = y == 1;
	return ret;
}

void
ba_clear (struct BITARRAY *ba)
{
	size_t i;
	player_t max; 
	size_t max_p;

	max = ba->max; 
	max_p = (size_t)max/64 + (max % 64 > 0?1:0);
	for (i = 0; i < max_p; i++) ba->pod[i] = 0;
}

void
ba_setnot (struct BITARRAY *ba)
{
	size_t i;
	player_t max; 
	size_t max_p;

	max = ba->max; 
	max_p = (size_t)max/64 + (max % 64 > 0?1:0);
	for (i = 0; i < max_p; i++) ba->pod[i] = ~ba->pod[i];
}

bool_t
ba_init(struct BITARRAY *ba, player_t max)
{
	bool_t ok;
	uint64_t *ptr;
	size_t i;
	size_t max_p = (size_t)max/64 + (max % 64 > 0?1:0);

	assert(ba);
	ba->max = 0;
	ba->pod = NULL;
	ok = NULL != (ptr = memnew (sizeof(pod_t)*max_p));
	if (ok) {
		ba->max = max;
		ba->pod = ptr;
		for (i = 0; i < max_p; i++) ba->pod[i] = 0;
	} 
	return ok;
}

void
ba_done(struct BITARRAY *ba)
{
	assert(ba);
	assert(ba->pod);
	if (ba->pod) memrel (ba->pod);
	ba->pod = NULL;
	ba->max = 0;
}

//---------------------------------------------------------------------
