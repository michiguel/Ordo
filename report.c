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

#include <stddef.h>
#include <assert.h>

#include "mytypes.h"
#include "report.h"
#include "encount.h"
#include "cegt.h"
#include "string.h"
#include "gauss.h"
#include "math.h"
#include "ordolim.h"
#include "xpect.h"
#include "mymem.h"

#include "mytimer.h"

//=== duplicated in main.c =========

static ptrdiff_t
head2head_idx_sdev (ptrdiff_t x, ptrdiff_t y)
{	
	ptrdiff_t idx;
	if (y < x) 
		idx = (x*x-x)/2+y;					
	else
		idx = (y*y-y)/2+x;
	return idx;
}

//==================================

static int
compare__ (const player_t *a, const player_t *b, const double *reference )
{	
	const player_t *ja = a;
	const player_t *jb = b;
	const double *r = reference;

	const double da = r[*ja];
	const double db = r[*jb];

	return (da < db) - (da > db);
}

static void
insertion_sort (const double *reference, size_t n, player_t *vect)
{
	size_t i, j, pivot;
	player_t tmp;
	if (n < 2) return;
	for (j = n-1; j > 0; j--) {
		pivot = j - 1;
		for (i = j; i < n; i++) {
			if (!(0 < compare__(&vect[pivot], &vect[i], reference))) {
				break;
			}
		}
		if (i > j) {
			size_t k;
			tmp = vect[j-1];
			for (k = j; k < i; k++) {
				vect[k-1] = vect[k];
			}
			vect[i-1] = tmp;
		}
	}
}

//==== my qsort 

static size_t
partition (const double *reference, player_t *vect, size_t l, size_t h)
{
	size_t p;
	size_t i = l + 1;
	player_t tmp;
	player_t pivot_content = vect[l];

	while (i < h) {
		if (0 < compare__(&pivot_content, &vect[i], reference)) {
			i++;
		} else {
			tmp = vect[i]; vect[i] = vect[h]; vect[h] = tmp; // swap i, h
			h--;
		}
	}

	p = 0 < compare__(&pivot_content, &vect[i], reference)? i: i - 1;
	
	if (p > l) 	{
			tmp = vect[p]; vect[p] = vect[l]; vect[l] = tmp; // swap p, l
	}
	return p;
}

static void
range_qsort (const double *reference, player_t *vect, size_t l, size_t h)
{

	size_t m;
	assert (h >= l);

	if (h < l + 6) {
		// could be here:	insertion_sort (reference, h - l + 1, vect + l);
		// left unsorted and run a full insertion sort at the end.
		return;
	}

	// choose pivot and place it in --> l
	if (h <= l + 12) {
		player_t tmp;
		size_t j = l + (h-l)/2;

		if (0 < compare__(&vect[h], &vect[j], reference) && 0 < compare__(&vect[j], &vect[l], reference)) {
			tmp = vect[j]; vect[j] = vect[l]; vect[l] = tmp; // swap
		}
		if (0 < compare__(&vect[j], &vect[h], reference) && 0 < compare__(&vect[h], &vect[l], reference)) {
			tmp = vect[h]; vect[h] = vect[l]; vect[l] = tmp; // swap
		}

	}

	m = partition (reference, vect, l, h);

	if (m-l < h-m) { // do smallest range first, prevents stack problems
		if (m > l+1) range_qsort(reference, vect, l, m-1);
		if (h > m+1) range_qsort(reference, vect, m+1, h);
	} else {
		if (h > m+1) range_qsort(reference, vect, m+1, h);
		if (m > l+1) range_qsort(reference, vect, l, m-1);
	}
}

static void
my_qsort (const double *reference, size_t n, player_t *vect)
{
	range_qsort(reference, vect, 0, n-1);
	insertion_sort (reference, n, vect); // only if it was not sorted inside range_qsort
}

//==== end my qsort


#define MAXSYMBOLS_STR 5
static const char *SP_symbolstr[MAXSYMBOLS_STR] = {"<",">","*"," ","X"};

static const char *
get_super_player_symbolstr(player_t j, const struct PLAYERS *pPlayers)
{
	assert(pPlayers->perf_set);
	if (pPlayers->performance_type[j] == PERF_SUPERLOSER) {
		return SP_symbolstr[0];
	} else if (pPlayers->performance_type[j] == PERF_SUPERWINNER) {
		return SP_symbolstr[1];
	} else if (pPlayers->performance_type[j] == PERF_NOGAMES) {
		return SP_symbolstr[2];
	} else if (pPlayers->performance_type[j] == PERF_NORMAL) {
		return SP_symbolstr[3];
	} else
		return SP_symbolstr[4];
}

