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

#if 1
	#define DOPRIOR
#endif

#define MIN_RESOLUTION 0.000001
#define PRIOR_SMALLEST_SIGMA 0.0000001
#define WADV_RECALC
#undef WADV_RECALC

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

#include "csv.h"

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

#include "myopt.h"

const char *license_str = "\n"
"   Copyright (c) 2013 Miguel A. Ballicora\n"
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
	static bool_t ADJUST_WHITE_ADVANTAGE;

	static const char *copyright_str = 
		"Copyright (c) 2013 Miguel A. Ballicora\n"
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
		" -a <avg>    set rating for the pool average\n"
		" -A <player> anchor: rating given by '-a' is fixed for <player>, if provided\n"
		" -m <file>   multiple anchors: file contains rows of \"AnchorName\",AnchorRating\n"
		" -y <file>   loose anchors: file contains rows of \"Player\",Rating,Uncertainty\n"
		" -r <file>   relations: rows of \"PlayerA\",\"PlayerB\",delta_rating,uncertainty\n"
		" -R          remove older player versions (given by -r) from the ouput\n"
		" -w <value>  white advantage value (default=0.0)\n"
		" -u <value>  white advantage uncertainty value (default=0.0)\n"
		" -W          white advantage, automatically adjusted\n"
		" -z <value>  scaling: set rating for winning expectancy of 76% (default=202)\n"
		" -T          display winning expectancy table\n"
		" -p <file>   input file in PGN format\n"
		" -c <file>   output file (comma separated value format)\n"
		" -o <file>   output file (text format), goes to the screen if not present\n"
		" -g <file>   output file with group connection info (no rating output on screen)\n"
		" -s  #       perform # simulations to calculate errors\n"
		" -e <file>   saves an error matrix, if -s was used\n"
		" -F <value>  confidence (%) to estimate error margins. Default is 95.0\n"
		" -D <value>  Output decimals, minimum is 0 (default=1)\n"
		"\n"
		;

	const char *usage_options = 
		"[-OPTION]";
		;
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		

const char *OPTION_LIST = "vhHp:qWLa:A:m:r:y:o:g:c:s:w:u:z:e:TF:RD:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/

static char		Labelbuffer[LABELBUFFERSIZE] = {'\0'};
static char		*Labelbuffer_end = Labelbuffer;

/* players */
static char 	*Name   [MAXPLAYERS];
static int 		N_players = 0;
static bool_t	Flagged [MAXPLAYERS];
static bool_t	Prefed [MAXPLAYERS];////

enum 			AnchorSZ	{MAX_ANCHORSIZE=256};
static bool_t	Anchor_use = FALSE;
static int		Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

static bool_t 	Multiple_anchors_present = FALSE;
static bool_t	General_average_set = FALSE;

static int		Anchored_n = 0;

enum 			Player_Performance_Type {
				PERF_NORMAL = 0,
				PERF_SUPERWINNER = 1,
				PERF_SUPERLOSER = 2
};

static bool_t	Performance_type_set = FALSE;
static int		Performance_type[MAXPLAYERS];

/* games */
static int 		Whiteplayer	[MAXGAMES];
static int 		Blackplayer	[MAXGAMES];
static int 		Score		[MAXGAMES];
static int 		N_games = 0;

/* encounters */

struct ENC 		Encounter[MAXENCOUNTERS];
static int 		N_encounters = 0;


enum SELECTIVITY {
	ENCOUNTERS_FULL = 0,
	ENCOUNTERS_NOFLAGGED = 1
};

#if 1
#define NEW_ENC
#endif

/**/
static double	Confidence = 95;
static double	General_average = 2300.0;
static int		Sorted  [MAXPLAYERS]; /* sorted index by rating */
static double	Obtained[MAXPLAYERS];
static double	Expected[MAXPLAYERS];
static int		Playedby[MAXPLAYERS]; /* N games played by player "i" */
static double	Ratingof[MAXPLAYERS]; /* rating current */
static double	Ratingbk[MAXPLAYERS]; /* rating backup  */

static double	Changing[MAXPLAYERS]; /* rating backup  */

static double	Ratingof_results[MAXPLAYERS];
static double	Obtained_results[MAXPLAYERS];
static int		Playedby_results[MAXPLAYERS];

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

struct GAMESTATS {
	long int
		white_wins,
		draws,
		black_wins,
		noresult;
};

static struct GAMESTATS	Game_stats;

struct DEVIATION_ACC {
	double sum1;
	double sum2;
	double sdev;
};

struct DEVIATION_ACC *sim = NULL;

/*------------------------------------------------------------------------*/

struct prior {
	double rating;
	double sigma;	
	bool_t set;
};

static struct prior Wa_prior = {40.0,20.0,FALSE};
static struct prior PP[MAXPLAYERS];
static bool_t Some_prior_set = FALSE;

static int 	Priored_n = 0;

static void priors_reset(struct prior *p);
static bool_t set_prior (const char *prior_name, double x, double sigma);
static void priors_load(const char *fpriors_name);

#define MAX_RELPRIORS 10000

struct relprior {
	int player_a;
	int player_b;
	double delta;
	double sigma;	
};

struct relprior Ra[MAX_RELPRIORS];
long int N_relative_anchors = 0;
static bool_t hide_old_ver = FALSE;


static bool_t set_relprior (const char *player_a, const char *player_b, double x, double sigma);
static void relpriors_show(void);
static void relpriors_load(const char *f_name);
static double relative_anchors_unfitness_full(long int n_relative_anchors, const struct relprior *ra, const double *ratingof);
static double relative_anchors_unfitness_j(double R, int j, double *ratingof, long int n_relative_anchors, struct relprior *ra);

/*------------------------------------------------------------------------*/

static int		calc_encounters (int selectivity, struct ENC *enc);
static void		calc_obtained_playedby 	(const struct ENC *enc, int N_enc);
//static void		calc_expected 			(const struct ENC *enc, int N_enc);

static int		shrink_ENC (struct ENC *enc, int N_enc);
static int		purge_players    (bool_t quiet, struct ENC *enc);
static int		set_super_players(bool_t quiet, struct ENC *enc);

static void		clear_flagged (void);



void			all_report (FILE *csvf, FILE *textf);
void			init_rating (void);
static void		init_manchors(const char *fpins_name);
double			adjust_rating (double delta, double kappa);
static int		calc_rating (bool_t quiet, struct ENC *enc, int N_enc);
double 			deviation (void);

static void		ratings_restore (int n_players, const double *r_bk, double *r_of);
static void		ratings_backup  (int n_players, const double *r_of, double *r_bk);

static double	xpect (double a, double b);

static void 	ratings_results (void);
static void 	ratings_for_purged (void);

static void		simulate_scores(void);
static void		errorsout(const char *out);

/*------------------------------------------------------------------------*/

static void 	transform_DB(struct DATA *db, struct GAMESTATS *gs);
static bool_t	find_anchor_player(int *anchor);

/*------------------------------------------------------------------------*/
#if defined(WADV_RECALC)
static double 	overallerror_fwadv (double wadv);
static double 	adjust_wadv (double start_wadv);
#endif

static void 	table_output(double Rtng_76);

#if 0
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

		for (i = 0; i < N_games; i++) {

			if (Score[i] == DISCARD) continue;
	
			name_w = Name [Whiteplayer[i]];
			name_b = Name [Blackplayer[i]];		
			result = Result_string[Score[i]];

			fprintf(fout,"[White \"%s\"]\n",name_w);
			fprintf(fout,"[Black \"%s\"]\n",name_b);
			fprintf(fout,"[Result \"%s\"]\n",result);
			fprintf(fout,"%s\n\n",result);
		}

		fclose(fout);
	}
}
#endif

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

