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

#define MIN_RESOLUTION 0.000001
#define PRIOR_SMALLEST_SIGMA 0.0000001

#if 1
#define CALCIND_SWSL
#endif

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
		

const char *OPTION_LIST = "vhHp:qWDLa:A:Vm:r:y:Ro:Eg:j:c:s:w:u:d:z:e:C:TF:XN:";

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

static bool_t	Anchor_err_rel2avg = FALSE;

static bool_t 	Multiple_anchors_present = FALSE;
static bool_t	General_average_set = FALSE;

static int		Anchored_n = 0;
static bool_t	Performance_type_set = FALSE;

static int		Performance_type[MAXPLAYERS]; //enum Player_Performance_Type 

/* games */
static int 		Whiteplayer	[MAXGAMES];
static int 		Blackplayer	[MAXGAMES];
static int 		Score		[MAXGAMES];
static int 		N_games = 0;

/* encounters */

static struct ENC	Encounter[MAXENCOUNTERS];
static int 			N_encounters = 0;

/**/
static double	Confidence = 95;
static double	General_average = 2300.0;
static int		Sorted  [MAXPLAYERS]; /* sorted index by rating */
static double	Obtained[MAXPLAYERS];
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

static struct GAMESTATS	Game_stats;

static struct DEVIATION_ACC *sim = NULL;

static double Drawrate_evenmatch = STANDARD_DRAWRATE; //default
static double Drawrate_evenmatch_percent = 100*STANDARD_DRAWRATE; //default

static double Probarray [MAXPLAYERS] [4];

/*------------------------------------------------------------------------*/


static struct prior Wa_prior = {40.0,20.0,FALSE};
static struct prior PP[MAXPLAYERS];
static bool_t Some_prior_set = FALSE;

static int 	Priored_n = 0;

static void priors_reset(struct prior *p);
static bool_t set_prior (const char *prior_name, double x, double sigma);
static void priors_load(const char *fpriors_name);

#define MAX_RELPRIORS 10000

static struct relprior Ra[MAX_RELPRIORS];
static long int N_relative_anchors = 0;
static bool_t Hide_old_ver = FALSE;

static bool_t set_relprior (const char *player_a, const char *player_b, double x, double sigma);
static void relpriors_show(void);
static void relpriors_load(const char *f_name);

/*------------------------------------------------------------------------*/
static int		purge_players    (bool_t quiet, struct ENC *enc);
static int		set_super_players(bool_t quiet, struct ENC *enc);

static void		clear_flagged (void);

static void		all_report (FILE *csvf, FILE *textf);
static void		init_rating (void);
static void		init_manchors(const char *fpins_name);

static int		calc_rating (bool_t quiet, struct ENC *enc, int N_enc, double *pWhite_advantage, bool_t adjust_wadv, double *pDraw_rate);

static void 	ratings_results (void);
static void 	ratings_for_purged (void);

static void		simulate_scores(double deq);
static void		errorsout(const char *out);
static void		ctsout(const char *out);

/*------------------------------------------------------------------------*/

