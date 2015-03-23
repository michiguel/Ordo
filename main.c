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

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

#include "myopt.h"

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

	static bool_t QUIET_MODE;
	static bool_t SIM_UPDATES;
	static bool_t ADJUST_WHITE_ADVANTAGE;
	static bool_t ADJUST_DRAW_RATE;

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
		" -F <value>  confidence (%) to estimate error margins. Default is 95.0\n"
		" -X          ignore draws\n"
		" -t <value>  threshold of minimum games played for a participant to be included\n"
		" -N <value>  number of decimals in output, minimum is 0 (default=1)\n"
		" -M          force maximum-likelihood estimation to obtain ratings\n"
		"\n"
		;

	static const char *usage_options = 
		"[-OPTION]";
		;
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		

	static const char *OPTION_LIST = "vhHp:qQWDLa:A:Vm:r:y:Ro:EGg:j:c:s:w:u:d:k:z:e:C:TF:Xt:N:M";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/

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

/*------------------------------------------------------------------------*/

enum 			AnchorSZ	{MAX_ANCHORSIZE=1024};
static bool_t	Anchor_use = FALSE;
static player_t	Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

static bool_t	Anchor_err_rel2avg = FALSE;

static bool_t	General_average_set = FALSE;

static double	Confidence = 95;
static double	General_average = 2300.0;

static double	*Sum1; // to be dynamically assigned
static double	*Sum2; // to be dynamically assigned
static double	*Sdev; // to be dynamically assigned 

static long 	Simulate = 0;

#define INVBETA 175.25

static double	White_advantage = 0;
static double	White_advantage_SD = 0;
static double	Rtng_76 = 202;
static double	BETA = 1.0/INVBETA;
static double	Confidence_factor = 1.0;

static int		OUTDECIMALS = 1;

static struct GAMESTATS	Game_stats;

static struct DEVIATION_ACC *sim = NULL;

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

/*------------------------------------------------------------------------*/

static void	players_purge (bool_t quiet, struct PLAYERS *pl);
static void	players_set_priored_info (const struct prior *pr, const struct rel_prior_set *rps, struct PLAYERS *pl /*@out@*/);
static void	players_flags_reset (struct PLAYERS *pl);

#if !defined(NDEBUG)
static bool_t	players_have_clear_flags (struct PLAYERS *pl);
#endif

static player_t	set_super_players(bool_t quiet, const struct ENCOUNTERS *ee, struct PLAYERS *pl);

static void	init_rating (player_t n, double rat0, struct RATINGS *rat /*@out@*/);
static void	reset_rating (double general_average, player_t n_players, const bool_t *prefed, const bool_t *flagged, double *rating);
static void	ratings_copy (const double *r, player_t n, double *t);

static void 	ratings_results (struct PLAYERS *plyrs, struct RATINGS *rat);
static void	ratings_for_purged (const struct PLAYERS *p, struct RATINGS *r /*@out@*/);

static void
simulate_scores ( const double 	*ratingof_results
				, double 		deq
				, double 		wadv
				, double 		beta
				, struct GAMES *g
);

/*------------------------------------------------------------------------*/

static void 		table_output(double Rtng_76);
static ptrdiff_t	head2head_idx_sdev (ptrdiff_t x, ptrdiff_t y);
static void 		ratings_center_to_zero (player_t n_players, const bool_t *flagged, double *ratingof);

/*------------------------------------------------------------------------*/

#if 0
#define SAVE_SIMULATION
#define SAVE_SIMULATION_N 2
#endif

#if defined(SAVE_SIMULATION)
// This section is to save simulated results for debugging purposes

static const char *Result_string[4] = {"1-0","1/2-1/2","0-1","*"};

