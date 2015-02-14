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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "relprior.h"
#include "ordolim.h"
#include "csv.h"
#include "randfast.h"

static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}
static bool_t getnum(char *p, double *px) 
{ 	float x;
	bool_t ok = 1 == sscanf( p, "%f", &x );
	*px = (double) x;
	return ok;
}

//

void
relpriors_shuffle(struct rel_prior_set *rps /*@out@*/)
{
	size_t i;
	double value, sigma;
	struct relprior *rp;
	size_t n;

	n  = rps->n;
	rp = rps->x;

	for (i = 0; i < n; i++) {
		value = rp[i].delta;
		sigma =	rp[i].sigma;	
		rp[i].delta = rand_gauss (value, sigma);
	}
}

void
relpriors_copy(const struct rel_prior_set *r, struct rel_prior_set *s /*@out@*/)
{	
	size_t i;
	size_t n = r->n;
	struct relprior *rx = r->x;
	struct relprior *sx = s->x;
	for (i = 0; i < n; i++) {
		sx[i] = rx[i];
	}
	s->n = r->n;
}


void
relpriors_show (const struct PLAYERS *plyrs, const struct rel_prior_set *rps)
{
	size_t rn = rps->n;
	const struct relprior *rx = rps->x;
	size_t i;
	if (rn > 0) {
		printf ("Relative Anchors {\n");
		for (i = 0; i < rn; i++) {
			int a = rx[i].player_a;
			int b = rx[i].player_b;
			printf ("[%s] [%s] = %lf +/- %lf\n", plyrs->name[a], plyrs->name[b], rx[i].delta, rx[i].sigma);
		}
		printf ("}\n");
	} else {
		printf ("Relative Anchors = none\n");
	}
}

static bool_t
set_relprior (const struct PLAYERS *plyrs, const char *player_a, const char *player_b, double x, double sigma, struct rel_prior_set *rps)
{
	size_t j;
	int32_t p_a = -1; 
	int32_t p_b = -1;
	size_t n;
	bool_t found;

	assert(sigma > PRIOR_SMALLEST_SIGMA);

	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_a);
		if (found) {
			p_a = (int32_t) j; //FIXME size_t
		} 
	}

	if (!found) return found;

	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_b);
		if (found) {
			p_b = (int32_t) j; //FIXME size_t
		} 
	}

	if (!found) return found;

	assert (rps != NULL);
	assert (rps->x != NULL);

	n = rps->n++;

	//FIXME
	if (n >= MAX_RELPRIORS) {fprintf (stderr, "Maximum memory for relative anchors exceeded\n"); exit(EXIT_FAILURE);}

	if (p_a == -1 || p_b == -1) return FALSE; // defensive programming, not needed.

	rps->x[n].player_a = p_a;
	rps->x[n].player_b = p_b;
	rps->x[n].delta    = x;
	rps->x[n].sigma    = sigma;

	return found;
}

static bool_t
assign_relative_prior (const struct PLAYERS *plyrs, char *s, char *z, double x, double y, bool_t quiet, struct rel_prior_set *rps /*@out@*/)
{
	bool_t prior_success = TRUE;
	bool_t suc;

	if (y < 0) {
		fprintf (stderr, "Relative Prior, %s %s --> FAILED, error/uncertainty cannot be negative", s, z);	
		suc = FALSE;
	} else {
		if (y < PRIOR_SMALLEST_SIGMA) {
			fprintf (stderr,"sigma too small\n");
			exit(EXIT_FAILURE);
		} else {
			suc = set_relprior (plyrs, s, z, x, y, rps);
			if (suc) {
				if (!quiet)
				printf ("Relative Prior, %s, %s --> %.1lf, %.1lf\n", s, z, x, y);
			} else {
				fprintf (stderr, "Relative Prior, %s, %s --> FAILED, name/s not found in input file\n", s, z);					
			}	
		}
	}
	if (!suc) prior_success = FALSE;
	return prior_success;
}

void
relpriors_load (bool_t quietmode, const struct PLAYERS *plyrs, const char *f_name, struct rel_prior_set *rps /*@out@*/)
{
	#define MAX_MYLINE 1024

	FILE *fil;
	char myline[MAX_MYLINE];
	char name_a[MAX_MYLINE];
	char name_b[MAX_MYLINE];
	char *s, *z, *p;
	bool_t success;
	double x, y;
	bool_t prior_success = TRUE;
	bool_t file_success = TRUE;

	assert(NULL != f_name);

	if (NULL == f_name) {
		fprintf (stderr, "Error, file not provided, absent, or corrupted\n");
		exit(EXIT_FAILURE);
	}

	if (NULL != (fil = fopen (f_name, "r"))) {

		csv_line_t csvln;

		while (file_success && NULL != fgets(myline, MAX_MYLINE, fil)) {
			success = FALSE;
			p = myline;
			s = name_a;
			z = name_b;

			p = skipblanks(p);
			x = 0;
			y = 0;
			if (*p == '\0') continue;

			if (csv_line_init(&csvln, myline)) {
				success = csvln.n == 4 && getnum(csvln.s[2], &x) && getnum(csvln.s[3], &y);
				if (success) {
					strcpy(s, csvln.s[0]); //FIXME
					strcpy(z, csvln.s[1]); //FIXME
				}
				csv_line_done(&csvln);		
			} else {
				printf ("Failure to input -r file\n");	
				exit(EXIT_FAILURE);
			}
			
			if (!success) {
				printf ("Problems with input in -r file\n");	
				exit(EXIT_FAILURE);
			}

			file_success = success;

			prior_success = assign_relative_prior (plyrs, s, z, x, y, quietmode, rps);

		}

		fclose(fil);
	}
	else {
		file_success = FALSE;
	}

	if (!file_success) {
			fprintf (stderr, "Errors in file \"%s\"\n",f_name);
			exit(EXIT_FAILURE);
	}
	if (!prior_success) {
			fprintf (stderr, "Errors in file \"%s\" (not matching names)\n",f_name);
			exit(EXIT_FAILURE);
	}

	return;
}

