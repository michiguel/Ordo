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


#if !defined(H_BITARRAY)
#define H_BITARRAY
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "mytypes.h"

typedef uint64_t pod_t;

struct BITARRAY {
	pod_t *		pod;
	player_t 	max;
};

typedef struct BITARRAY bitarray_t;

extern void 	ba_put(struct BITARRAY *ba, player_t x);
extern bool_t 	ba_ison(struct BITARRAY *ba, player_t x);
extern void 	ba_clear (struct BITARRAY *ba);
extern void		ba_setnot (struct BITARRAY *ba);
extern bool_t 	ba_init(struct BITARRAY *ba, player_t max);
extern void 	ba_done(struct BITARRAY *ba);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
