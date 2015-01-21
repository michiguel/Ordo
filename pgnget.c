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
#include <ctype.h>
#include <string.h>
#include <assert.h>

#include "mystr.h"
#include "pgnget.h"
#include "boolean.h"
#include "ordolim.h"
#include "mytypes.h"


#if 0
static void	hashstat(void);
#endif

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/


#define PGNSTRSIZE 1024

struct pgn_result {	
	int 	wtag_present;
	int 	btag_present;
	int 	result_present;	
	int 	result;
	char 	wtag[PGNSTRSIZE];
	char 	btag[PGNSTRSIZE];
};

static struct DATA DaBa;

static bool_t	data_init (struct DATA *d);

/*------------------------------------------------------------------------*/

static bool_t	addplayer (struct DATA *d, const char *s, player_t *i);
static void		report_error 	(long int n);
static int		res2int 		(const char *s);
static bool_t 	fpgnscan (FILE *fpgn, bool_t quiet);
static bool_t 	is_complete (struct pgn_result *p);
static void 	pgn_result_reset (struct pgn_result *p);
static bool_t 	pgn_result_collect (struct pgn_result *p, struct DATA *d);

/*
|
|
\*--------------------------------------------------------------*/

struct DATA *
database_init_frompgn (const char *pgn, bool_t quiet)
{
	struct DATA *pDAB = NULL;
	FILE *fpgn;
	bool_t ok = FALSE;

	ok = data_init (&DaBa);

	if (ok) {
		if (NULL != (fpgn = fopen (pgn, "r"))) {
			ok = fpgnscan (fpgn, quiet);
			fclose(fpgn);
		}
		pDAB = &DaBa;
	}
	return ok? pDAB: NULL;

	#if 0
	hashstat();
	#endif
}

static void data_done (struct DATA *d);

void 
database_done (struct DATA *p)
{
	data_done (p);
	return;
}


const char *
database_ptr2name (const struct DATA *db, player_t i)
{
	const char *r;
	size_t j = (size_t)i / MAXNAMESxBLOCK;
	size_t k = (size_t)i % MAXNAMESxBLOCK;
	r = db->nm[j]->p[k];
	return r;	
}


/*--------------------------------------------------------------*\
|
|
\**/

static bool_t
data_init (struct DATA *d)
{
	struct GAMEBLOCK *p;
	struct NAMEBLOCK *t;
	char			 *l;
	bool_t ok = TRUE;

	ok = ok && NULL != (l = malloc (LABELBUFFERSIZE));
	if (ok)	l[0] = '\0';
	d->labels = l;

	d->labels_end_idx = 0;
	d->n_players = 0;
	d->n_games = 0;

	d->gb_filled = 0;;
	d->gb_idx = 0;
	d->gb_allocated = 0;

	d->nm_filled = 0;;
	d->nm_idx = 0;
	d->nm_allocated = 0;

	ok = ok && NULL != (p = malloc (sizeof(struct GAMEBLOCK)));
	if (ok)	d->gb_allocated++;
	d->gb[0] = p;

	ok = ok && NULL != (t = malloc (sizeof(struct NAMEBLOCK)));
	if (ok)	d->nm_allocated++;
	d->nm[0] = t;

	if (!ok) data_done(d);

	return ok;
}

static void
data_done (struct DATA *d)
{
	size_t n;

	if (d->labels != NULL) {
		d->labels[0] = '\0';
		free(d->labels);
	}

	d->labels_end_idx = 0;
	d->n_players = 0;
	d->n_games = 0;

//
	n = d->gb_allocated;

	while (n-->0) {
		free(d->gb[n]);
		d->gb[n] = NULL;
	}

	d->gb_filled = 0;;
	d->gb_idx = 0;
	d->gb_allocated = 0;

//
	n = d->nm_allocated;

	while (n-->0) {
		free(d->nm[n]);
		d->nm[n] = NULL;
	}

	d->nm_filled = 0;;
	d->nm_idx = 0;
	d->nm_allocated = 0;
}


static const char *
addname (struct DATA *d, const char *s)
{
	char *b = d->labels + d->labels_end_idx;
	ptrdiff_t remaining = (&d->labels[LABELBUFFERSIZE] - b - 1);
	ptrdiff_t len = (ptrdiff_t)strlen(s);
	char *nameptr = b;

	bool_t success = len < remaining;

if (!success) {

	char *p;

	if (NULL != (p = malloc(len+1))) {
		ptrdiff_t i;
		b = p;
		for (i = 0; i < len; i++) {*b++ = *s++;}
		*b++ = '\0';
		return p;		
	} 

}



	if (success) {
		ptrdiff_t i;

		for (i = 0; i < len; i++) {*b++ = *s++;}
		*b++ = '\0';
		d->labels_end_idx = b - d->labels;

	} else {
		nameptr = NULL;
	}

	return nameptr;
} 

