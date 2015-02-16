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

//====================== RELATIVE PRIORS ====================================================================

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

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

#include "mytypes.h"

typedef struct relprior rpunit_t;

#define MAX_RPBLOCK 100

struct relprior_block;

struct relprior_block {
	rpunit_t 				u[MAX_RPBLOCK];
	size_t 					n;
	size_t					sz;
	struct relprior_block *	next;
};

typedef struct relprior_block rpblock_t;

struct rpmanager {
	rpblock_t *	first;
	rpblock_t *	last;
	rpblock_t *	p;
	size_t 		i;
};

//================================================

void
rpblock_clear (rpblock_t *r)
{
	assert (r);
	r->n = 0;
	r->sz = MAX_RPBLOCK;
	r->next = NULL;
}

rpblock_t *
rpblock_create(void)
{
	rpblock_t *q = malloc (sizeof(rpblock_t));
	if (q) rpblock_clear(q);
	return q;
}

void
rpblock_destroy(rpblock_t *p)
{
	assert(p);
	rpblock_clear(p);
	free(p);
	return;
}

bool_t
rpblock_add (rpblock_t *b, rpunit_t *u)
{
	rpblock_t *q;
	rpblock_t *t;

	assert(b);
	assert(u);

	q = b;
	assert(q->n <= q->size);

	if (q->n == q->sz) {
		t = rpblock_create();
		if (t) {
				q->next = t;
				q = t;
				q->n = 0;
		} else {
			return FALSE;
		}
	}

	q->u[q->n++] = *u;

	return TRUE;
}

//

bool_t
rpman_init (struct rpmanager *rm)
{
	rpblock_t *q = rpblock_create();
	assert(rm);
	rm->first = q;
	rm->last = q;
	rm->p = NULL;
	rm->i = 0;
	return q != NULL;
}

void
rpman_done (struct rpmanager *rm)
{
	rpblock_t *p;
	rpblock_t *q;
	assert(rm);
	p = rm->first;
	rm->first = NULL;
	rm->last  = NULL;
	rm->p     = NULL;
	rm->i 	  = 0;
	while(p) {
		q = p->next;
		rpblock_destroy(p);
		p = q;
	}
	return;
}


size_t
rpman_count(struct rpmanager *rm)
{
	rpblock_t *p;
	size_t count = 0;
	assert(rm);
	for (p = rm->first; p != NULL; p = p->next) {
			count += p->n;
	}
	return count;
}

void
rpman_parkstart(struct rpmanager *rm)
{
	assert(rm);
	rm->i = 0;
	rm->p = rm->first;
}

rpunit_t *
rpman_pointnext_unit(struct rpmanager *rm)
{
	size_t i;
	rpblock_t *p;
	rpunit_t *pu;

	assert(rm);
	p = rm->p;
	i = rm->i;
	assert(i <= p->n);
	
	while (i == p->n) {
		p = p->next;
		if (NULL == p) {
			return NULL;
		}
		i = 0;
	}

	pu = &p->u[i++];

	rm->p = p;
	rm->i = i;

	return pu;
}


bool_t
rpman_add_unit(struct rpmanager *rm, rpunit_t *u)
{
	bool_t ok;
	rpblock_t *p;

	assert(rm);
	p = rm->last;
	assert(p);

	ok = rpblock_add (p, u);
	if (ok && NULL != p->next) rm->last = p->next; // next was created
	return ok;
}


rpunit_t *
rpman_to_newarray (struct rpmanager *rm, size_t *psz)
{
	rpunit_t *p_ret;
	rpunit_t *p;
	rpunit_t *pu;
	size_t sz = rpman_count (rm);

	p_ret = p = malloc(sizeof(rpunit_t) * sz);

	if (p == NULL) return p;

	rpman_parkstart (rm);
	while (NULL != (pu = rpman_pointnext_unit(rm))) {
		*p++ = *pu;
	}

	*psz = sz;
	return p_ret;
}

//==============================

void
rpunit_build (int32_t p_a, int32_t p_b, double x, double sigma, rpunit_t *u /*@out@*/)
{
	u->player_a = p_a;
	u->player_b = p_b;
	u->delta    = x;
	u->sigma    = sigma;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


bool_t
players_name2idx (const struct PLAYERS *plyrs, const char *player_name, int32_t *pi)
{
	size_t j;
	bool_t found;
	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_name);
		if (found) {
			*pi = (int32_t) j; //FIXME size_t
		} 
	}
	return found;
}

static bool_t
rman_set_relprior__ (const struct PLAYERS *plyrs, const char *player_a, const char *player_b, double x, double sigma, struct rpmanager *rm)
{
	size_t j;
	int32_t p_a = -1; 
	int32_t p_b = -1;
	size_t n;
	bool_t found, ok;
	rpunit_t u;

	assert(sigma > PRIOR_SMALLEST_SIGMA);

	found = players_name2idx (plyrs, player_a, &p_a) && 
			players_name2idx (plyrs, player_b, &p_b);
	if (!found) return found;

	rpunit_build (p_a, p_b, x, sigma, &u);

	ok = rpman_add_unit(rm, &u);

	if (!ok) { {fprintf (stderr, "Maximum memory for relative anchors exceeded\n"); exit(EXIT_FAILURE);}}

	return ok;
}

