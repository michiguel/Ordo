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

#include "mytimer.h"

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

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
static void usage (FILE *outf);

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


	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/

//-------------------

#include "myhelp.h"

char OPTION_LIST[1024];

static struct option *long_options;

struct helpline SH[] = {

{'h',	"help",			no_argument,		NULL,		0,	"print this help and exit"},
{'H',	"show-switches",no_argument,		NULL,		0,	"print switch information and exit"},
{'v',	"version",		no_argument,		NULL,		0,	"print version number and exit"},
{'L',	"license",		no_argument,		NULL,		0,	"print license information  and exit"},
{'q',	"quiet",		no_argument,		NULL,		0,	"quiet mode (no progress updates on screen)"},
{'\0',	"silent",		no_argument,		NULL,		0,	"same as --quiet"},
{'Q',	"terse",		no_argument,		NULL,		0,	"same as --quiet, but shows simulation counter"},
{'\0',	"timelog",		no_argument,		NULL,		0,	"outputs elapsed time after each step"},
{'a',	"average",		required_argument,	"NUM",		0,	"set rating for the pool average"},
{'A',	"anchor",		required_argument,	"<player>",	0,	"anchor: rating given by '-a' is fixed for <player>"},
{'V',	"pool-relative",no_argument,		NULL,		0,	"errors relative to pool average, not to the anchor"},
{'m',	"multi-anchors",required_argument,	"FILE",		0,	"multiple anchors: file contains rows of \"AnchorName\",AnchorRating"},
{'y',	"loose-anchors",required_argument,	"FILE",		0,	"loose anchors: file contains rows of \"Player\",Rating,Uncertainty"},
{'r',	"relations",	required_argument,	"FILE",		0,	"relations: rows of \"PlayerA\",\"PlayerB\",delta_rating,uncertainty"},
{'R',	"remove-older",	no_argument,		NULL,		0,	"no output of older 'related' versions (given by -r)"},
{'w',	"white", 		required_argument,	"NUM",		0,	"white advantage value (default=0.0)"},
{'u',	"white-error",	required_argument,	"NUM",		0,	"white advantage uncertainty value (default=0.0)"},
{'W',	"white-auto",	no_argument,		NULL,		0,	"white advantage will be automatically adjusted"},
{'d',	"draw",			required_argument,	"NUM",		0,	"draw rate value % (default=50.0)"},
{'k',	"draw-error",	required_argument,	"NUM",		0,	"draw rate uncertainty value % (default=0.0 %)"},
{'D',	"draw-auto",	no_argument,		NULL,		0,	"draw rate value will be automatically adjusted"},
{'z',	"scale",		required_argument,	"NUM",		0,	"set rating for winning expectancy of 76% (default=202)"},
{'T',	"table",		no_argument,		NULL,		0,	"display winning expectancy table"},
{'p',	"pgn",			required_argument,	"FILE",		0,	"input file, PGN format"},
{'P',	"pgn-list",		required_argument,	"FILE",		0,	"multiple input: file with a list of PGN files"},
{'o',	"output",		required_argument,	"FILE",		0,	"output file, text format"},
{'c',	"csv",			required_argument,	"FILE",		0,	"output file, comma separated value format"},
{'E',	"elostat",		no_argument,		NULL,		0,	"output files in elostat format (rating.dat, programs.dat & general.dat)"},
{'j',	"head2head",	required_argument,	"FILE",		0,	"output file with head to head information"},
{'g',	"groups",		required_argument,	"FILE",		0,	"outputs group connection info (no rating output)"},
{'G',	"force",		no_argument,		NULL,		0,	"force program to run ignoring isolated-groups warning"},
{'s',	"simulations",	required_argument,	"NUM",		0,	"perform NUM simulations to calculate errors"},
{'e',	"error-matrix",	required_argument,	"FILE",		0,	"save an error matrix (use of -s required)"},
{'C',	"cfs-matrix",	required_argument,	"FILE",		0,	"save a matrix (comma separated value .csv) with confidence for superiority (-s was used)"},
{'J',	"cfs-show",		no_argument,		NULL,		0,	"output an extra column with confidence for superiority (relative to the player in the next row)"},
{'F',	"confidence",	required_argument,	"NUM",		0,	"confidence to estimate error margins (default=95.0)"},
{'X',	"ignore-draws",	no_argument,		NULL,		0,	"do not take into account draws in the calculation"},
{'t',	"threshold",	required_argument,	"NUM",		0,	"threshold of games for a participant to be included"},
{'N',	"decimals",		required_argument,	"<a,b>",	0,	"a=rating decimals, b=score decimals (optional)"},
{'M',	"ML",			no_argument,		NULL,		0,	"force maximum-likelihood estimation to obtain ratings"},
{'n',	"cpus",			required_argument,	"NUM",		0,	"number of processors used in simulations"},
{'U',	"columns",		required_argument,	"<a,..,z>",	0,	"info in output (default columns are \"0,1,2,3,4,5\")"},
{'Y',	"synonyms",		required_argument,	"FILE",		0,	"name synonyms (comma separated value format). Each line: main,syn1,syn2 or \"main\",\"syn1\",\"syn2\""},
{'\0',	"aliases",		required_argument,	"FILE",		0,	"same as --synonyms FILE"},
{'i',	"include",		required_argument,	"FILE",		0,	"include only games of participants present in FILE"},
{'x',	"exclude",		required_argument,	"FILE",		0,	"names in FILE will not have their games included"},
{'\0',	"no-warnings",	no_argument,		NULL,		0,	"supress warnings of names from -x or -i that do not match names in input file"},
{'b',	"column-format",required_argument,	"FILE",		0,	"format column output, each line form FILE being <column>,<width>,\"Header\""},

{0,		NULL,			0,					NULL,		0,	NULL},

};



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