static bool_t
addplayer (struct DATA *d, const char *s, player_t *idx)
{
	const char *nameptr;
	bool_t success = NULL != (nameptr = addname(d,s));

	if (success) {

		*idx = d->n_players++;

		d->nm[d->nm_filled]->p[d->nm_idx++] = nameptr;

		if (d->nm_idx == MAXNAMESxBLOCK) { // hit new block

			struct NAMEBLOCK *nm;

			d->nm_idx = 0;
			d->nm_filled++;

			success = NULL != (nm = malloc (sizeof(struct NAMEBLOCK)));
			if (success) {
				d->nm_allocated++;
			}
			d->nm[d->nm_filled] = nm;
		}
	}

	return success;
}

static void report_error (long int n) 
{
	fprintf(stderr, "\nParsing error in line: %ld\n", n+1);
}

#define PEAXPOD 8
#define PODBITS 12
#define PODMASK ((1<<PODBITS)-1)
#define PODMAX   (1<<PODBITS)
#define PEA_REM_MAX (256*256)

struct NAMEPEA {
	player_t itos;
	uint32_t hash;
};

struct NAMEPOD {
	struct NAMEPEA pea[PEAXPOD];
	int n;
};

struct NAMEPOD namehashtab[PODMAX];

struct NAMEPEA nameremains[PEA_REM_MAX];
int nameremains_n;

static const char *get_DB_name(int i) 
{
	size_t j = (size_t)i / MAXNAMESxBLOCK;
	size_t k = (size_t)i % MAXNAMESxBLOCK;
	return DaBa.nm[j]->p[k];
}

#if 0
static void
hashstat(void)
{
	int i, level;
	int hist[9] = {0,0,0,0,0,0,0,0,0};

	for (i = 0; i < PODMAX; i++) {
		level = namehashtab[i].n;
		hist[level]++;
	}
	for (i = 0; i < 9; i++) {
		printf ("level[%d]=%d\n",i,hist[i]);
	}
}
#endif

static bool_t
name_ispresent (const char *s, uint32_t hash, /*out*/ player_t *out_index)
{
	struct NAMEPOD *ppod = &namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;
	bool_t 			found= FALSE;
	int i;


	ppea = ppod->pea;
	n = ppod->n;
	for (i = 0; i < n; i++) {
		if (ppea[i].hash == hash) {
			const char *name_str = get_DB_name(ppea[i].itos);
			assert(name_str);
			if (!strcmp(s, name_str)) {
				found = TRUE;
				*out_index = ppea[i].itos;
				break;
			}
		}
	}
	if (found) return found;

	ppea = nameremains;
	n = nameremains_n;
	for (i = 0; i < n; i++) {
		if (ppea[i].hash == hash && !strcmp(s, get_DB_name(ppea[i].itos))) {
			found = TRUE;
			*out_index = ppea[i].itos;
			break;
		}
	}

	return found;
}

