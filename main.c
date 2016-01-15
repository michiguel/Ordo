/*
	Ordo is program for calculating ratings of engine or chess players
    Copyright 2015 Miguel A. Ballicora

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
#include "mymem.h"
#include "report.h"
#include "plyrs.h"
#include "namehash.h"
#include "relprior.h"
#include "inidone.h"
#include "rtngcalc.h"
#include "ra.h"
#include "sim.h"
#include "summations.h"
#include "myopt.h"
#include "sysport/sysport.h"

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

#if 0
#define SAVE_SIMULATION
#define SAVE_SIMULATION_N 2
#endif

const char *license_str = "\n"
"   Copyright (c) 2015 Miguel A. Ballicora\n"
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

	static const char *copyright_str = 
		"Copyright (c) 2015 Miguel A. Ballicora\n"
		"There is NO WARRANTY of any kind\n"
		;

	static const char *intro_str =
		"Program to calculate individual ratings\n"
		;

	static const char *example_options = 
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
		" -G          force program to run and ignore warnings for isolated groups \n"
		" -j <file>   output file with head to head information\n"
		" -s  #       perform # simulations to calculate errors\n"
		" -e <file>   save an error matrix, if -s was used\n"
		" -C <file>   save a matrix (.csv) with confidence for superiority (-s was used)\n"
		" -J          add an output column with confidence for superiority (next player)\n"
		" -F <value>  confidence (%) to estimate error margins. Default is 95.0\n"
		" -X          ignore draws\n"
		" -t <value>  threshold of minimum games played for a participant to be included\n"
		" -N <value>  number of decimals in output, minimum is 0 (default=1)\n"
		" -M          force maximum-likelihood estimation to obtain ratings\n"
		" -n <value>  number of processors for parallel calculation of simulations\n"
		" -U <a,..,z> info in output. Default columns are \"1,2,3,4,5\"\n"
		" -Y <file>   Name synonyms (csv format). Each line: main,syn1,syn2 etc.\n"
		"\n"
		;

	static const char *usage_options = 
		"[-OPTION]";
		;
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		

	static const char *OPTION_LIST = "vhHp:qQWDLa:A:Vm:r:y:Ro:EGg:j:c:s:w:u:d:k:z:e:C:JTF:Xt:N:Mn:U:Y:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/

enum 			AnchorSZ	{MAX_ANCHORSIZE=1024};
static bool_t	Anchor_use = FALSE;
static player_t	Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

static bool_t	Anchor_err_rel2avg = FALSE;

static bool_t	General_average_set = FALSE;

static double	Confidence = 95;
static double	General_average = 2300.0;

static long 	Simulate = 0;

#define INVBETA 175.25

static double	White_advantage = 0;
static double	White_advantage_SD = 0;
static double	Rtng_76 = 202;
static double	BETA = 1.0/INVBETA;
static double	Confidence_factor = 1.0;

static int		OUTDECIMALS = 1;
static bool_t	Decimals_set = FALSE;

static struct GAMESTATS	Game_stats;

static double 	Drawrate_evenmatch = STANDARD_DRAWRATE; //default
static double 	Drawrate_evenmatch_percent = 100*STANDARD_DRAWRATE; //default
static double 	Drawrate_evenmatch_percent_SD = 0;

/*------------------------------------------------------------------------*/

static struct prior Wa_prior = {40.0,20.0,FALSE};
static struct prior Dr_prior = { 0.5, 0.1,FALSE};

static struct prior *PP;		// to be dynamically assigned
static struct prior *PP_store; 	// to be dynamically assigned

static struct rel_prior_set		RPset = {0, NULL};
static struct rel_prior_set		RPset_store = {0, NULL};;

static bool_t 	Hide_old_ver = FALSE;
static bool_t	Prior_mode;

/*---- static functions --------------------------------------------------*/

static void 		table_output(double Rtng_76);

static int 			compare_GAME (const void * a, const void * b);

static void
calc_encounters__
				( int selectivity
				, const struct GAMES *g
				, const bool_t *flagged
				, struct ENCOUNTERS	*e
) 
{
	e->n = 
	calc_encounters	( selectivity
					, g
					, flagged
					, e->enc);
}