static void
save_simulated(struct PLAYERS *pPlayers, struct GAMES *pGames, int num)
{
	gamesnum_t i;
	const char *name_w;
	const char *name_b;
	const char *result;
	char filename[256] = "";	
	FILE *fout;

	sprintf (filename, "simulated_%04d.pgn", num);

	printf ("\n--> filename=%s\n\n",filename);

	if (NULL != (fout = fopen (filename, "w"))) {

		for (i = 0; i < pGames->n; i++) {

			int32_t score_i = pGames->ga[i].score;
			player_t wp_i = pGames->ga[i].whiteplayer;
			player_t bp_i = pGames->ga[i].blackplayer;

			if (score_i == DISCARD) continue;
	
			name_w = pPlayers->name [wp_i];
			name_b = pPlayers->name [bp_i];		
			result = Result_string[score_i];

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

static double
get_sdev (double s1, double s2, double n)
{
	double xx = n*s2 - s1 * s1;
	xx = sqrt(xx*xx); // removes problems with -0.00000000000;
	return sqrt( xx ) /n;
}

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/



int main (int argc, char *argv[])
{
	struct DATA *pdaba;

	struct GAMES 		Games;
	struct PLAYERS 		Players;
	struct RATINGS 		RA;
	struct ENCOUNTERS 	Encounters;

	double white_advantage_result;
	double drawrate_evenmatch_result;
	double wa_sum1;
	double wa_sum2;				
	double dr_sum1;
	double dr_sum2; 
	double wa_sdev = 0;				
	double dr_sdev = 0;

//
	struct output_qualifiers outqual = {FALSE, 0};
	long int mingames = 0;

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
	bool_t group_is_output, Elostat_output, Ignore_draws, groups_no_check, Forces_ML;
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
	groups_no_check = FALSE;
	groupstr 	 = NULL;
	Elostat_output = FALSE;
	head2head_str= NULL;
	Ignore_draws = FALSE;
	Forces_ML 	 = FALSE;

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
			case 't': 	if (1 != sscanf(opt_arg,"%ld", &mingames) || mingames < 0) {
							fprintf(stderr, "wrong threshold parameter\n");
							exit(EXIT_FAILURE);
						} else {
							outqual.mingames = (gamesnum_t)mingames;
							outqual.mingames_set = TRUE;
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

	/*==== SET INPUT ====*/

	if (NULL != (pdaba = database_init_frompgn(inputf, QUIET_MODE))) {
		if (0 == pdaba->n_players || 0 == pdaba->n_games) {
			fprintf (stderr, "ERROR: Input file contains no games\n");
			return EXIT_FAILURE; 			
		}
		if (Ignore_draws) database_ignore_draws(pdaba);
	} else {
		fprintf (stderr, "Problems reading results from: %s\n", inputf);
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
	/**/

	database_transform(pdaba, &Games, &Players, &Game_stats); /* convert database to global variables */
	if (0 == Games.n) {
		fprintf (stderr, "ERROR: Input file contains no games\n");
		return EXIT_FAILURE; 			
	}
	qsort (Games.ga, (size_t)Games.n, sizeof(struct gamei), compare_GAME);


	if (!supporting_auxmem_init (Players.n, &Sum1, &Sum2, &Sdev, &PP, &PP_store)) {
		ratings_done (&RA);
		games_done (&Games);
		encounters_done (&Encounters);
		players_done(&Players);
		fprintf (stderr, "Could not initialize auxiliary Players memory\n"); exit(0);
	}

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

	if (!QUIET_MODE) {
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

	init_rating(Players.n, General_average, &RA);

	// PRIORS
	priors_reset(PP, Players.n);

	if (priorsstr != NULL) {
		priors_load(QUIET_MODE, priorsstr, &RA, &Players, PP);
	}

	// multiple anchors here
	if (pinsstr != NULL) {
		init_manchors(QUIET_MODE, pinsstr, &RA, &Players); 
	}
	// multiple anchors done

	if (relstr != NULL) {
		relpriors_init(QUIET_MODE, &Players, relstr, &RPset, &RPset_store); 
	}

	if (!QUIET_MODE) {
		priors_show(&Players, PP, Players.n);
		relpriors_show(&Players, &RPset);
		players_set_priored_info (PP, &RPset, &Players);
	}

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

	/*==== BEGIN... =======*/

	randfast_init (1324561);

	BETA = (-log(1.0/0.76-1.0)) / Rtng_76;

	{	player_t i;
		for (i = 0; i < Players.n; i++) {
			Sum1[i] = 0;
			Sum2[i] = 0;
			Sdev[i] = 0;
		}
	}
	wa_sum1 = 0;				
	dr_sum1 = 0;
	wa_sum2 = 0;				
	dr_sum2 = 0;

	/*===== GROUPS ========*/

	assert(players_have_clear_flags(&Players));
	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (group_is_output) {
		bool_t ok;
		ok = groups_process (QUIET_MODE, &Encounters, &Players, groupf);
		if (!ok) {
			fprintf (stderr, "not enough memory for encounters allocation\n");
			exit(EXIT_FAILURE);
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

	/*==== Ratings Calc ===*/

	assert(players_have_clear_flags(&Players));
	calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	players_set_priored_info (PP, &RPset, &Players);
	if (0 < set_super_players(QUIET_MODE, &Encounters, &Players)) {
		players_purge (QUIET_MODE, &Players);
		calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	}

	if (!groups_no_check && group_is_problematic (&Encounters, &Players)) {
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

	Encounters.n = calc_rating 	( QUIET_MODE
								, Forces_ML || Prior_mode
								, ADJUST_WHITE_ADVANTAGE
								, ADJUST_DRAW_RATE
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

	ratings_results (&Players, &RA);
	white_advantage_result = White_advantage;
	drawrate_evenmatch_result = Drawrate_evenmatch;

	wa_sum1 += White_advantage;
	wa_sum2 += White_advantage * White_advantage;				
	dr_sum1 += Drawrate_evenmatch;
	dr_sum2 += Drawrate_evenmatch * Drawrate_evenmatch;

	/*=====================*/

	/* Simulation block, begin */
	if (Simulate > 1) {
		int failed_sim = 0;
		ptrdiff_t i,j;
		ptrdiff_t topn = (ptrdiff_t)Players.n;
		long z = Simulate;
		double n = (double) (z);
		ptrdiff_t np = topn;
		ptrdiff_t est = (ptrdiff_t)((np*np-np)/2); /* elements of simulation table */
		ptrdiff_t idx;
		size_t allocsize = sizeof(struct DEVIATION_ACC) * (size_t)est;
		double diff;

		assert(allocsize > 0);
		sim = memnew(allocsize);

		if (sim == NULL) {
			fprintf(stderr, "Memory for simulations could not be allocated\n");
			exit(EXIT_FAILURE);
		} else {
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

					failed_sim = 0;
					do {
						if (!QUIET_MODE && failed_sim > 0) printf("--> Simulation: [Rejected]\n\n");

						players_flags_reset (&Players);

						simulate_scores ( RA.ratingof_results
										, drawrate_evenmatch_result
										, white_advantage_result
										, BETA
										, &Games //out
						);

						relpriors_copy(&RPset, &RPset_store); 
						priors_copy(PP, Players.n, PP_store);
						relpriors_shuffle(&RPset);
						priors_shuffle(PP, Players.n);

						// may improve convergence in pathological cases, it should not be needed.
						reset_rating (General_average, Players.n, Players.prefed, Players.flagged, RA.ratingof);
						reset_rating (General_average, Players.n, Players.prefed, Players.flagged, RA.ratingbk);
						assert(ratings_sanity (Players.n, RA.ratingof));
						assert(ratings_sanity (Players.n, RA.ratingbk));

						assert(players_have_clear_flags(&Players));
						calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

						players_set_priored_info (PP, &RPset, &Players);
						if (0 < set_super_players(QUIET_MODE, &Encounters, &Players)) {
							players_purge (QUIET_MODE, &Players);
							calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
						}

					} while (failed_sim++ < 100 && group_is_problematic (&Encounters, &Players));

					if (!QUIET_MODE) printf("--> Simulation: [Accepted]\n");

					#if defined(SAVE_SIMULATION)
					if ((Simulate-z) == SAVE_SIMULATION_N) {
						save_simulated(&Players, &Games, (int)(Simulate-z)); 
					}
					#endif

					Encounters.n = calc_rating 
								( QUIET_MODE
								, Forces_ML || Prior_mode
								, ADJUST_WHITE_ADVANTAGE
								, ADJUST_DRAW_RATE
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

					ratings_for_purged (&Players, &RA);

					relpriors_copy(&RPset_store, &RPset);
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

					for (i = 0; i < topn; i++) {
						Sum1[i] += RA.ratingof[i];
						Sum2[i] += RA.ratingof[i]*RA.ratingof[i];
					}
					wa_sum1 += White_advantage;
					wa_sum2 += White_advantage * White_advantage;				
					dr_sum1 += Drawrate_evenmatch;
					dr_sum2 += Drawrate_evenmatch * Drawrate_evenmatch;	

					for (i = 0; i < topn; i++) {
						for (j = 0; j < i; j++) {
							//idx = (i*i-i)/2+j;
							idx = head2head_idx_sdev ((ptrdiff_t)i, (ptrdiff_t)j);
							assert(idx < est || !printf("idx=%ld est=%ld\n",(long)idx,(long)est));
							diff = RA.ratingof[i] - RA.ratingof[j];	

							sim[idx].sum1 += diff; 
							sim[idx].sum2 += diff * diff;
						}
					}

					if (Anchor_err_rel2avg) {
						ratings_copy (RA.ratingbk, Players.n, RA.ratingof); //restore
					}
				}

				for (i = 0; i < topn; i++) {
					Sdev[i] = get_sdev (Sum1[i], Sum2[i], n);
				}
	
				for (i = 0; i < est; i++) {
					sim[i].sdev = get_sdev (sim[i].sum1, sim[i].sum2, n);
				}

				wa_sdev = get_sdev (wa_sum1, wa_sum2, n+1);
				dr_sdev = get_sdev (dr_sum1, dr_sum2, n+1);
			}
		}

		/* retransform database, to restore original data */
		database_transform(pdaba, &Games, &Players, &Game_stats); 
		qsort (Games.ga, (size_t)Games.n, sizeof(struct gamei), compare_GAME);
	
		/* recalculate encounters */
		calc_encounters__(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

		players_set_priored_info (PP, &RPset, &Players);
		if (0 < set_super_players(QUIET_MODE, &Encounters, &Players)) {
			players_purge (QUIET_MODE, &Players);
			calc_encounters__(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
		}
	}
	/* Simulation block, end */

	/*==== Reports ====*/

	all_report 	( &Games
				, &Players
				, &RA
				, &RPset
				, &Encounters
				, Sdev
				, Simulate
				, Hide_old_ver
				, Confidence_factor
				, csvf
				, textf
				, white_advantage_result
				, drawrate_evenmatch_result
				, OUTDECIMALS
				, outqual
				, wa_sdev
				, dr_sdev
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
		errorsout(&Players, &RA, sim, ematstr, Confidence_factor);
	}
	if (Simulate > 1 && NULL != ctsmatstr) {
		ctsout (&Players, &RA, sim, ctsmatstr);
	}
	if (head2head_str != NULL) {
		head2head_output
					( &Games
					, &Players
					, &RA
					, &Encounters
					, Sdev
					, Simulate
					, Confidence_factor
					, &Game_stats
					, sim
					, head2head_str
					, OUTDECIMALS
					);
	}

	// CEGT output style
	if (Elostat_output) {
		cegt_output	( QUIET_MODE
					, &Games
					, &Players
					, &RA
					, &Encounters
					, Sdev
					, Simulate
					, Confidence_factor
					, &Game_stats
					, sim
					, outqual);
	}

	/*==== Clean up ====*/

	if (textf_opened) 	fclose (textf);
	if (csvf_opened)  	fclose (csvf); 
	if (groupf_opened) 	fclose(groupf);

	if (sim != NULL) 
		memrel(sim);

	if (pdaba != NULL)
		database_done (pdaba);

	ratings_done (&RA);
	games_done (&Games);
	encounters_done (&Encounters);
	players_done (&Players);
	supporting_auxmem_done (&Sum1, &Sum2, &Sdev, &PP, &PP_store);

	if (relstr != NULL)
		relpriors_done (&RPset, &RPset_store);

	name_storage_done();

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

//=====================================

static void
init_rating (player_t n, double rat0, struct RATINGS *rat)
{
	player_t i;
	for (i = 0; i < n; i++) {
		rat->ratingof[i] = rat0;
	}
	for (i = 0; i < n; i++) {
		rat->ratingbk[i] = rat0;
	}
}

// no globals
static void
reset_rating (double general_average, player_t n_players, const bool_t *prefed, const bool_t *flagged, double *rating)
{
	player_t i;
	for (i = 0; i < n_players; i++) {
		if (!prefed[i] && !flagged[i])
			rating[i] = general_average;
	}
}

static void
ratings_copy (const double *r, player_t n, double *t)
{
	player_t i;
	for (i = 0; i < n; i++) {
		t[i] = r[i];
	}
}

//=====================================

// no globals
static void
players_purge (bool_t quiet, struct PLAYERS *pl)
{
	player_t n_players = pl->n;
	const int *performance_type = pl->performance_type;
	const char **name = pl->name;
	bool_t *flagged = pl->flagged;

	player_t j;
	assert(pl->perf_set);
	for (j = 0; j < n_players; j++) {
		if (flagged[j]) continue;
		if (performance_type[j] != PERF_NORMAL) {
			flagged[j]= TRUE;
			if (!quiet) printf ("purge --> %s\n", name[j]);
		} 
	}
}

// no globals
static void
players_set_priored_info (const struct prior *pr, const struct rel_prior_set *rps, struct PLAYERS *pl /*@out@*/)
{
	player_t 			i, j;
	player_t 			n_players = pl->n;
	struct relprior *	rp = rps->x;
	player_t 			rn = rps->n;

	// priors
	for (j = 0; j < n_players; j++) {
		pl->priored[j] = pr[j].isset;
	}

	// relative priors
	for (i = 0; i < rn; i++) {
		pl->priored[rp[i].player_a] = TRUE;
		pl->priored[rp[i].player_b] = TRUE;
	}
}

#if !defined(NDEBUG)
static bool_t
players_have_clear_flags (struct PLAYERS *pl)
{
	bool_t		found;
	player_t 	j;
	player_t 	n_players;
	bool_t *	flagged;

	assert(pl);
	n_players = pl->n;
	flagged = pl->flagged;
	assert(flagged);
	found = FALSE;
	for (j = 0; j < n_players; j++) {
		if (flagged[j]) {
			found = TRUE; 
			break;
		}
	}
	return !found;
}
#endif

static void
ratings_results (struct PLAYERS *plyrs, struct RATINGS *rat)
{
	double excess;

	player_t j;
	ratings_for_purged(plyrs, rat);
	for (j = 0; j < plyrs->n; j++) {
		rat->ratingof_results[j] = rat->ratingof[j];
		rat->obtained_results[j] = rat->obtained[j];
		rat->playedby_results[j] = rat->playedby[j];
	}

	// shifting ratings to fix the anchor.
	// Only done if the error is relative to the average.
	// Otherwise, it is taken care in the rating calculation already.
	// If Anchor_err_rel2avg is set, shifting in the calculation (later) is deactivated.
	excess = 0.0;
	if (Anchor_err_rel2avg && Anchor_use) {
		excess = General_average - rat->ratingof_results[Anchor];		
		for (j = 0; j < plyrs->n; j++) {
			rat->ratingof_results[j] += excess;
		}
	}

}

static void
players_flags_reset (struct PLAYERS *pl)
{
	player_t 	j;
	player_t 	n;
	bool_t *	flagged;

	assert(pl);
	n = pl->n;
	flagged = pl->flagged;
	assert(flagged);
	for (j = 0; j < n; j++) {
		flagged[j] = FALSE;
	}	
}

static void
ratings_for_purged (const struct PLAYERS *p, struct RATINGS *r /*@out@*/)
{
	player_t j;
	player_t n = p->n;
	for (j = 0; j < n; j++) {
		if (p->flagged[j]) {
			r->ratingof[j] = 0;
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
simulate_scores ( const double 	*ratingof_results
				, double 		deq
				, double 		wadv
				, double 		beta
				, struct GAMES *g	// output
)
{
	gamesnum_t n_games = g->n;
	struct gamei *gam = g->ga;

	gamesnum_t i;
	player_t w, b;
	const double *rating = ratingof_results;
	double pwin, pdraw, plos;
	assert(deq <= 1 && deq >= 0);

	for (i = 0; i < n_games; i++) {
		if (gam[i].score != DISCARD) {
			w = gam[i].whiteplayer;
			b = gam[i].blackplayer;
			get_pWDL(rating[w] + wadv - rating[b], &pwin, &pdraw, &plos, deq, beta);
			gam[i].score = rand_threeway_wscore(pwin,pdraw);
		}
	}
}

//=============================


// no globals
static void
ratings_apply_excess_correction(double excess, player_t n_players, const bool_t *flagged, double *ratingof /*out*/)
{
	player_t j;
	for (j = 0; j < n_players; j++) {
		if (!flagged[j])
			ratingof[j] -= excess;
	}
}

static void
ratings_center_to_zero (player_t n_players, const bool_t *flagged, double *ratingof)
{
	player_t 	j, notflagged;
	double 	excess, average;
	double 	accum = 0;

	// general average
	for (notflagged = 0, accum = 0, j = 0; j < n_players; j++) {
		if (!flagged[j]) {
			notflagged++;
			accum += ratingof[j];
		}
	}
	average = accum / (double) notflagged;
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
static player_t	
set_super_players(bool_t quiet, const struct ENCOUNTERS *ee, struct PLAYERS *pl)

{
	// encounters
	gamesnum_t N_enc = ee->n;
	const struct ENC *enc = ee->enc;

	// players
	player_t n_players = pl->n;
	int *perftype  = pl->performance_type;
	bool_t *ispriored = pl->priored; 
	const char **name    = pl->name;

	double 		*obt;
	gamesnum_t	*pla;
	gamesnum_t	e;
	player_t 	j;
	player_t 	w, b;
	player_t 	super = 0;

	obt = memnew (sizeof(double) * (size_t)n_players);
	pla = memnew (sizeof(gamesnum_t) * (size_t)n_players);
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

		assert(( w >= 0 && w < n_players) || !fprintf(stderr,"w=%ld np=%ld\n",(long)w,(long)n_players));
		assert(( b >= 0 && b < n_players) || !fprintf(stderr,"b=%ld np=%ld\n",(long)b,(long)n_players));

		obt[w] += enc[e].wscore;
		obt[b] += (double)enc[e].played - enc[e].wscore;

		pla[w] += enc[e].played;
		pla[b] += enc[e].played;

	}
	for (j = 0; j < n_players; j++) {
		//bool_t gotprior = has_a_prior(PP,j);
		bool_t gotprior = ispriored[j];
		perftype[j] = PERF_NORMAL;
		if (pla[j] == 0) {
			perftype[j] = PERF_NOGAMES;			
			if (!quiet) printf ("detected (player without games) --> %s\n", name[j]);
		} else {
			if (obt[j] < 0.001) {
				perftype[j] = gotprior? PERF_NORMAL: PERF_SUPERLOSER;			
				if (!quiet) printf ("detected (all-losses player) --> %s: seed rating present = %s\n", name[j], gotprior? "Yes":"No");
			}	
			if ((double)pla[j] - obt[j] < 0.001) {
				perftype[j] = gotprior? PERF_NORMAL: PERF_SUPERWINNER;
				if (!quiet) printf ("detected (all-wins player)   --> %s: seed rating present = %s\n", name[j], gotprior? "Yes":"No");
			}
		}
		if (perftype[j] != PERF_NORMAL) super++;
	}
	for (j = 0; j < n_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
	pl->perf_set = TRUE;

	memrel(obt);
	memrel(pla);
	return super;
}

//**************************************************************


