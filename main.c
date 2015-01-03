/*
	Ordo is program for calculating ratings of engine or chess players
    Copyright 2014 Miguel A. Ballicora

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

#if 1
	#define DOPRIOR
#endif

#define PRIOR_SMALLEST_SIGMA 0.0000001

/*
| 
|	ORDO
|
\-------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stddef.h>

#include "mystr.h"
#include "proginfo.h"
#include "boolean.h"
#include "pgnget.h"
#include "randfast.h"
#include "gauss.h"
#include "groups.h"
#include "mytypes.h"

#include "cegt.h"

#include "indiv.h"
#include "encount.h"
#include "rating.h"
#include "ratingb.h"

#include "xpect.h"
#include "csv.h"

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

#include "myopt.h"

const char *license_str = "\n"
"   Copyright (c) 2014 Miguel A. Ballicora\n"
"   Ordo is program for calculating ratings of engine or chess players\n"
"\n"
"   Ordo is free software: you can redistribute it and/or modify\n"
"   it under the terms of the GNU General Public License as published by\n"
"   the Free Software Foundation, either version 3 of the License, or\n"
"   (at your option) any later version.\n"
"\n"
"   Ordo is distributed in the hope that it will be useful,\n"
"   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
"   GNU General Public License for more details.\n"
"\n"
"   You should have received a copy of the GNU General Public License\n"
"   along with Ordo.  If not, see <http://www.gnu.org/licenses/>.\n"
;

static void parameter_error(void);
static void example (void);
static void usage (void);

/* VARIABLES */

	static bool_t QUIET_MODE;
	static bool_t SIM_UPDATES;
	static bool_t ADJUST_WHITE_ADVANTAGE;
	static bool_t ADJUST_DRAW_RATE;

	static const char *copyright_str = 
		"Copyright (c) 2014 Miguel A. Ballicora\n"
		"There is NO WARRANTY of any kind\n"
		;

	static const char *intro_str =
		"Program to calculate individual ratings\n"
		;

	const char *example_options = 
		"-a 2500 -p input.pgn -o output.txt";

	static const char *example_str =
		"  - Processes input.pgn (PGN file) to calculate ratings to output.txt.\n"
		"  - The general pool will have an average of 2500\n"
		;

	static const char *help_str =
		" -h          print this help\n"
		" -H          print just the switches\n"
		" -v          print version number and exit\n"
		" -L          display the license information\n"
		" -q          quiet mode (no screen progress updates)\n"
		" -Q          quiet mode (no screen progress except simulation count)\n"
		" -a <avg>    set rating for the pool average\n"
		" -A <player> anchor: rating given by '-a' is fixed for <player>, if provided\n"
		" -V          errors relative to pool average, not to the anchor\n"
		" -m <file>   multiple anchors: file contains rows of \"AnchorName\",AnchorRating\n"
		" -y <file>   loose anchors: file contains rows of \"Player\",Rating,Uncertainty\n"
		" -r <file>   relations: rows of \"PlayerA\",\"PlayerB\",delta_rating,uncertainty\n"
		" -R          remove older player versions (given by -r) from the output\n"
		" -w <value>  white advantage value (default=0.0)\n"
		" -u <value>  white advantage uncertainty value (default=0.0)\n"
		" -W          white advantage will be automatically adjusted\n"
		" -d <value>  draw rate value % (default=50.0)\n"
		" -k <value>  draw rate uncertainty value % (default=0.0 %)\n"
		" -D          draw rate value will be automatically adjusted\n"
		" -z <value>  scaling: set rating for winning expectancy of 76% (default=202)\n"
		" -T          display winning expectancy table\n"
		" -p <file>   input file in PGN format\n"
		" -c <file>   output file (comma separated value format)\n"
		" -o <file>   output file (text format), goes to the screen if not present\n"
		" -E          output in Elostat format (rating.dat, programs.dat & general.dat)\n"
		" -g <file>   output file with group connection info (no rating output on screen)\n"
		" -j <file>   output file with head to head information\n"
		" -s  #       perform # simulations to calculate errors\n"
		" -e <file>   saves an error matrix, if -s was used\n"
		" -C <file>   saves a matrix (.csv) with confidence for superiority (-s was used)\n"
		" -F <value>  confidence (%) to estimate error margins. Default is 95.0\n"
		" -X          Ignore draws\n"
		" -N <value>  Output, number of decimals, minimum is 0 (default=1)\n"
		"\n"
		;

	const char *usage_options = 
		"[-OPTION]";
		;
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		

const char *OPTION_LIST = "vhHp:qQWDLa:A:Vm:r:y:Ro:Eg:j:c:s:w:u:d:k:z:e:C:TF:XN:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/

struct GAMES {
	int32_t	 	n; //FIXME transform to size_t
	int32_t		size;
	int32_t 	*whiteplayer;
	int32_t 	*blackplayer;
	int32_t 	*score;
};

struct PLAYERS {
	int32_t		n; //FIXME transform to size_t
	int32_t		size;
	char 		**name;
	bool_t		*flagged;
	bool_t		*prefed;
	int			*performance_type; 
};

struct RATINGS {
//	int32_t		n;
	int32_t		size;
	int32_t		*sorted; 	/* sorted index by rating */
	double		*obtained;
	int32_t		*playedby; 	/* N games played by player "i" */
 	double		*ratingof; 	/* rating current */
 	double		*ratingbk; 	/* rating backup  */
 	double		*changing; 	/* rating backup  */
	double		*ratingof_results;
	double		*obtained_results;
	int32_t		*playedby_results;
};

struct ENCOUNTERS {
	int32_t		n;		//FIXME transform to size_t
	int32_t		size;
	struct ENC	*enc;
};

struct GAMES 		Games;
struct PLAYERS 		Players;
struct RATINGS 		RA;
struct ENCOUNTERS 	Encounters;

#if 1
#define NEWFUNC
#endif

#ifdef NEWFUNC
static bool_t 
ratings_init (int32_t n, struct RATINGS *r) 
{
	enum {MAXU=3, MAXD=6};
	int32_t 	*pu[MAXU];
	double		*pd[MAXD];
	bool_t		failed;
	int i,u,d;

	assert (n > 0);

	r->size = n;

	for (failed = FALSE, u = 0, i = 0; i < MAXU && !failed; i++) {
		if (NULL != (pu[i] = malloc (sizeof(uint32_t) * (size_t)n))) { 
			u++;
		} else {
			while (u-->0) free(pu[u]);
			failed = TRUE;
		}
	}
	if (failed) return FALSE;

	for (failed = FALSE, d = 0, i = 0; i < MAXD && !failed; i++) {
		if (NULL != (pd[d] = malloc (sizeof(double) * (size_t)n))) { 
			d++;
		} else {
			while (d-->0) free(pd[d]);
			while (u-->0) free(pu[u]);
			failed = TRUE;
		}
	}
	if (failed) return FALSE;

//	r->n		 		= 0; /* empty for now */
	r->size				= n;
	r->sorted 			= pu[0];
	r->playedby 		= pu[1];
	r->playedby_results = pu[2];

	r->obtained 		= pd[0];
 	r->ratingof 		= pd[1];
 	r->ratingbk 		= pd[2];
 	r->changing 		= pd[3];
	r->ratingof_results = pd[4];
	r->obtained_results = pd[5];
	return TRUE;
}

static void 
ratings_done (struct RATINGS *r)
{
//	r->n = 0;
	r->size	= 0;

	free(r->sorted);
	free(r->playedby);
	free(r->playedby_results);
	free(r->obtained);
 	free(r->ratingof);
 	free(r->ratingbk);
 	free(r->changing);
	free(r->ratingof_results);
	free(r->obtained_results);
} 


static bool_t 
games_init (int32_t n, struct GAMES *g)
{
	enum {MAXU=3};
	int32_t 	*pu[MAXU];
	bool_t		failed;
	int i,u;

	assert (n > 0);

	for (failed = FALSE, u = 0, i = 0; i < MAXU && !failed; i++) {
		if (NULL != (pu[i] = malloc (sizeof(int32_t) * (size_t)n))) { 
			u++;
		} else {
			while (u-->0) free(pu[u]);
			failed = TRUE;
		}
	}
	if (failed) return FALSE;

	g->n	 		= 0; /* empty for now */
	g->size			= n;
	g->whiteplayer 	= pu[0];
	g->blackplayer 	= pu[1];
	g->score		= pu[2];
	return TRUE;
}

static void 
games_done (struct GAMES *g)
{
	free(g->whiteplayer);
	free(g->blackplayer);
	free(g->score);
	g->n	 		= 0;
	g->size			= 0;
	g->whiteplayer 	= NULL;
	g->blackplayer 	= NULL;
	g->score		= NULL;
} 