static bool_t
is_old_version(player_t j, const struct rel_prior_set *rps)
{
	player_t i;
	bool_t found;
	player_t rn = rps->n;
	const struct relprior *rx = rps->x;
	for (i = 0, found = FALSE; !found && i < rn; i++) {
		found = j == rx[i].player_b;
	}
	return found;
}

static double
rating_round(double x, int d)
{
	const int al[6] = {1,10,100,1000,10000,100000};
	int i;
	double y;
	if (d > 5) d = 5;
	if (d < 0) d = 0;
	y = x * al[d] + 0.5; 
	i = (int) floor(y);
	return (double)i/al[d];
}

#define NOSDEV "  ----"

static char *
get_sdev_str (double sdev, double confidence_factor, char *str, int decimals)
{
	double x = sdev * confidence_factor;
	if (sdev > 0.00000001) {
		sprintf(str, "%.*f", decimals, rating_round(x, decimals));
	} else {
		sprintf(str, "%s", NOSDEV);
	}
	return str;
}

#define NOSDEV_csv "\"-\""

static char *
get_sdev_str_csv (double sdev, double confidence_factor, char *str, int decimals)
{
	double x = sdev * confidence_factor;
	if (sdev > 0.00000001) {
		sprintf(str, "%.*f", decimals, rating_round(x, decimals));
	} else {
		sprintf(str, "%s", NOSDEV_csv);
	}
	return str;
}

static bool_t
ok_to_out (player_t j, const struct output_qualifiers outqual, const struct PLAYERS *p, const struct RATINGS *r)
{
	gamesnum_t games = r->playedby_results[j];
	bool_t ok = !p->flagged[j]
				&& games > 0
				&& (!outqual.mingames_set || games >= outqual.mingames);
	return ok;
} 

//======================

void 
cegt_output	( bool_t quiet
			, const struct GAMES 	*g
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, struct ENCOUNTERS 	*e  // memory just provided for local calculations
			, double 				*sdev
			, long 					simulate
			, double				confidence_factor
			, const struct GAMESTATS *pgame_stats
			, const struct DEVIATION_ACC *s
			, struct output_qualifiers outqual
			, int decimals)
{
	struct CEGT cegt;
	player_t j;

	assert (g);
	assert (p);
	assert (r);
	assert (e);
	assert (pgame_stats);

	encounters_calculate(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);
	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);
	for (j = 0; j < p->n; j++) {
		r->sorted[j] = j;
	}

	my_qsort(r->ratingof_results, (size_t)p->n, r->sorted);

	cegt.n_enc = e->n; 
	cegt.enc = e->enc;
	cegt.simulate = simulate;
	cegt.n_players = p->n;
	cegt.sorted = r->sorted;
	cegt.ratingof_results = r->ratingof_results;
	cegt.obtained_results = r->obtained_results;
	cegt.playedby_results = r->playedby_results;
	cegt.sdev = sdev; 
	cegt.flagged = p->flagged;
	cegt.name = p->name;
	cegt.confidence_factor = confidence_factor;

	cegt.gstat = pgame_stats;

	cegt.sim = s;

	cegt.outqual = outqual;

	cegt.decimals = decimals;

	output_cegt_style (quiet, "general.dat", "rating.dat", "programs.dat", &cegt);
}

static const char *Header_INI[MAX_prnt] = {
	"PLAYER",
	"RATING",
	"ERROR", 
	"POINTS",
	"PLAYED",
	"(%)",
	"CFS(%)",
	"W",
	"D",
	"L",
	"D(%)",
	"OppAvg",
	"OppErr",
	"OppN",
	"OppDiv"
};

static int Shift_INI[MAX_prnt] = {0, 6, 6, 7, 7, 5, 7, 4, 4, 4, 5, 7, 7, 5, 7 };

static char *Header[MAX_prnt];
static int Shift[MAX_prnt];


static char *string_dup (const char *s);
static void string_free (char *s);

void
report_columns_unset (int i)
{
	if (i < MAX_prnt && Header[i] != NULL) {
		string_free(Header[i]);
		Header[i] = NULL;
	}
}

bool_t
report_columns_set (int i, int shft, const char *hdr)
{
	char *h;
	bool_t ok = i < MAX_prnt && NULL != (h = string_dup(hdr));
	if (ok) {
		report_columns_unset (i);	
		Header[i] = h;		
		Shift[i] = shft;	
	}
	return ok;
}

