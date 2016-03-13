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

/*
* PROCEDURE ADOPTED AND MODIFIED FROM "THE BOOK ON C", Banahan.
*
* options() parses option letters and option arguments from the argv list.
* Succesive calls return succesive option letters which match one of
* those in the legal list. Option letters may require option arguments
* as indicated by a ':' following the letter in the legal list.
* for example, a legal list of "ab:c" implies that a, b and c are
* all valid options and that b takes an option argument. The option
* argument is passed back to the calling function in the value
* of the global OptArg pointer. The OptIndex gives the next string
* in the argv[] array that has not already been processed by options().
*
* options() returns -1 if there are no more option letters or if
* double SwitchChar is found. Double SwitchChar forces options()
* to finish processing options.
*
* options() returns '?' if an option not in the legal set is
* encountered or an option needing an argument is found without an
* argument following it.
*
*/

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "myopt.h"

static const char SWITCH_CHAR = '-';
static const char UNKNOWN_CHAR = '?';

int opt_index = 1;       	/* first option should be argv[1] 	*/
char *opt_arg = NULL;    	/* global option argument pointer 	*/
static char *posn = NULL;  	/* position in argv[opt_index] 		*/


int options(int argc, char *argv[], const char *legal)
{
	char *legal_index = NULL;
	int letter = 0;

	if (NULL == posn || '\0' == *posn) {

		/* no more args, no SWITCH_CHAR or no option letter ? */
		if (opt_index >= argc || SWITCH_CHAR != *(posn = argv[opt_index]))
			return END_OF_OPTIONS;

		assert (NULL != posn);
		assert (SWITCH_CHAR == *posn);

		if ('\0' == *(++posn))
			return UNKNOWN_CHAR;

		/* find double SWITCH_CHAR ? */
		if (SWITCH_CHAR == *posn) {
			opt_index++;
			return END_OF_OPTIONS;
		}
	}

	letter = *posn++;

	if (NULL == (legal_index = strchr(legal, letter))) {
		if ('\0' == *posn)
			opt_index++;
		return UNKNOWN_CHAR;
	}

	++legal_index;

	if (':' == *legal_index) {		/* option argument */

		if ('\0' == *posn) {
			/* space between opt and opt arg */
			++opt_index;
			if (opt_index < argc) {
				opt_arg = argv[opt_index];
			} else {
				posn = NULL;
				return UNKNOWN_CHAR;
			}

		} else {
			/* no space between opt and opt arg */
			opt_arg = posn;
		}

		posn = NULL;
		opt_index++;

	} else {						/* no option argument */
		opt_arg = NULL;
		if ('\0' == *posn)
			opt_index++;

	}
	return letter;
}


#include <string.h>

static int terminates(int c) {return '\0' == c || '=' == c;}

static int 
findidx (const struct option *longopts, char *a, int *longindex)
{
	int i, found = 0;
	if (NULL == a || NULL == longopts) return 0;

	for (i = 0; longopts[i].name != NULL; i++) {
		found = a == strstr(a, longopts[i].name) && terminates(a[strlen(longopts[i].name)]);
		if (found) {
			*longindex = i; 
			break;
		}
	}
	return found;
}

static int
optl(	int argc, 
		char * const argv[],
		const struct option *longopts, 
		int *longindex
	)
{
	int letter;
	int i;

	if (!findidx (longopts, posn, &i)) {
		return UNKNOWN_CHAR;
	}

 	if (longopts[i].flag == NULL) {
		letter = longopts[i].val;
	} else {
		*longopts[i].flag = longopts[i].val;
		letter = 0;
	}

	*longindex = i;
	posn += strlen(longopts[i].name); 
	if ('=' == *posn) posn++;	

	switch (longopts[i].has_arg) {
		case no_argument:
				assert('\0' == *posn);
				opt_arg = NULL;
				posn = NULL;
				opt_index++;
			break;
 		case required_argument:	
				/* no space between opt and opt arg */
				if ('\0' != *posn) {
					opt_arg = posn;
					posn = NULL;
					opt_index++;
					return letter;
				}
				/* space between opt and opt arg */
				++opt_index;
				/* out of args*/
				if (opt_index >= argc) {
					posn = NULL;
					return UNKNOWN_CHAR;
				}
				opt_arg = argv[opt_index];
				posn = NULL;
				opt_index++;
			break;
		case optional_argument:	
				/* no space between opt and opt arg */
				if ('\0' != *posn) {
					opt_arg = posn;
					posn = NULL;
					opt_index++;
					return letter;
				}
				/* space between opt and opt arg */
				++opt_index;
				/* out of args? */
				if (opt_index >= argc) {

					opt_arg = NULL;
					posn = NULL;
					return letter;

				}

				opt_arg = argv[opt_index];

				/* no argument? */
				if (opt_arg[0] == SWITCH_CHAR) {
					opt_arg = NULL;
					posn = NULL;
					return letter;
				}

				posn = NULL;
				opt_index++;	
			break;
		default:
				assert(0);
				letter = UNKNOWN_CHAR;	
			break;
	}	
	return letter;
}


int options_l	(
				int argc, 
				char * const argv[],
				const char *legal,
				const struct option *longopts, 
				int *longindex
				)
{
	char *legal_index = NULL;
	int letter = 0;

	if (NULL == posn || '\0' == *posn) {

		/* no more args, no SWITCH_CHAR or no option letter ? */
		if (opt_index >= argc || SWITCH_CHAR != *(posn = argv[opt_index]))
			return END_OF_OPTIONS;

		assert (NULL != posn);
		assert (SWITCH_CHAR == *posn);

		if ('\0' == *(++posn))
			return UNKNOWN_CHAR;

		/* find double SWITCH_CHAR ? */
		if (SWITCH_CHAR == *posn) {

			if ('\0' == *(++posn)) {
				opt_index++;
				return END_OF_OPTIONS;
			} else {
				//long options	
				letter = optl (argc, argv, longopts, longindex);
				return letter;
			}
		}
	}

	letter = *posn++;

	/* search letter, return if unknown */
	if (NULL == (legal_index = strchr(legal, letter))) {
		if ('\0' == *posn)
			opt_index++;
		return UNKNOWN_CHAR;
	}

	++legal_index;

	if (':' == *legal_index) {		/* option argument */

		if ('\0' == *posn) {
			/* space between opt and opt arg */
			++opt_index;
			if (opt_index < argc) {
				opt_arg = argv[opt_index];
			} else {
				posn = NULL;
				return UNKNOWN_CHAR;
			}

		} else {
			/* no space between opt and opt arg */
			opt_arg = posn;
		}

		posn = NULL;
		opt_index++;

	} else {						/* no option argument */
		opt_arg = NULL;
		if ('\0' == *posn)
			opt_index++;

	}
	return letter;
}




