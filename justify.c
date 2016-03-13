/*
	ordoprep is program for preparing/compacting input for Ordo
    Copyright 2016 Miguel A. Ballicora

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

#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stddef.h>

#include "justify.h"

static bool_t 
iswordbreak (int c)
{
	return c == '\0' || isspace(c);
}

static const char *
goto_nextstop(const char *p, ptrdiff_t rem)
{
	const char *q = p;
	while (q < p + rem && !iswordbreak(*q)) {
		q++;
	}
	return q;
}

static const char *
print_stretch(FILE *f, const char *sta, const char *end)
{
	while (*sta && sta < end) {
		fprintf(f,"%c", *sta++);
	}
	return sta;
}

static bool_t
fit_horizontally(const char *p, ptrdiff_t HOR)
{
	ptrdiff_t i;
	bool_t found = FALSE;
	for (i = 0; !found && i < (HOR+1); i++) {
		found = iswordbreak(p[i]);
	}
	return found;
}

static void
carriage(FILE *f, int m)
{
	while (m-->0) fprintf(f," ");
}

static void
carriage_return(FILE *f, int m)
{	
	fprintf (f,"\n"); 
	carriage(f, m);
}

//----------------------------------------------------

void
fprint_justified (FILE *f, const char *p, int blankheader, int margin, int hor)
{
	const char *q;
	ptrdiff_t rem, HOR;

	assert (margin >= 0);
	assert (hor > 0);

	HOR = (ptrdiff_t)hor;

	rem = HOR;
	carriage(f,blankheader);

	while (*p != '\0') {
		if (rem == HOR && isspace(*p)) {
			p++;
			continue;
		}
		if (rem > 0 && isspace(*p)) {
			fprintf(f,"%s", *p=='\n'?"\n":" ");
			p++;
			rem--;
			continue;
		}
		if (rem == 0) {
			carriage_return(f,margin);
			rem = HOR;
			continue;
		}	

		q = goto_nextstop(p, rem);

		if (iswordbreak(*q)) {
			const char *x;
			x = print_stretch (f, p, q);
			rem -=  q - p;
			p = x;
		} else {
			if (fit_horizontally(p,HOR)) {
				carriage_return(f,margin);
				rem = HOR;
			} else {
				print_stretch (f, p, p + rem);
				p += rem;
				rem = HOR;
				carriage_return(f, margin);
			}
		}

	} //loop

} //function


void
print_justified (const char *p, int blankheader, int margin, int hor)
{
	fprint_justified (stdout, p, blankheader, margin, hor);
}