static bool_t
rman_assign_relative_prior__ (const struct PLAYERS *plyrs, char *s, char *z, double x, double y, bool_t quiet, struct rpmanager *rm /*@out@*/)
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
			suc = rman_set_relprior__ (plyrs, s, z, x, y, rm);
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
relpriors_init (bool_t quietmode, const struct PLAYERS *plyrs, const char *f_name
				, struct rel_prior_set *rps /*@out@*/, struct rel_prior_set *bak /*@out@*/)
{
	#define MAX_MYLINE 1024

	struct rpmanager rpmanager = {NULL, NULL, NULL, 0};

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

		if (rpman_init(&rpmanager)) {

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

				prior_success = rman_assign_relative_prior__ (plyrs, s, z, x, y, quietmode, &rpmanager);

			}

			rps->x 	= rpman_to_newarray (&rpmanager, &rps->n);
			bak->x	= rpman_to_newarray (&rpmanager, &bak->n);

			if (rps->x == NULL || bak->x == NULL) {
				if (rps->x != NULL) {free (rps->x); rps->n = 0;}
				if (bak->x != NULL) {free (bak->x); bak->n = 0;}
				fprintf (stderr, "Not enough memory for relative priors\n");
				exit(EXIT_FAILURE);
			}

			rpman_done(&rpmanager);

		} else {
			prior_success = FALSE;
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



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


void
relpriors_done (struct rel_prior_set *rps /*@out@*/, struct rel_prior_set *rps_backup /*@out@*/)
{
	assert(rps);
	assert(rps_backup);
	rps->n = 0;
	rps_backup->n = 0;
	assert(rps->x);
	assert(rps_backup->x);
	free(rps->x);
	free(rps_backup->x);
	return;
}

//====================== PRIORS =============================================================================

#include <math.h>

bool_t 	Some_prior_set = FALSE;
int 	Priored_n = 0;

bool_t 
has_a_prior(struct prior *pr, size_t j) {return pr[j].isset;}

void
priors_reset(struct prior *p, size_t n)
{	size_t i;
	for (i = 0; i < n; i++) {
		p[i].value = 0;
		p[i].sigma = 1;
		p[i].isset = FALSE;
	}
	Some_prior_set = FALSE;
	Priored_n = 0;
}

void
priors_copy(const struct prior *p, size_t n, struct prior *q)
{	size_t i;
	for (i = 0; i < n; i++) {
		q[i] = p[i];
	}
}

void
priors_shuffle(struct prior *p, size_t n)
{
	size_t i;
	double value, sigma;
	for (i = 0; i < n; i++) {
		if (p[i].isset) {
			value = p[i].value;
			sigma = p[i].sigma;
			p[i].value = rand_gauss (value, sigma);
		}
	}
}

void
priors_show (const struct PLAYERS *plyrs, struct prior *p, size_t n)
{ 
	size_t i;
	if (Priored_n > 0) {
		printf ("Loose Anchors {\n");
		for (i = 0; i < n; i++) {
			if (p[i].isset) {
				printf ("  [%s] = %lf +/- %lf\n", plyrs->name[i], p[i].value, p[i].sigma);
			}
		}
		printf ("}\n");
	} else {
		printf ("Loose Anchors = none\n");
	}
}

static bool_t
set_prior (const struct PLAYERS *plyrs, const char *player_name, double x, double sigma, struct prior *pr)
{
	size_t j;
	bool_t found;
	assert(sigma > PRIOR_SMALLEST_SIGMA);
	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_name);
		if (found) {
			pr[j].value = x;
			pr[j].sigma = sigma;
			pr[j].isset = TRUE;
			Some_prior_set = TRUE;
			Priored_n++;
		} 
	}

	return found;
}

// comes later
static bool_t set_anchor (const char *player_name, double x, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/);


static bool_t
assign_prior (char *name_prior, double x, double y, bool_t quiet, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/, struct prior *pr /*@out@*/)
{
	bool_t prior_success = TRUE;
	bool_t suc;
	if (y < 0) {
			fprintf (stderr, "Prior, %s --> FAILED, error/uncertainty cannot be negative", name_prior);	
			suc = FALSE;
	} else { 
		if (y < PRIOR_SMALLEST_SIGMA) {
			suc = set_anchor (name_prior, x, rat, plyrs);
			if (!suc) {
				fprintf (stderr, "Prior, %s --> FAILED, name not found in input file\n", name_prior);
			} 
			else {
				if (!quiet)
				printf ("Anchoring, %s --> %.1lf\n", name_prior, x);
			} 
		} else {
			suc = set_prior (plyrs, name_prior, x, y, pr);
			if (!suc) {
				fprintf (stderr, "Prior, %s --> FAILED, name not found in input file\n", name_prior);					
			} 
		}
	}
	if (!suc) prior_success = FALSE;
	return prior_success;
}