static void 	transform_DB(struct DATA *db, struct GAMESTATS *gs);
static bool_t	find_anchor_player(int *anchor);
static void 	DB_ignore_draws (struct DATA *db);

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
	const char *inputf, *textstr, *csvstr, *ematstr, *groupstr, *pinsstr;
	const char *priorsstr, *relstr;
	const char *head2head_str;
	const char *ctsmatstr;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;
	bool_t group_is_output, Elostat_output, Ignore_draws;
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
			case 'd': 	if (1 != sscanf(opt_arg,"%lf", &Drawrate_evenmatch_percent)) {
							fprintf(stderr, "wrong white drawrate parameter\n");
							exit(EXIT_FAILURE);
						}
						if (Drawrate_evenmatch_percent >= 0.0 && Drawrate_evenmatch_percent <= 100.0) {
							Drawrate_evenmatch = Drawrate_evenmatch_percent/100.0;
						} else {
							fprintf(stderr, "drawrate parameter is out of range\n");
							exit(EXIT_FAILURE);
						}					
						break;
			case 'z': 	if (1 != sscanf(opt_arg,"%lf", &Rtng_76)) {
							fprintf(stderr, "wrong scaling parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'T':	table_mode = TRUE;	break;
			case 'q':	QUIET_MODE = TRUE;	break;
			case 'R':	Hide_old_ver=TRUE;	break;
			case 'W':	ADJUST_WHITE_ADVANTAGE = TRUE;	
						Wa_prior.set = FALSE;//TRUE; 
						Wa_prior.rating = 0; //40.0; 
						Wa_prior.sigma = 200.0; //20;
						switch_W = TRUE;
						break;
			case 'D':	ADJUST_DRAW_RATE = TRUE;	break;
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
	if (NULL != priorsstr && General_average_set) {
		fprintf (stderr, "Setting a general average (-a) is incompatible with having a file with rating seeds (-y)\n\n");
		exit(EXIT_FAILURE);
	}				

	/*==== SET INPUT ====*/

	if (!pgn_getresults(inputf, QUIET_MODE)) {
		fprintf (stderr, "Problems reading results from: %s\n", inputf);
		return EXIT_FAILURE; 
	}
	

	if (Ignore_draws) {
		DB_ignore_draws(&DB);
	}

	transform_DB(&DB, &Game_stats); /* convert DB to global variables */

	if (Anchor_use) {
		if (!find_anchor_player(&Anchor)) {
			fprintf (stderr, "ERROR: No games of anchor player, mispelled, wrong capital letters, or extra spaces = \"%s\"\n", Anchor_name);
			fprintf (stderr, "Surround the name with \"quotes\" if it contains spaces\n\n");
			return EXIT_FAILURE; 			
		} 
	}

	N_encounters = calc_encounters(ENCOUNTERS_FULL, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);

	if (!QUIET_MODE) {
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

	// multiple anchors here
	if (pinsstr != NULL) {
		init_manchors(pinsstr); 
	}
	// multiple anchors done

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

	N_encounters = calc_encounters(ENCOUNTERS_FULL, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);
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
	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);

	N_encounters = calc_rating(QUIET_MODE, Encounter, N_encounters, &White_advantage, ADJUST_WHITE_ADVANTAGE, &Drawrate_evenmatch);

	ratings_results();

	/* Simulation block, begin */
	{	
		long i,j;
		long z = Simulate;
		double n = (double) (z);
		ptrdiff_t est = (ptrdiff_t)((N_players*N_players-N_players)/2); /* elements of simulation table */
		ptrdiff_t idx;
		size_t allocsize = sizeof(struct DEVIATION_ACC) * (size_t)est;
		double diff;
		double sim_draw_rate = Drawrate_evenmatch; // temporarily used and modified

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

					simulate_scores(Drawrate_evenmatch);

					// if ((Simulate-z) == 801) save_simulated((int)(Simulate-z)); 

					N_encounters = set_super_players(QUIET_MODE, Encounter);
					N_encounters = purge_players(QUIET_MODE, Encounter);
					N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);
					N_encounters = calc_rating(QUIET_MODE, Encounter, N_encounters, &White_advantage, FALSE, &sim_draw_rate);
					ratings_for_purged ();

					for (i = 0; i < N_players; i++) {
						Sum1[i] += Ratingof[i];
						Sum2[i] += Ratingof[i]*Ratingof[i];
					}
	
					for (i = 0; i < (N_players); i++) {
						for (j = 0; j < i; j++) {
							//idx = (i*i-i)/2+j;
							idx = head2head_idx_sdev (i, j);
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

		transform_DB(&DB, &Game_stats); /* convert DB to global variables, to restore original data */
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
		for (i = 0; i < N_players; i++) {
			fprintf(f, ",%ld",i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < N_players; i++) {
			y = Sorted[i];

			fprintf(f, "%ld,\"%21s\"",i,Name[y]);

			for (j = 0; j < i; j++) {
				x = Sorted[j];

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
		for (i = 0; i < N_players; i++) {
			fprintf(f, ",%ld",i);		
		}
		fprintf(f, "\n");	

		for (i = 0; i < N_players; i++) {
			y = Sorted[i];
			fprintf(f, "%ld,\"%21s\"",i,Name[y]);

			for (j = 0; j < N_players; j++) {
				double ctrs, sd, dr;
				x = Sorted[j];
				if (x != y) {
					dr = Ratingof_results[y] - Ratingof_results[x];
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

	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);
	calc_obtained_playedby(Encounter, N_encounters, N_players, Obtained, Playedby);

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

					if (showrank
						|| !Hide_old_ver
					){
						fprintf(f, "%4s %-*s %s :%7.*f %9.1f %7d %6.1f%s\n", 
							sbuffer,
							(int)ml+1,
							Name[j],
							get_super_player_symbolstr(j),
							OUTDECIMALS,
							rating_round(Ratingof_results[j], OUTDECIMALS), 
							Obtained_results[j], 
							Playedby_results[j], 
							Playedby_results[j]==0? 0: 100.0*Obtained_results[j]/Playedby_results[j], 
							"%"
						);
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
						sdev_str, 
						Obtained_results[j], 
						Playedby_results[j], 
						Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], 
						"%"
					);
				} else if (!is_super_player(j)) {
					fprintf(f, "%4d %-*s   :%7.*f %s %8.1f %7d %6.1f%s\n", 
						i+1,
						(int)ml+1, 
						Name[j], 
						OUTDECIMALS,
						rating_round(Ratingof_results[j], OUTDECIMALS), 
						"  ****", 
						Obtained_results[j], 
						Playedby_results[j], 
						Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], 
						"%"
					);
				} else {
					fprintf(f, "%4d %-*s   :%7s %s %8s %7s %6s%s\n", 
						i+1,
						(int)ml+1,
						Name[j], 
						"----", "  ----", "----", "----", "----","%"
					);
				}
			}
		}

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
		for (i = 0; i < N_players; i++) {
			j = Sorted[i];

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
			,Name[j]
			,Ratingof_results[j] 
			,sdev_str
			,Obtained_results[j]
			,Playedby_results[j]
			,Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j] 
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
	for (i = 0; i < N_players; i++) {
		Ratingof[i] = General_average;
	}
	for (i = 0; i < N_players; i++) {
		Ratingbk[i] = General_average;
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
			Anchored_n++;
		} 
	}
	return found;
}

static bool_t
assign_anchor (char *name_pinned, double x)
{
	bool_t pin_success = set_anchor (name_pinned, x);
	if (pin_success) {
		Multiple_anchors_present = TRUE;
		printf ("Anchoring, %s --> %.1lf\n", name_pinned, x);
	} else {
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

static int
purge_players(bool_t quiet, struct ENC *enc)
{
	bool_t foundproblem;
	int j;
	int N_enc;

	assert(Performance_type_set);
	do {
		N_enc = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, enc);
		calc_obtained_playedby(enc, N_enc, N_players, Obtained, Playedby);

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

static void
ratings_results (void)
{
	double excess;

	int j;
	ratings_for_purged();
	for (j = 0; j < N_players; j++) {
		Ratingof_results[j] = Ratingof[j];
		Obtained_results[j] = Obtained[j];
		Playedby_results[j] = Playedby[j];
	}

	// shifting ratings to fix the anchor.
	// Only done if the error is relative to the average.
	// Otherwise, it is taken care in the rating calculation already.
	// If Anchor_err_rel2avg is set, shifting in the calculation (later) is deactivated.
	excess = 0.0;
	if (Anchor_err_rel2avg && Anchor_use) {
		excess = General_average - Ratingof_results[Anchor];		
		for (j = 0; j < N_players; j++) {
			Ratingof_results[j] += excess;
		}
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

static void
simulate_scores(double deq)
{
	long int i, w, b;
	double	*rating = Ratingof_results;
	double pwin, pdraw, plos;

	for (i = 0; i < N_games; i++) {

		if (Score[i] == DISCARD) continue;

		w = Whiteplayer[i];
		b = Blackplayer[i];

		get_pWDL(rating[w] + White_advantage - rating[b], &pwin, &pdraw, &plos, deq, BETA);
		Score [i] = rand_threeway_wscore(pwin,pdraw);
	}
}

//==== CALCULATE INDIVIDUAL RATINGS =========================

static int
calc_rating (bool_t quiet, struct ENC *enc, int N_enc, double *pWhite_advantage, bool_t adjust_wadv, double *pDraw_rate)
{
	double dr = *pDraw_rate;

	int ret;
	ret = calc_rating_bayes2 
				( quiet
				, enc
				, N_enc

				, N_players
				, Obtained
				, Playedby
				, Ratingof
				, Ratingbk
				, Performance_type

				, Flagged
				, Prefed

				, pWhite_advantage
				, General_average

				, Multiple_anchors_present
				, Anchor_use && !Anchor_err_rel2avg
				, Anchor
				
				, N_games
				, Score
				, Whiteplayer
				, Blackplayer

				, Name
				, BETA

				, Changing
				, N_relative_anchors
				, PP
				, Probarray 
				, Ra
				, Some_prior_set
				, Wa_prior

				, adjust_wadv

				, ADJUST_DRAW_RATE
				, &dr
	);

	*pDraw_rate = dr;

	if (!quiet) {
		printf ("White advantage = %.2f\n",*pWhite_advantage);
		printf ("Draw rate = %.2f %s\n",100*dr, "%");
	}

	return ret;
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


static int		
set_super_players(bool_t quiet, struct ENC *enc)
{
	static double 	obt [MAXPLAYERS];
	static int		pla [MAXPLAYERS];

	int e, j, w, b;
	int N_enc;

	N_enc = calc_encounters(ENCOUNTERS_FULL, N_games, Score, Flagged, Whiteplayer, Blackplayer, enc);

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

// Function provided for a special out of CEGT organization. EloStat Format

static void cegt_output(void)
{
	struct CEGT cegt;
	int j;

	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);
	calc_obtained_playedby(Encounter, N_encounters, N_players, Obtained, Playedby);
	for (j = 0; j < N_players; j++) {
		Sorted[j] = j;
	}
	qsort (Sorted, (size_t)N_players, sizeof (Sorted[0]), compareit);

	cegt.n_enc = N_encounters;
	cegt.enc = Encounter;
	cegt.simulate = Simulate;
	cegt.n_players = N_players;
	cegt.sorted = Sorted;
	cegt.ratingof_results = Ratingof_results;
	cegt.obtained_results = Obtained_results;
	cegt.playedby_results = Playedby_results;
	cegt.sdev = Sdev; 
	cegt.flagged = Flagged;
	cegt.name = Name;
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

	N_encounters = calc_encounters(ENCOUNTERS_NOFLAGGED, N_games, Score, Flagged, Whiteplayer, Blackplayer, Encounter);
	calc_obtained_playedby(Encounter, N_encounters, N_players, Obtained, Playedby);
	for (j = 0; j < N_players; j++) {
		Sorted[j] = j;
	}
	qsort (Sorted, (size_t)N_players, sizeof (Sorted[0]), compareit);

	cegt.n_enc = N_encounters;
	cegt.enc = Encounter;
	cegt.simulate = Simulate;
	cegt.n_players = N_players;
	cegt.sorted = Sorted;
	cegt.ratingof_results = Ratingof_results;
	cegt.obtained_results = Obtained_results;
	cegt.playedby_results = Playedby_results;
	cegt.sdev = Sdev; 
	cegt.flagged = Flagged;
	cegt.name = Name;
	cegt.confidence_factor = Confidence_factor;

	cegt.gstat = &Game_stats;

	cegt.sim = sim;

	output_report_individual (head2head_str, &cegt, (int)Simulate);
}