bool_t
report_columns_set_str (const char *i_str, const char *s_str, const char *hdr)
{
	int int1 = 0, int2 = 0;
	bool_t ok = TRUE;
	size_t min_len = 1 + strlen(hdr);
	ok = ok && 1 == sscanf (i_str, "%d", &int1);
	ok = ok && 1 == sscanf (s_str, "%d", &int2);
	ok = ok && report_columns_set (int1, ((size_t)int2 < min_len? (int)min_len: int2), hdr);
	return ok;
}


bool_t
report_columns_init (void)
{
	bool_t ok = TRUE;
	int i;
	for (i = 0; ok && i < MAX_prnt; i++) {
		Header[i] = NULL;
	}
	for (i = 0; ok && i < MAX_prnt; i++) {
		ok = ok && report_columns_set (i, Shift_INI[i], Header_INI[i]);
	}
	if (!ok) {
		while (i-->0) report_columns_unset (i);
	}
	return ok;
}

void
report_columns_done (void)
{
	int i;
	for (i = 0; i < MAX_prnt; i++) {
		report_columns_unset (i);
	}
}

// Function provided to have all head to head information

void 
head2head_output( const struct GAMES 	*		g
				, const struct PLAYERS 	*		p
				, const struct RATINGS 	*		r
				, struct ENCOUNTERS 	*		e  // memory just provided for local calculations
				, double 				*		sdev
				, long 							simulate
				, double						confidence_factor
				, const struct GAMESTATS *		pgame_stats
				, const struct DEVIATION_ACC *	s
				, const char *					head2head_str
				, int 							decimals)
{
	struct CEGT cegt;
	player_t j;

	assert (g);
	assert (p);
	assert (r);
	assert (e);
	assert (pgame_stats);
	//assert (s); // maybe NULL

	encounters_calculate(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);
	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);
	for (j = 0; j < p->n; j++) {
		r->sorted[j] = j; 
	}

	my_qsort(r->ratingof_results, (size_t)p->n, r->sorted);

	cegt.n_enc = e->n;
	cegt.enc = e->enc;
	cegt.simulate = simulate;
	cegt.n_players = p->n;
	cegt.sorted = r->sorted;
	cegt.ratingof_results = r->ratingof_results;
	cegt.obtained_results = r->obtained_results;
	cegt.playedby_results = r->playedby_results;
	cegt.sdev = sdev; 
	cegt.flagged = p->flagged;
	cegt.name = p->name;
	cegt.confidence_factor = confidence_factor;

	cegt.gstat = pgame_stats;

	cegt.sim = s;

	cegt.decimals = decimals;
	output_report_individual (head2head_str, &cegt, (int)simulate);
}

#ifndef NDEBUG
static bool_t 
is_empty_player(player_t j, const struct PLAYERS *pPlayers)
{
	assert(pPlayers->perf_set);
	return pPlayers->performance_type[j] == PERF_NOGAMES
	;		
}
#endif

static double
get_cfs (const struct DEVIATION_ACC *sim, double dr, player_t target, player_t oth)
{
	double ret;
	if (sim != NULL) {
		ptrdiff_t idx;
		double ctrs;
		double sd;
		idx = head2head_idx_sdev ((ptrdiff_t)target, (ptrdiff_t)oth);
		sd = sim[idx].sdev;
		ctrs = 100*gauss_integral(dr/sd);
		ret = ctrs;
	}
	else {
		ret = 0;
	}
	return ret;
}

//=====================

static char *
get_rank_str (int rank, char *buffer)
{
	sprintf(buffer,"%d",rank);
	return buffer;
}

static char *
quoted_str (const char *s, char *buffer)
{
	sprintf(buffer, "\"%s\"", s);
	return buffer;
}

#define N_EXTRA 10000

struct outextra {
	player_t	j;
	bool_t 		rnk_is_ok;
	int			rnk_value;
	bool_t		cfs_is_ok;
	double 		cfs_value;
};

struct REPORT_INFO {
	int 					decimals;
	int						decimals_score;
	const struct PLAYERS *	p;
	const struct RATINGS *	r;
	size_t 					ml;
	const struct OUT_INFO *	oi;
	double *				sdev;
	struct outextra *		pq;
	double 					confidence_factor;
};


#include <ctype.h>

static size_t
stripped_len(const char *s)
{
	const char *p, *q;
	p = s;
	q = s + strlen(s);
	while (isspace(*p)) p++;
	if (p == q) return 0;
	q--;
	while (isspace(*q)) q--;
	return (size_t) (q - p + 1);
}

static void
prnt_singleitem_
			( int item
			, size_t x
			, player_t j
			, const struct REPORT_INFO *p_rprt_info
			, FILE *f
			, size_t *plen
)
{
	int decimals, decperc;
	const struct PLAYERS 	*p;
	const struct RATINGS 	*r;
	size_t ml;
	const struct OUT_INFO *oi;

	double *sdev;
	struct outextra *pq;
	double confidence_factor;

	char s[1024];
	int sh;
	const char *cfs_str, *sdev_str, *rank_str, *oerr_str;
	char cfs_buf[80], sdev_buf[80], rank_buf[80], oerr_buf[80];

	assert(item < MAX_prnt);

	decimals 			= p_rprt_info->decimals;
	decperc				= p_rprt_info->decimals_score; 
	p 					= p_rprt_info->p;
	r 					= p_rprt_info->r;
	ml 					= p_rprt_info->ml;
	oi 	 				= p_rprt_info->oi;
	sdev 				= p_rprt_info->sdev;
	pq 					= p_rprt_info->pq;
	confidence_factor	= p_rprt_info->confidence_factor;

	sprintf(cfs_buf, "%3.0lf", pq[x].cfs_value);
	cfs_str  = pq[x].cfs_is_ok? cfs_buf : "---";
	sdev_str = sdev? get_sdev_str (sdev[j]     , confidence_factor, sdev_buf, decimals): NOSDEV;
	oerr_str = sdev? get_sdev_str (oi[j].opperr, confidence_factor, oerr_buf, decimals): NOSDEV;
	rank_str = pq[x].rnk_is_ok? get_rank_str (pq[x].rnk_value, rank_buf): "";

	sh = Shift[item];
	switch (item) {
		case  0: sprintf(s, "%*s %-*s %s :", 4,	rank_str, (int)ml+1,	p->name[j],	get_super_player_symbolstr(j,p) ); break;
		case  1: sprintf(s, " %*.*f"	, sh  , decimals, rating_round (r->ratingof_results[j], decimals) ); break;
		case  2: sprintf(s, " %*s"		, sh  , sdev_str ); break;
		case  3: sprintf(s, " %*.1f"	, sh  , r->obtained_results[j] ); break;
		case  4: sprintf(s, " %*ld" 	, sh  , (long)r->playedby_results[j] ); break;
		case  5: sprintf(s, " %*.*f"	, sh  , decperc, r->playedby_results[j]==0? 0: 100.0*r->obtained_results[j]/(double)r->playedby_results[j]); break;
		case  6: sprintf(s, " %*s" 		, sh  , cfs_str); break;
		case  7: sprintf(s, " %*ld"		, sh  , (long)oi[j].W ); break;
		case  8: sprintf(s, " %*ld"		, sh  , (long)oi[j].D ); break;
		case  9: sprintf(s, " %*ld"		, sh  , (long)oi[j].L ); break;
		case 10: sprintf(s, " %*.*f"	, sh  , decperc, 0==oi[j].D?0.0:(100.0*(double)oi[j].D/(double)(oi[j].W+oi[j].D+oi[j].L)) ); break;
		case 11: sprintf(s, " %*.*f"	, sh  , decimals, oi[j].opprat);  break;
		case 12: sprintf(s, " %*s"		, sh  , oerr_str ); break;
		case 13: sprintf(s, " %*ld"		, sh  , (long)oi[j].n_opp ); break;
		case 14: sprintf(s, " %*.*f"	, sh  , 1, oi[j].diversity);  break;
		default:  break;
	}

	if (f) fprintf(f,"%s",s);
	if (plen) *plen = stripped_len(s);
}

static void
prnt_itemlist_
			( int *list
			, size_t x
			, player_t j
			, const struct REPORT_INFO *p_rprt_info
			, FILE *f
)
{
	int item, i;
	for (i = 0; list[i] != -1; i++) {
		item = list[i];
		prnt_singleitem_( item, x, j, p_rprt_info, f, NULL);
	}
}


static void
prnt_header_single (int item, FILE *f, size_t ml)
{
	int sh;
	assert(item < MAX_prnt);
	sh = Shift[item];
	if (item >= MAX_prnt)
		return;
	if (item == 0) {
		fprintf(f, "\n%*s %-*s    :",  4, "#", (int)ml, Header[item]);
	} else {
		fprintf(f, " %*s", sh, Header[item]);
	}
}

static void
prnt_header (FILE *f, size_t ml, int *list)
{
	int item;
	for (item = 0; list[item] != -1; item++)
		prnt_header_single (list[item], f, ml);
}

static void
prnt_header_single_csv (int item, FILE *f)
{
	char buffer[80];
	assert(item < MAX_prnt);
	if (item >= MAX_prnt)
		return;
	if (item == 0) {
		fprintf(f,  "%s", quoted_str(          "#",buffer)); 
		fprintf(f, ",%s", quoted_str( Header[item],buffer));
	} else {
		fprintf(f, ",%s", quoted_str( Header[item],buffer));
	}
}

static void
prnt_header_csv (FILE *f, int *list)
{
	int item;
	for (item = 0; list[item] != -1; item++)
		prnt_header_single_csv (list[item], f);
}

static void
prnt_singleitem_csv
			( int item
			, FILE *f
			, int decimals
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, player_t j
			, int rank
			, const char *sdev_str
			, const char *oerr_str
			, const char *cfs_str
			, const struct OUT_INFO *oi
)
{
	assert(item < MAX_prnt);
	switch (item) {
		case 0:	
				fprintf(f, "%d,"	,rank);
				fprintf(f, "\"%s\""	,p->name[j]);
 				break;
		case 1:	fprintf(f, ",%.*f"	,decimals, r->ratingof_results[j]);  break;
		case 2:	fprintf(f, ",%s"	,sdev_str); break;
		case 3:	fprintf(f, ",%.2f"	,r->obtained_results[j]); break;
		case 4:	fprintf(f, ",%ld"	,(long)r->playedby_results[j]); break;
		case 5:	fprintf(f, ",%.2f"	,r->playedby_results[j]==0?0:100.0*r->obtained_results[j]/(double)r->playedby_results[j]); break;
		case 6:	fprintf(f, ",%s"	,cfs_str); break;
		case 7:	fprintf(f, ",%ld"	,(long)oi[j].W ); break;
		case 8:	fprintf(f, ",%ld"	,(long)oi[j].D ); break;
		case 9:	fprintf(f, ",%ld"	,(long)oi[j].L ); break;
		case 10:fprintf(f, ",%.1f"	,0==oi[j].D?0.0:(100.0*(double)oi[j].D/(double)(oi[j].W+oi[j].D+oi[j].L)) ); break;
		case 11:fprintf(f, ",%.*f"	,decimals, oi[j].opprat);  break;
		case 12:fprintf(f, ",%s"	,oerr_str); break;
		case 13:fprintf(f, ",%ld"	,(long)oi[j].n_opp);  break;
		case 14:fprintf(f, ",%.*f"	,1, oi[j].diversity);  break;
		default:  break;
	}
}

static void
prnt_itemlist_csv
			( FILE *f
			, int decimals
			, const struct PLAYERS 	*p
			, const struct RATINGS 	*r
			, player_t j
			, int rank
			, double *sdev
			, int *list
			, struct outextra *pq
			, size_t x
			, double confidence_factor
			, const struct OUT_INFO *oi
)
{
	int item;
	const char *cfs_str, *sdev_str, *oerr_str;
	char cfs_buf[80], sdev_buf[80], oerr_buf[80];

	sprintf(cfs_buf, "%.0lf", pq[x].cfs_value);
	cfs_str  = pq[x].cfs_is_ok? cfs_buf : "\"-\"";

	sdev_str = sdev? get_sdev_str_csv (sdev[j]     , confidence_factor, sdev_buf, decimals): NOSDEV_csv;
	oerr_str = sdev? get_sdev_str_csv (oi[j].opperr, confidence_factor, oerr_buf, decimals): NOSDEV_csv;

	for (item = 0; list[item] != -1; item++)
		prnt_singleitem_csv (list[item], f, decimals, p, r, j, rank, sdev_str, oerr_str, cfs_str, oi);
}

static void
list_remove_item (int *list, int x);

static size_t
listlen(int *x)
{
	int *ori = x;
	while (*x != -1) x++;
	return (size_t)(x - ori);
}

static void
listcopy (const int *s, int *t)
{
	while (*s != -1) *t++ = *s++;
	*t = -1;
}

static void
addabsent (int *list_chosen, int a)
{
	int z;
	for (z = 0; list_chosen[z] != -1 && list_chosen[z] != a; z++) {
		;
	}
	if (list_chosen[z] == -1) {list_chosen[z] = a; list_chosen[z+1] = -1;}
}

/* 
|
|	Major report function 
|
\--------------------------------*/

static void fatal_mem(const char *s)
{
	fprintf(stderr, "%s\n",s);
	exit (EXIT_FAILURE);
}

static size_t
find_maxlen (const char *nm[], struct outextra *q, size_t x_max)
{
	size_t maxl = 0;
	size_t length;
	size_t x;
	player_t j;
	for (x = 0; x < x_max; x++) {
		j = q[x].j;
		length = strlen(nm[j]);
		if (length > maxl) maxl = length;	
	}
	return maxl;
}


void
all_report 	( const struct GAMES 			*g
			, const struct PLAYERS 			*p
			, const struct RATINGS 			*r
			, const struct rel_prior_set 	*rps
			, struct ENCOUNTERS 			*e  // memory just provided for local calculations
			, double 						*sdev
			, long 							simulate
			, bool_t						hide_old_ver
			, double						confidence_factor
			, FILE 							*csvf
			, FILE 							*textf
			, double 						white_advantage
			, double 						drawrate_evenmatch
			, int							decimals
			, int							decimals_score
			, struct output_qualifiers		outqual
			, double						wa_sdev				
			, double						dr_sdev
			, const struct DEVIATION_ACC *	s
			, bool_t 						csf_column
			, int							*inp_list
			)
{
	struct outextra *q;
	struct OUT_INFO *out_info;
	struct REPORT_INFO rprt_info;

	int *list_chosen = NULL;
	int *listbuff = NULL;

	size_t	x = 0;
	size_t  x_max = 0;

	FILE *f;
	player_t i;
	player_t j;
	size_t ml;
	int rank = 0;
	bool_t showrank = TRUE;

	player_t sorted_n = 0;

	assert (g);
	assert (p);
	assert (r);
	assert (e);
	assert (rps);

	if (NULL == (out_info = memnew (sizeof(struct OUT_INFO) * (size_t)p->n))) {
		fatal_mem("Not enough memory for reporting results");
	}
	if (NULL == (q = memnew (((size_t)p->n + 1) * sizeof(struct outextra)))) {
		memrel(out_info);
		fatal_mem("Not enough memory for creation of internal buffers for reporting results");
	}
	if (NULL == (listbuff = memnew ( sizeof (inp_list[0]) * (listlen (inp_list) + 2)))) {
		memrel(out_info);
		memrel(q);
		fatal_mem("Not enough memory for list creation");
	}
	listcopy (inp_list, listbuff);

	encounters_calculate(ENCOUNTERS_NOFLAGGED, g, p->flagged, e);

	calc_obtained_playedby(e->enc, e->n, p->n, r->obtained, r->playedby);



	// build a list of players that will actually be printed
	for (sorted_n = 0, j = 0; j < p->n; j++) {
		if (ok_to_out (j, outqual, p, r)) {
			r->sorted[sorted_n++] = j;
		}
	}

	timelog("calculate output info...");

	calc_output_info( e->enc, e->n, r->ratingof_results, p->n, sdev, out_info, r->sorted, sorted_n);

	timelog_ld("sort... N=",(long)sorted_n);

	my_qsort(r->ratingof_results, (size_t)sorted_n, r->sorted);

	list_chosen = listbuff;
	if (simulate < 2) 
		list_remove_item (list_chosen, 2);

	if (csf_column) { // force 6 in list_chosen
		addabsent (list_chosen, 6);
	}

	timelog("calculation for printing...");

	/* calculation for printing */
	for (i = 0; i < sorted_n; i++) {
		j = r->sorted[i]; 

		assert(r->playedby_results[j] != 0 || is_empty_player(j,p));

		if (ok_to_out (j, outqual, p, r)) {
			showrank = !is_old_version(j, rps);
			if (showrank) {
				rank++;
			} 

			if (showrank || !hide_old_ver) {

				if (x > 0 && s && simulate > 1) 
				{		
					player_t prev_j = q[x-1].j;	
					double delta_rating = r->ratingof_results[prev_j] - r->ratingof_results[j];
					q[x-1].cfs_value = get_cfs(s, delta_rating, prev_j, j);
					q[x-1].cfs_is_ok = TRUE;
				}

				q[x].j = j;
				q[x].rnk_value = rank;
				q[x].rnk_is_ok = showrank;
				q[x].cfs_value = 0; 
				q[x].cfs_is_ok = FALSE;
				x++;
			}
		}
	}
	x_max = x;

	/* 
	|
	|	output in text format 
	|
	\--------------------------------*/

	f = textf;
	if (f != NULL) {

		ml = find_maxlen (p->name, q, x_max);
		if (ml > 50) ml = 50;
		if (ml < strlen(Header[0])) ml = strlen(Header[0]);

		rprt_info.decimals			= decimals;
		rprt_info.decimals_score	= decimals_score;
		rprt_info.p					= p;
		rprt_info.r					= r;
		rprt_info.ml				= ml;
		rprt_info.oi				= out_info;
		rprt_info.sdev				= sdev;
		rprt_info.pq				= q;
		rprt_info.confidence_factor = confidence_factor;

		// find max length for each column
		{
			size_t width, width_max, top;
			int item, z;
			for (z = 0; list_chosen[z] != -1; z++) {
				item = list_chosen[z];
				width = 0;
				width_max = stripped_len(Header[item]);
				for (x = 0; x < x_max; x++) {
					j = q[x].j;
					prnt_singleitem_( item, x, j, &rprt_info, NULL, &width);
					if (width > width_max) width_max = width;
				}
				top = width_max + 1;
				if (Shift[item] < (int)top) Shift[item] = (int)top;
			}
		}

		/* actual printing */
		prnt_header (f, ml, list_chosen);
		fprintf(f, "\n");

		for (x = 0; x < x_max; x++) {
			j = q[x].j;
			prnt_itemlist_ (list_chosen, x, j, &rprt_info, f);
			fprintf (f, "\n");
		}

		if (simulate < 2) {
			fprintf (f,"\n");
			fprintf (f,"White advantage = %.2f\n", white_advantage);
			fprintf (f,"Draw rate (equal opponents) = %.2f %s\n",100*drawrate_evenmatch, "%");
			fprintf (f,"\n");
		} else {
			fprintf (f,"\n");
			fprintf (f,"White advantage = %.2f +/- %.2f\n",white_advantage, wa_sdev);
			fprintf (f,"Draw rate (equal opponents) = %.2f %s +/- %.2f\n",100*drawrate_evenmatch, "%", 100*dr_sdev);
			fprintf (f,"\n");
		}

	} /*if*/

	/* 
	|
	|	output in a comma separated value file 
	|
	\-------------------------------------------*/

	f = csvf;

	if (f != NULL) {

		listcopy (inp_list, listbuff);
		list_chosen = listbuff;
	
		if (csf_column) { // force 6 in list_chosen
			addabsent (list_chosen, 6);
		}

		prnt_header_csv (f, list_chosen);
		fprintf(f, "\n");

		/* actual printing */
		for (x = 0; x < x_max; x++) {
			int rank_int;
			j = q[x].j;
			rank_int = q[x].rnk_value;
			prnt_itemlist_csv (f, decimals, p, r, j, rank_int, sdev, list_chosen, q, x, confidence_factor, out_info);
			fprintf (f, "\n");
		}
	}

	memrel (out_info);
	memrel (q);
	memrel (listbuff);

	timelog("done with all reports.");

	return;
}

static char *
string_dup (const char *s)
{
	char *p;
	char *q;
	size_t len = strlen(s);
	p = memnew (len + 1);
	if (p == NULL) return NULL;
	q = p;
	while (*s) *p++ = *s++;
	*p = '\0';
	return q;
}

static void
string_free (char *s)
{
	memrel(s);
}

bool_t
str2list (const char *inp_str, int max, int *n, int *t)
{
	bool_t end_reached = FALSE;
	int value;
	int counter = 0;
	char *s;
	char *p;
	char *d;
	bool_t ok = TRUE;

	if (NULL != (d = string_dup(inp_str))) {

		p = d;
		s = d;

		for (end_reached = FALSE; !end_reached && ok;) {
			while (*p != ',' && *p != '\0') 
				p++;
			end_reached = *p == '\0';
			*p++ = '\0';
			ok = 1 == sscanf (s, "%d", &value) && counter < max;
			if (ok) {*t++ = value; counter++;}
			s = p;
		}

		string_free(d);
	}

	if (ok)	*t = -1;
	*n = counter;
	return ok;
}

static void
list_remove_item (int *list, int x)
{
	int *t;
	int *p = list;
	while (*p != -1 && *p != x) {p++;}
	while (*p != -1) {t = p++; *t = *p;}

}

void
errorsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out, double confidence_factor)
{
	FILE *f;
	ptrdiff_t idx;
	player_t y,x;
	player_t i;
	player_t j;

	assert (p);
	assert (r);
	assert (s);

	if (NULL != (f = fopen (out, "w"))) {
		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < p->n; i++) {
			fprintf(f, ",%ld", (long) i);		
		}
		fprintf(f, "\n");	
		for (i = 0; i < p->n; i++) {
			y = r->sorted[i];
			fprintf(f, "%ld,\"%21s\"", (long) i, p->name[y]);
			for (j = 0; j < i; j++) {
				x = r->sorted[j];
				idx = head2head_idx_sdev ((ptrdiff_t)x, (ptrdiff_t)y);
				fprintf(f,",%.1f", s[idx].sdev * confidence_factor);
			}
			fprintf(f, "\n");
		}
		fclose(f);
	} else {
		fprintf(stderr, "Errors with file: %s\n",out);	
	}
	return;
}