//

static bool_t 
encounters_init (int32_t n, struct ENCOUNTERS *e)
{
	bool_t		failed;
	struct ENC 	*p;

	assert (n > 0);

	if (NULL != (p = malloc (sizeof(struct ENC) * (size_t)n))) {
		failed = FALSE;
	} else {
		free(p);
		failed = TRUE;
	}

	if (failed) return FALSE;

	e->n	 	= 0; /* empty for now */
	e->size 	= n;
	e->enc		= p;
	return TRUE;
}

static void 
encounters_done (struct ENCOUNTERS *e)
{
	free(e->enc);
	e->n	 = 0;
	e->size	 = 0;
	e->enc	 = NULL;
} 


static bool_t 
players_init (int32_t n, struct PLAYERS *x)
{
	enum VARIAB {NV = 4};
	bool_t failed;
	size_t sz[NV];
	void * pv[NV];
	int i, j;

	assert (n > 0);

	// size of the individual elements
	sz[0] = sizeof(char *);
	sz[1] = sizeof(bool_t);
	sz[2] = sizeof(bool_t);
	sz[3] = sizeof(int);

	for (failed = FALSE, i = 0; !failed && i < NV; i++) {
		if (NULL == (pv[i] = malloc (sz[i] * (size_t)n))) {
			for (j = 0; j < i; j++) free(pv[j]);
			failed = TRUE;
		}
	}
	if (failed) return FALSE;

	x->n				= 0; /* empty for now */
	x->size				= n;
	x->name 			= pv[0];
	x->flagged			= pv[1];
	x->prefed			= pv[2];
	x->performance_type = pv[3]; 
	return TRUE;
}

static void 
players_done (struct PLAYERS *x)
{
	free(x->name);
	free(x->flagged);
	free(x->prefed);
	free(x->performance_type);
	x->n = 0;
	x->size	= 0;
	x->name = NULL;
	x->flagged = NULL;
	x->prefed = NULL;
	x->performance_type = NULL;
} 
#endif

//=====================================================

static char		Labelbuffer[LABELBUFFERSIZE] = {'\0'};
static char		*Labelbuffer_end = Labelbuffer;

enum 			AnchorSZ	{MAX_ANCHORSIZE=256};
static bool_t	Anchor_use = FALSE;
static int		Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

static bool_t	Anchor_err_rel2avg = FALSE;

static bool_t 	Multiple_anchors_present = FALSE;
static bool_t	General_average_set = FALSE;

static int		Anchored_n = 0;
static bool_t	Performance_type_set = FALSE;

static double	Confidence = 95;
static double	General_average = 2300.0;

static double	Sum1[MAXPLAYERS]; 
static double	Sum2[MAXPLAYERS]; 
static double	Sdev[MAXPLAYERS]; 

static long 	Simulate = 0;

#define INVBETA 175.25

static double	White_advantage = 0;
static double	White_advantage_SD = 0;
static double	Rtng_76 = 202;
static double	Inv_beta = INVBETA;
static double	BETA = 1.0/INVBETA;
static double	Confidence_factor = 1.0;

static int		OUTDECIMALS = 1;

static struct GAMESTATS	Game_stats;

static struct DEVIATION_ACC *sim = NULL;

static double 	Drawrate_evenmatch = STANDARD_DRAWRATE; //default
static double 	Drawrate_evenmatch_percent = 100*STANDARD_DRAWRATE; //default
static double 	Drawrate_evenmatch_percent_SD = 0;

/*------------------------------------------------------------------------*/

static double Probarray [MAXPLAYERS] [4];

static struct prior Wa_prior = {40.0,20.0,FALSE};
static struct prior Dr_prior = { 0.5, 0.1,FALSE};

static struct prior PP[MAXPLAYERS];
static struct prior PP_store[MAXPLAYERS];

static bool_t 	Some_prior_set = FALSE;

static int 		Priored_n = 0;

static void 	priors_reset(struct prior *p);
static void		priors_copy(const struct prior *p, long int n, struct prior *q);
static void 	priors_shuffle(struct prior *p, long int n);
static void		priors_show (struct prior *p, long int n);
static bool_t 	set_prior (const char *prior_name, double x, double sigma);
static void 	priors_load(const char *fpriors_name);

static void		anchor_j (long int j, double x);

#define MAX_RELPRIORS 10000

static struct relprior Ra[MAX_RELPRIORS];
static struct relprior Ra_store[MAX_RELPRIORS];

static long int N_relative_anchors = 0;
static bool_t 	Hide_old_ver = FALSE;

static void		relpriors_shuffle(struct relprior *rp, long int n);
static void		relpriors_copy(const struct relprior *r, long int n, struct relprior *s);
static bool_t 	set_relprior (const char *player_a, const char *player_b, double x, double sigma);
static void 	relpriors_show(void);
static void 	relpriors_load(const char *f_name);

static bool_t	Prior_mode;

/*------------------------------------------------------------------------*/

static void		purge_players (bool_t quiet, long n_players, const int *performance_type, char **name, bool_t *flagged);

static long 	set_super_players(bool_t quiet, long N_enc, const struct ENC *enc, long n_players, int *perftype, char *name[], bool_t *perftype_set);

static void		clear_flagged (long n_players, bool_t *flagged);

static void		all_report (FILE *csvf, FILE *textf);
static void		init_rating (void);
static void		reset_rating (double general_average, long n_players, const bool_t *prefed, const bool_t *flagged, double *rating);
static void		ratings_copy (const double *r, long n, double *t);
static void		init_manchors(const char *fpins_name);

static int32_t	calc_rating (bool_t quiet, struct ENC *enc, long N_enc, double *pWhite_advantage, bool_t adjust_wadv, double *pDraw_rate);

static void 	ratings_results (void);
static void 	ratings_for_purged (void);

static void
simulate_scores ( double 		deq
				, double 		wadv
				, double 		beta
				, long 			n_games
				, const double 	*ratingof_results
				, const int		*whiteplayer
				, const int		*blackplayer
				, int			*score // out
);

static void		errorsout(const char *out);
static void		ctsout(const char *out);

/*------------------------------------------------------------------------*/

static void 	DB_ignore_draws (struct DATA *db);
static void 	DB_transform(const struct DATA *db, struct GAMES *g, struct PLAYERS *p, struct GAMESTATS *gs);
static bool_t	find_anchor_player(int *anchor);

/*------------------------------------------------------------------------*/

#if 0
static double 	overallerror_fwadv (double wadv);
static double 	adjust_wadv (double start_wadv);
#endif

static void 	table_output(double Rtng_76);

/*------------------------------------------------------------------------*/

static void		cegt_output(void);
static void 	head2head_output(const char *head2head_str);

/*------------------------------------------------------------------------*/

static ptrdiff_t	head2head_idx_sdev (long x, long y);

/*------------------------------------------------------------------------*/

static void 	ratings_center_to_zero (long int n_players, const bool_t *flagged, double *ratingof);

/*------------------------------------------------------------------------*/

#if 0
#define SAVE_SIMULATION
#define SAVE_SIMULATION_N 23
#endif

#if defined(SAVE_SIMULATION)
// This section is to save simulated results for debugging purposes

static const char *Result_string[4] = {"1-0","1/2-1/2","0-1","*"};

static void
save_simulated(int num)
{
	int i;
	const char *name_w;
	const char *name_b;
	const char *result;
	char filename[256] = "";	
	FILE *fout;

	sprintf (filename, "simulated_%04d.pgn", num);

	printf ("\n--> filename=%s\n\n",filename);

	if (NULL != (fout = fopen (filename, "w"))) {

		for (i = 0; i < Games.n; i++) {

			if (Games.score[i] == DISCARD) continue;
	
			name_w = Players.name [Games.whiteplayer[i]];
			name_b = Players.name [Games.blackplayer[i]];		
			result = Result_string[Games.score[i]];

			fprintf(fout,"[White \"%s\"]\n",name_w);
			fprintf(fout,"[Black \"%s\"]\n",name_b);
			fprintf(fout,"[Result \"%s\"]\n",result);
			fprintf(fout,"%s\n\n",result);
		}

		fclose(fout);
	}
}
#endif


static void
calc_encounters__
				( int selectivity
				, struct GAMES *g
				, const bool_t *flagged
				, struct ENCOUNTERS	*e
) 
{
	e->n = (int)
	calc_encounters	( selectivity
					, g->n
					, g->score
					, flagged
					, g->whiteplayer
					, g->blackplayer
					, e->enc);
}

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

static struct DATA *pdaba;