int main (int argc, char *argv[])
{
	bool_t csvf_opened;
	bool_t textf_opened;
	bool_t groupf_opened;
	FILE *csvf;
	FILE *textf;
	FILE *groupf;

	int op;
	const char *inputf, *textstr, *csvstr, *ematstr, *groupstr, *pinsstr, *priorsstr, *relstr;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;
	bool_t group_is_output;
	bool_t switch_w=FALSE, switch_W=FALSE, switch_u=FALSE;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode    = FALSE;
	switch_mode  = FALSE;
	input_mode   = FALSE;
	table_mode   = FALSE;
	QUIET_MODE   = FALSE;
	ADJUST_WHITE_ADVANTAGE = FALSE;
	inputf       = NULL;
	textstr 	 = NULL;
	csvstr       = NULL;
	ematstr 	 = NULL;
	pinsstr		 = NULL;
	priorsstr	 = NULL;
	relstr		 = NULL;
	group_is_output = FALSE;
	groupstr 	 = NULL;

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
			case 'e': 	ematstr = opt_arg;
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
							Wa_prior.set = FALSE;
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
			case 'z': 	if (1 != sscanf(opt_arg,"%lf", &Rtng_76)) {
							fprintf(stderr, "wrong scaling parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'T':	table_mode = TRUE;	break;
			case 'q':	QUIET_MODE = TRUE;	break;
			case 'R':	hide_old_ver=TRUE;	break;
			case 'W':	ADJUST_WHITE_ADVANTAGE = TRUE;	
						Wa_prior.set = FALSE;//TRUE; 
						Wa_prior.rating = 0; //40.0; 
						Wa_prior.sigma = 200.0; //20;
						switch_W = TRUE;
						break;
			case 'D': 	if (1 != sscanf(opt_arg,"%d", &OUTDECIMALS) || OUTDECIMALS < 0) {
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
	if (NULL != priorsstr && General_average_set) {
		fprintf (stderr, "Setting a general average (-a) is incompatible with having a file with rating seeds (-y)\n\n");
		exit(EXIT_FAILURE);
	}				

	/*==== SET INPUT ====*/

	if (!pgn_getresults(inputf, QUIET_MODE)) {
		fprintf (stderr, "Problems reading results from: %s\n", inputf);
		return EXIT_FAILURE; 
	}
	
	transform_DB(&DB, &Game_stats); /* convert DB to global variables */

	if (Anchor_use) {
		if (!find_anchor_player(&Anchor)) {
			fprintf (stderr, "ERROR: No games of anchor player, mispelled, wrong capital letters, or extra spaces = \"%s\"\n", Anchor_name);
			fprintf (stderr, "Surround the name with \"quotes\" if it contains spaces\n\n");
			return EXIT_FAILURE; 			
		} 
	}


	if (!QUIET_MODE) {

		N_encounters = calc_encounters(ENCOUNTERS_FULL, Encounter);

		printf ("Total games         %8ld\n", Game_stats.white_wins
											 +Game_stats.draws
											 +Game_stats.black_wins
											 +Game_stats.noresult);
		printf (" - White wins       %8ld\n", Game_stats.white_wins);
		printf (" - Draws            %8ld\n", Game_stats.draws);
		printf (" - Black wins       %8ld\n", Game_stats.black_wins);
		printf (" - Truncated        %8ld\n", Game_stats.noresult);
		printf ("Unique head to head %8.2f%s\n", 100.0*N_encounters/N_games, "%");
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

	// multiple anchors, do after priors
	if (pinsstr != NULL) {
		init_manchors(pinsstr); 
	}

//~~
	if (relstr != NULL) {
		relpriors_load(relstr); 
	}
	relpriors_show();
	//FIXME do not allow relpriors to be purged
//~~

	if (switch_w && switch_u) {
		if (White_advantage_SD > PRIOR_SMALLEST_SIGMA) {
			Wa_prior.set = TRUE; 
			Wa_prior.rating = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			ADJUST_WHITE_ADVANTAGE = TRUE;	
		} else {
			Wa_prior.set = FALSE; 
			Wa_prior.rating = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			ADJUST_WHITE_ADVANTAGE = FALSE;	
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
		for (i = 0; i < N_players; i++) {
			Sum1[i] = 0;
			Sum2[i] = 0;
			Sdev[i] = 0;
		}
	}

	/*===== GROUPS ========*/

	N_encounters = calc_encounters(ENCOUNTERS_FULL, Encounter);
	scan_encounters(Encounter, N_encounters, N_players); 
	if (group_is_output) {

		static struct ENC 		Encounter2[MAXENCOUNTERS];
		static int 				N_encounters2 = 0;
		static struct ENC 		Encounter3[MAXENCOUNTERS];
		static int 				N_encounters3 = 0;

		convert_to_groups(groupf, N_players, Name);
		sieve_encounters(Encounter, N_encounters, Encounter2, &N_encounters2, Encounter3, &N_encounters3);
		printf ("Encounters, Total=%d, Main=%d, @ Interface between groups=%d\n",N_encounters, N_encounters2, N_encounters3);

		if (textstr == NULL && csvstr == NULL)	
			exit(EXIT_SUCCESS);
	}

	/*=====================*/

	N_encounters = set_super_players(QUIET_MODE, Encounter);
	N_encounters = purge_players(QUIET_MODE, Encounter);
	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, Encounter);
	N_encounters = calc_rating(QUIET_MODE, Encounter, N_encounters);

	ratings_results();

//FIXME
#if defined(WADV_RECALC)
	if (ADJUST_WHITE_ADVANTAGE && switch_W) {
		double new_wadv = adjust_wadv (White_advantage);
		if (!QUIET_MODE)
			printf ("\nAdjusted White Advantage = %f\n\n",new_wadv);
		White_advantage = new_wadv;
	
		N_encounters = purge_players(QUIET_MODE, Encounter);
		N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, Encounter);
		N_encounters = calc_rating(QUIET_MODE, Encounter, N_encounters);
		ratings_results();
	}
#endif

	/* Simulation block, begin */
	{	
		long i,j;
		long z = Simulate;
		double n = (double) (z);
		ptrdiff_t est = (ptrdiff_t)((N_players*N_players-N_players)/2); /* elements of simulation table */
		ptrdiff_t idx;
		size_t allocsize = sizeof(struct DEVIATION_ACC) * (size_t)est;
		double diff;

		sim = malloc(allocsize);

		if (sim != NULL) {

			for (idx = 0; idx < est; idx++) {
				sim[idx].sum1 = 0;
				sim[idx].sum2 = 0;
				sim[idx].sdev = 0;
			}
	
			if (z > 1) {
				while (z-->0) {
					if (!QUIET_MODE) printf ("\n==> Simulation:%ld/%ld\n",Simulate-z,Simulate);
					clear_flagged ();
					simulate_scores();

					// if ((Simulate-z) == 801) save_simulated((int)(Simulate-z)); 

					N_encounters = set_super_players(QUIET_MODE, Encounter);
					N_encounters = purge_players(QUIET_MODE, Encounter);
					N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, Encounter);
					N_encounters = calc_rating(QUIET_MODE, Encounter, N_encounters);
					ratings_for_purged ();

					for (i = 0; i < N_players; i++) {
						Sum1[i] += Ratingof[i];
						Sum2[i] += Ratingof[i]*Ratingof[i];
					}
	
					for (i = 0; i < (N_players); i++) {
						for (j = 0; j < i; j++) {
							idx = (i*i-i)/2+j;
							assert(idx < est || !printf("idx=%ld est=%ld\n",idx,est));
							diff = Ratingof[i] - Ratingof[j];	
							sim[idx].sum1 += diff; 
							sim[idx].sum2 += diff * diff;
						}
					}
				}

				for (i = 0; i < N_players; i++) {
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
	}
	/* Simulation block, end */

	all_report (csvf, textf);
	if (Simulate > 1 && NULL != ematstr) {
		errorsout (ematstr);
	}

	if (textf_opened) 	fclose (textf);
	if (csvf_opened)  	fclose (csvf); 
	if (groupf_opened) 	fclose(groupf);

	if (sim != NULL) free(sim);

	/*==== END CALCULATION ====*/

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
transform_DB(struct DATA *db, struct GAMESTATS *gs)
{
	int i;
	ptrdiff_t x;
	long int gamestat[4] = {0,0,0,0};

	for (x = 0; x < db->labels_end_idx; x++) {
		Labelbuffer[x] = db->labels[x];
	}
	Labelbuffer_end = Labelbuffer + db->labels_end_idx;
	N_players = db->n_players;
	N_games   = db->n_games;

	for (i = 0; i < db->n_players; i++) {
		Name[i] = Labelbuffer + db->name[i];
		Flagged[i] = FALSE;
	}

	for (i = 0; i < db->n_games; i++) {
		Whiteplayer[i] = db->white[i];
		Blackplayer[i] = db->black[i]; 
		Score[i]       = db->score[i];
		if (Score[i] <= DISCARD) gamestat[Score[i]]++;
	}

	gs->white_wins	= gamestat[WHITE_WIN];
	gs->draws		= gamestat[RESULT_DRAW];
	gs->black_wins	= gamestat[BLACK_WIN];
	gs->noresult	= gamestat[DISCARD];

	return;
}

static bool_t
find_anchor_player(int *anchor)
{
	int i;
	bool_t found = FALSE;
	for (i = 0; i < N_players; i++) {
		if (!strcmp(Name[i], Anchor_name)) {
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

	const double da = Ratingof_results[*ja];
	const double db = Ratingof_results[*jb];
    
	return (da < db) - (da > db);
}

static void
errorsout(const char *out)
{
	FILE *f;
	ptrdiff_t idx;
	long i,j,y,x;

	if (NULL != (f = fopen (out, "w"))) {

		fprintf(f, "\"N\",\"NAME\"");	
		for (i = 0; i < N_players; i++) {
			fprintf(f, ",%ld",i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < N_players; i++) {
			y = Sorted[i];

			fprintf(f, "%ld,\"%21s\"",i,Name[y]);

			for (j = 0; j < i; j++) {
				x = Sorted[j];

				if (y < x) 
					idx = (x*x-x)/2+y;					
				else
					idx = (y*y-y)/2+x;
				fprintf(f,",%.1f",sim[idx].sdev * Confidence_factor);
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
	return Performance_type[j] == PERF_SUPERLOSER || Performance_type[j] == PERF_SUPERWINNER;		
}

static const char *SP_symbolstr[3] = {"<",">"," "};

static const char *
get_super_player_symbolstr(int j)
{
	assert(Performance_type_set);
	if (Performance_type[j] == PERF_SUPERLOSER) {
		return SP_symbolstr[0];
	} else if (Performance_type[j] == PERF_SUPERWINNER) {
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
	i = (int) y;
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

void
all_report (FILE *csvf, FILE *textf)
{
	FILE *f;
	int i, j;
	size_t ml;
	char sdev_str_buffer[80];
	const char *sdev_str;

	int rank = 0;
	bool_t showrank;

	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, Encounter);
	calc_obtained_playedby(Encounter, N_encounters);

	for (j = 0; j < N_players; j++) {
		Sorted[j] = j;
	}

	qsort (Sorted, (size_t)N_players, sizeof (Sorted[0]), compareit);

	/* output in text format */
	f = textf;
	if (f != NULL) {

		ml = find_maxlen (Name, N_players);
		//intf ("max length=%ld\n", ml);
		if (ml > 50) ml = 50;

		if (Simulate < 2) {
			fprintf(f, "\n%s %-*s    :%7s %9s %7s %6s\n", 
				"   #", 			
				(int)ml,
				"PLAYER", "RATING", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {

				j = Sorted[i];
				if (!Flagged[j]) {

					char sbuffer[80];
					showrank = !is_old_version(j);
					if (showrank) {
						rank++;
						sprintf(sbuffer,"%d",rank);
					} else {
						sbuffer[0] = '\0';
					}

					if (showrank || !hide_old_ver) {
						fprintf(f, "%4s %-*s %s :%7.*f %9.1f %7d %6.1f%s\n", 
							sbuffer,
							(int)ml+1,
							Name[j],
							get_super_player_symbolstr(j),
							OUTDECIMALS,
							rating_round(Ratingof_results[j], OUTDECIMALS), 
							Obtained_results[j], Playedby_results[j]
							, Playedby_results[j]==0? 0: 100.0*Obtained_results[j]/Playedby_results[j], "%");
					}
	
				} else {
				fprintf(f, "%4d %-*s   :%7s %9s %7s %6s%s\n", 
					i+1,
					(int)ml+1,
					Name[j], 
					"----", "----", "----", "----","%");
				}
			}
		} else {
			fprintf(f, "\n%s %-*s    :%7s %6s %8s %7s %6s\n", 
				"   #", 
				(int)ml, 
				"PLAYER", "RATING", "ERROR", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {
				j = Sorted[i];
				if (Sdev[j] > 0.00000001) {
					sprintf(sdev_str_buffer, "%6.1f", Sdev[j] * Confidence_factor);
					sdev_str = sdev_str_buffer;
				} else {
					sdev_str = "  ----";
				}
				if (!Flagged[j]) {
				fprintf(f, "%4d %-*s %s :%7.*f %s %8.1f %7d %6.1f%s\n", 
					i+1,
					(int)ml+1, 
					Name[j],
 					get_super_player_symbolstr(j),
					OUTDECIMALS,
					rating_round(Ratingof_results[j], OUTDECIMALS), 
					sdev_str, Obtained_results[j], Playedby_results[j], 
					Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], "%");
				} else if (!is_super_player(j)) {
				fprintf(f, "%4d %-*s   :%7.*f %s %8.1f %7d %6.1f%s\n", 
					i+1,
					(int)ml+1, 
					Name[j], 
					OUTDECIMALS,
					rating_round(Ratingof_results[j], OUTDECIMALS), 
					"  ****", Obtained_results[j], Playedby_results[j], 
					Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], "%");
				} else {
				fprintf(f, "%4d %-*s   :%7s %s %8s %7s %6s%s\n", 
					i+1,
					(int)ml+1,
					Name[j], 
					"----", "  ----", "----", "----", "----","%");
				}
			}
		}

	} /*if*/

	/* output in a comma separated value file */
	f = csvf;
	if (f != NULL) {
		for (i = 0; i < N_players; i++) {
			j = Sorted[i];
			fprintf(f, "\"%s\", %6.1f,"
			",%.2f"
			",%d"
			",%.2f"
			"\n"		
			,Name[j]
			,Ratingof_results[j] 
			,Obtained_results[j]
			,Playedby_results[j]
			,Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j] 
			);
		}
	}

	return;
}

/************************************************************************/

static int compare_ENC (const void * a, const void * b)
{
	const struct ENC *ap = a;
	const struct ENC *bp = b;
	if (ap->wh == bp->wh && ap->bl == bp->bl) return 0;
	if (ap->wh == bp->wh) {
		if (ap->bl > bp->bl) return 1; else return -1;
	} else {	 
		if (ap->wh > bp->wh) return 1; else return -1;
	}
	return 0;	
}

static int
calc_encounters (int selectivity, struct ENC *enc)
{
	int i, e = 0;
	int N_enc;

	for (i = 0; i < N_games; i++) {

		if (Score[i] >= DISCARD) continue;

		if (selectivity == ENCOUNTERS_NOFLAGGED) {
			if (Flagged[Whiteplayer[i]] || Flagged[Blackplayer[i]])
				continue;
		}

		enc[e].W = enc[e].D = enc[e].L = 0;

		switch (Score[i]) {
			case WHITE_WIN: 	enc[e].wscore = 1.0; enc[e].W = 1; break;
			case RESULT_DRAW:	enc[e].wscore = 0.5; enc[e].D = 1; break;
			case BLACK_WIN:		enc[e].wscore = 0.0; enc[e].L = 1; break;
		}

		enc[e].wh = Whiteplayer[i];
		enc[e].bl = Blackplayer[i];
		enc[e].played = 1;
		e++;
	}
	N_enc = e;

	N_enc = shrink_ENC (enc, N_enc);
	qsort (enc, (size_t)N_enc, sizeof(struct ENC), compare_ENC);
	N_enc = shrink_ENC (enc, N_enc);

	return N_enc;
}

static void
calc_obtained_playedby (const struct ENC *enc, int N_enc)
{
	int e, j, w, b;

	for (j = 0; j < N_players; j++) {
		Obtained[j] = 0.0;	
		Playedby[j] = 0;
	}	

	for (e = 0; e < N_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		Obtained[w] += enc[e].wscore;
		Obtained[b] += (double)enc[e].played - enc[e].wscore;

		Playedby[w] += enc[e].played;
		Playedby[b] += enc[e].played;
	}
}

#if 0
static void
calc_expected (const struct ENC *enc, int N_enc)
{
	int e, j, w, b;
	double f;
	double wperf,bperf;

	for (j = 0; j < N_players; j++) {
		Expected[j] = 0.0;	
	}	

	for (e = 0; e < N_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		f = xpect (Ratingof[w] + White_advantage, Ratingof[b]);

		wperf = enc[e].played * f;
		bperf = enc[e].played - wperf;

		Expected [w] += wperf; 
		Expected [b] += bperf; 

	}
}
#endif

static struct ENC 
encounter_merge (const struct ENC *a, const struct ENC *b)
{
		struct ENC r;	
		assert(a->wh == b->wh);
		assert(a->bl == b->bl);
		r.wh = a->wh;
		r.bl = a->bl; 
		r.wscore = a->wscore + b->wscore;
		r.played = a->played + b->played;
		r.W = a->W + b->W;
		r.D = a->D + b->D;
		r.L = a->L + b->L;
		return r;
}

static int
shrink_ENC (struct ENC *enc, int N_enc)
{
	int e, j, g;

	for (j = 0; j < N_players; j++) {Expected[j] = 0.0;}	

	if (N_enc == 0) return 0; 

	g = 0;
	for (e = 1; e < N_enc; e++) {
	
		if (enc[e].wh == enc[g].wh && enc[e].bl == enc[g].bl) {
			enc[g] = encounter_merge (&enc[g], &enc[e]);
		}
		else {
			g++;
			enc[g] = enc[e];
		}
	}
	g++;
	return g; // New N_encounters
}

static int
purge_players(bool_t quiet, struct ENC *enc)
{
	bool_t foundproblem;
	int j;
	int N_enc;

	assert(Performance_type_set);
	do {
		N_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, enc);
		calc_obtained_playedby(enc, N_enc);

		foundproblem = FALSE;
		for (j = 0; j < N_players; j++) {
			if (Flagged[j]) continue;
			if (Performance_type[j] != PERF_NORMAL) {
				Flagged[j]= TRUE;
				if (!quiet) printf ("purge --> %s\n", Name[j]);
				foundproblem = TRUE;
			} 
		}
	} while (foundproblem);

	return N_enc;
}

//=====================================

void
init_rating (void)
{
	int i;
	for (i = 0; i < N_players; i++) {
		Ratingof[i] = General_average;
	}
	for (i = 0; i < N_players; i++) {
		Ratingbk[i] = General_average;
	}
}


static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}
static bool_t getnum(char *p, double *px) 
{ 	float x;
	bool_t ok = 1 == sscanf( p, "%f", &x );
	*px = (double) x;
	return ok;
}

static bool_t
set_anchor (const char *player_name, double x)
{
	int j;
	bool_t found;
	for (j = 0, found = FALSE; !found && j < N_players; j++) {
		found = !strcmp(Name[j], player_name);
		if (found) {
			Prefed[j] = TRUE;
			Ratingof[j] = x;
			Multiple_anchors_present = TRUE;
			Anchored_n++;
		} 
	}
	return found;
}

static bool_t
assign_anchor (char *name_pinned, double x)
{
	bool_t pin_success;
	if (set_anchor (name_pinned, x)) {
		pin_success = TRUE;
		printf ("Anchoring, %s --> %.1lf\n", name_pinned, x);
	} else {
		pin_success = FALSE;
		printf ("Anchoring, %s --> FAILED, name not found in input file\n", name_pinned);					
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

		while (file_success && NULL != fgets(myline, MAX_MYLINE, fpins)) {
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
			pin_success = assign_anchor (name_pinned, x);
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

	for (j = 0, found = FALSE; !found && j < N_players; j++) {
		found = !strcmp(Name[j], player_a);
		if (found) {
			p_a = j;
		} 
	}
	if (!found) return found;

	for (j = 0, found = FALSE; !found && j < N_players; j++) {
		found = !strcmp(Name[j], player_b);
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

static void
relpriors_show(void)
{ 
	int i;
	printf ("\nRelative Anchors {\n");
	for (i = 0; i < N_relative_anchors; i++) {
		int a = Ra[i].player_a;
		int b = Ra[i].player_b;
		printf ("[%s] [%s] = %lf +/= %lf\n",Name[a], Name[b], Ra[i].delta, Ra[i].sigma);
	}
	printf ("}\n");
}

static bool_t
assign_relative_prior (char *s, char *z, double x, double y)
{
	bool_t prior_success = TRUE;
	bool_t suc;

	if (y < 0) {
		printf ("Relative Prior, %s %s --> FAILED, error/uncertainty cannot be negative", s, z);	
		suc = FALSE;
	} else {
		if (y < PRIOR_SMALLEST_SIGMA) {
			fprintf (stderr,"sigma too small\n");
			exit(EXIT_FAILURE);
		} else {
			suc = set_relprior (s, z, x, y);
			if (suc) {
				printf ("Relative Prior, %s, %s --> %.1lf, %.1lf\n", s, z, x, y);
			} else {
				printf ("Relative Prior, %s, %s --> FAILED, name/s not found in input file\n", s, z);					
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

			prior_success = assign_relative_prior (s, z, x, y);

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

// no globals
static double
relative_anchors_unfitness_full(long int n_relative_anchors, const struct relprior *ra, const double *ratingof)
{
	int a, b, i;
	double d, x;
	double accum = 0;
	for (i = 0; i < n_relative_anchors; i++) {
		a = ra[i].player_a;
		b = ra[i].player_b;
		d = ratingof[a] - ratingof[b];
		x = (d - ra[i].delta)/ra[i].sigma;
		accum += 0.5 * x * x;
	}

	return accum;
}

// no globals
static double
relative_anchors_unfitness_j(double R, int j, double *ratingof, long int n_relative_anchors, struct relprior *ra)
{
	int a, b, i;
	double d, x;
	double accum = 0;
	double rem;

	rem = ratingof[j];
	ratingof[j] = R;

	for (i = 0; i < n_relative_anchors; i++) {
		a = ra[i].player_a;
		b = ra[i].player_b;
		if (a == j || b == j) {
			d = ratingof[a] - ratingof[b];
			x = (d - ra[i].delta)/ra[i].sigma;
			accum += 0.5 * x * x;
		}
	}

	ratingof[j] = rem;
	return accum;
}



//== PRIORS ============================================

static void
priors_reset(struct prior *p)
{	int i;
	for (i = 0; i < MAXPLAYERS; i++) {
		p[i].rating = 0;
		p[i].sigma = 1;
		p[i].set = FALSE;
	}
	Some_prior_set = FALSE;
	Priored_n = 0;
}

static bool_t
set_prior (const char *player_name, double x, double sigma)
{
	int j;
	bool_t found;
	assert(sigma > PRIOR_SMALLEST_SIGMA);
	for (j = 0, found = FALSE; !found && j < N_players; j++) {
		found = !strcmp(Name[j], player_name);
		if (found) {
			PP[j].rating = x;
			PP[j].sigma = sigma;
			PP[j].set = TRUE;
			Some_prior_set = TRUE;
			Priored_n++;
		} 
	}

	return found;
}

static bool_t has_a_prior(int j) {return PP[j].set;}

static bool_t
assign_prior (char *name_prior, double x, double y)
{
	bool_t prior_success = TRUE;
	bool_t suc;
	if (y < 0) {
			printf ("Prior, %s --> FAILED, error/uncertainty cannot be negative", name_prior);	
			suc = FALSE;
	} else { 
		if (y < PRIOR_SMALLEST_SIGMA) {
			suc = set_anchor (name_prior, x);
			if (suc) {
				printf ("Anchoring, %s --> %.1lf\n", name_prior, x);
			} else {
				printf ("Prior, %s --> FAILED, name not found in input file\n", name_prior);
			}
		} else {
			suc = set_prior (name_prior, x, y);
			if (suc) {
				printf ("Prior, %s --> %.1lf, %.1lf\n", name_prior, x, y);
			} else {
				printf ("Prior, %s --> FAILED, name not found in input file\n", name_prior);					
			}	
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
				printf ("Failure to input -y file\n");
				exit(EXIT_FAILURE);
			}

			file_success = success;
			prior_success = assign_prior (name_prior, x, y);
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



// ================= Testing Bayes concept 
#if 1

static void get_pWDL(double dr /*delta rating*/, double *pw, double *pd, double *pl);

static double
wdl_probabilities (int ww, int dd, int ll, double pw, double pd, double pl)
{
	return 	 	(ww > 0? ww * log(pw) : 0) 
			+ 	(dd > 0? dd * log(pd) : 0) 
			+ 	(ll > 0? ll * log(pl) : 0)
			;
}

#if 0
static double
calc_bayes_unfitness_partial (struct ENC *enc, int j) 
{
	double pw, pd, pl, accum;
	int e,w,b;

	for (accum = 0, e = 0; e < N_encounters; e++) {
		w = enc[e].wh;	b = enc[e].bl;
		if (b == j || w == j) {
			get_pWDL(Ratingof[w] + White_advantage - Ratingof[b], &pw, &pd, &pl);
			accum += wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);
		}
	}

#if defined(DOPRIOR)
	// Prior
	if (PP[j].set) {
		double x;
		x = (Ratingof[j] - PP[j].rating)/PP[j].sigma;
		accum -= 0.5 * x * x;
	}
#endif

	return -accum;
}
#endif

// no globals
static double
prior_unfitness	( int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
)
{
	int j;
	double x;
	double accum = 0;
	for (j = 0; j < n_players; j++) {
		if (p[j].set) {
			x = (ratingof[j] - p[j].rating)/p[j].sigma;
			accum += 0.5 * x * x;
		}
	}

	if (wa_prior.set) {
		x = (wadv - wa_prior.rating)/wa_prior.sigma;
		accum += 0.5 * x * x;		
	}

	//FIXME this could be slow!
	accum += relative_anchors_unfitness_full(n_relative_anchors, ra, ratingof); //~~

	return accum;
}

// no globals
static double
calc_bayes_unfitness_full	
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
)
{
	double pw, pd, pl, accum;
	int e,w,b, ww,dd,ll;

	for (accum = 0, e = 0; e < n_enc; e++) {
	
		w = enc[e].wh;
		b = enc[e].bl;

		get_pWDL(ratingof[w] + wadv - ratingof[b], &pw, &pd, &pl);

		ww = enc[e].W;
		dd = enc[e].D;
		ll = enc[e].L;

		accum 	+= 	(ww > 0? ww * log(pw) : 0) 
				+ 	(dd > 0? dd * log(pd) : 0) 
				+ 	(ll > 0? ll * log(pl) : 0);
	}
	// Priors
	accum += -prior_unfitness
				( n_players
				, p
				, wadv
				, wa_prior
				, n_relative_anchors
				, ra
				, ratingof
				);

	return -accum;
}

//============================================

double Probarray [MAXPLAYERS] [4];

// no globals
static double
get_extra_unfitness_j (double R, int j, struct prior *p, double *ratingof, long int n_relative_anchors, struct relprior *ra)
{
	double x;
	double u = 0;
	if (p[j].set) {
		x = (R - p[j].rating)/p[j].sigma;
		u = 0.5 * x * x;
	} 

	//FIXME this could be slow!
	u += relative_anchors_unfitness_j(R, j, ratingof, n_relative_anchors, ra); //~~

	return u;
}

// no globals
static void
probarray_reset(int n_players, double probarray[MAXPLAYERS][4])
{
	int j, k;
	for (j = 0; j < n_players; j++) {
		for (k = 0; k < 4; k++) {
			probarray[j][k] = 0;
		}	
	}
}

// no globals
static void
probarray_build(int n_enc, const struct ENC *enc, double inputdelta, double *ratingof, double white_advantage)
{
	double pw, pd, pl, delta;
	double p;
	int e,w,b;

	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh;	b = enc[e].bl;

		delta = 0;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [1] -= p;			
		Probarray [b] [1] -= p;	

		delta = +inputdelta;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [2] -= p;			
		Probarray [b] [0] -= p;	

		delta = -inputdelta;
		get_pWDL(ratingof[w] + delta + white_advantage - ratingof[b], &pw, &pd, &pl);
		p = wdl_probabilities (enc[e].W, enc[e].D, enc[e].L, pw, pd, pl);

		Probarray [w] [0] -= p;			
		Probarray [b] [2] -= p;	

	}
}

// no globals
static double
derivative_single (int j, double delta, double *ratingof, long int n_relative_anchors, struct relprior *ra, double probarray[MAXPLAYERS][4])
{
	double decrem, increm, center;
	double change;

	decrem = probarray [j] [0] + get_extra_unfitness_j (ratingof[j] - delta, j, PP, ratingof, n_relative_anchors, ra);
	center = probarray [j] [1] + get_extra_unfitness_j (ratingof[j]        , j, PP, ratingof, n_relative_anchors, ra);
	increm = probarray [j] [2] + get_extra_unfitness_j (ratingof[j] + delta, j, PP, ratingof, n_relative_anchors, ra);

	if (center < decrem && center < increm) {
		change = decrem > increm? 0.5: -0.5; 
	} else {
		change = decrem > increm? 1: -1; 
	}
	return change;
}

// no globals
static void
derivative_vector_calc 	( double delta
						, int n_encounters
						, const struct ENC *enc
						, int n_players
						, double *ratingof
						, bool_t *flagged
						, bool_t *prefed
						, double white_advantage
						, long int n_relative_anchors
						, struct relprior *ra
						, double probarray[MAXPLAYERS][4]
						, double *vector 
)
{
	int j;
	probarray_reset(n_players, probarray);
	probarray_build(n_encounters, enc, delta, ratingof, white_advantage);

	for (j = 0; j < n_players; j++) {
		if (flagged[j] || prefed[j]) {
			vector[j] = 0.0;
		} else {
			vector[j] = derivative_single (j, delta, ratingof, n_relative_anchors, ra, probarray);
		}
	}	
}

static double fitexcess ( int n_players
						, const struct prior *p
						, double wadv
						, struct prior wa_prior
						, long int n_relative_anchors
						, const struct relprior *ra
						, double *ratingof
						, double *ratingbk
						, const bool_t *flagged);

static void ratings_apply_excess_correction(double excess, int n_players, const bool_t *flagged, double *ratingof /*out*/);

static double absol(double x) {return x < 0? -x: x;}

// no globals
static double
adjust_rating_bayes 
				( double delta
				, bool_t multiple_anchors_present
				, bool_t some_prior_set
				, bool_t anchor_use
				, double general_average 
				, int n_players 
				, const struct prior *p
				, double white_advantage
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *change_vector
				, const bool_t *flagged
				, const bool_t *prefed
				, double *ratingof // out 
				, double *ratingbk // out 
)
{
	int 	j, notflagged;
	double 	excess, average;
	double 	y, ymax = 0;
	double 	accum = 0;

	for (j = 0; j < n_players; j++) {
		if (	flagged[j]	// player previously removed
			|| 	prefed[j]	// already fixed, one of the multiple anchors
		) continue; 

		// max
		y = absol(change_vector[j]);
		if (y > ymax) ymax = y;

		// execute adjustment
		ratingof[j] += delta * change_vector[j];	

	}	

	// Normalization to a common reference (Global --> General_average)
	// The average could be normalized, or the rating of an anchor.
	// Skip in case of multiple anchors present

	if (multiple_anchors_present) {

		excess = 0; // do nothing, was done before

	} else if (some_prior_set) {
 
		excess = fitexcess
					( n_players
					, p
					, white_advantage
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);

	} else if (anchor_use) {

		excess  = ratingof[Anchor] - general_average;

	} else {

		// general average
		for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
			if (!flagged[j]) {
				notflagged++;
				accum += ratingof[j];
			}
		}
		average = accum / notflagged;
		excess  = average - general_average;
	}

	// Correct the excess
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);

	// Return maximum increase/decrease ==> "resolution"
	return ymax * delta;
}
#endif


static void
ratings_apply_excess_correction(double excess, int n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	int j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

// no globals
static double
ufex
				( double excess
				, int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, double *ratingof
				, double *ratingbk
				, const bool_t *flagged
)
{
	double u;
	ratings_backup  (n_players, ratingof, ratingbk);
	ratings_apply_excess_correction(excess, n_players, flagged, ratingof);

	u = prior_unfitness

				( n_players
				, p
				, wadv
				, wa_prior
				, n_relative_anchors
				, ra
				, ratingof
				);

	ratings_restore (n_players, ratingbk, ratingof);
	return u;
}

// no globals
static double
fitexcess 		( int n_players
				, const struct prior *p
				, double wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, double *ratingof
				, double *ratingbk
				, const bool_t *flagged
)
{
	double ub, ut, uc, newb, newt, newc;
	double c = 0;
	double t = c + 100;
	double b = c - 100;

	do {

		ub = ufex 	( b
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);

		uc = ufex 	( c
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);

		ut = ufex 	( t
					, n_players
					, p
					, wadv
					, wa_prior
					, n_relative_anchors
					, ra
					, ratingof
					, ratingbk
					, flagged);


		if (uc <= ub && uc <= ut) {
			newb = (b+c)/4;
			newt = (t+c)/4; 
			newc = c;
		} else if (ub < ut) {
			newb = b - (c - b);
			newc = b;
			newt = c;
		} else {
			newb = c;
			newc = t;
			newt = t + ( t - c);
		}
		c = newc;
		t = newt;
		b = newb;
	} while ((t-b) > MIN_RESOLUTION);

	return c;
}

//==============================================================

double
adjust_rating (double delta, double kappa)
{
	int 	j, notflagged;
	double 	d, excess, average;
	double 	y = 1.0;
	double 	ymax = 0;
	double 	accum = 0;

	/*
	|	1) delta and 2) kappa control convergence speed:
	|	Delta is the standard increase/decrease for each player
	|	But, not every player gets that "delta" since it is modified by
	|	by multiplier "y". The bigger the difference between the expected 
	|	performance and the observed, the bigger the "y". However, this
	|	is controled so y won't be higher than 1.0. It will be asymptotic
	|	to 1.0, and the parameter that controls how fast this saturation is 
	|	reached is "kappa". Smaller kappas will allow to reach 1.0 faster.	
	|
	|	Uses globals:
	|	arrays:	Flagged, Prefed, Expected, Obtained, Playedby, Ratingof
	|	variables: N_players, General_average
	|	flags:	Multiple_anchors_present, Anchor_use
	*/

	for (j = 0; j < N_players; j++) {
		if (	Flagged[j]	// player previously removed
			|| 	Prefed[j]	// already fixed, one of the multiple anchors
		) continue; 

		// find multiplier "y"
		d = (Expected[j] - Obtained[j]) / Playedby[j];
		d = d < 0? -d: d;
		y = d / (kappa + d);
		if (y > ymax) ymax = y;

		// execute adjustment
		if (Expected[j] > Obtained [j]) {
			Ratingof[j] -= delta * y;
		} else {
			Ratingof[j] += delta * y;
		}
	}

	// Normalization to a common reference (Global --> General_average)
	// The average could be normalized, or the rating of an anchor.
	// Skip in case of multiple anchors present

	if (!Multiple_anchors_present && !Some_prior_set) {

		if (Anchor_use) {
			excess  = Ratingof[Anchor] - General_average;
		} else {
			for (notflagged = 0, accum = 0, j = 0; j < N_players; j++) {
				if (!Flagged[j]) {
					notflagged++;
					accum += Ratingof[j];
				}
			}
			average = accum / notflagged;
			excess  = average - General_average;
		}
		for (j = 0; j < N_players; j++) {
			if (!Flagged[j]) Ratingof[j] -= excess;
		}	
	}	

	// Return maximum increase/decrease ==> "resolution"

	return ymax * delta;
}


static void
adjust_rating_byanchor (bool_t anchor_use, int anchor, double general_average)
{
	double excess;
	int j;
	if (anchor_use) {
		excess  = Ratingof[anchor] - general_average;	
		for (j = 0; j < N_players; j++) {
			if (!Flagged[j]) Ratingof[j] -= excess;
		}	
	}
}

// no globals
static void
ratings_restore (int n_players, const double *r_bk, double *r_of)
{
	int j;
	for (j = 0; j < n_players; j++) {
		r_of[j] = r_bk[j];
	}	
}

// no globals
static void
ratings_backup (int n_players, const double *r_of, double *r_bk)
{
	int j;
	for (j = 0; j < n_players; j++) {
		r_bk[j] = r_of[j];
	}	
}

#if 1
static void
ratings_results (void)
{
	int j;
	ratings_for_purged();
	for (j = 0; j < N_players; j++) {
		Ratingof_results[j] = Ratingof[j];
		Obtained_results[j] = Obtained[j];
		Playedby_results[j] = Playedby[j];
	}
}

static void
clear_flagged (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		Flagged[j] = FALSE;
	}	
}

static void
ratings_for_purged (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		if (Flagged[j]) {
			Ratingof[j] = 0;
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


#define DRAWRATE_AT_EQUAL_STRENGTH 0.33
#define DRAWFACTOR (1/(2*(DRAWRATE_AT_EQUAL_STRENGTH))-0.5)

static void
get_pWDL(double dr /*delta rating*/, double *pw, double *pd, double *pl)
{
	// Performance comprises wins and draws.
	// if f is expected performance from 0 to 1.0, then
	// f = pwin + pdraw/2
	// from that, dc is the fraction of points that come from draws, not wins, so
	// pdraw (probability of draw) = 2 * f * dc
	// calculation of dc is an empirical formula to fit average data from CCRL:
	// Draw rate of equal engines is near 0.33, and decays on uneven matches.

	double f;
	double pdra, pwin, plos;
// 	double dc; 
	bool_t switched;
	
	switched = dr < 0;

	if (switched) dr = -dr;

	f = xpect (dr,0);

#if 0
	dc = 0.5 / (0.5 + DRAWFACTOR * exp(dr*BETA));
	pdra = 2 * f * dc;
	pwin = f - pdra/2;
	plos = 1 - pwin - pdra; 
#else
	pwin = f * f;
	plos = (1-f) * (1-f);
	pdra = 1 - pwin - plos;
#endif

	if (switched) {
		*pw = plos;
		*pd = pdra;
		*pl = pwin;
	} else {
		*pw = pwin;
		*pd = pdra;
		*pl = plos;
	}
	return;
}


static void
simulate_scores(void)
{
	long int i, w, b;
	double	*rating = Ratingof_results;
	double pwin, pdraw, plos;

	for (i = 0; i < N_games; i++) {

		if (Score[i] == DISCARD) continue;

		w = Whiteplayer[i];
		b = Blackplayer[i];

		get_pWDL(rating[w] + White_advantage - rating[b], &pwin, &pdraw, &plos);
		Score [i] = rand_threeway_wscore(pwin,pdraw);
	}
}
#endif

double 
deviation (void)
{
	double accum = 0;
	double diff;
	int j;

	for (accum = 0, j = 0; j < N_players; j++) {
		if (!Flagged[j]) {
			diff = Expected[j] - Obtained [j];
			accum += diff * diff / Playedby[j];
		}
	}		
	return accum;
}


//==== CALCULATE INDIVIDUAL RATINGS =========================

#if 1
#define CALCIND_SWSL
#endif

#ifdef CALCIND_SWSL

static double calc_ind_rating(double cume_score, double *rtng, double *weig, int r);
static double calc_ind_rating_superplayer (int perf_type, double x_estimated, double *rtng, double *weig, int r);

static int
rate_super_players(bool_t quiet, struct ENC *enc)
{
	int j, e;
	int myenc_n = 0;
	static struct ENC myenc[MAXENCOUNTERS];
	int N_enc;

	N_enc = calc_encounters(ENCOUNTERS_FULL, enc);
	calc_obtained_playedby(enc, N_enc);

	assert(Performance_type_set);

	for (j = 0; j < N_players; j++) {

		if (Performance_type[j] != PERF_SUPERWINNER && Performance_type[j] != PERF_SUPERLOSER) 
			continue;

		myenc_n = 0; // reset

		if (Performance_type[j] == PERF_SUPERWINNER)
			if (!quiet) printf ("  all wins   --> %s\n", Name[j]);

		if (Performance_type[j] == PERF_SUPERLOSER) 
			if (!quiet) printf ("  all losses --> %s\n", Name[j]);

		for (e = 0; e < N_enc; e++) {
			int w = enc[e].wh;
			int b = enc[e].bl;
			if (j == w /*&& Performance_type[b] == PERF_NORMAL*/) {
				myenc[myenc_n++] = enc[e];
			} else
			if (j == b /*&& Performance_type[w] == PERF_NORMAL*/) {
				myenc[myenc_n++] = enc[e];
			}
		}

		{
			double	cume_score = 0; 
			double	cume_total = 0;
			static double weig[MAXPLAYERS];
			static double rtng[MAXPLAYERS];
			int		r = 0;
 	
			while (myenc_n-->0) {
				int n = myenc_n;
				if (myenc[n].wh == j) {
					int opp = myenc[n].bl;
					weig[r	] = myenc[n].played;
					rtng[r++] = Ratingof[opp] - White_advantage;
					cume_score += myenc[n].wscore;
					cume_total += myenc[n].played;
			 	} else 
				if (myenc[myenc_n].bl == j) {
					int opp = myenc[n].wh;
					weig[r	] = myenc[n].played;
					rtng[r++] = Ratingof[opp] + White_advantage;
					cume_score += myenc[n].played - myenc[n].wscore;
					cume_total += myenc[n].played;
				} else {
					fprintf(stderr,"ERROR!!\n");
					exit(0);
					continue;
				} 
			}

			if (Performance_type[j] == PERF_SUPERWINNER) {
				double ori_estimation = calc_ind_rating (cume_score-0.25, rtng, weig, r); 
				Ratingof[j] = calc_ind_rating_superplayer (PERF_SUPERWINNER, ori_estimation, rtng, weig, r);
			}
			if (Performance_type[j] == PERF_SUPERLOSER) {
				double ori_estimation = calc_ind_rating (cume_score+0.25, rtng, weig, r); 
				Ratingof[j] = calc_ind_rating_superplayer (PERF_SUPERLOSER,  ori_estimation, rtng, weig, r);
			}
			Flagged[j] = FALSE;

		}
	}

	N_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, enc);
	calc_obtained_playedby(enc, N_enc);

	return N_enc;
}
#endif

static bool_t
super_players_present(void)
{ 
	bool_t found = FALSE;
	int j;
	for (j = 0; j < N_players && !found; j++) {
		found = Performance_type[j] == PERF_SUPERWINNER || Performance_type[j] == PERF_SUPERLOSER; 
	}
	return found;
}

static double
adjust_wadv_bayes 
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
);

static int
calc_rating_bayes (bool_t quiet, struct ENC *enc, int N_enc, double *pwadv)
{
	double 	olddev, curdev, outputdev;
	int 	i;
	int		rounds = 10000;
	double 	delta = Rtng_76; //should be proportional to the scale
	double 	denom = 3;
	int 	phase = 0;
	int 	n = 40;
	double 	resol = delta;
	double 	resol_prev = delta;

double white_advantage = *pwadv;

	// initial deviation
	olddev = curdev = calc_bayes_unfitness_full	
							( N_enc
							, enc
							, N_players
							, PP
							, white_advantage
							, Wa_prior
							, N_relative_anchors
							, Ra
							, Ratingof);

	if (!quiet) printf ("Converging...\n\n");
	if (!quiet) printf ("%3s %4s %10s %10s\n", "phase", "iteration", "unfitness","resolution");

	while (n-->0) {

		for (i = 0; i < rounds; i++) {
			double kappa = 1.0;
			ratings_backup  (N_players, Ratingof, Ratingbk);
			olddev = curdev;

			// Calc "Changing" vector
			derivative_vector_calc
						( delta
						, N_encounters
						, enc
						, N_players
						, Ratingof
						, Flagged
						, Prefed
						, white_advantage
						, N_relative_anchors
						, Ra
						, Probarray
						, Changing );


			resol_prev = resol;
			resol = 

					// adjust_rating_bayes(delta*kappa,Changing);
					adjust_rating_bayes 
						( delta*kappa
						, Multiple_anchors_present
						, Some_prior_set
						, Anchor_use
						, General_average 
						, N_players 
						, PP
						, White_advantage
						, Wa_prior
						, N_relative_anchors
						, Ra
						, Changing
						, Flagged
						, Prefed
						, Ratingof // out 
						, Ratingbk // out 
					);


			resol = (resol_prev + resol) / 2;

			curdev = calc_bayes_unfitness_full	
							( N_enc
							, enc
							, N_players
							, PP
							, white_advantage
							, Wa_prior
							, N_relative_anchors
							, Ra
							, Ratingof);

			if (curdev >= olddev) {
				ratings_restore (N_players, Ratingbk, Ratingof);
				curdev = olddev;
				assert (curdev == olddev);
				break;
			} else {
				ratings_backup  (N_players, Ratingof, Ratingbk);
				olddev = curdev;
			}	

			if (resol < MIN_RESOLUTION) break;
		}

		delta /=  denom;
		outputdev = curdev/N_games;

		if (!quiet) {
			printf ("%3d %7d %14.5f", phase, i, outputdev);
			printf ("%11.5f",resol);
			printf ("\n");
		}
		phase++;

		if (ADJUST_WHITE_ADVANTAGE 
//			&& Wa_prior.set // if !Wa_prior set, it will give no error based on the wa
							// but it will adjust it based on the results. This is useful
							// for -W
			) {
			white_advantage = adjust_wadv_bayes 
							( N_enc
							, enc
							, N_players
							, PP
							, white_advantage
							, Wa_prior
							, N_relative_anchors
							, Ra
							, Ratingof
							, resol);

			*pwadv = white_advantage;
		}

		if (resol < MIN_RESOLUTION) break;

	}

	if (!quiet) printf ("done\n\n");

	printf ("White_advantage = %lf\n\n", white_advantage);

#ifdef CALCIND_SWSL
	if (!quiet && super_players_present()) printf ("Post-Convergence rating estimation for all-wins / all-losses players\n\n");
	N_enc = rate_super_players(QUIET_MODE, enc);
#endif

	if (!Multiple_anchors_present && !Some_prior_set)
		adjust_rating_byanchor (Anchor_use, Anchor, General_average);

	return N_enc;
}

static int
calc_rating (bool_t quiet, struct ENC *enc, int N_enc)
{
	return calc_rating_bayes (quiet, enc, N_enc, &White_advantage);
}

#if 0
static int
calc_rating_ori (bool_t quiet, struct ENC *enc, int N_enc)
{
	double 	olddev, curdev, outputdev;
	int 	i;
	int		rounds = 10000;
	double 	delta = 200.0;
	double 	kappa = 0.05;
	double 	denom = 2;
	int 	phase = 0;
	int 	n = 20;
	double resol;

	calc_obtained_playedby(enc, N_enc);
	calc_expected(enc, N_enc);
	olddev = curdev = deviation();

	if (!quiet) printf ("\nConvergence rating calculation\n\n");
	if (!quiet) printf ("%3s %4s %10s %10s\n", "phase", "iteration", "deviation","resolution");

	while (n-->0) {
		double kk = 1.0;
		for (i = 0; i < rounds; i++) {

			ratings_backup  (N_players, Ratingof, Ratingbk);
			olddev = curdev;

			resol = adjust_rating(delta,kappa*kk);
			calc_expected(enc, N_enc);
			curdev = deviation();

			if (curdev >= olddev) {
				ratings_restore (N_players, Ratingbk, Ratingof);
				calc_expected(enc, N_enc);
				curdev = deviation();	
				assert (curdev == olddev);
				break;
			};	

			outputdev = 1000*sqrt(curdev/N_games);
			if (outputdev < 0.000001) break;
			kk *= 0.995;
		}

		delta /= denom;
		kappa *= denom;
		outputdev = 1000*sqrt(curdev/N_games);

		if (!quiet) {
			printf ("%3d %7d %14.5f", phase, i, outputdev);
			printf ("%11.5f",resol);
			printf ("\n");
		}
		phase++;

		if (outputdev < 0.000001) break;
	}

	if (!quiet) printf ("done\n\n");

#ifdef CALCIND_SWSL
	if (!quiet) printf ("Post-Convergence rating estimation\n\n");
	N_enc = rate_super_players(QUIET_MODE, enc);
#endif

	if (!Multiple_anchors_present)
		adjust_rating_byanchor (Anchor_use, Anchor, General_average);

	return N_enc;
}
#endif

static double
xpect (double a, double b)
{
	return 1.0 / (1.0 + exp((b-a)*BETA));
}

/*==================================================================*/
#if defined(WADV_RECALC)
static double
overallerror_fwadv (double wadv)
{
	int i;
	double e, e2, f, rw, rb;
	double s[3];

	s[WHITE_WIN] = 1.0;
	s[BLACK_WIN] = 0.0;
	s[RESULT_DRAW] = 0.5;

	assert (WHITE_WIN < 3);
	assert (BLACK_WIN < 3);
	assert (RESULT_DRAW < 3);
	assert (DISCARD > 2);

	e2 = 0;

	for (i = 0; i < N_games; i++) {

		if (Score[i] > 2) continue;

		rw = Ratingof[Whiteplayer[i]];
		rb = Ratingof[Blackplayer[i]];

		f = xpect (rw + wadv, rb);

		e   = f - s[Score[i]];
		e2 += e * e;
	}

	return e2;
}

static double
adjust_wadv (double start_wadv)
{
	double delta, wa, ei, ej, ek;

	delta = 100.0;
	wa = start_wadv;

	do {	
		ei = overallerror_fwadv (wa - delta);
		ej = overallerror_fwadv (wa);
		ek = overallerror_fwadv (wa + delta);

		if (ei >= ej && ej <= ek) {
			delta = delta / 2;
		} else
		if (ej >= ei && ei <= ek) {
			wa -= delta;
		} else
		if (ei >= ek && ek <= ej) {
			wa += delta;
		}

	} while (delta > 0.01 && -1000 < wa && wa < 1000);
	
	return wa;
}
#endif

// no globals
static double
adjust_wadv_bayes 
				( int n_enc
				, const struct ENC *enc
				, int n_players
				, const struct prior *p
				, double start_wadv
				, struct prior wa_prior
				, long int n_relative_anchors
				, const struct relprior *ra
				, const double *ratingof
				, double resol
)
{
	double delta, wa, ei, ej, ek;

	delta = resol;
	wa = start_wadv;

	do {	
//		ei = calc_bayes_unfitness_full (enc, wa - delta);
//		ej = calc_bayes_unfitness_full (enc, wa        );
//		ek = calc_bayes_unfitness_full (enc, wa + delta);

		ei = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa - delta //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof);

		ej = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa         //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof);

		ek = calc_bayes_unfitness_full	
							( n_enc
							, enc
							, n_players
							, p
							, wa + delta //
							, wa_prior
							, n_relative_anchors
							, ra
							, ratingof);

		if (ei >= ej && ej <= ek) {
			delta = delta / 4;
		} else
		if (ej >= ei && ei <= ek) {
			wa -= delta;
		} else
		if (ei >= ek && ek <= ej) {
			wa += delta;
		}

	} while (delta > resol/10 && -1000 < wa && wa < 1000);
	
	return wa;
}

static double inv_xpect(double invbeta, double p) {return (-1.0*invbeta) * log(100.0/p-1.0);}

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


static int		
set_super_players(bool_t quiet, struct ENC *enc)
{
	static double 	obt [MAXPLAYERS];
	static int		pla [MAXPLAYERS];

	int e, j, w, b;
	int N_enc;

	N_enc = calc_encounters(ENCOUNTERS_FULL, enc);

	for (j = 0; j < N_players; j++) {
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

	for (j = 0; j < N_players; j++) {
		Performance_type[j] = PERF_NORMAL;
		if (obt[j] < 0.001) {
			Performance_type[j] = has_a_prior(j)? PERF_NORMAL: PERF_SUPERLOSER;			
			if (!quiet) printf ("detected (all-losses player) --> %s: seed rating present = %s\n", Name[j], has_a_prior(j)? "Yes":"No");
		}	
		if (pla[j] - obt[j] < 0.001) {
			Performance_type[j] = has_a_prior(j)? PERF_NORMAL: PERF_SUPERWINNER;
			if (!quiet) printf ("detected (all-wins player)   --> %s: seed rating present = %s\n", Name[j], has_a_prior(j)? "Yes":"No");

		}
	}

	for (j = 0; j < N_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	

	Performance_type_set = TRUE;

	return N_enc;
}

//**************************************************************

#if 1
static double
ind_expected (double x, double *rtng, double *weig, int n)
{
	int i;
	double cume = 0;
	for (i = 0; i < n; i++) {
		cume += weig[i] * xpect (x, rtng[i]);
	}
	return cume;
}

static double 
adjust_x (double x, double xp, double sc, double delta, double kappa)
{
	double y;
	double	d;
	d = xp - sc;
	
	d = d < 0? -d: d;
	y = d / (kappa+d);
	if (xp > sc) {
		x -= delta * y;
	} else {
		x += delta * y;
	}
	return x;	
}


static double
calc_ind_rating(double cume_score, double *rtng, double *weig, int r)
{
	int 	i;
	double 	olddev, curdev;
	int		rounds = 10000;
	double 	delta = 200.0;
	double 	kappa = 0.05;
	double 	denom = 2;
	int 	phase = 0;
	int 	n = 20;

	double  D,sc,oldx;

	double x = 2000;
	double xp;

	D = cume_score - ind_expected(x,rtng,weig,r) ;
	curdev = D*D;
	olddev = curdev;

	while (n-->0) {
		double kk = 1.0;
		for (i = 0; i < rounds; i++) {
			oldx = x;
			olddev = curdev;

			sc = cume_score;
			xp = ind_expected(x,rtng,weig,r);
			x  = adjust_x (x, xp, sc, delta, kappa*kk);
			xp = ind_expected(x,rtng,weig,r);
			D = xp - sc;
			curdev = D*D;

			if (curdev >= olddev) {
				x = oldx;
				D = cume_score - ind_expected(x,rtng,weig,r) ;
				curdev = D*D;	
				assert (curdev == olddev);
				break;
			};	

			if (curdev < 0.000001) break;
			kk *= 0.995;
		}

		delta /= denom;
		kappa *= denom;

		phase++;

		if (curdev < 0.000001) break;
	}

	return x;
}
#endif

//=========================================

static double
prob2absolute_result (int perftype, double myrating, double *rtng, double *weig, int n)
{
	int i;
	double p, cume;
	double pwin, pdraw, ploss;
	assert(n);
	assert(perftype == PERF_SUPERWINNER || perftype == PERF_SUPERLOSER);

	cume = 1.0;
	if (PERF_SUPERWINNER == perftype) {
		for (i = 0; i < n; i++) {
			get_pWDL(myrating - rtng[i], &pwin, &pdraw, &ploss);
			p = pwin;
			cume *= exp(weig[i] * log (p)); // p ^ weight
		}	
	} else {
		for (i = 0; i < n; i++) {
			get_pWDL(myrating - rtng[i], &pwin, &pdraw, &ploss);
			p = ploss;
			cume *= exp(weig[i] * log (p)); // p ^ weight
		}
	}
	return cume;
}


static double
calc_ind_rating_superplayer (int perf_type, double x_estimated, double *rtng, double *weig, int r)
{
	int 	i;
	double 	old_unfit, cur_unfit;
	int		rounds = 2000;
	double 	delta = 200.0;
	double 	denom = 2;
	double fdelta;
	double  D, oldx;

	double x = x_estimated;

	if (perf_type == PERF_SUPERLOSER) 
		D = - 0.5 + prob2absolute_result(perf_type, x, rtng, weig, r);		
	else
		D = + 0.5 - prob2absolute_result(perf_type, x, rtng, weig, r);

	cur_unfit = D * D;
	old_unfit = cur_unfit;

	fdelta = D < 0? -delta:  delta;	

	for (i = 0; i < rounds; i++) {

		oldx = x;
		old_unfit = cur_unfit;

		x += fdelta;

		if (perf_type == PERF_SUPERLOSER) 
			D = - 0.5 + prob2absolute_result(perf_type, x, rtng, weig, r);		
		else
			D = + 0.5 - prob2absolute_result(perf_type, x, rtng, weig, r);

		cur_unfit = D * D;
		fdelta = D < 0? -delta: delta;


		if (cur_unfit >= old_unfit) {
			x = oldx;
			cur_unfit = old_unfit;
			delta /= denom;

		} else {

		}	

		if (cur_unfit < 0.0000000001) break;
	}

	return x;
}