void
ctsout(const struct PLAYERS *p, const struct RATINGS *r, const struct DEVIATION_ACC *s, const char *out)
{
	FILE *f;
	ptrdiff_t idx;
	player_t y;
	player_t x;
	player_t i,j;

	assert (p);
	assert (r);
	assert (s);

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < p->n; i++) {
			fprintf(f, ",%ld",(long) i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < p->n; i++) {
			y = r->sorted[i];
			fprintf(f, "%ld,\"%21s\"", (long) i, p->name[y]);

			for (j = 0; j < p->n; j++) {
				double ctrs, sd, dr;
				x = r->sorted[j];
				if (x != y) {
					dr = r->ratingof_results[y] - r->ratingof_results[x];
					idx = head2head_idx_sdev ((ptrdiff_t)x, (ptrdiff_t)y);
					sd = s[idx].sdev;
					ctrs = 100*gauss_integral(dr/sd);
					fprintf(f,",%.1f", ctrs);
				} else {
					fprintf(f,",");
				}
			}
			fprintf(f, "\n");
		}
		fclose(f);
	} else {
		fprintf(stderr, "Errors with file: %s\n",out);	
	}
	return;
}


void
look_at_individual_deviation 
			( player_t 			n_players
			, const bool_t *	flagged
			, struct RATINGS *	rat
			, struct ENC *		enc
			, gamesnum_t		n_enc
			, double			white_adv
			, double			beta)
{
	double accum = 0;
	double diff;
	player_t j;
	size_t allocsize = sizeof(double) * (size_t)(n_players+1);
	double *expected = memnew(allocsize);

	if (expected) {
		printf ("\nResidues\n\n");
		calc_expected(enc, n_enc, white_adv, n_players, rat->ratingof, expected, beta);
		for (accum = 0, j = 0; j < n_players; j++) {
			if (!flagged[j]) {
				diff = expected[j] - rat->obtained [j];
				accum += diff * diff / (double)rat->playedby[j];
				printf ("player[%lu] = %+lf\n", (long unsigned) j, diff);
			}
		}		
		memrel (expected);
	} else {
		fprintf(stderr, "Lack of memory to show individual deviations\n");
	}
	printf ("\nAverage residues = %lf\n", sqrt(accum));
	return;
}


void
look_at_predictions (gamesnum_t n_enc, const struct ENC *enc, const double *ratingof, double beta, double wadv, double dr0)
{
	gamesnum_t e;
	player_t w, b;
	double W,D,L;

	gamesnum_t white_wins = 0, black_wins = 0, total_draw = 0;
	double white_win_exp = 0;
	double white_dra_exp = 0;
	double white_bla_exp = 0;
	
	double pw, pd, pl;

	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		W = (double)enc[e].W;
		D = (double)enc[e].D;
		L = (double)enc[e].L;

		get_pWDL(ratingof[w] + wadv - ratingof[b], &pw, &pd, &pl, dr0, beta);

		white_wins += enc[e].W;
		total_draw += enc[e].D;
		black_wins += enc[e].L;

		white_win_exp += (W + D + L) * pw;
		white_dra_exp += (W + D + L) * pd;
		white_bla_exp += (W + D + L) * pl;
	}

	printf ("------------------------------------\n");
	printf ("Draw Rate = %lf\n", dr0);
	printf ("White Adv = %lf\n", wadv);
	printf ("\n");
	printf ("Observed\n");
	printf ("white wins = %ld\n", (long) white_wins);
	printf ("total draw = %ld\n", (long) total_draw);
	printf ("black wins = %ld\n", (long) black_wins);
	printf ("\n");
	printf ("Predicted\n");
	printf ("white wins = %.2lf\n", white_win_exp);
	printf ("total draw = %.2lf\n", white_dra_exp);
	printf ("black wins = %.2lf\n", white_bla_exp);
	printf ("------------------------------------\n");
	return;
}

//
#include <ctype.h>
#include "csv.h"

static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}

void
report_columns_load_settings (bool_t quietmode, const char *finp_name)
{
	FILE *finp;
	char myline[MAXSIZE_CSVLINE];
	char *p;
	bool_t line_success = TRUE;
	bool_t file_success = TRUE;

	if (NULL == finp_name) {
		return;
	}

	if (NULL != (finp = fopen (finp_name, "r"))) {

		csv_line_t csvln;
		line_success = TRUE;

		while ( line_success && NULL != fgets(myline, MAXSIZE_CSVLINE, finp)) {

			p = skipblanks(myline);
			if (*p == '\0') continue;

			if (csv_line_init(&csvln, myline)) {
				line_success = csvln.n == 3 && report_columns_set_str (csvln.s[0], csvln.s[1], csvln.s[2]);
				csv_line_done(&csvln);		
			} else {
				line_success = FALSE;
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
		fprintf (stderr, "Errors in file \"%s\"\n",finp_name);
		exit(EXIT_FAILURE);
	} 
	if (!quietmode)	printf ("Column settings uploaded succesfully\n");

	return;
}