void
priors_load (bool_t quietmode, const char *fpriors_name, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/, struct prior *pr /*@out@*/)
{
	#define MAX_MYLINE 1024
	FILE *fpriors;
	char myline[MAX_MYLINE];
	char name_prior[MAX_MYLINE];
	char *p;
	bool_t success;
	double x, y;
	bool_t prior_success = TRUE;
	bool_t file_success = TRUE;

	assert(NULL != fpriors_name);

	if (NULL == fpriors_name) {
		fprintf (stderr, "Error, file not provided, absent, or corrupted\n");
		exit(EXIT_FAILURE);
	}

	if (NULL != (fpriors = fopen (fpriors_name, "r"))) {

		csv_line_t csvln;

		while (file_success && NULL != fgets(myline, MAX_MYLINE, fpriors)) {
			success = FALSE;
			p = myline;
			p = skipblanks(p);
			x = 0;
			y = 0;
			if (*p == '\0') continue;

			if (csv_line_init(&csvln, myline)) {
				success = csvln.n >= 3 && getnum(csvln.s[1], &x) && getnum(csvln.s[2], &y);
				if (success) {
					strcpy(name_prior, csvln.s[0]); //FIXME
				}
				if (success && csvln.n > 3) {
					int i;
					double acc = y*y;
					for (i = 3; success && i < csvln.n; i++) {
						success = getnum(csvln.s[i], &y);
						if (success) acc += y*y;		
					}
					y = sqrt(acc);
				}
				csv_line_done(&csvln);		
			} else {
				fprintf (stderr, "Failure to input -y file\n");
				exit(EXIT_FAILURE);
			}

			file_success = success;
			prior_success = assign_prior (name_prior, x, y, quietmode, rat, plyrs, pr);
		}

		fclose(fpriors);
	}
	else {
		file_success = FALSE;
	}

	if (!file_success) {
			fprintf (stderr, "Errors in file \"%s\"\n",fpriors_name);
			exit(EXIT_FAILURE);
	}
	if (!prior_success) {
			fprintf (stderr, "Errors in file \"%s\" (not matching names)\n",fpriors_name);
			exit(EXIT_FAILURE);
	}

	return;
}

//====================== ANCHORS ============================================================================

void
anchor_j (size_t j, double x, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/)
{	
	plyrs->anchored_n++;
	plyrs->prefed[j] = TRUE;
	rat->ratingof[j] = x;
}

static bool_t
set_anchor (const char *player_name, double x, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/)
{
	size_t j;
	bool_t found;
	for (j = 0, found = FALSE; !found && j < plyrs->n; j++) {
		found = !strcmp(plyrs->name[j], player_name);
		if (found) {
			anchor_j (j, x, rat, plyrs);
		} 
	}
	return found;
}

static bool_t
assign_anchor (char *name_pinned, double x, bool_t quiet, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/)
{
	bool_t pin_success = set_anchor (name_pinned, x, rat, plyrs);
	if (pin_success) {
		if (!quiet)
		printf ("Anchoring, %s --> %.1lf\n", name_pinned, x);
	} else {
		fprintf (stderr, "Anchoring, %s --> FAILED, name not found in input file\n", name_pinned);					
	}
	return pin_success;
}

void
init_manchors (bool_t quietmode, const char *fpins_name, struct RATINGS *rat /*@out@*/, struct PLAYERS *plyrs /*@out@*/)
{
	#define MAX_MYLINE 1024
	FILE *fpins;
	char myline[MAX_MYLINE];
	char name_pinned[MAX_MYLINE];
	char *p;
	bool_t success;
	double x;
	bool_t pin_success = TRUE;
	bool_t file_success = TRUE;

	assert(NULL != fpins_name);

	if (NULL == fpins_name) {
		fprintf (stderr, "Error, file not provided, absent, or corrupted\n");
		exit(EXIT_FAILURE);
	}

	if (NULL != (fpins = fopen (fpins_name, "r"))) {

		csv_line_t csvln;

		while (pin_success && file_success && NULL != fgets(myline, MAX_MYLINE, fpins)) {
			success = FALSE;
			p = myline;

			p = skipblanks(p);
			x = 0;
			if (*p == '\0') continue;

			if (csv_line_init(&csvln, myline)) {
				success = csvln.n >= 2 && getnum(csvln.s[1], &x);
				if (success) {
					strcpy(name_pinned, csvln.s[0]); //FIXME
				}
				csv_line_done(&csvln);		
			} else {
				printf ("Failure to input -m file\n");
				exit(EXIT_FAILURE);
			}
			file_success = success;
			pin_success = assign_anchor (name_pinned, x, quietmode, rat, plyrs);
		}

		fclose(fpins);
	}
	else {
		file_success = FALSE;
	}

	if (!file_success) {
			fprintf (stderr, "Errors in file \"%s\"\n",fpins_name);
			exit(EXIT_FAILURE);
	}
	if (!pin_success) {
			fprintf (stderr, "Errors in file \"%s\" (not matching names)\n",fpins_name);
			exit(EXIT_FAILURE);
	}
	return;
}

