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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "justify.h"
#include "myhelp.h"


static bool_t
eo_helplist(struct helpline *h)
{
	return NULL == h->longc && '\0' == h->c;
}

struct option *
optionlist_new (struct helpline *hl)
{
	size_t n;
	struct option *k, *k_start;
	struct helpline *i = hl;
	while (!eo_helplist(i)) i++;
	n = (size_t)(i - hl + 1);

	if (NULL == (k_start = malloc (sizeof(struct option) * n)))
		return NULL;

	for (i = hl, k = k_start; !eo_helplist(i); i++, k++) {
		k->name = i->longc;
		k->has_arg = i->has_arg;
		k->flag = i->variable;
		k->val = i->c;
	}
	k->name = NULL;
	k->has_arg = 0;
	k->flag = NULL;
	k->val = 0;

	return k_start;
}

void
optionlist_done (struct option *o)
{
	assert(o);
	free(o);
}

char *
optionshort_build (struct helpline *hl, char *opt_input)
{
	char *opt = opt_input;
	struct helpline *i = hl;

	for (i = hl, opt = opt_input; !eo_helplist(i); i++) {
		int c = i->c;
		if (c != '\0') {
			*opt++ = (char)c;
			if (required_argument == i->has_arg) *opt++ = ':';
		}
	}
	*opt++ = '\0';

	return opt_input;
}


static void 
build_head (char *head, struct helpline *h)
{
	sprintf (head, " %c%c%s %s%s%s%s%s%s", 
				h->c!='\0'?'-' :' ',
				h->c!='\0'?h->c:' ', 
				h->longc!=NULL && h->c!='\0'?",":"",////
				h->longc!=NULL?(h->c!='\0'?"--":" --"):"", 
				h->longc!=NULL?h->longc:"", 
				h->has_arg==optional_argument && h->longc!=NULL?"[":"",
				h->has_arg!=no_argument && h->longc!=NULL? "=" :"",
				h->has_arg!=no_argument? h->argstr :"",
				h->has_arg==optional_argument && h->longc!=NULL?"]":""
	);
}

void
printlonghelp (FILE *outf, struct helpline *h_inp)
{
	struct helpline *h;
	char head[80];
	size_t longest = 0;
	int	 left_tab;
	int	 right_tab = 80;

	for (h = h_inp; !eo_helplist(h); h++) {
		build_head (head, h);
		if (longest < strlen(head)) longest = strlen(head);
	}

	left_tab = (int)longest + 1;

	fprintf (outf, "\n");
	for (h = h_inp; !eo_helplist(h); h++) {
		build_head (head, h);
		fprintf (outf, "%-*s ", left_tab-1, head);
		fprint_justified (outf, h->helpstr, 0, left_tab, right_tab - left_tab);
		fprintf (outf, "\n");
	}
	fprintf (outf, "\n");
}

