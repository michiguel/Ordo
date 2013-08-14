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
#include "myopt.h"


static const char SWITCH_CHAR = '-';
static const char UNKNOWN_CHAR = '?';

int opt_index = 1;       /* first option should be argv[1] */
const char *opt_arg = NULL;    /* global option argument pointer */

int options(int argc, char *argv[], const char *legal)
{
	static const char *posn = "";  /* position in argv[opt_index] */
	char *legal_index = NULL;
	int letter = 0;

	if ('\0' == *posn) {

		/* no more args, no SWITCH_CHAR or no option letter ? */
		if (opt_index >= argc || SWITCH_CHAR != *(posn = argv[opt_index]))
			return END_OF_OPTIONS;

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
				posn = "";
				return UNKNOWN_CHAR;
			}

		} else {
			/* no space between opt and opt arg */
			opt_arg = posn;
		}

		posn = "";
		opt_index++;

	} else {						/* no option argument */
		opt_arg = NULL;
		if ('\0' == *posn)
			opt_index++;

	}
	return letter;
}

