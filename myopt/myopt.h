/*
	Ordoprep 
	is a program for calculating ratings of engine or chess players
    Copyright 2013 Miguel A. Ballicora

    This file is part of Ordoprep.

    Ordoprep is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Ordoprep is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Ordoprep.  If not, see <http://www.gnu.org/licenses/>.
*/

#if !defined(H_MYOPT)
#define H_MYOPT
/*>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>*/

enum char_options {END_OF_OPTIONS = -1};

extern int 		opt_index;      
extern char *	opt_arg;    

extern int 		options(int argc, char *argv[], const char *legal);

// Long options

struct option {
    const char *name;
    int         has_arg;
    int        *flag;
    int         val;
};

#define no_argument			0
#define required_argument	1
#define optional_argument	2

extern
int options_l	(
				int argc, 
				char * const argv[],
				const char *legal,
				const struct option *longopts, 
				int *longindex
				);

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<*/
#endif

