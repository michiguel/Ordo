/*
	ordoprep is program for preparing/compacting input for Ordo
    Copyright 2015 Miguel A. Ballicora

    This file is part of ordoprep.

    ordoprep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    ordoprep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ordoprep.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(H_HELP)
#define H_HELP
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

#include <stdio.h>
#include "myopt/myopt.h"

struct helpline {
	int 			c;
	const char * 	longc;
	int 			has_arg;
	const char *	argstr;
	int	*			variable;
	const char *	helpstr;
};

extern void 			printlonghelp (FILE *outf, struct helpline *h_inp);

extern struct option * 	optionlist_new  (struct helpline *hl);

extern void				optionlist_done (struct option *o);

extern char * 			optionshort_build (struct helpline *hl, char *opt_input);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif
