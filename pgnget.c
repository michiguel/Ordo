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

#include "mymem.h"

#include "namehash.h"

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

static struct DATA * structdata_init (void);
static void	structdata_done (struct DATA *d);

/*------------------------------------------------------------------------*/


static bool_t	addplayer (struct DATA *d, const char *s, player_t *i);
static void		report_error 	(long int n);
static int		res2int 		(const char *s);
static bool_t 	fpgnscan (FILE *fpgn, bool_t quiet, struct DATA *d);
static bool_t 	is_complete (struct pgn_result *p);
static void 	pgn_result_reset (struct pgn_result *p);
static bool_t 	pgn_result_collect (struct pgn_result *p, struct DATA *d);

static void		syn_preload (bool_t quiet, const char *synfile_name, struct DATA *d);

/*
|
|
\*--------------------------------------------------------------*/

static struct DATA *
structdata_init (void)
{
	struct DATA *d = NULL;
	struct GAMEBLOCK *p = NULL;
	struct NAMEBLOCK *t = NULL;
	bool_t ok = TRUE;

	ok = ok && NULL != (d = memnew (sizeof(struct DATA)));
	if (ok) {
		d->labels_head.buf = NULL;
		d->labels_head.nxt = NULL;
		d->labels_head.idx = 0;
		d->curr = &d->labels_head;

		d->n_players = 0;
		d->n_games = 0;

		d->gb_filled = 0;;
		d->gb_idx = 0;
		d->gb_allocated = 0;

		d->nm_filled = 0;;
		d->nm_idx = 0;
		d->nm_allocated = 0;

		ok = ok && NULL != (p = memnew (sizeof(struct GAMEBLOCK)));
		if (ok)	d->gb_allocated++;
		d->gb[0] = p;

		ok = ok && NULL != (t = memnew (sizeof(struct NAMEBLOCK)));
		if (ok)	d->nm_allocated++;
		d->nm[0] = t;

		if (!ok) structdata_done(d);
	}
	return ok? d: NULL;
}

static void
structdata_done (struct DATA *d)
{
	size_t n;
	namenode_t *p;
	namenode_t *q;

	d->n_players = 0;
	d->n_games = 0;

//
	n = d->gb_allocated;

	while (n-->0) {
		memrel(d->gb[n]);
		d->gb[n] = NULL;
	}

	d->gb_filled = 0;;
	d->gb_idx = 0;
	d->gb_allocated = 0;

//
	n = d->nm_allocated;

	while (n-->0) {
		memrel(d->nm[n]);
		d->nm[n] = NULL;
	}

	d->nm_filled = 0;;
	d->nm_idx = 0;
	d->nm_allocated = 0;

//
	p = d->labels_head.nxt;

	while (p) {
		if (p->buf) {memrel(p->buf); p->buf = NULL; p->idx = 0;}
		q = p->nxt;	
		memrel(p);
		p = q;
	}

	p = &d->labels_head;
		if (p->buf) {memrel(p->buf); p->buf = NULL; p->idx = 0;}

}


/*
|
|
\*--------------------------------------------------------------*/

#include "strlist.h"

struct DATA *
database_init_frompgn (strlist_t *sl, const char *synfile_name, bool_t quiet)
{

	struct DATA *pDAB = NULL;
	FILE *fpgn;
	bool_t ok = FALSE;
	const char *pgn;

	ok = NULL != (pDAB = structdata_init ());

	if (NULL != synfile_name) // not provided
		syn_preload (quiet, synfile_name, pDAB); 

	strlist_rwnd(sl);

	pgn = strlist_next(sl);
	while (ok && pgn) {
		if (!quiet)	printf ("\nFile: %s\n",pgn);
		if (NULL != (fpgn = fopen (pgn, "r"))) {
			ok = fpgnscan (fpgn, quiet, pDAB);
			fclose(fpgn);
		} else {
			ok = FALSE;
		}
		if (ok) pgn = strlist_next(sl);
	}
	return ok? pDAB: NULL;

	#if 0
	hashstat();
	#endif
}

void 
database_done (struct DATA *p)
{
	structdata_done (p);
	return;
}

const char *
database_getname(const struct DATA *d, player_t i) 
{
	size_t j = (size_t)i / MAXNAMESxBLOCK;
	size_t k = (size_t)i % MAXNAMESxBLOCK;
	return d->nm[j]->p[k];
}


#include "mytypes.h"

void 
database_transform(const struct DATA *db, struct GAMES *g, struct PLAYERS *p, struct GAMESTATS *gs)
{
	enum maxresults {MAXRESTYPE = 8};
	player_t j;
	player_t topn;
	gamesnum_t gamestat[MAXRESTYPE] = {0,0,0,0,0,0,0,0};

	assert(db && p && g && gs);
	assert(p->name && p->flagged && p->present_in_games && p->prefed && p->priored && p->performance_type);

	p->n = db->n_players; 
	g->n = db->n_games; 

	topn = db->n_players; 
	for (j = 0; j < topn; j++) {
		p->name[j] = database_getname(db,j);
		p->flagged[j] = FALSE;
		p->present_in_games[j] = FALSE;
		p->prefed [j] = FALSE;
		p->priored[j] = FALSE;
		p->performance_type[j] = PERF_NORMAL;
	}

{
	player_t wp, bp;

	size_t blk_filled  = db->gb_filled;
	size_t blk;
	size_t idx_last = db->gb_idx;
	size_t idx;
	gamesnum_t i = 0;

	for (blk = 0; blk < blk_filled; blk++) {

		for (idx = 0; idx < MAXGAMESxBLOCK; idx++) {

			g->ga[i].whiteplayer = wp = db->gb[blk]->white[idx];
			g->ga[i].blackplayer = bp = db->gb[blk]->black[idx]; 
			g->ga[i].score       =      db->gb[blk]->score[idx];
			if (g->ga[i].score < MAXRESTYPE) gamestat[g->ga[i].score]++;

			if (g->ga[i].score < DISCARD) {
				p->present_in_games[wp] = TRUE;
				p->present_in_games[bp] = TRUE;
			}
			
			i++;
		}
	
	}

	blk = blk_filled;

		for (idx = 0; idx < idx_last; idx++) {

			g->ga[i].whiteplayer = wp = db->gb[blk]->white[idx];
			g->ga[i].blackplayer = bp = db->gb[blk]->black[idx]; 
			g->ga[i].score       =      db->gb[blk]->score[idx];
			if (g->ga[i].score < MAXRESTYPE) gamestat[g->ga[i].score]++;

			if (g->ga[i].score < DISCARD) {
				p->present_in_games[wp] = TRUE;
				p->present_in_games[bp] = TRUE;
			}

			i++;
		}

	if (i != db->n_games) {
		fprintf (stderr, "Error, games not loaded propely\n");
		exit(EXIT_FAILURE);
	}
}

	gs->white_wins	= gamestat[WHITE_WIN];
	gs->draws		= gamestat[RESULT_DRAW];
	gs->black_wins	= gamestat[BLACK_WIN];
	gs->noresult	= gamestat[DISCARD] 
					+ gamestat[IGNORED|WHITE_WIN]
					+ gamestat[IGNORED|RESULT_DRAW]
					+ gamestat[IGNORED|BLACK_WIN]
					;

	assert ((long)g->n == (gs->white_wins + gs->draws + gs->black_wins + gs->noresult));

	return;
}


void 
database_ignore_draws (struct DATA *db)
{
	size_t blk_filled = db->gb_filled;
	size_t idx_last   = db->gb_idx;
	size_t blk;
	size_t idx;

	blk_filled = db->gb_filled;
	idx_last   = db->gb_idx;

	for (blk = 0; blk < blk_filled; blk++) {
		for (idx = 0; idx < MAXGAMESxBLOCK; idx++) {
			if (db->gb[blk]->score[idx] == RESULT_DRAW)
				db->gb[blk]->score[idx] |= IGNORED;
		}
	}

	blk = blk_filled;

		for (idx = 0; idx < idx_last; idx++) {
			if (db->gb[blk]->score[idx] == RESULT_DRAW)
				db->gb[blk]->score[idx] |= IGNORED;
		}

	return;
}

#include "bitarray.h"

void 
database_include_only (struct DATA *db, bitarray_t *pba)
{
	player_t wp, bp;
	size_t blk;
	size_t idx;
	size_t blk_filled = db->gb_filled;
	size_t idx_last   = db->gb_idx;

	for (blk = 0; blk < blk_filled; blk++) {
		for (idx = 0; idx < MAXGAMESxBLOCK; idx++) {
			wp = db->gb[blk]->white[idx];
			bp = db->gb[blk]->black[idx];
			if (!ba_ison(pba, wp) || !ba_ison(pba, bp))
				db->gb[blk]->score[idx] |= IGNORED;
		}
	}

	blk = blk_filled;

		for (idx = 0; idx < idx_last; idx++) {
			wp = db->gb[blk]->white[idx];
			bp = db->gb[blk]->black[idx];
			if (!ba_ison(pba, wp) || !ba_ison(pba, bp))
				db->gb[blk]->score[idx] |= IGNORED;
		}

	return;
}


/*--------------------------------------------------------------*\
|
|
\**/


static const char *
addname (struct DATA *d, const char *s)
{
	char *b;
	char *nameptr;
	char *bf = NULL;
	namenode_t *nd;
	size_t sz = strlen(s) + 1;
	bool_t ok;

	assert (d->curr != NULL);

	ok = d->curr->buf != NULL;

	if (!ok) {
		assert (d->curr->nxt == NULL);
		assert (d->curr->idx == 0);
		ok = NULL != (bf = memnew (LABELBUFFERSIZE));
		if (ok) {
			bf[0] = '\0';
			d->curr->buf = bf;
			d->curr->nxt = NULL;
			d->curr->idx = 0;
		}
	}

	if (!ok) return NULL;	

	ok = ok && (LABELBUFFERSIZE > d->curr->idx + sz);

	if (!ok) {
		ok = NULL != (nd = memnew (sizeof(namenode_t)));
		if (ok) {
			ok = NULL != (bf = memnew (LABELBUFFERSIZE));
			if (ok) bf[0] = '\0'; else memrel(nd);
		}

		if (ok) {
	
			nd->nxt = NULL;
			nd->buf = bf;
			nd->idx = 0;

			d->curr->nxt = nd;
			d->curr = nd;

			ok = LABELBUFFERSIZE > d->curr->idx + sz;
		}
	}

	if (ok) {
		size_t i;
		nameptr = b = &d->curr->buf[d->curr->idx];
		for (i = 0; i < sz; i++) {*b++ = *s++;}
		d->curr->idx += sz;

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

			success = NULL != (nm = memnew (sizeof(struct NAMEBLOCK)));
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


#include "csv.h"
static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}

static bool_t
syn_apply_pair (struct DATA *d, const char *m, const char *s)
{
	#define NOPLAYER -1
	bool_t 		ok = TRUE;
	player_t 	plyr_0 = NOPLAYER; // to silence warnings
	player_t 	plyr_i = NOPLAYER; // to silence warnings
	const char *tagstr;
	uint32_t 	taghsh;

	tagstr = m;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (d, tagstr, taghsh, &plyr_0)) {
		ok = addplayer (d, tagstr, &plyr_0) && name_register(taghsh,plyr_0,plyr_0);
	}

	tagstr = s;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (d, tagstr, taghsh, &plyr_i)) {
		ok = addplayer (d, tagstr, &plyr_i) && name_register(taghsh,plyr_i,plyr_0);
	}

	return ok;
}

static bool_t
csvline_syn_apply (csv_line_t *pcsvln, struct DATA *d /*, bool_t quiet*/)
{
	bool_t ok = TRUE;
	int i;
	for (i = 1; ok && i < pcsvln->n; i++) {
		// if (!quiet) printf ("[%s] synonym of [%s]\n",pcsvln->s[0],pcsvln->s[i]);
		ok = ok && syn_apply_pair (d, pcsvln->s[0], pcsvln->s[i]);
	}
	return ok;
}

static void
syn_preload (bool_t quietmode, const char *fsyns_name, struct DATA *d)
{
	FILE *fsyns;
	char myline[MAXSIZE_CSVLINE];
	char *p;
	bool_t success;
	bool_t line_success = TRUE;
	bool_t file_success = TRUE;

	assert(NULL != fsyns_name);

	if (NULL == fsyns_name) {
		fprintf (stderr, "Error, synonym file not provided, absent, or corrupted\n");
		exit(EXIT_FAILURE);
	}

	if (NULL != (fsyns = fopen (fsyns_name, "r"))) {

		csv_line_t csvln;
		line_success = TRUE;
		file_success = TRUE;

		while ( line_success && 
				file_success && 
				NULL != fgets(myline, MAXSIZE_CSVLINE, fsyns)) {

			success = FALSE;
			p = myline;
			p = skipblanks(p);

			if (*p == '\0') continue;

			if (csv_line_init(&csvln, myline)) {
				if (csvln.n >= 1) {
					success = csvline_syn_apply (&csvln, d /*, quietmode*/);
				}
				csv_line_done(&csvln);		
			} else {
				fprintf (stderr,"Failure to input synonym file\n");
				exit(EXIT_FAILURE);
			}
			file_success = success;
		}

		fclose(fsyns);
	}
	else {
		file_success = FALSE;
	}

	if (!file_success) {
			fprintf (stderr, "Errors in file \"%s\"\n",fsyns_name);
			exit(EXIT_FAILURE);
	} else {
		if (!quietmode)
			printf ("Synonyms uploaded succesfully\n");
	}
	if (!line_success) {
			fprintf (stderr, "Errors in file \"%s\" (not matching names)\n",fsyns_name);
			exit(EXIT_FAILURE);
	}
	return;
}


//---- for name preload

static bool_t
name2player (const struct DATA *d, const char *namestr, player_t *plyr)
{
	player_t p = 0; // to silence warning
	uint32_t hsh = namehash(namestr);
	if (name_ispresent (d, namestr, hsh, &p)) {
		*plyr = p;
		return TRUE;
	}
	return FALSE;
}

static bool_t
do_tick (const struct DATA *d, const char *namestr, bitarray_t *pba) 
{
	player_t p = 0; // to silence warnings
	bool_t ok = name2player (d, namestr, &p);
	if (ok)	ba_put (pba, p);
	return ok;
}

static void
warning(bool_t do_warning, const char *filename, const char *myline, size_t linenumber)
{
	if (!do_warning) return;
	fprintf(stderr, "WARNING (not matching name): file \"%s\", line %ld = %s\n", filename, (long)linenumber, myline);
}

static bool_t
isblankline(const char *s)
{
	while (isspace(*s)) s++; 
	return *s == '\0';
}

void
namelist_to_bitarray (bool_t quietmode, bool_t do_warning, const char *finp_name, const struct DATA *d, bitarray_t *pba)
{
	FILE *finp;
	char myline[MAXSIZE_CSVLINE];
	size_t linenumber = 0;
	bool_t line_success = TRUE;
	bool_t file_success = TRUE;

	assert (pba);
	assert (d);

	ba_clear (pba);

	if (NULL == finp_name) {
		return;
	}

	if (NULL != (finp = fopen (finp_name, "r"))) {

		csv_line_t csvln;
		line_success = TRUE;

		while ( line_success && NULL != fgets(myline, MAXSIZE_CSVLINE, finp)) {

			linenumber++;
			if (isblankline(myline)) continue;

			if (TRUE == (line_success = csv_line_init(&csvln, myline))) {
				if (!(csvln.n == 1 && do_tick (d, csvln.s[0], pba))) {
					warning(do_warning, finp_name, myline, linenumber);
				}
				csv_line_done(&csvln);		
			}
		}

		fclose(finp);
	} else {
		file_success = FALSE;
	}

	if (!file_success) {
		fprintf (stderr, "Errors in file \"%s\"\n",finp_name);
		exit(EXIT_FAILURE);
	} else 
	if (!line_success) {
		fprintf (stderr, "Errors in file \"%s\", line %ld (line parsing problem or lack of memory)\n",finp_name, (long)linenumber);
		exit(EXIT_FAILURE);
	} 
	if (!quietmode)	printf ("Names uploaded succesfully\n");

	return;
}

//--------------------------------------

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
	#define NOPLAYER -1
	player_t 	i;
	player_t 	j;
	bool_t 		ok = TRUE;
	player_t 	plyr = NOPLAYER; // to silence warnings
	const char *tagstr;
	uint32_t 	taghsh;

	tagstr = p->wtag;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (d, tagstr, taghsh, &plyr)) {
		ok = addplayer (d, tagstr, &plyr) && name_register(taghsh,plyr,plyr);
	}
	i = plyr;

	tagstr = p->btag;
	taghsh = namehash(tagstr);

	if (ok && !name_ispresent (d, tagstr, taghsh, &plyr)) {
		ok = addplayer (d, tagstr, &plyr) && name_register(taghsh,plyr,plyr);
	}
	j = plyr;

	ok = ok && (uint64_t)d->n_games < ((uint64_t)MAXGAMESxBLOCK*(uint64_t)MAXBLOCKS);

	assert (i != NOPLAYER && j != NOPLAYER);

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
			if (NULL == (g = memnew (sizeof(struct GAMEBLOCK) * (size_t)1))) {
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
fpgnscan (FILE *fpgn, bool_t quiet, struct DATA *d)
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
		printf("Loading data (%d games x dot): \n\n",games_x_dot); fflush(stdout);

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
			if (!pgn_result_collect (&result, d)) {
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
		exit(EXIT_FAILURE);
		return DISCARD;
	}
}

/************************************************************************/