/*
|
|	MAIN
|
\*--------------------------------------------------------------*/


int main (int argc, char *argv[])
{
	enum mainlimits {INPUTMAX=1024, COLSMAX=256};

	struct summations sfe; // summations for errors

	struct DATA *pdaba;

	struct GAMES 		Games;
	struct PLAYERS 		Players;
	struct RATINGS 		RA;
	struct ENCOUNTERS 	Encounters;

	double white_advantage_result;
	double drawrate_evenmatch_result;

	struct output_qualifiers outqual = {FALSE, 0};
	long int mingames = 0;

	bool_t csvf_opened;
	bool_t textf_opened;
	bool_t groupf_opened;
	FILE *csvf;
	FILE *textf;
	FILE *groupf;

	bool_t quiet_mode;
	bool_t sim_updates;
	bool_t adjust_white_advantage;
	bool_t adjust_draw_rate;

	int input_n;
	const char *inputfile[INPUTMAX+1];

	int columns_n;
	int columns[COLSMAX+1];

	int cpus = 1;
	int op;
	const char *textstr, *csvstr, *ematstr, *groupstr, *pinsstr;
	const char *priorsstr, *relstr;
	const char *head2head_str;
	const char *ctsmatstr, *synstr;
	const char *output_columns;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;
	bool_t group_is_output, Elostat_output, Ignore_draws, groups_no_check, Forces_ML, cfs_column;
	bool_t switch_w=FALSE, switch_W=FALSE, switch_u=FALSE, switch_d=FALSE, switch_k=FALSE, switch_D=FALSE;

	/* defaults */
	input_n = 0;
	inputfile[input_n] = NULL;
	adjust_white_advantage = FALSE;
	adjust_draw_rate = FALSE;
	quiet_mode   	= FALSE;
	sim_updates  	= FALSE;
	version_mode 	= FALSE;
	license_mode 	= FALSE;
	help_mode    	= FALSE;
	switch_mode  	= FALSE;
	input_mode   	= FALSE;
	table_mode   	= FALSE;
	textstr 	 	= NULL;
	csvstr       	= NULL;
	ematstr 	 	= NULL;
	ctsmatstr	 	= NULL;
	pinsstr		 	= NULL;
	priorsstr	 	= NULL;
	relstr		 	= NULL;
	synstr			= NULL;
	output_columns  = NULL;
	group_is_output = FALSE;
	groups_no_check = FALSE;
	groupstr 	 	= NULL;
	Elostat_output 	= FALSE;
	head2head_str	= NULL;
	Ignore_draws 	= FALSE;
	Forces_ML 	 	= FALSE;
	cfs_column      = FALSE;

	while (END_OF_OPTIONS != (op = options (argc, argv, OPTION_LIST))) {
		switch (op) {
			case 'v':	version_mode = TRUE; 	break;
			case 'L':	version_mode = TRUE; 	
						license_mode = TRUE;
						break;
			case 'h':	help_mode = TRUE;		break;
			case 'H':	switch_mode = TRUE;		break;
			case 'p': 	input_mode = TRUE;
						inputfile[input_n++] = opt_arg;
						inputfile[input_n] = NULL;
						break;
			case 'c': 	csvstr = opt_arg;
						break;
			case 'o': 	textstr = opt_arg;
						break;
			case 'g': 	group_is_output = TRUE;
						groupstr = opt_arg;
						break;
			case 'G': 	groups_no_check = TRUE;
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
							mystrncpy (Anchor_name, opt_arg, MAX_ANCHORSIZE-1);
							Anchor_use = TRUE;
						} else {
							fprintf(stderr, "ERROR: anchor name is too long\n");
							exit(EXIT_FAILURE);	
						}
						break;
			case 'V':	Anchor_err_rel2avg = TRUE;
						break;
			case 'M':	Forces_ML = TRUE;
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
							adjust_white_advantage = FALSE;	
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
							adjust_draw_rate = FALSE;	
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
			case 'J':	cfs_column = TRUE;	break;
			case 'T':	table_mode = TRUE;	break;
			case 'q':	quiet_mode = TRUE;	break;
			case 'Q':	quiet_mode = TRUE;	sim_updates = TRUE; break;
			case 'R':	Hide_old_ver=TRUE;	break;
			case 'W':	adjust_white_advantage = TRUE;	
						Wa_prior.isset = FALSE;	
						Wa_prior.value = 0; 	 
						Wa_prior.sigma = 200.0; 
						switch_W = TRUE;
						break;
			case 'D':	adjust_draw_rate = TRUE;	
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
						} else {
							Decimals_set = TRUE;
						}
						break;
			case 't': 	if (1 != sscanf(opt_arg,"%ld", &mingames) || mingames < 0) {
							fprintf(stderr, "wrong threshold parameter\n");
							exit(EXIT_FAILURE);
						} else {
							outqual.mingames = (gamesnum_t)mingames;
							outqual.mingames_set = TRUE;
						}
						break;
			case 'n': 	if (1 != sscanf(opt_arg,"%d", &cpus) || cpus < 1) {
							fprintf(stderr, "wrong nuber of processors selected\n");
							exit(EXIT_FAILURE);
						} 
						break;
			case 'U':	output_columns = opt_arg;
						break;
			case 'Y': 	synstr = opt_arg;
						break;
			case '?': 	parameter_error();
						exit(EXIT_FAILURE);
						break;
			default:	fprintf (stderr, "ERROR: %d\n", op);
						exit(EXIT_FAILURE);
						break;
		}		
	}

	/*
	|	Deal with switches & input
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
	if (!input_mode && argc == opt_index) {
		fprintf (stderr, "Need file name to proceed\n\n");
		exit(EXIT_FAILURE);
	}
	if (output_columns != NULL) {
		if (!str2list (output_columns, COLSMAX, &columns_n, columns)) {
			fprintf (stderr, "Columns number provided are wrong or exceeded limit (%d)\n\n", COLSMAX);
			exit(EXIT_FAILURE);		
		}
	} else {
		if (!str2list ("0,1,2,3,4,5", COLSMAX, &columns_n, columns)) {
			fprintf (stderr, "Default number of columns is wrong or exceeded limit (%d)\n\n", COLSMAX);
			exit(EXIT_FAILURE);		
		}
	}
	/* get folder, should be only one at this point */
	while (opt_index < argc && input_n < INPUTMAX) {
		inputfile[input_n++] = argv[opt_index++];
		inputfile[input_n] = NULL;
	}
	if (opt_index < argc && input_n == INPUTMAX) {
		fprintf (stderr, "Too many input files\n\n");
		exit(EXIT_FAILURE);
	}

	/*==== Incorrect combination of switches ====*/

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
	if (group_is_output && groups_no_check) {
		fprintf (stderr, "Switches -g and -G cannot be used at the same time\n\n");
		exit(EXIT_FAILURE);
	}				

	Prior_mode = switch_k || switch_u || NULL != relstr || NULL != priorsstr;

	/*==== mem init ====*/

	if (!name_storage_init()) {
		fprintf (stderr, "Problem initializing buffers for reading names, probably lack of memory.\n");	
		exit(EXIT_FAILURE);
	}

	/*==== set input ====*/

	if (NULL != (pdaba = database_init_frompgn (inputfile, synstr, quiet_mode))) {
		if (0 == pdaba->n_players || 0 == pdaba->n_games) {
			fprintf (stderr, "ERROR: Input file contains no games\n");
			return EXIT_FAILURE; 			
		}
		if (Ignore_draws) database_ignore_draws(pdaba);
	} else {
		fprintf (stderr, "Problems reading results\n");
		return EXIT_FAILURE; 
	}

	/*==== memory initialization ====*/
	{
		player_t mpr 	= pdaba->n_players; 
		player_t mpp 	= pdaba->n_players; 
		gamesnum_t mg  	= pdaba->n_games;
		gamesnum_t me  	= pdaba->n_games;

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
	assert(players_have_clear_flags(&Players));

	/*==== data translation ====*/

	database_transform (pdaba, &Games, &Players, &Game_stats); /* convert database to global variables */
	if (0 == Games.n) {
		fprintf (stderr, "ERROR: Input file contains no games\n");
		return EXIT_FAILURE; 			
	}
	qsort (Games.ga, (size_t)Games.n, sizeof(struct gamei), compare_GAME);

	/*==== more memory initialization ====*/

	if (!supporting_auxmem_init (Players.n, &PP, &PP_store)) {
		ratings_done (&RA);
		games_done (&Games);
		encounters_done (&Encounters);
		players_done(&Players);
		fprintf (stderr, "Could not initialize auxiliary Players memory\n"); exit(0);
	}

	/*==== process anchor ====*/

	if (Anchor_use) {
		player_t anch_idx;
		if (players_name2idx(&Players, Anchor_name, &anch_idx)) {
			Anchor = anch_idx;
			anchor_j (anch_idx, General_average, &RA, &Players);
		} else {
			fprintf (stderr, "ERROR: No games of anchor player, mispelled, wrong capital letters, or extra spaces = \"%s\"\n", Anchor_name);
			fprintf (stderr, "Surround the name with \"quotes\" if it contains spaces\n\n");
			return EXIT_FAILURE; 			
		} 
	}

	/*==== more wrong input ====*/

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

	assert(players_have_clear_flags(&Players));
	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (0 == Encounters.n) {
		fprintf (stderr, "ERROR: Input file contains no games to process\n");
		return EXIT_FAILURE; 			
	}

	/*==== report, input checked ====*/

	if (!quiet_mode) {
		printf ("Total games         %8ld\n",(long)
											 (Game_stats.white_wins
											 +Game_stats.draws
											 +Game_stats.black_wins
											 +Game_stats.noresult));
		printf (" - White wins       %8ld\n", (long) Game_stats.white_wins);
		printf (" - Draws            %8ld\n", (long) Game_stats.draws);
		printf (" - Black wins       %8ld\n", (long) Game_stats.black_wins);
		printf (" - Truncated        %8ld\n", (long) Game_stats.noresult);
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

	ratings_starting_point (Players.n, General_average, &RA);

	// priors
	priors_reset (PP, Players.n);
	if (priorsstr != NULL) {
		priors_load (quiet_mode, priorsstr, &RA, &Players, PP);
	}

	// multiple anchors here
	if (pinsstr != NULL) {
		init_manchors (quiet_mode, pinsstr, &RA, &Players); 
	}

	// relative priors
	if (relstr != NULL) {
		relpriors_init (quiet_mode, &Players, relstr, &RPset, &RPset_store); 
	}

	// show priored information
	if (!quiet_mode) {
		priors_show(&Players, PP, Players.n);
		relpriors_show(&Players, &RPset);
		players_set_priored_info (PP, &RPset, &Players);
	}

	// process draw and white adv. switches
	if (switch_w && switch_u) {
		if (White_advantage_SD > PRIOR_SMALLEST_SIGMA) {
			Wa_prior.isset = TRUE; 
			Wa_prior.value = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			adjust_white_advantage = TRUE;	
		} else {
			Wa_prior.isset = FALSE; 
			Wa_prior.value = White_advantage; 
			Wa_prior.sigma = White_advantage_SD; 
			adjust_white_advantage = FALSE;	
		}
	}

	if (switch_d && switch_k) {
		if (Drawrate_evenmatch_percent_SD > PRIOR_SMALLEST_SIGMA) {
			Dr_prior.isset = TRUE; 
			Dr_prior.value = Drawrate_evenmatch_percent/100.0; 
			Dr_prior.sigma = Drawrate_evenmatch_percent_SD/100.0;  
			adjust_draw_rate = TRUE;	
		} else {
			Dr_prior.isset = FALSE; 
			Dr_prior.value = Drawrate_evenmatch_percent/100.0;  
			Dr_prior.sigma = Drawrate_evenmatch_percent_SD/100.0;  
			adjust_draw_rate = FALSE;	
		}
	}

	// open files
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

	/*
	|  BEGIN...
	\*----------------------------------*/

	mythread_mutex_init		(&Smpcount);
	mythread_mutex_init		(&Groupmtx);
	mythread_mutex_init		(&Summamtx);
	mythread_mutex_init		(&Printmtx);

	randfast_init (1324561);

	BETA = (-log(1.0/0.76-1.0)) / Rtng_76;

	summations_init(&sfe);

	/*===== groups ========*/

	assert(players_have_clear_flags (&Players));
	calc_encounters__ (ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (group_is_output) {
		bool_t ok;
		player_t groups_n;
		gamesnum_t intra;
		gamesnum_t inter;
		ok = groups_process (&Encounters, &Players, groupf, &groups_n, &intra, &inter);
		if (!ok) {
			fprintf (stderr, "not enough memory for encounters allocation\n");
			exit(EXIT_FAILURE);
		}
		if (!quiet_mode) {
			printf ("Groups=%ld\n", (long)groups_n);
			printf ("Encounters: Total=%ld, within groups=%ld, @ interface between groups=%ld\n"
						,(long)Encounters.n, (long)intra, (long)inter);
		}

		if (textstr == NULL && csvstr == NULL)	{
			exit(EXIT_SUCCESS);
		}
 	} else if (!groups_no_check) {
		player_t groups_n;
		bool_t ok;
		ok = groups_process_to_count (&Encounters, &Players, &groups_n);
		if (!ok) {
			fprintf (stderr, "not enough memory for encounters allocation\n");
			exit(EXIT_FAILURE);
		}
		if (groups_n > 1) {
			fprintf (stderr, "\n\n");
			fprintf (stderr, "********************[ WARNING ]**********************\n");
			fprintf (stderr, "*     Database is not well connected by games       *\n");
			fprintf (stderr, "* Run switch -g to find what groups need more games *\n");
			fprintf (stderr, "*****************************************************\n");
			fprintf (stderr, " ====> Groups detected: %ld\n\n\n", (long) groups_n);
		}
	}

	/*==== ratings calc ===*/

	assert(players_have_clear_flags(&Players));
	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	players_set_priored_info (PP, &RPset, &Players);
	if (0 < players_set_super (quiet_mode, &Encounters, &Players)) {
		players_purge (quiet_mode, &Players);
		calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	}

	if (!groups_no_check && !groups_are_ok (&Encounters, &Players)) {
			fprintf (stderr, "\n\n");
			fprintf (stderr, "*************************[ WARNING ]*************************\n");
			fprintf (stderr, "*       Database is not well connected by games...          *\n");
			fprintf (stderr, "* ...even after purging players with all-wins/all-losses    *\n");
			fprintf (stderr, "*                                                           *\n");
			fprintf (stderr, "*    Run switch -g to find what groups need more games      *\n");
			fprintf (stderr, "*   Run switch -G to ignore warnings and force calculation  *\n");
			fprintf (stderr, "*   (Attempting it may be very slow and may not converge)   *\n");
			fprintf (stderr, "*************************************************************\n");
			exit(EXIT_FAILURE);
	}

	Encounters.n = calc_rating 	( quiet_mode
								, Forces_ML || Prior_mode
								, adjust_white_advantage
								, adjust_draw_rate
								, Anchor_use
								, Anchor_err_rel2avg

								, General_average
								, Anchor
								, Priored_n
								, BETA

								, &Encounters
								, &RPset
								, &Players
								, &RA
								, &Games

								, PP
								, Wa_prior
								, Dr_prior

								, &White_advantage
								, &Drawrate_evenmatch

								);

	ratings_results	( Anchor_err_rel2avg
					, Anchor_use 
					, Anchor
					, General_average				
					, &Players
					, &RA);

	white_advantage_result = White_advantage;
	drawrate_evenmatch_result = Drawrate_evenmatch;

	/*== simulation ========*/

	/* Simulation block, begin */
	if (Simulate > 1) {

		simul_smp
				( cpus
				, Simulate
				, sim_updates
				, quiet_mode
				, Forces_ML || Prior_mode
				, adjust_white_advantage
				, adjust_draw_rate
				, Anchor_use
				, Anchor_err_rel2avg

				, General_average
				, Anchor
				, Priored_n
				, BETA

				, drawrate_evenmatch_result
				, white_advantage_result
				, &RPset
				, PP
				, Wa_prior
				, Dr_prior

				, &Encounters
				, &Players
				, &RA
				, &Games

				, RPset_store
				, PP_store

				, &sfe
				);

	}
	/* Simulation block, end */

	if (Simulate > 1) {
		/* retransform database, to restore original data */
		database_transform(pdaba, &Games, &Players, &Game_stats); 
		qsort (Games.ga, (size_t)Games.n, sizeof(struct gamei), compare_GAME);
	
		/* recalculate encounters */
		calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

		players_set_priored_info (PP, &RPset, &Players);
		if (0 < players_set_super (quiet_mode, &Encounters, &Players)) {
			players_purge (quiet_mode, &Players);
			calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
		}
	}


	/*==== reports ====*/

	all_report 	( &Games
				, &Players
				, &RA
				, &RPset
				, &Encounters
				, sfe.sdev
				, Simulate
				, Hide_old_ver
				, Confidence_factor
				, csvf
				, textf
				, white_advantage_result
				, drawrate_evenmatch_result
				, OUTDECIMALS
				, outqual
				, sfe.wa_sdev
				, sfe.dr_sdev
				, sfe.relative
				, cfs_column
				, columns
				);

	#if 0
	look_at_predictions 
				( Encounters.n
				, Encounters.enc
				, RA.ratingof
				, BETA
				, white_advantage_result
				, drawrate_evenmatch_result);
	#endif
	#if 0
	look_at_individual_deviation 
				( Players.n
				, Players.flagged
				, &RA
				, Encounters.enc
				, Encounters.n
				, white_advantage_result
				, BETA);
	#endif

	if (Simulate > 1 && NULL != ematstr) {
		errorsout(&Players, &RA, sfe.relative, ematstr, Confidence_factor);
	}
	if (Simulate > 1 && NULL != ctsmatstr) {
		ctsout (&Players, &RA, sfe.relative, ctsmatstr);
	}

	if (head2head_str != NULL) {
		head2head_output
					( &Games
					, &Players
					, &RA
					, &Encounters
					, sfe.sdev
					, Simulate
					, Confidence_factor
					, &Game_stats
					, sfe.relative
					, head2head_str
					, OUTDECIMALS
					);
	}

	if (Elostat_output) {
		cegt_output	( quiet_mode
					, &Games
					, &Players
					, &RA
					, &Encounters
					, sfe.sdev
					, Simulate
					, Confidence_factor
					, &Game_stats
					, sfe.relative
					, outqual
					, Decimals_set? OUTDECIMALS: 0);
	}

	/*==== clean up ====*/

	if (textf_opened) 	fclose (textf);
	if (csvf_opened)  	fclose (csvf); 
	if (groupf_opened) 	fclose(groupf);

	summations_done(&sfe); 

	if (pdaba != NULL)
		database_done (pdaba);

	ratings_done (&RA);
	games_done (&Games);
	encounters_done (&Encounters);
	players_done (&Players);
	supporting_auxmem_done (&PP, &PP_store);

	if (relstr != NULL)
		relpriors_done2 (&RPset, &RPset_store);

	name_storage_done();

	mythread_mutex_destroy (&Smpcount);
	mythread_mutex_destroy (&Groupmtx);
	mythread_mutex_destroy (&Summamtx);
	mythread_mutex_destroy (&Printmtx);

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

//*==================================================================*/

static int compare_GAME (const void * a, const void * b)
{
	const struct gamei *ap = a;
	const struct gamei *bp = b;
	if (ap->whiteplayer == bp->whiteplayer && ap->blackplayer == bp->blackplayer) return 0;
	if (ap->whiteplayer == bp->whiteplayer) {
		if (ap->blackplayer > bp->blackplayer) return 1; else return -1;
	} else {	 
		if (ap->whiteplayer > bp->whiteplayer) return 1; else return -1;
	}
	return 0;	
}


