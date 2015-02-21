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
#include <string.h>

#include "namehash.h"
#include "pgnget.h"


#define PEAXPOD 8
#define PODBITS 12
#define PODMASK ((1<<PODBITS)-1)
#define PODMAX   (1<<PODBITS)
#define PEA_REM_MAX (256*256)

struct NAMEPEA {
	player_t pidx; // player index
	uint32_t hash; // name hash
};

struct NAMEPOD {
	struct NAMEPEA pea[PEAXPOD];
	int n;
};


// ----------------- PRIVATE DATA---------------
static struct NAMEPOD Namehashtab[PODMAX];
static struct NAMEPEA Nameremains[PEA_REM_MAX];
static int Nameremains_n;
//----------------------------------------------




#if 0
static void
hashstat(void)
{
	int i, level;
	int hist[9] = {0,0,0,0,0,0,0,0,0};

	for (i = 0; i < PODMAX; i++) {
		level = Namehashtab[i].n;
		hist[level]++;
	}
	for (i = 0; i < 9; i++) {
		printf ("level[%d]=%d\n",i,hist[i]);
	}
}
#endif

bool_t
name_ispresent (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index)
{
	struct NAMEPOD *ppod = &Namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;
	bool_t 			found= FALSE;
	int i;


	ppea = ppod->pea;
	n = ppod->n;
	for (i = 0; i < n; i++) {
		if (ppea[i].hash == hash) {
			const char *name_str = database_getname(d, ppea[i].pidx);
			assert(name_str);
			if (!strcmp(s, name_str)) {
				found = TRUE;
				*out_index = ppea[i].pidx;
				break;
			}
		}
	}
	if (found) return found;

	ppea = Nameremains;
	n = Nameremains_n;
	for (i = 0; i < n; i++) {
		if (ppea[i].hash == hash && !strcmp(s, database_getname(d, ppea[i].pidx))) {
			found = TRUE;
			*out_index = ppea[i].pidx;
			break;
		}
	}

	return found;
}

bool_t
name_register (uint32_t hash, player_t i)
{
	struct NAMEPOD *ppod = &Namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;

	ppea = ppod->pea;
	n = ppod->n;	

	if (n < PEAXPOD) {
		ppea[n].pidx = i;
		ppea[n].hash = hash;
		ppod->n++;
		return TRUE;
	}
	else if (Nameremains_n < PEA_REM_MAX) {
		Nameremains[Nameremains_n].pidx = i;
		Nameremains[Nameremains_n].hash = hash;
		Nameremains_n++;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*http://www.cse.yorku.ca/~oz/hash.html*/

uint32_t
namehash(const char *str)
{
	uint32_t hash = 5381;
	char chr;
	unsigned int c;
	while ('\0' != *str) {
		chr = *str++;
		c = (unsigned int) ((unsigned char)(chr));
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

/************************************************************************/