static bool_t
name_register (uint32_t hash, int i)
{
	struct NAMEPOD *ppod = &namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;

	ppea = ppod->pea;
	n = ppod->n;	

	if (n < PEAXPOD) {
		ppea[n].itos = i;
		ppea[n].hash = hash;
		ppod->n++;
		return TRUE;
	}
	else if (nameremains_n < PEA_REM_MAX) {
		nameremains[nameremains_n].itos = i;
		nameremains[nameremains_n].hash = hash;
		nameremains_n++;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/*http://www.cse.yorku.ca/~oz/hash.html*/

static uint32_t
namehash(const char *str)
{
	uint32_t hash = 5381;
	char chr;
	unsigned int c;
	while ('\0' != *str) {
		chr = *str++;
		c = (unsigned int) ((unsigned char)(chr));
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

static void
pgn_result_reset (struct pgn_result *p)
{
	p->wtag_present   = FALSE;
	p->btag_present   = FALSE;
	p->result_present = FALSE;	
	p->wtag[0] = '\0';
	p->btag[0] = '\0';
	p->result = 0;
}

static bool_t
pgn_result_collect (struct pgn_result *p, struct DATA *d)
{
	player_t 	i;
	player_t 	j;
	bool_t 		ok = TRUE;
	player_t 	plyr;
	const char *tagstr;
	uint32_t 	taghsh;

	tagstr = p->wtag;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (tagstr, taghsh, &plyr)) {
		ok = addplayer (d, tagstr, &plyr) && name_register(taghsh,plyr);
	}
	i = plyr;

	tagstr = p->btag;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (tagstr, taghsh, &plyr)) {
		ok = addplayer (d, tagstr, &plyr) && name_register(taghsh,plyr);
	}
	j = plyr;

	ok = ok && (uint64_t)d->n_games < ((uint64_t)MAXGAMESxBLOCK*(uint64_t)MAXBLOCKS);

	if (ok) {

		struct GAMEBLOCK *g;

		size_t idx = d->gb_idx;
		size_t blk = d->gb_filled;

		d->gb[blk]->white [idx] = i;
		d->gb[blk]->black [idx] = j;
		d->gb[blk]->score [idx] = p->result;
		d->n_games++;
		d->gb_idx++;

		if (d->gb_idx == MAXGAMESxBLOCK) { // hit new block

			d->gb_idx = 0;
			d->gb_filled++;

			blk = d->gb_filled;
			if (NULL == (g = malloc (sizeof(struct GAMEBLOCK) * (size_t)1))) {
				d->gb[blk] = NULL;
				ok = FALSE; // failed
			} else {
				d->gb[blk] = g;
				d->gb_allocated++;
				ok = TRUE;
			}
		}
	}

	return ok;
}


static bool_t 
is_complete (struct pgn_result *p)
{
	return p->wtag_present && p->btag_present && p->result_present;
}


static void
parsing_error(long line_counter)
{
	report_error (line_counter);
	fprintf(stderr, "Parsing problems\n");
	exit(EXIT_FAILURE);
}


static bool_t
fpgnscan (FILE *fpgn, bool_t quiet)
{
#define MAX_MYLINE 40000

	char myline[MAX_MYLINE];

	const char *whitesep = "[White \"";
	const char *whiteend = "\"]";
	const char *blacksep = "[Black \"";
	const char *blackend = "\"]";
	const char *resulsep = "[Result \"";
	const char *resulend = "\"]";

	size_t whitesep_len, blacksep_len, resulsep_len;
	char *x, *y;

	struct pgn_result 	result;
	long int			line_counter = 0;
	long int			game_counter = 0;
	int					games_x_dot  = 2000;

	if (NULL == fpgn)
		return FALSE;

	if (!quiet) 
		printf("\nLoading data (%d games x dot): \n\n",games_x_dot); fflush(stdout);

	pgn_result_reset  (&result);

	whitesep_len = strlen(whitesep); 
	blacksep_len = strlen(blacksep); 
	resulsep_len = strlen(resulsep); 

	while (NULL != fgets(myline, MAX_MYLINE, fpgn)) {

		line_counter++;

		if (NULL != (x = strstr (myline, whitesep))) {
			x += whitesep_len;
			if (NULL != (y = strstr (myline, whiteend))) {
				*y = '\0';
						strcpy (result.wtag, x);
						result.wtag_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (NULL != (x = strstr (myline, blacksep))) {
			x += blacksep_len;
			if (NULL != (y = strstr (myline, blackend))) {
				*y = '\0';
						strcpy (result.btag, x);
						result.btag_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (NULL != (x = strstr (myline, resulsep))) {
			x += resulsep_len;
			if (NULL != (y = strstr (myline, resulend))) {
				*y = '\0';
						result.result = res2int (x);
						result.result_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (is_complete (&result)) {
			if (!pgn_result_collect (&result, &DaBa)) {
				fprintf (stderr, "\nCould not collect more games: Limits reached\n");
				exit(EXIT_FAILURE);
			}
			pgn_result_reset  (&result);
			game_counter++;

			if (!quiet) {
				if ((game_counter%games_x_dot)==0) {
					printf ("."); fflush(stdout);
				}
				if ((game_counter%100000)==0) {
					printf ("|  %4ldk\n", game_counter/1000); fflush(stdout);
				}
			}
		}

	} /* while */

	if (!quiet) printf("|\n\n"); fflush(stdout);

	return TRUE;
}

static int
res2int (const char *s)
{
	if (!strcmp(s, "1-0")) {
		return WHITE_WIN;
	} else if (!strcmp(s, "0-1")) {
		return BLACK_WIN;
	} else if (!strcmp(s, "1/2-1/2")) {
		return RESULT_DRAW;
	} else if (!strcmp(s, "=-=")) {
		return RESULT_DRAW;
	} else if (!strcmp(s, "*")) {
		return DISCARD;
	} else {
		fprintf(stderr, "PGN reading problems in Result tag: %s\n",s);
		exit(0);
		return DISCARD;
	}
}

/************************************************************************/



