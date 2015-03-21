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


#if !defined(H_CSV)
#define H_CSV
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include "boolean.h"

//FIXME make dynamic
#define MAXSIZE_CSVLINE 4096

struct csv_line {
	int 	n;
	char 	*mem; // points to block of memory
	char 	*s[MAXSIZE_CSVLINE]; //FIXME make it dynamic
};

typedef struct csv_line csv_line_t;

extern bool_t 	csv_line_init(csv_line_t *c, char *p);
extern void 	csv_line_done(csv_line_t *c);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