static int		OUTDECIMALS = 1; //FIXME Replace by decimals array
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

static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}

static bool_t
strlist_multipush (strlist_t *sl, const char *finp_name)
{		
	csv_line_t csvln;
	FILE *finp;
	char myline[MAXSIZE_CSVLINE];
	char *p;
	bool_t line_success = TRUE;

	if (NULL == finp_name || NULL == (finp = fopen (finp_name, "r"))) {
		return FALSE;
	}

	while (line_success && NULL != fgets(myline, MAXSIZE_CSVLINE, finp)) {

		p = skipblanks(myline);
		if (*p == '\0') continue;

		if (csv_line_init(&csvln, myline)) {
			line_success = csvln.n == 1 && strlist_push (sl, csvln.s[0]);
			csv_line_done(&csvln);		
		} else {
			line_success = FALSE;
		}
	}

	fclose(finp);

	return line_success;
}

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

#include "strlist.h"

static bool_t
validate_dec_array (int max, int decimals_array_n, int *decimals_array)
{
	int i;
	bool_t ok = decimals_array_n <= max;
	for (i = 0; ok && i < decimals_array_n; i++) {
		ok = decimals_array[i] >= 0;
	}
	return ok;
}

int main (int argc, char *argv[])
{
	enum mainlimits {INPUTMAX=1024, COLSMAX=256, DECMAX=2};

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
	bool_t dowarning;

	int columns_n;
	int columns[COLSMAX+1];

	int decimals_array_n;
	int decimals_array[DECMAX+1];

	int cpus = 1;

	int op;
	int longoidx=0;

	const char *textstr, *csvstr, *ematstr, *groupstr, *pinsstr;
	const char *priorsstr, *relstr;
	const char *head2head_str;
	const char *ctsmatstr, *synstr;
	const char *output_columns;
	const char *output_decimals;
	const char *includes_str, *excludes_str, *columns_format_str, *multi_pgn, *single_pgn;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;
	bool_t group_is_output, Elostat_output, Ignore_draws, groupcheck, Forces_ML, cfs_column;
	bool_t switch_w=FALSE, switch_W=FALSE, switch_u=FALSE, switch_d=FALSE, switch_k=FALSE, switch_D=FALSE;

	strlist_t SL;
	strlist_t *psl = &SL;

	group_var_t *gv = NULL;

	/* defaults */
	adjust_white_advantage 	= FALSE;
	adjust_draw_rate 		= FALSE;
	quiet_mode   			= FALSE;
	sim_updates  			= FALSE;
	version_mode 			= FALSE;
	license_mode 			= FALSE;
	help_mode    			= FALSE;
	switch_mode  			= FALSE;
	input_mode   			= FALSE;
	table_mode   			= FALSE;
	textstr 	 			= NULL;
	csvstr       			= NULL;
	ematstr 	 			= NULL;
	ctsmatstr	 			= NULL;
	pinsstr		 			= NULL;
	priorsstr	 			= NULL;
	relstr		 			= NULL;
	synstr					= NULL;
	includes_str			= NULL;
	excludes_str			= NULL;
	columns_format_str 		= NULL;
	single_pgn				= NULL;
	multi_pgn       		= NULL;
	output_columns  		= NULL;
	output_decimals  		= NULL;
	group_is_output			= FALSE;
	groupcheck					= TRUE;
	groupstr 	 			= NULL;
	Elostat_output 			= FALSE;
	head2head_str			= NULL;
	Ignore_draws 			= FALSE;
	Forces_ML 	 			= FALSE;
	cfs_column      		= FALSE;
	dowarning				= TRUE;

	// global default
	TIMELOG = FALSE;

	strlist_init(psl);



	if (NULL == (long_options = optionlist_new (SH))) {
		fprintf(stderr, "Memory error to initialize options\n");
		exit(EXIT_FAILURE);
	}

	optionshort_build(SH, OPTION_LIST);

	while (END_OF_OPTIONS != (op = options_l (argc, argv, OPTION_LIST, long_options, &longoidx))) {

		switch (op) {

			case '\0':
						if (!strcmp(long_options[longoidx].name, "silent")) {
							quiet_mode = TRUE;
						} else if (!strcmp(long_options[longoidx].name, "aliases")) {
							synstr = opt_arg;
						} else if (!strcmp(long_options[longoidx].name, "no-warnings")) {
							dowarning = FALSE;
						} else if (!strcmp(long_options[longoidx].name, "timelog")) {
							TIMELOG = TRUE;
						} else {
							fprintf (stderr, "ERROR: %d\n", op);
							exit(EXIT_FAILURE);
						}

						break;
			case 'v':	version_mode = TRUE; 	break;
			case 'L':	version_mode = TRUE; 	
						license_mode = TRUE;
						break;
			case 'h':	help_mode = TRUE;		break;
			case 'H':	switch_mode = TRUE;		break;
			case 'p': 	input_mode = TRUE;
						single_pgn = opt_arg;
						break;
			case 'P': 	input_mode = TRUE;
						multi_pgn = opt_arg;
						break;
			case 'c': 	csvstr = opt_arg;
						break;
			case 'o': 	textstr = opt_arg;
						break;
			case 'g': 	group_is_output = TRUE;
						groupstr = opt_arg;
						break;
			case 'G': 	groupcheck = FALSE;
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
			case 'q':	quiet_mode = TRUE;	sim_updates = FALSE; break;
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
			case 'N': 	output_decimals = opt_arg;
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
			case 'i': 	includes_str = opt_arg;
						break;
			case 'x': 	excludes_str = opt_arg;
						break;
			case 'b': 	columns_format_str = opt_arg;
						break;
			case '?': 	parameter_error();
						exit(EXIT_FAILURE);
						break;
			default:	fprintf (stderr, "ERROR: %d\n", op);
						exit(EXIT_FAILURE);
						break;
		}		
	}

	optionlist_done (long_options);

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
		usage(stdout);
		printf ("%s\n", copyright_str);
		exit (EXIT_SUCCESS);
	}
	if (switch_mode && !help_mode) {
		usage(stdout);
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
	if (output_decimals != NULL) {
		if (!str2list (output_decimals, DECMAX, &decimals_array_n, decimals_array)) {
			fprintf (stderr, "Decimals number provided are wrong or exceeded limit (%d)\n\n", DECMAX);
			exit(EXIT_FAILURE);		
		}
		if (!validate_dec_array (DECMAX, decimals_array_n, decimals_array) || decimals_array_n == 0) {
			fprintf (stderr, "Decimals cannot be negative or exceeded limit\n\n");
			exit(EXIT_FAILURE);	
		}
		OUTDECIMALS = decimals_array[0];
		Decimals_set = TRUE;
	} else {
		if (!str2list ("1,0", DECMAX, &decimals_array_n, decimals_array)) {
			fprintf (stderr, "Default number of decimals is wrong or exceeded limit (%d)\n\n", DECMAX);
			exit(EXIT_FAILURE);		
		}
		OUTDECIMALS = decimals_array[0];
		Decimals_set = TRUE;
	}
	
	/* -------- input files, remaining args --------- */

	if (single_pgn) {
		if (!strlist_push(psl,single_pgn)) {
			fprintf (stderr, "Lack of memory\n\n");
			exit(EXIT_FAILURE);		
		}
	}

	if (multi_pgn) {
		if (!strlist_multipush (psl, multi_pgn)) {
			fprintf (stderr, "Errors in file \"%s\", or lack of memory\n", multi_pgn);
			exit(EXIT_FAILURE);
		}
	}

	while (opt_index < argc) {
		if (!strlist_push(psl,argv[opt_index++])) {
			fprintf (stderr, "Too many input files\n\n");
			exit(EXIT_FAILURE);
		}
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
	if (group_is_output && !groupcheck) {
		fprintf (stderr, "Switches -g and -G cannot be used at the same time\n\n");
		exit(EXIT_FAILURE);
	}				
	if (includes_str && excludes_str) {
		fprintf (stderr, "Switches -x and -i cannot be used at the same time\n\n");
		exit(EXIT_FAILURE);
	}	

	Prior_mode = switch_k || switch_u || NULL != relstr || NULL != priorsstr;

	/*==== mem init ====*/

	if (!name_storage_init()) {
		fprintf (stderr, "Problem initializing buffers for reading names, probably lack of memory.\n");	
		exit(EXIT_FAILURE);
	}

	/*==== report init ====*/

	if (!report_columns_init()) {
		fprintf (stderr, "Problem initializing report settings.\n");	
		exit(EXIT_FAILURE);
	}

	if (columns_format_str)
		report_columns_load_settings (quiet_mode, columns_format_str);

	/*==== set input ====*/

	timer_reset();
	timelog("start");
	timelog("input...");

	if (NULL != (pdaba = database_init_frompgn (psl, synstr, quiet_mode))) {
		if (0 == pdaba->n_players || 0 == pdaba->n_games) {
			fprintf (stderr, "ERROR: Input file contains no games\n");
			return EXIT_FAILURE; 			
		}
		if (Ignore_draws) database_ignore_draws(pdaba);

		if (NULL != includes_str) {
			bitarray_t ba;
			if (ba_init (&ba,pdaba->n_players)) {
				namelist_to_bitarray (quiet_mode, dowarning, includes_str, pdaba, &ba);
				database_include_only(pdaba, &ba);
			} else {
				fprintf (stderr, "ERROR\n");
				exit(EXIT_FAILURE);
			}
		}
		if (NULL != excludes_str) {
			bitarray_t ba;
			if (ba_init (&ba,pdaba->n_players)) {
				namelist_to_bitarray (quiet_mode, dowarning, excludes_str, pdaba, &ba);
				ba_setnot(&ba);
				database_include_only(pdaba, &ba);
			} else {
				fprintf (stderr, "ERROR\n");
				exit(EXIT_FAILURE);
			}
		}

	} else {
		fprintf (stderr, "Problems reading results\n");
		return EXIT_FAILURE; 
	}

	strlist_done(psl); // string list not used anymore from this point on

	/*==== memory initialization ====*/
	{
		player_t mpr 	= pdaba->n_players; 
		player_t mpp 	= pdaba->n_players; 
		gamesnum_t mg  	= pdaba->n_games;
		gamesnum_t me  	= pdaba->n_games;

		if (!ratings_init (mpr, &RA)) {
			fprintf (stderr, "Could not initialize rating memory\n"); exit(EXIT_FAILURE);	
		} else 
		if (!games_init (mg, &Games)) {
			ratings_done (&RA);
			fprintf (stderr, "Could not initialize Games memory\n"); exit(EXIT_FAILURE);
		} else 
		if (!encounters_init (me, &Encounters)) {
			ratings_done (&RA);
			games_done (&Games);
			fprintf (stderr, "Could not initialize Encounters memory\n"); exit(EXIT_FAILURE);
		} else 
		if (!players_init (mpp, &Players)) {
			ratings_done (&RA);
			games_done (&Games);
			encounters_done (&Encounters);
			fprintf (stderr, "Could not initialize Players memory\n"); exit(EXIT_FAILURE);
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
		fprintf (stderr, "Could not initialize auxiliary Players memory\n"); exit(EXIT_FAILURE);
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

	#if 0
	// to ignore a given player --> test_player
	// here, set:
	// Players.flagged[test_player] = TRUE;
	// Players.present_in_games[test_player] = FALSE;
	#endif

	assert(players_have_clear_flags(&Players));
	encounters_calculate(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (0 == Encounters.n) {
		fprintf (stderr, "ERROR: Input file contains no games to process\n");
		return EXIT_FAILURE; 			
	}

	/*==== report, input checked ====*/

	if (!quiet_mode) {
		printf ("Total games            %8ld\n",(long)
											 (Game_stats.white_wins
											 +Game_stats.draws
											 +Game_stats.black_wins
											 +Game_stats.noresult));
		printf (" - White wins          %8ld\n", (long) Game_stats.white_wins);
		printf (" - Draws               %8ld\n", (long) Game_stats.draws);
		printf (" - Black wins          %8ld\n", (long) Game_stats.black_wins);
		printf (" - Truncated/Discarded %8ld\n", (long) Game_stats.noresult);
		printf ("Unique head to head    %8.2f%s\n", 100.0*(double)Encounters.n/(double)Games.n, "%");
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

	timelog("done with input");
	timelog("process groups if needed...");

	/*===== groups ========*/

	gv = NULL;

	assert(players_have_clear_flags (&Players));
	encounters_calculate (ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	if (group_is_output || groupcheck) {
		timelog("processing groups...");
		if (NULL == (gv = GV_make (&Encounters, &Players))) {
			fprintf (stderr, "not enough memory for encounters allocation\n");
			exit(EXIT_FAILURE);
		}
	}

	if (group_is_output) {
		gamesnum_t intra, inter;

		timelog("sieve groups...");
		GV_sieve (gv, &Encounters, &intra, &inter);

		GV_out (gv, groupf);
		if (!quiet_mode) {
			printf ("\nGroups=%ld\n", (long)GV_counter(gv));
			printf ("Encounters: Total=%ld, within groups=%ld, @ interface between groups=%ld\n"
					, (long)Encounters.n, (long)intra, (long)inter);
		}

		if (textstr == NULL && csvstr == NULL)	{
			if (groupf_opened) 	fclose(groupf);
			GV_kill(gv);
			exit(EXIT_SUCCESS);
		}
 	} else if (groupcheck) {

		player_t groups_n = GV_counter (gv);
		if (groups_n > 1) {
			fprintf (stderr, "\n\n");
			fprintf (stderr, "********************[ WARNING ]**********************\n");
			fprintf (stderr, "*     Database is not well connected by games       *\n");
			fprintf (stderr, "* Run switch -g to find what groups need more games *\n");
			fprintf (stderr, "*****************************************************\n");
			fprintf (stderr, " ====> Groups detected: %ld\n\n\n", (long) groups_n);
		}
	}

	// Not used anymore beyond this point
	if (gv) {
		GV_kill(gv);
		gv = NULL;
	}

	/*==== ratings calc ===*/

	assert(players_have_clear_flags(&Players));
	encounters_calculate(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

	players_set_priored_info (PP, &RPset, &Players);
	if (0 < players_set_super (quiet_mode, &Encounters, &Players)) {
		players_purge (quiet_mode, &Players);
		encounters_calculate(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
	}

	if (groupcheck && !well_connected (&Encounters, &Players)) {
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

	timelog("calculate rating...");

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
		timelog("simulation block...");
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
		encounters_calculate(ENCOUNTERS_FULL, &Games, Players.flagged, &Encounters);

		players_set_priored_info (PP, &RPset, &Players);
		if (0 < players_set_super (quiet_mode, &Encounters, &Players)) {
			players_purge (quiet_mode, &Players);
			encounters_calculate(ENCOUNTERS_NOFLAGGED, &Games, Players.flagged, &Encounters);
		}
	}

	/*==== reports ====*/

	timelog("output reports...");

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
				, decimals_array_n > 0? decimals_array[0]: 1 //OUTDECIMALS
				, decimals_array_n > 1? decimals_array[1]: 1 //OUTDECIMALS
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
		timelog("head to head output...");
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
		timelog("output in elostat format...");
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

	timelog("release memory...");

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
	report_columns_done();

	mythread_mutex_destroy (&Smpcount);
	mythread_mutex_destroy (&Groupmtx);
	mythread_mutex_destroy (&Summamtx);
	mythread_mutex_destroy (&Printmtx);

	timelog("DONE!!");
	if (!quiet_mode) printf ("\ndone!\n");

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
usage (FILE *outf)
{
	const char *usage_options = "[-OPTION]";
	fprintf (outf, "\n"
		"usage: %s %s\n"
		, proginfo_name()
		, usage_options
		);

		printlonghelp(outf, SH);
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