int main (int argc, char *argv[])
{
	bool_t csvf_opened;
	bool_t textf_opened;
	bool_t groupf_opened;
	FILE *csvf;
	FILE *textf;
	FILE *groupf;

	int op;
	const char *inputf, *textstr, *csvstr, *ematstr, *groupstr, *pinsstr;
	const char *priorsstr, *relstr;
	const char *head2head_str;
	const char *ctsmatstr;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;
	bool_t group_is_output, Elostat_output, Ignore_draws;
	bool_t switch_w=FALSE, switch_W=FALSE, switch_u=FALSE, switch_d=FALSE, switch_k=FALSE, switch_D=FALSE;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode    = FALSE;
	switch_mode  = FALSE;
	input_mode   = FALSE;
	table_mode   = FALSE;
	QUIET_MODE   = FALSE;
	SIM_UPDATES  = FALSE;
	ADJUST_WHITE_ADVANTAGE = FALSE;
	ADJUST_DRAW_RATE = FALSE;
	inputf       = NULL;
	textstr 	 = NULL;
	csvstr       = NULL;
	ematstr 	 = NULL;
	ctsmatstr	 = NULL;
	pinsstr		 = NULL;
	priorsstr	 = NULL;
	relstr		 = NULL;
	group_is_output = FALSE;
	groupstr 	 = NULL;
	Elostat_output = FALSE;
	head2head_str = NULL;
	Ignore_draws = FALSE;

	while (END_OF_OPTIONS != (op = options (argc, argv, OPTION_LIST))) {
		switch (op) {
			case 'v':	version_mode = TRUE; 	break;
			case 'L':	version_mode = TRUE; 	
						license_mode = TRUE;
						break;
			case 'h':	help_mode = TRUE;		break;
			case 'H':	switch_mode = TRUE;		break;
			case 'p': 	input_mode = TRUE;
					 	inputf = opt_arg;
						break;
			case 'c': 	csvstr = opt_arg;
						break;
			case 'o': 	textstr = opt_arg;
						break;
			case 'g': 	group_is_output = TRUE;
						groupstr = opt_arg;
						break;
			case 'j': 	head2head_str = opt_arg;
						break;
			case 'e': 	ematstr = opt_arg;
						break;
			case 'C': 	ctsmatstr = opt_arg;
						break;
			case 'm': 	pinsstr = opt_arg;
						break;
			case 'r': 	relstr = opt_arg;
						break;
			case 'y': 	priorsstr = opt_arg;
						break;
			case 'a': 	if (1 != sscanf(opt_arg,"%lf", &General_average)) {
							fprintf(stderr, "wrong average parameter\n");
							exit(EXIT_FAILURE);
						} else {
							General_average_set = TRUE;
						}
						break;
			case 'F': 	if (1 != sscanf(opt_arg,"%lf", &Confidence) ||
								(Confidence > 99.999 || Confidence < 50.001)
							) {
							fprintf(stderr, "wrong confidence parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'A': 	if (strlen(opt_arg) < MAX_ANCHORSIZE-1) {
							strcpy (Anchor_name, opt_arg); //FIXME
							Anchor_use = TRUE;
						} else {
							fprintf(stderr, "ERROR: anchor name is too long\n");
							exit(EXIT_FAILURE);	
						}
						break;
			case 'V':	Anchor_err_rel2avg = TRUE;
						break;
			case 's': 	if (1 != sscanf(opt_arg,"%lu", &Simulate) || Simulate < 0) {
							fprintf(stderr, "wrong simulation parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'w': 	if (1 != sscanf(opt_arg,"%lf", &White_advantage)) {
							fprintf(stderr, "wrong white advantage parameter\n");
							exit(EXIT_FAILURE);
						} else {
							ADJUST_WHITE_ADVANTAGE = FALSE;	
							Wa_prior.isset = FALSE;
							switch_w = TRUE;
						}
						break;
			case 'u': 	if (1 != sscanf(opt_arg,"%lf", &White_advantage_SD)) {
							fprintf(stderr, "wrong white advantage uncertainty parameter\n");
							exit(EXIT_FAILURE);
						} else {
							switch_u = TRUE;
						}
						break;
			case 'd': 	if (1 != sscanf(opt_arg,"%lf", &Drawrate_evenmatch_percent)) {
							fprintf(stderr, "wrong white drawrate parameter\n");
							exit(EXIT_FAILURE);
						} else {
							ADJUST_DRAW_RATE = FALSE;	
							Dr_prior.isset = FALSE;
							switch_d = TRUE;
						}
						if (Drawrate_evenmatch_percent >= 0.0 && Drawrate_evenmatch_percent <= 100.0) {
							Drawrate_evenmatch = Drawrate_evenmatch_percent/100.0;
						} else {
							fprintf(stderr, "drawrate parameter is out of range\n");
							exit(EXIT_FAILURE);
						}					
						break;
			case 'k': 	if (1 != sscanf(opt_arg,"%lf", &Drawrate_evenmatch_percent_SD)) { //drsd
							fprintf(stderr, "wrong draw rate uncertainty parameter\n");
							exit(EXIT_FAILURE);
						} else {
							switch_k = TRUE;
						}
						break;
			case 'z': 	if (1 != sscanf(opt_arg,"%lf", &Rtng_76)) {
							fprintf(stderr, "wrong scaling parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'T':	table_mode = TRUE;	break;
			case 'q':	QUIET_MODE = TRUE;	break;
			case 'Q':	QUIET_MODE = TRUE;	SIM_UPDATES = TRUE; break;
			case 'R':	Hide_old_ver=TRUE;	break;
			case 'W':	ADJUST_WHITE_ADVANTAGE = TRUE;	
						Wa_prior.isset = FALSE;	
						Wa_prior.value = 0; 	 
						Wa_prior.sigma = 200.0; 
						switch_W = TRUE;
						break;
			case 'D':	ADJUST_DRAW_RATE = TRUE;	
						Dr_prior.isset = FALSE;	
						Dr_prior.value = 0.5; 	 
						Dr_prior.sigma = 0.5; 
						switch_D = TRUE;
						break;
			case 'E':	Elostat_output = TRUE;	break;
			case 'X':	Ignore_draws = TRUE;	break;
			case 'N': 	if (1 != sscanf(opt_arg,"%d", &OUTDECIMALS) || OUTDECIMALS < 0) {
							fprintf(stderr, "wrong decimals parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case '?': 	parameter_error();
						exit(EXIT_FAILURE);
						break;
			default:	fprintf (stderr, "ERROR: %d\n", op);
						exit(EXIT_FAILURE);
						break;
		}		
	}

	/*----------------------------------*\
	|	Return version
	\*----------------------------------*/
	if (version_mode) {
		printf ("%s %s\n",proginfo_name(),proginfo_version());
		if (license_mode)
 			printf ("%s\n", license_str);
		return EXIT_SUCCESS;
	}
	if (argc < 2) {
		fprintf (stderr, "%s %s\n",proginfo_name(),proginfo_version());
		fprintf (stderr, "%s", copyright_str);
		fprintf (stderr, "for help type:\n%s -h\n\n", proginfo_name());
		exit (EXIT_FAILURE);
	}
	if (help_mode) {
		printf ("\n%s", intro_str);
		example();
		usage();
		printf ("%s\n", copyright_str);
		exit (EXIT_SUCCESS);
	}
	if (switch_mode && !help_mode) {
		usage();
		exit (EXIT_SUCCESS);
	}
	if (table_mode) {
		table_output(Rtng_76);
		exit (EXIT_SUCCESS);
	}
	if ((argc - opt_index) > 1) {
		/* too many parameters */
		fprintf (stderr, "ERROR: Extra parameters present\n");
		fprintf (stderr, "Make sure to surround parameters with \"quotes\" if they contain spaces\n\n");
		exit(EXIT_FAILURE);
	}
	if (input_mode && argc != opt_index) {
		fprintf (stderr, "Extra parameters present\n");
		fprintf (stderr, "Make sure to surround parameters with \"quotes\" if they contain spaces\n\n");
		exit(EXIT_FAILURE);
	}
	if (!input_mode && argc == opt_index) {
		fprintf (stderr, "Need file name to proceed\n\n");
		exit(EXIT_FAILURE);
	}
	/* get folder, should be only one at this point */
	while (opt_index < argc ) {
		inputf = argv[opt_index++];
	}

	if (NULL != pinsstr && (General_average_set || Anchor_use)) {
		fprintf (stderr, "Setting a general average (-a) or a single anchor (-A) is incompatible with multiple anchors (-m)\n\n");
		exit(EXIT_FAILURE);
	}
	if ((switch_w || switch_u) && switch_W) {
		fprintf (stderr, "Switches -w/-u and -W are incompatible and will not work simultaneously\n\n");
		exit(EXIT_FAILURE);
	}
	if ((switch_d || switch_k) && switch_D) { //drsd
		fprintf (stderr, "Switches -d/-k and -D are incompatible and will not work simultaneously\n\n");
		exit(EXIT_FAILURE);
	}
	if (NULL != priorsstr && General_average_set) {
		fprintf (stderr, "Setting a general average (-a) is incompatible with having a file with rating seeds (-y)\n\n");
		exit(EXIT_FAILURE);
	}				

	Prior_mode = switch_k || switch_u || NULL != relstr || NULL != priorsstr;

	/*==== SET INPUT ====*/

	if (NULL != (pdaba = database_init_frompgn(inputf, QUIET_MODE))) {
		if (Ignore_draws) DB_ignore_draws(pdaba);
	} else {
		fprintf (stderr, "Problems reading results from: %s\n", inputf);
		return EXIT_FAILURE; 
	}

	/*==== memory initialization ====*/
	{
	int32_t mpr = pdaba->n_players; 
	int32_t mpp = pdaba->n_players; 
	int32_t mg  = pdaba->n_games;
	int32_t me  = pdaba->n_games;
	if (!ratings_init (mpr, &RA)) {
		fprintf (stderr, "Could not initialize rating memory\n"); exit(0);	
	} else 
	if (!games_init (mg, &Games)) {
		ratings_done (&RA);
		fprintf (stderr, "Could not initialize Games memory\n"); exit(0);
	} else 
	if (!encounters_init (me, &Encounters)) {
		ratings_done (&RA);
		games_done (&Games);
		fprintf (stderr, "Could not initialize Encounters memory\n"); exit(0);
	} else 
	if (!players_init (mpp, &Players)) {
		ratings_done (&RA);
		games_done (&Games);
		encounters_done (&Encounters);
		fprintf (stderr, "Could not initialize Players memory\n"); exit(0);
	}
	}
	/**/

	DB_transform(pdaba, &Games, &Players, &Game_stats); /* convert DB to global variables */

	if (Anchor_use) {
		if (find_anchor_player(&Anchor)) {
			anchor_j (Anchor, General_average);
		} else {
			fprintf (stderr, "ERROR: No games of anchor player, mispelled, wrong capital letters, or extra spaces = \"%s\"\n", Anchor_name);
			fprintf (stderr, "Surround the name with \"quotes\" if it contains spaces\n\n");
			return EXIT_FAILURE; 			
		} 
	}

	if (Drawrate_evenmatch < 0.0 || Drawrate_evenmatch > 1.0) {
			fprintf (stderr, "ERROR: Invalide draw rate set\n");
			return EXIT_FAILURE; 		
	}

	if (!(Drawrate_evenmatch > 0.0) && Game_stats.draws > 0 && Prior_mode) {
			fprintf (stderr, "ERROR: Draws present in the database but -d switch specified an invalid number\n");
			return EXIT_FAILURE; 
	}

	if (Drawrate_evenmatch > 0.999) {
			fprintf (stderr, "ERROR: Draw rate set with -d switch is too high, > 99.9%s\n", "%");
			return EXIT_FAILURE; 
	}

	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (!QUIET_MODE) {
		printf ("Total games         %8ld\n", Game_stats.white_wins
											 +Game_stats.draws
											 +Game_stats.black_wins
											 +Game_stats.noresult);
		printf (" - White wins       %8ld\n", Game_stats.white_wins);
		printf (" - Draws            %8ld\n", Game_stats.draws);
		printf (" - Black wins       %8ld\n", Game_stats.black_wins);
		printf (" - Truncated        %8ld\n", Game_stats.noresult);
		printf ("Unique head to head %8.2f%s\n", 100.0*(double)Encounters.n/(double)Games.n, "%");
		if (Anchor_use) {
			printf ("Reference rating    %8.1lf",General_average);
			printf (" (set to \"%s\")\n", Anchor_name);
		} else if (priorsstr != NULL || pinsstr != NULL) { // priors
			printf ("Reference rating    ");
			printf (" --> not fixed, determined by anchors\n");
		} else {	
			printf ("Reference rating    %8.1lf",General_average);
			printf (" (average of the pool)\n");	
		}
		printf ("\n");	
	}

	//===

	Confidence_factor = confidence2x(Confidence/100.0);
	// printf("confidence factor = %f\n",Confidence_factor);

	init_rating();

	// PRIORS
	priors_reset(PP);

	if (priorsstr != NULL) {
		priors_load(priorsstr);
		#if !defined(DOPRIOR)
		Some_prior_set = FALSE;
		#endif
	}

	// multiple anchors here
	if (pinsstr != NULL) {
		init_manchors(pinsstr); 
	}
	// multiple anchors done

//~~
	if (relstr != NULL) {
		relpriors_load(relstr); 
	}
	if (!QUIET_MODE) {
		priors_show(PP, Players.n);
		relpriors_show();
	//FIXME do not allow relpriors to be purged
	}
//~~

	if (switch_w && switch_u) {
		if (White_advantage_SD > PRIOR_SMALLEST_SIGMA) {
			Wa_prior.isset = TRUE; 
			Wa_prior.value = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			ADJUST_WHITE_ADVANTAGE = TRUE;	
		} else {
			Wa_prior.isset = FALSE; 
			Wa_prior.value = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			ADJUST_WHITE_ADVANTAGE = FALSE;	
		}
	}

	if (switch_d && switch_k) {
		if (Drawrate_evenmatch_percent_SD > PRIOR_SMALLEST_SIGMA) {
			Dr_prior.isset = TRUE; 
			Dr_prior.value = Drawrate_evenmatch_percent/100.0; 
			Dr_prior.sigma = Drawrate_evenmatch_percent_SD/100.0;  
			ADJUST_DRAW_RATE = TRUE;	
		} else {
			Dr_prior.isset = FALSE; 
			Dr_prior.value = Drawrate_evenmatch_percent/100.0;  
			Dr_prior.sigma = Drawrate_evenmatch_percent_SD/100.0;  
			ADJUST_DRAW_RATE = FALSE;	
		}
	}

	textf = NULL;
	textf_opened = FALSE;
	if (textstr == NULL) {
		textf = stdout;
	} else {
		textf = fopen (textstr, "w");
		if (textf == NULL) {
			fprintf(stderr, "Errors with file: %s\n",textstr);			
		} else {
			textf_opened = TRUE;
		}
	}

	csvf = NULL;
	csvf_opened = FALSE;
	if (csvstr != NULL) {
		csvf = fopen (csvstr, "w");
		if (csvf == NULL) {
			fprintf(stderr, "Errors with file: %s\n",csvstr);			
		} else {
			csvf_opened = TRUE;
		}
	}

	groupf_opened = FALSE;
	groupf = NULL;
	if (group_is_output) {
		if (groupstr != NULL) {
			groupf = fopen (groupstr, "w");
			if (groupf == NULL) {
				fprintf(stderr, "Errors with file: %s\n",groupstr);			
				exit(EXIT_FAILURE);
			} else {
				groupf_opened = TRUE;
			}
		}
	}

	/*==== CALCULATIONS ====*/

	randfast_init (1324561);

	Inv_beta = Rtng_76/(-log(1.0/0.76-1.0));

	BETA = 1.0/Inv_beta;
	
	{	long i;
		for (i = 0; i < Players.n; i++) {
			Sum1[i] = 0;
			Sum2[i] = 0;
			Sdev[i] = 0;
		}
	}

	/*===== GROUPS ========*/

	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);
	scan_encounters(Encounters.enc, Encounters.n, Players.n); 
	if (group_is_output) {

		static struct ENC 		Encounter2[MAXENCOUNTERS];
		static long	int			N_encounters2 = 0;
		static struct ENC 		Encounter3[MAXENCOUNTERS];
		static long	int			N_encounters3 = 0;

		convert_to_groups(groupf, Players.n, Players.name);
		sieve_encounters(Encounters.enc, Encounters.n, Encounter2, &N_encounters2, Encounter3, &N_encounters3);
		printf ("Encounters, Total=%ld, Main=%ld, @ Interface between groups=%ld\n",(long)Encounters.n, N_encounters2, N_encounters3);

		if (textstr == NULL && csvstr == NULL)	
			exit(EXIT_SUCCESS);
	}

	/*=====================*/

	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);
	if (0 < set_super_players(QUIET_MODE, Encounters.n, Encounters.enc, Players.n
										, Players.performance_type, Players.name, &Performance_type_set)) {
		purge_players (QUIET_MODE, Players.n, Players.performance_type, Players.name, Players.flagged);
		calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	}
	Encounters.n = calc_rating(QUIET_MODE, Encounters.enc, Encounters.n, &White_advantage, ADJUST_WHITE_ADVANTAGE, &Drawrate_evenmatch);

	ratings_results();

	/* Simulation block, begin */
	{	
		long i,j;
		long z = Simulate;
		double n = (double) (z);
		ptrdiff_t np = (ptrdiff_t) Players.n;
		ptrdiff_t est = (ptrdiff_t)((np*np-np)/2); /* elements of simulation table */
		ptrdiff_t idx;
		size_t allocsize = sizeof(struct DEVIATION_ACC) * (size_t)est;
		double diff;
		double sim_draw_rate = Drawrate_evenmatch; // temporarily used and modified

		sim = malloc(allocsize);

		if (sim != NULL) {
			double fraction = 0.0;
			double asterisk = (double)Simulate/50.0;
			int astcount = 0;

			if (SIM_UPDATES && z > 1) {
				printf ("0   10   20   30   40   50   60   70   80   90   100 (%s)\n","%");
				printf ("|----|----|----|----|----|----|----|----|----|----|\n");
			}
			
			for (idx = 0; idx < est; idx++) {
				sim[idx].sum1 = 0;
				sim[idx].sum2 = 0;
				sim[idx].sdev = 0;
			}
	
			if (z > 1) {
				while (z-->0) {
					if (!QUIET_MODE) {		
						printf ("\n==> Simulation:%ld/%ld\n",Simulate-z,Simulate);
					} 

					if (SIM_UPDATES) {
						fraction += 1.0;
						while (fraction > asterisk) {
							fraction -= asterisk;
							astcount++;
							printf ("*"); fflush(stdout);
						}
					}
					clear_flagged (Players.n, Players.flagged);

					simulate_scores ( Drawrate_evenmatch
									, White_advantage
									, BETA
									, Games.n
									, RA.ratingof_results
									, Games.whiteplayer
									, Games.blackplayer
									, Games.score // out
					);

					relpriors_copy(Ra, N_relative_anchors, Ra_store);
					priors_copy(PP, Players.n, PP_store);
					relpriors_shuffle(Ra, N_relative_anchors);
					priors_shuffle(PP, Players.n);
					//priors_show (PP, Players.n);

					#if defined(SAVE_SIMULATION)
					if ((Simulate-z) == SAVE_SIMULATION_N) {
						save_simulated((int)(Simulate-z)); 
					}
					#endif

					// may improve convergence in pathological cases
					reset_rating (General_average, Players.n, Players.prefed, Players.flagged, RA.ratingof);
					reset_rating (General_average, Players.n, Players.prefed, Players.flagged, RA.ratingbk);

					calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);
					if (0 < set_super_players(QUIET_MODE, Encounters.n, Encounters.enc, Players.n
														, Players.performance_type, Players.name, &Performance_type_set)) {
						purge_players (QUIET_MODE, Players.n, Players.performance_type, Players.name, Players.flagged);
						calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
					}
					Encounters.n = calc_rating(QUIET_MODE, Encounters.enc, Encounters.n, &White_advantage, FALSE, &sim_draw_rate);
					ratings_for_purged ();

					relpriors_copy(Ra_store, N_relative_anchors, Ra);
					priors_copy(PP_store, Players.n, PP);

					if (SIM_UPDATES && z == 0) {
						int x = 51-astcount;
						while (x-->0) {printf ("*"); fflush(stdout);}
						printf ("\n");
					}

					if (Anchor_err_rel2avg) {
						ratings_copy (RA.ratingof, Players.n, RA.ratingbk);
						ratings_center_to_zero (Players.n, Players.flagged, RA.ratingof);
					}

					for (i = 0; i < Players.n; i++) {
						Sum1[i] += RA.ratingof[i];
						Sum2[i] += RA.ratingof[i]*RA.ratingof[i];
					}
	
					for (i = 0; i < (Players.n); i++) {
						for (j = 0; j < i; j++) {
							//idx = (i*i-i)/2+j;
							idx = head2head_idx_sdev (i, j);
							assert(idx < est || !printf("idx=%ld est=%ld\n",idx,est));
							diff = RA.ratingof[i] - RA.ratingof[j];	
							sim[idx].sum1 += diff; 
							sim[idx].sum2 += diff * diff;
						}
					}

					if (Anchor_err_rel2avg) {
						ratings_copy (RA.ratingbk, Players.n, RA.ratingof); //restore
					}
				}

				for (i = 0; i < Players.n; i++) {
					double xx = n*Sum2[i] - Sum1[i] * Sum1[i];
					xx = sqrt(xx*xx); // removes problems with -0.00000000000;
					Sdev[i] = sqrt( xx ) /n;
				}
	
				for (i = 0; i < est; i++) {
					double xx = n*sim[i].sum2 - sim[i].sum1 * sim[i].sum1;
					xx = sqrt(xx*xx); // removes problems with -0.00000000000;
					sim[i].sdev = sqrt( xx ) /n;
				}

			}
		}

		DB_transform(pdaba, &Games, &Players, &Game_stats); /* convert DB to global variables, to restore original data */
	}
	/* Simulation block, end */

	// Reports
	all_report (csvf, textf);
	if (Simulate > 1 && NULL != ematstr) {
		errorsout (ematstr);
	}
	if (Simulate > 1 && NULL != ctsmatstr) {
		ctsout (ctsmatstr);
	}

	//
	if (head2head_str != NULL)
		head2head_output(head2head_str);		

	// CEGT output style
	if (Elostat_output)
		cegt_output();

	// Cleanup
	if (textf_opened) 	fclose (textf);
	if (csvf_opened)  	fclose (csvf); 
	if (groupf_opened) 	fclose(groupf);

	if (sim != NULL) free(sim);

	if (pdaba != NULL)
	database_done (pdaba);

	/*==== END CALCULATION ====*/

	// memory done
	ratings_done (&RA);
	games_done (&Games);
	encounters_done (&Encounters);
	players_done (&Players);

	return EXIT_SUCCESS;
}


/*--------------------------------------------------------------*\
|
|	END OF MAIN
|
\**/


static void parameter_error(void) {	printf ("Error in parameters\n"); return;}

static void
example (void)
{
	printf ("\n"
		"quick example: %s %s\n"
		"%s"
		, proginfo_name()
		, example_options
		, example_str);
	return;
}

static void
usage (void)
{
	printf ("\n"
		"usage: %s %s\n"
		"%s"
		, proginfo_name()
		, usage_options
		, help_str);
}

/*--------------------------------------------------------------*\
|
|	ORDO ROUTINES
|
\**/


static void 
DB_transform(const struct DATA *db, struct GAMES *g, struct PLAYERS *p, struct GAMESTATS *gs)
{
	int i;
	ptrdiff_t x;
	long int gamestat[4] = {0,0,0,0};

	for (x = 0; x < db->labels_end_idx; x++) {
		Labelbuffer[x] = db->labels[x];
	}
	Labelbuffer_end = Labelbuffer + db->labels_end_idx;
	p->n = db->n_players;
	g->n = db->n_games;

	for (i = 0; i < db->n_players; i++) {
		p->name[i] = Labelbuffer + db->name[i];
		p->flagged[i] = FALSE;
	}

	for (i = 0; i < db->n_games; i++) {
		g->whiteplayer[i] = db->white[i];
		g->blackplayer[i] = db->black[i]; 
		g->score[i]       = db->score[i];
		if (g->score[i] <= DISCARD) gamestat[g->score[i]]++;
	}

	gs->white_wins	= gamestat[WHITE_WIN];
	gs->draws		= gamestat[RESULT_DRAW];
	gs->black_wins	= gamestat[BLACK_WIN];
	gs->noresult	= gamestat[DISCARD];

	return;
}


static void 
DB_ignore_draws (struct DATA *db)
{
	long int i;
	for (i = 0; i < db->n_games; i++) {
		if (db->score[i] == RESULT_DRAW)
			db->score[i] = DISCARD;
	}
	return;
}

static bool_t
find_anchor_player(int *anchor)
{
	int i;
	bool_t found = FALSE;
	for (i = 0; i < Players.n; i++) {
		if (!strcmp(Players.name[i], Anchor_name)) {
			*anchor = i;
			found = TRUE;
		} 
	}
	return found;
}

static int
compareit (const void *a, const void *b)
{
	const int *ja = (const int *) a;
	const int *jb = (const int *) b;

	const double da = RA.ratingof_results[*ja];
	const double db = RA.ratingof_results[*jb];
    
	return (da < db) - (da > db);
}

static ptrdiff_t
head2head_idx_sdev (long x, long y)
{	
	ptrdiff_t idx;
	if (y < x) 
		idx = (x*x-x)/2+y;					
	else
		idx = (y*y-y)/2+x;
	return idx;
}

static void
errorsout(const char *out)
{
	FILE *f;
	ptrdiff_t idx;
	long i,j,y,x;

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < Players.n; i++) {
			fprintf(f, ",%ld",i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < Players.n; i++) {
			y = RA.sorted[i];

			fprintf(f, "%ld,\"%21s\"", i, Players.name[y]);

			for (j = 0; j < i; j++) {
				x = RA.sorted[j];

				idx = head2head_idx_sdev (x, y);

				fprintf(f,",%.1f", sim[idx].sdev * Confidence_factor);
			}

			fprintf(f, "\n");

		}

		fclose(f);

	} else {
		fprintf(stderr, "Errors with file: %s\n",out);	
	}
	return;
}

static void
ctsout(const char *out)
{
	FILE *f;
	ptrdiff_t idx;
	long i,j,y,x;

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < Players.n; i++) {
			fprintf(f, ",%ld",i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < Players.n; i++) {
			y = RA.sorted[i];
			fprintf(f, "%ld,\"%21s\"", i, Players.name[y]);

			for (j = 0; j < Players.n; j++) {
				double ctrs, sd, dr;
				x = RA.sorted[j];
				if (x != y) {
					dr = RA.ratingof_results[y] - RA.ratingof_results[x];
					idx = head2head_idx_sdev (x, y);
					sd = sim[idx].sdev;
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


static size_t
find_maxlen (char *nm[], long int n)
{
	size_t maxl = 0;
	size_t length;
	long int i;
	for (i = 0; i < n; i++) {
		length = strlen(nm[i]);
		if (length > maxl) maxl = length;
	}
	return maxl;
}

static bool_t 
is_super_player(int j)
{
	assert(Performance_type_set);
	return Players.performance_type[j] == PERF_SUPERLOSER || Players.performance_type[j] == PERF_SUPERWINNER;		
}

static const char *SP_symbolstr[3] = {"<",">"," "};

static const char *
get_super_player_symbolstr(int j)
{
	assert(Performance_type_set);
	if (Players.performance_type[j] == PERF_SUPERLOSER) {
		return SP_symbolstr[0];
	} else if (Players.performance_type[j] == PERF_SUPERWINNER) {
		return SP_symbolstr[1];
	} else
		return SP_symbolstr[2];
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

static bool_t
is_old_version(int j)
{
	int i;
	bool_t found;
	for (i = 0, found = FALSE; !found && i < N_relative_anchors; i++) {
		found = j == Ra[i].player_b;
	}
	return found;
}

static char *
get_sdev_str (double sdev, double confidence_factor, char *str)
{
	double x = sdev * confidence_factor;
	if (sdev > 0.00000001) {
		sprintf(str, "%6.*f", OUTDECIMALS, rating_round(x, OUTDECIMALS));
	} else {
		sprintf(str, "%s", "  ----");
	}
	return str;
}

static void
all_report (FILE *csvf, FILE *textf)
{
	FILE *f;
	int i, j;
	size_t ml;
	char sdev_str_buffer[80];
	const char *sdev_str;

	int rank = 0;
	bool_t showrank = TRUE;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);

	calc_obtained_playedby(Encounters.enc, Encounters.n, Players.n, RA.obtained, RA.playedby);

	for (j = 0; j < Players.n; j++) {
		RA.sorted[j] = j;
	}

	qsort (RA.sorted, (size_t)Players.n, sizeof (RA.sorted[0]), compareit);

	/* output in text format */
	f = textf;
	if (f != NULL) {

		ml = find_maxlen (Players.name, Players.n);
		if (ml > 50) ml = 50;

		if (Simulate < 2) {
			fprintf(f, "\n%s %-*s    :%7s %9s %7s %6s\n", 
				"   #", 			
				(int)ml,
				"PLAYER", "RATING", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < Players.n; i++) {

				j = RA.sorted[i];
				if (!Players.flagged[j]) {

					char rankbuf[80];
					showrank = !is_old_version(j);
					if (showrank) {
						rank++;
						sprintf(rankbuf,"%d",rank);
					} else {
						rankbuf[0] = '\0';
					}

					if (showrank
						|| !Hide_old_ver
					){
						fprintf(f, "%4s %-*s %s :%7.*f %9.1f %7d %6.1f%s\n", 
							rankbuf,
							(int)ml+1,
							Players.name[j],
							get_super_player_symbolstr(j),
							OUTDECIMALS,
							rating_round (RA.ratingof_results[j], OUTDECIMALS), 
							RA.obtained_results[j], 
							RA.playedby_results[j], 
							RA.playedby_results[j]==0? 0: 100.0*RA.obtained_results[j]/RA.playedby_results[j], 
							"%"
						);
					}

				} else {

						fprintf(f, "%4d %-*s   :%7s %9s %7s %6s%s\n", 
							i+1,
							(int)ml+1,
							Players.name[j], 
							"----", "----", "----", "----","%");
				}
			}
		} else {
			fprintf(f, "\n%s %-*s    :%7s %6s %8s %7s %6s\n", 
				"   #", 
				(int)ml, 
				"PLAYER", "RATING", "ERROR", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < Players.n; i++) {
				j = RA.sorted[i];

				sdev_str = get_sdev_str (Sdev[j], Confidence_factor, sdev_str_buffer);

				if (!Players.flagged[j]) {

					char rankbuf[80];
					showrank = !is_old_version(j);
					if (showrank) {
						rank++;
						sprintf(rankbuf,"%d",rank);
					} else {
						rankbuf[0] = '\0';
					}

					if (showrank
						|| !Hide_old_ver
					){

						fprintf(f, "%4s %-*s %s :%7.*f %s %8.1f %7d %6.1f%s\n", 
						rankbuf,
						(int)ml+1, 
						Players.name[j],
 						get_super_player_symbolstr(j),
						OUTDECIMALS,
						rating_round(RA.ratingof_results[j], OUTDECIMALS), 
						sdev_str, 
						RA.obtained_results[j], 
						RA.playedby_results[j], 
						RA.playedby_results[j]==0?0:100.0*RA.obtained_results[j]/RA.playedby_results[j], 
						"%"
						);
					}

				} else if (!is_super_player(j)) {
					fprintf(f, "%4d %-*s   :%7.*f %s %8.1f %7d %6.1f%s\n", 
						i+1,
						(int)ml+1, 
						Players.name[j], 
						OUTDECIMALS,
						rating_round(RA.ratingof_results[j], OUTDECIMALS), 
						"  ****", 
						RA.obtained_results[j], 
						RA.playedby_results[j], 
						RA.playedby_results[j]==0?0:100.0*RA.obtained_results[j]/RA.playedby_results[j], 
						"%"
					);
				} else {
					fprintf(f, "%4d %-*s   :%7s %s %8s %7s %6s%s\n", 
						i+1,
						(int)ml+1,
						Players.name[j], 
						"----", "  ----", "----", "----", "----","%"
					);
				}
			}
		}


//		if (!quiet) {
			fprintf (f,"\n");
			fprintf (f,"White advantage = %.2f\n",White_advantage);
			fprintf (f,"Draw rate (equal opponents) = %.2f %s\n",100*Drawrate_evenmatch, "%");
			fprintf (f,"\n");
//		}


	} /*if*/

	/* output in a comma separated value file */
	f = csvf;
	if (f != NULL) {
			fprintf(f, "\"%s\""
			",%s"
			",%s"
			",%s"
			",%s"
			",%s"
			"\n"		
			,"Player"
			,"\"Rating\"" 
			,"\"Error\"" 
			,"\"Score\""
			,"\"Games\""
			,"\"(%)\"" 
			);
		for (i = 0; i < Players.n; i++) {
			j = RA.sorted[i];

				if (Sdev[j] > 0.00000001) {
					sprintf(sdev_str_buffer, "%.1f", Sdev[j] * Confidence_factor);
					sdev_str = sdev_str_buffer;
				} else {
					sdev_str = "\"-\"";
				}

			fprintf(f, "\"%s\",%.1f"
			",%s"
			",%.2f"
			",%d"
			",%.2f"
			"\n"		
			,Players.name[j]
			,RA.ratingof_results[j] 
			,sdev_str
			,RA.obtained_results[j]
			,RA.playedby_results[j]
			,RA.playedby_results[j]==0?0:100.0*RA.obtained_results[j]/RA.playedby_results[j] 
			);
		}
	}

	return;
}



//=====================================

static void
init_rating (void)
{
	int i;
	for (i = 0; i < Players.n; i++) {
		RA.ratingof[i] = General_average;
	}
	for (i = 0; i < Players.n; i++) {
		RA.ratingbk[i] = General_average;
	}
}

// no globals
static void
reset_rating (double general_average, long n_players, const bool_t *prefed, const bool_t *flagged, double *rating)
{
	int i;
	for (i = 0; i < n_players; i++) {
		if (!prefed[i] && !flagged[i])
			rating[i] = general_average;
	}
}

static void
ratings_copy (const double *r, long n, double *t)
{
	long i;
	for (i = 0; i < n; i++) {
		t[i] = r[i];
	}
}

//=====================================

static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}
static bool_t getnum(char *p, double *px) 
{ 	float x;
	bool_t ok = 1 == sscanf( p, "%f", &x );
	*px = (double) x;
	return ok;
}

static void
anchor_j (long int j, double x)
{
	Multiple_anchors_present = TRUE;
	Players.prefed[j] = TRUE;
	RA.ratingof[j] = x;
	Anchored_n++;
}

static bool_t
set_anchor (const char *player_name, double x)
{
	int j;
	bool_t found;
	for (j = 0, found = FALSE; !found && j < Players.n; j++) {
		found = !strcmp(Players.name[j], player_name);
		if (found) {
			anchor_j (j, x);
		} 
	}
	return found;
}

static bool_t
assign_anchor (char *name_pinned, double x, bool_t quiet)
{
	bool_t pin_success = set_anchor (name_pinned, x);
	if (pin_success) {
		if (!quiet)
		printf ("Anchoring, %s --> %.1lf\n", name_pinned, x);
	} else {
		fprintf (stderr, "Anchoring, %s --> FAILED, name not found in input file\n", name_pinned);					
	}
	return pin_success;
}

static void
init_manchors(const char *fpins_name)
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
			pin_success = assign_anchor (name_pinned, x, QUIET_MODE);
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

//== REL PRIORS ========================================


static bool_t
set_relprior (const char *player_a, const char *player_b, double x, double sigma)
{
	int j;
	int p_a = -1; 
	int p_b = -1;
	long int n;
	bool_t found;

	assert(sigma > PRIOR_SMALLEST_SIGMA);

	for (j = 0, found = FALSE; !found && j < Players.n; j++) {
		found = !strcmp(Players.name[j], player_a);
		if (found) {
			p_a = j;
		} 
	}
	if (!found) return found;

	for (j = 0, found = FALSE; !found && j < Players.n; j++) {
		found = !strcmp(Players.name[j], player_b);
		if (found) {
			p_b = j;
		} 
	}
	if (!found) return found;



	n = N_relative_anchors++;

//FIXME
if (n >= MAX_RELPRIORS) {fprintf (stderr, "Maximum memory for relative anchors exceeded\n"); exit(EXIT_FAILURE);}

if (p_a == -1 || p_b == -1) return FALSE; // defensive programming, not needed.

	Ra[n].player_a = p_a;
	Ra[n].player_b = p_b;
	Ra[n].delta    = x;
	Ra[n].sigma    = sigma;

	return found;
}

#if 1
void
relpriors_shuffle(struct relprior *rp, long int n)
{
	int i;
	double value, sigma;
	for (i = 0; i < n; i++) {
		value = rp[i].delta;
		sigma =	rp[i].sigma;	
		rp[i].delta = rand_gauss (value, sigma);
	}
}

static void
relpriors_copy(const struct relprior *r, long int n, struct relprior *s)
{	int i;
	for (i = 0; i < n; i++) {
		s[i] = r[i];
	}
}
#endif

static void
relpriors_show (void)
{ 
	int i;
	if (N_relative_anchors > 0) {
		printf ("Relative Anchors {\n");
		for (i = 0; i < N_relative_anchors; i++) {
			int a = Ra[i].player_a;
			int b = Ra[i].player_b;
			printf ("[%s] [%s] = %lf +/- %lf\n", Players.name[a], Players.name[b], Ra[i].delta, Ra[i].sigma);
		}
		printf ("}\n");
	} else {
		printf ("Relative Anchors = none\n");
	}
}

static bool_t
assign_relative_prior (char *s, char *z, double x, double y, bool_t quiet)
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
			suc = set_relprior (s, z, x, y);
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

static void
relpriors_load(const char *f_name)
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

			prior_success = assign_relative_prior (s, z, x, y, QUIET_MODE);

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




//== PRIORS ============================================

static void
priors_reset(struct prior *p)
{	int i;
	for (i = 0; i < MAXPLAYERS; i++) {
		p[i].value = 0;
		p[i].sigma = 1;
		p[i].isset = FALSE;
	}
	Some_prior_set = FALSE;
	Priored_n = 0;
}

static void
priors_copy(const struct prior *p, long int n, struct prior *q)
{	int i;
	for (i = 0; i < n; i++) {
		q[i] = p[i];
	}
}

#if 1
static void
priors_shuffle(struct prior *p, long int n)
{
	int i;
	double value, sigma;
	for (i = 0; i < n; i++) {
		if (p[i].isset) {
			value = p[i].value;
			sigma = p[i].sigma;
			p[i].value = rand_gauss (value, sigma);
		}
	}
}
#endif

#if 1
static void
priors_show (struct prior *p, long int n)
{ 
	int i;
	if (Priored_n > 0) {
		printf ("Loose Anchors {\n");
		for (i = 0; i < n; i++) {
			if (p[i].isset) {
				printf ("  [%s] = %lf +/- %lf\n", Players.name[i], p[i].value, p[i].sigma);
			}
		}
		printf ("}\n");
	} else {
		printf ("Loose Anchors = none\n");
	}
}
#endif

static bool_t
set_prior (const char *player_name, double x, double sigma)
{
	int j;
	bool_t found;
	assert(sigma > PRIOR_SMALLEST_SIGMA);
	for (j = 0, found = FALSE; !found && j < Players.n; j++) {
		found = !strcmp(Players.name[j], player_name);
		if (found) {
			PP[j].value = x;
			PP[j].sigma = sigma;
			PP[j].isset = TRUE;
			Some_prior_set = TRUE;
			Priored_n++;
		} 
	}

	return found;
}

static bool_t has_a_prior(int j) {return PP[j].isset;}

static bool_t
assign_prior (char *name_prior, double x, double y, bool_t quiet)
{
	bool_t prior_success = TRUE;
	bool_t suc;
	if (y < 0) {
			fprintf (stderr, "Prior, %s --> FAILED, error/uncertainty cannot be negative", name_prior);	
			suc = FALSE;
	} else { 
		if (y < PRIOR_SMALLEST_SIGMA) {
			suc = set_anchor (name_prior, x);
			if (!suc) {
				fprintf (stderr, "Prior, %s --> FAILED, name not found in input file\n", name_prior);
			} 
			else {
				if (!quiet)
				printf ("Anchoring, %s --> %.1lf\n", name_prior, x);
			} 
		} else {
			suc = set_prior (name_prior, x, y);
			if (!suc) {
				fprintf (stderr, "Prior, %s --> FAILED, name not found in input file\n", name_prior);					
			} 
//			else {
//				if (!quiet)
//				printf ("Prior, %s --> %.1lf, %.1lf\n", name_prior, x, y);
//			}
		}
	}
	if (!suc) prior_success = FALSE;
	return prior_success;
}

static void
priors_load(const char *fpriors_name)
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
			prior_success = assign_prior (name_prior, x, y, QUIET_MODE);
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



//== END PRIORS ======================================================

// no globals
static void
purge_players (bool_t quiet, long n_players, const int *performance_type, char **name, bool_t *flagged)
{
	int j;
	assert(Performance_type_set);
	for (j = 0; j < n_players; j++) {
		if (flagged[j]) continue;
		if (performance_type[j] != PERF_NORMAL) {
			flagged[j]= TRUE;
			if (!quiet) printf ("purge --> %s\n", name[j]);
		} 
	}
}

static void
ratings_results (void)
{
	double excess;

	int j;
	ratings_for_purged();
	for (j = 0; j < Players.n; j++) {
		RA.ratingof_results[j] = RA.ratingof[j];
		RA.obtained_results[j] = RA.obtained[j];
		RA.playedby_results[j] = RA.playedby[j];
	}

	// shifting ratings to fix the anchor.
	// Only done if the error is relative to the average.
	// Otherwise, it is taken care in the rating calculation already.
	// If Anchor_err_rel2avg is set, shifting in the calculation (later) is deactivated.
	excess = 0.0;
	if (Anchor_err_rel2avg && Anchor_use) {
		excess = General_average - RA.ratingof_results[Anchor];		
		for (j = 0; j < Players.n; j++) {
			RA.ratingof_results[j] += excess;
		}
	}

}

static void
clear_flagged (long n_players, bool_t *flagged)
{
	int j;
	for (j = 0; j < n_players; j++) {
		flagged[j] = FALSE;
	}	
}

static void
ratings_for_purged (void)
{
	int j;
	for (j = 0; j < Players.n; j++) {
		if (Players.flagged[j]) {
			RA.ratingof[j] = 0;
		}
	}	
}

static int
rand_threeway_wscore(double pwin, double pdraw)
{	
	long z,x,y;
	z = (long)((unsigned)(pwin * (0xffff+1)));
	x = (long)((unsigned)((pwin+pdraw) * (0xffff+1)));
	y = randfast32() & 0xffff;

	if (y < z) {
		return WHITE_WIN;
	} else if (y < x) {
		return RESULT_DRAW;
	} else {
		return BLACK_WIN;		
	}
}


// no globals
static void
simulate_scores ( double 		deq
				, double 		wadv
				, double 		beta
				, long 			n_games
				, const double 	*ratingof_results
				, const int		*whiteplayer
				, const int		*blackplayer
				, int			*score // out
)
{
	long int i, w, b;
	const double *rating = ratingof_results;
	double pwin, pdraw, plos;

	for (i = 0; i < n_games; i++) {
		if (score[i] != DISCARD) {
			w = whiteplayer[i];
			b = blackplayer[i];
			get_pWDL(rating[w] + wadv - rating[b], &pwin, &pdraw, &plos, deq, beta);
			score [i] = rand_threeway_wscore(pwin,pdraw);
		}
	}
}

//==== CALCULATE INDIVIDUAL RATINGS =========================

static int32_t
calc_rating (bool_t quiet, struct ENC *enc, long N_enc, double *pWhite_advantage, bool_t adjust_wadv, double *pDraw_rate)
{
	double dr = *pDraw_rate;

	long ret;

	if (Prior_mode) {

		ret = calc_rating_bayes2 
				( quiet
				, enc
				, N_enc

				, Players.n
				, RA.obtained
				, RA.playedby
				, RA.ratingof
				, RA.ratingbk
				, Players.performance_type

				, Players.flagged
				, Players.prefed

				, pWhite_advantage
				, General_average

				, Multiple_anchors_present
				, Anchor_use && !Anchor_err_rel2avg
				, Anchor
				
				, Games.n
				, Games.score
				, Games.whiteplayer
				, Games.blackplayer

				, Players.name
				, BETA

				, RA.changing
				, N_relative_anchors
				, PP
				, Probarray 
				, Ra
				, Some_prior_set
				, Wa_prior
				, Dr_prior

				, adjust_wadv

				, ADJUST_DRAW_RATE
				, &dr
		);

	} else {

		double *ratingtmp_memory;

		if (NULL != (ratingtmp_memory = malloc (sizeof(double) * MAXPLAYERS))) {

			ret = calc_rating2 	
					( quiet
					, enc
					, N_enc
	
					, Players.n
					, RA.obtained
					, RA.playedby
					, RA.ratingof
					, RA.ratingbk
					, Players.performance_type

					, Players.flagged
					, Players.prefed

					, pWhite_advantage
					, General_average
	
					, Multiple_anchors_present
					, Anchor_use && !Anchor_err_rel2avg
					, Anchor
					
					, Games.n
					, Games.score
					, Games.whiteplayer
					, Games.blackplayer
	
					, Players.name
					, BETA

					, adjust_wadv

					, ADJUST_DRAW_RATE
					, &dr
					, ratingtmp_memory
			);

			free(ratingtmp_memory);

		} else {
			fprintf(stderr, "Not enough memory available\n");
			exit(EXIT_FAILURE);
		}	
	}

	*pDraw_rate = dr;

	return (int32_t)ret;
}

// no globals
static void
ratings_apply_excess_correction(double excess, long int n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	int j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

static void
ratings_center_to_zero (long int n_players, const bool_t *flagged, double *ratingof)
{
	int 	j, notflagged;
	double 	excess, average;
	double 	accum = 0;

	// general average
	for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			notflagged++;
			accum += ratingof[j];
		}
	}
	average = accum / notflagged;
	excess  = average;

	// Correct the excess
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);
}

/*==================================================================*/

static void
table_output(double rtng_76)
{
	int p,h; 

	double invbeta = rtng_76/(-log(1.0/0.76-1.0));

	printf("\n%4s: Performance expected (%s)\n","p%","%");
	printf("Rtng: Rating difference\n");
	printf("\n");
	for (p = 0; p < 5; p++) {
		printf("%3s%6s   "," p%", "Rtng");
	}
	printf("\n");
	for (p = 0; p < 58; p++) {printf("-");}	printf("\n");
	for (p = 50; p < 60; p++) {
		for (h = 0; h < 50; h+=10) {
			printf ("%3d%6.1f   ", p+h   , (p+h)==50?0:inv_xpect (invbeta,(double)(p+h)));
		}
		printf ("\n");
	}
	for (p = 0; p < 58; p++) {printf("-");}	printf("\n");
	printf("\n");
}

// no globals
static long int		
set_super_players(bool_t quiet, long N_enc, const struct ENC *enc, long n_players, int *perftype, char *name[], bool_t *perftype_set)
{
	double 	*obt;
	int		*pla;
	int e, j, w, b;
	long super = 0;

	obt = malloc (sizeof(double) * (size_t)n_players);
	pla = malloc (sizeof(int) * (size_t)n_players);
	if (NULL==obt || NULL==pla) {
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	for (j = 0; j < n_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
	for (e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;

		obt[w] += enc[e].wscore;
		obt[b] += (double)enc[e].played - enc[e].wscore;

		pla[w] += enc[e].played;
		pla[b] += enc[e].played;

	}
	for (j = 0; j < n_players; j++) {
		perftype[j] = PERF_NORMAL;
		if (obt[j] < 0.001) {
			perftype[j] = has_a_prior(j)? PERF_NORMAL: PERF_SUPERLOSER;			
			if (!quiet) printf ("detected (all-losses player) --> %s: seed rating present = %s\n", name[j], has_a_prior(j)? "Yes":"No");
		}	
		if (pla[j] - obt[j] < 0.001) {
			perftype[j] = has_a_prior(j)? PERF_NORMAL: PERF_SUPERWINNER;
			if (!quiet) printf ("detected (all-wins player)   --> %s: seed rating present = %s\n", name[j], has_a_prior(j)? "Yes":"No");
		}
		if (perftype[j] != PERF_NORMAL) super++;
	}
	for (j = 0; j < n_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
	*perftype_set = TRUE;

	free(obt);
	free(pla);
	return super;
}

//**************************************************************

// Function provided for a special out of CEGT organization. EloStat Format

static void cegt_output(void)
{
	struct CEGT cegt;
	int j;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	calc_obtained_playedby(Encounters.enc, Encounters.n, Players.n, RA.obtained, RA.playedby);
	for (j = 0; j < Players.n; j++) {
		RA.sorted[j] = j;
	}
	qsort (RA.sorted, (size_t)Players.n, sizeof (RA.sorted[0]), compareit);

	cegt.n_enc = Encounters.n; 
	cegt.enc = Encounters.enc;
	cegt.simulate = Simulate;
	cegt.n_players = Players.n;
	cegt.sorted = RA.sorted;
	cegt.ratingof_results = RA.ratingof_results;
	cegt.obtained_results = RA.obtained_results;
	cegt.playedby_results = RA.playedby_results;
	cegt.sdev = Sdev; 
	cegt.flagged = Players.flagged;
	cegt.name = Players.name;
	cegt.confidence_factor = Confidence_factor;

	cegt.gstat = &Game_stats;

	cegt.sim = sim;

	output_cegt_style ("general.dat", "rating.dat", "programs.dat", &cegt);
}


// Function provided to have all head to head information

static void head2head_output(const char *head2head_str)
{
	struct CEGT cegt;
	int j;

	calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	calc_obtained_playedby(Encounters.enc, Encounters.n, Players.n, RA.obtained, RA.playedby);
	for (j = 0; j < Players.n; j++) {
		RA.sorted[j] = j;
	}
	qsort (RA.sorted, (size_t)Players.n, sizeof (RA.sorted[0]), compareit);

	cegt.n_enc = Encounters.n;
	cegt.enc = Encounters.enc;
	cegt.simulate = Simulate;
	cegt.n_players = Players.n;
	cegt.sorted = RA.sorted;
	cegt.ratingof_results = RA.ratingof_results;
	cegt.obtained_results = RA.obtained_results;
	cegt.playedby_results = RA.playedby_results;
	cegt.sdev = Sdev; 
	cegt.flagged = Players.flagged;
	cegt.name = Players.name;
	cegt.confidence_factor = Confidence_factor;

	cegt.gstat = &Game_stats;

	cegt.sim = sim;

	output_report_individual (head2head_str, &cegt, (int)Simulate);
}

