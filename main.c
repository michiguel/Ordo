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

/*
|
|	GENERAL OPTIONS
|
\*--------------------------------------------------------------*/

#include "myopt.h"

const char *license_str =
"Copyright (c) 2012 Miguel A. Ballicora\n"
"\n"
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n"
"EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES\n"
"OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND\n"
"NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT\n"
"HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,\n"
"WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING\n"
"FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR\n"
"OTHER DEALINGS IN THE SOFTWARE."
;

static void parameter_error(void);
static void example (void);
static void usage (void);

/* VARIABLES */

	static bool_t QUIET_MODE;
	static bool_t ADJUST_WHITE_ADVANTAGE;

	static const char *copyright_str = 
		"Copyright (c) 2012 Miguel A. Ballicora\n"
		"There is NO WARRANTY of any kind\n"
		;

	static const char *intro_str =
		"Program to calculate individual ratings\n"
		;

	const char *example_options = 
		"-a 2500 -p input.pgn -o output.txt";

	static const char *example_str =
		"  - Processes input.pgn (PGN file) and calculates ratings.\n"
		"  - The general pool will have an average of 2500\n"
		"  - Output is in output.txt (text file)\n"
		;

	static const char *help_str =
		" -h          print this help\n"
		" -H          print just the switches\n"
		" -v          print version number and exit\n"
		" -L          display the license information\n"
		" -q          quiet mode (no screen progress updates)\n"
		" -a <avg>    set rating for the pool average\n"
		" -A <player> anchor: rating given by '-a' is fixed for <player>, if provided\n"
		" -w <value>  white advantage value (default=0.0)\n"
		" -W          white advantage, automatically adjusted\n"
		" -z <value>  scaling: set rating for winning expectancy of 76% (default=202)\n"
		" -T          display winning expectancy table\n"
		" -p <file>   input file in PGN format\n"
		" -c <file>   output file (comma separated value format)\n"
		" -o <file>   output file (text format), goes to the screen if not present\n"
		" -s  #       perform # simulations to calculate errors\n"
		" -e <file>   saves an error matrix, if -s was used\n"
		"\n"
		;

	const char *usage_options = 
		"[-OPTION]";
		;
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		

const char *OPTION_LIST = "vhHp:qWLa:A:o:c:s:w:z:e:T";

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

enum 			AnchorSZ	{MAX_ANCHORSIZE=256};
static bool_t	Anchor_use = FALSE;
static int		Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

enum 			Player_Performance_Type {
				PERF_NORMAL = 0,
				PERF_SUPERWINNER = 1,
				PERF_SUPERLOSER = 2
};

static int		Performance_type[MAXPLAYERS];

/* games */
static int 		Whiteplayer	[MAXGAMES];
static int 		Blackplayer	[MAXGAMES];
static int 		Score		[MAXGAMES];
static int 		N_games = 0;

/* encounters */
struct ENC {
	double 	wscore;
	int		played;
	int 	wh;
	int 	bl;
};

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
static double	General_average = 2300.0;
static int		Sorted  [MAXPLAYERS]; /* sorted index by rating */
static double	Obtained[MAXPLAYERS];
static double	Expected[MAXPLAYERS];
static int		Playedby[MAXPLAYERS]; /* N games played by player "i" */
static double	Ratingof[MAXPLAYERS]; /* rating current */
static double	Ratingbk[MAXPLAYERS]; /* rating backup  */

static double	Ratingof_results[MAXPLAYERS];
static double	Obtained_results[MAXPLAYERS];
static int		Playedby_results[MAXPLAYERS];

static double	Sum1[MAXPLAYERS]; 
static double	Sum2[MAXPLAYERS]; 
static double	Sdev[MAXPLAYERS]; 

static long 	Simulate = 0;

#define INVBETA 175.25

static double	White_advantage = 0;
static double	Rtng_76 = 202;
static double	Inv_beta = INVBETA;
static double	BETA = 1.0/INVBETA;

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

void			calc_encounters (bool_t useflag);
void			calc_obtained_playedby_ENC (void);
void			calc_expected_ENC (void);
void			shrink_ENC (void);
static void		purge_players(bool_t quiet);
static void		clear_flagged (void);

static void		set_super_players(bool_t quiet);

void			all_report (FILE *csvf, FILE *textf);
void			init_rating (void);
void			calc_expected (void);
double			adjust_rating (double delta, double kappa);
void			calc_rating (bool_t quiet);
double 			deviation (void);
void			ratings_restore (void);
void			ratings_backup  (void);

static double	xpect (double a, double b);

static void 	ratings_results (void);
static void 	ratings_for_purged (void);

static void		simulate_scores(void);
static void		errorsout(const char *out);

/*------------------------------------------------------------------------*/

static void 	transform_DB(struct DATA *db, struct GAMESTATS *gs);
static bool_t	find_anchor_player(int *anchor);

/*------------------------------------------------------------------------*/

static double 	overallerror_fwadv (double wadv);
static double 	adjust_wadv (double start_wadv);
static void 	table_output(double Rtng_76);

void scan_encounters(void);
static void convert_to_groups(void);

//
#if 0
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
//

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

int main (int argc, char *argv[])
{
	bool_t csvf_opened;
	bool_t textf_opened;
	FILE *csvf;
	FILE *textf;

	int op;
	const char *inputf, *textstr, *csvstr, *ematstr;
	int version_mode, help_mode, switch_mode, license_mode, input_mode, table_mode;

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
			case 'e': 	ematstr = opt_arg;
						break;
			case 'a': 	if (1 != sscanf(opt_arg,"%lf", &General_average)) {
							fprintf(stderr, "wrong average parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'A': 	if (strlen(opt_arg) < MAX_ANCHORSIZE-1) {
							strcpy (Anchor_name, opt_arg);
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
						}
						break;
			case 'z': 	if (1 != sscanf(opt_arg,"%lf", &Rtng_76)) {
							fprintf(stderr, "wrong scaling parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'T':	table_mode = TRUE;	break;
			case 'q':	QUIET_MODE = TRUE;	break;
			case 'W':	ADJUST_WHITE_ADVANTAGE = TRUE;	break;
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
		printf ("Total games         %8ld\n", Game_stats.white_wins
											 +Game_stats.draws
											 +Game_stats.black_wins
											 +Game_stats.noresult);
		printf ("White wins          %8ld\n", Game_stats.white_wins);
		printf ("Draws               %8ld\n", Game_stats.draws);
		printf ("Black wins          %8ld\n", Game_stats.black_wins);
		printf ("No result           %8ld\n", Game_stats.noresult);
		printf ("Unique head to head %8.2f%s\n", 100.0*N_encounters/N_games, "%");
		printf ("Reference rating    %8.1lf",General_average);
		if (Anchor_use) 
			printf (" (set to \"%s\")\n", Anchor_name);
		else	
			printf (" (average of the pool)\n");	
		printf ("\n");	
	}

	init_rating();

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

	/*=====================*/

	calc_encounters(ENCOUNTERS_FULL);
	scan_encounters(); 
	convert_to_groups();

exit(0);
//
set_super_players(QUIET_MODE);
	purge_players(QUIET_MODE);

//	calc_encounters(ENCOUNTERS_NOFLAGGED);
//	calc_obtained_playedby_ENC();
	
	calc_rating(QUIET_MODE);
	ratings_results();

	if (ADJUST_WHITE_ADVANTAGE) {
		double new_wadv = adjust_wadv (White_advantage);
		if (!QUIET_MODE)
			printf ("\nAdjusted White Advantage = %f\n\n",new_wadv);
		White_advantage = new_wadv;
	
		purge_players(QUIET_MODE);
		calc_rating(QUIET_MODE);
		ratings_results();
	}

	/* Simulation block */
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

					//save_simulated((int)z);

set_super_players(QUIET_MODE);

					purge_players(QUIET_MODE);
					calc_rating(QUIET_MODE);
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
					Sdev[i] = sqrt( Sum2[i]/n - (Sum1[i]/n) * (Sum1[i]/n));
				}
	
				for (i = 0; i < est; i++) {
					sim[i].sdev = sqrt( sim[i].sum2/n - (sim[i].sum1/n) * (sim[i].sum1/n));
				}
			}
		}
	}

	all_report (csvf, textf);
	if (Simulate > 1 && NULL != ematstr)
		errorsout (ematstr);

	if (textf_opened) fclose (textf);
	if (csvf_opened)  fclose (csvf); 

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
				fprintf(f,",%.1f",sim[idx].sdev);
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
	return Obtained_results[j] < 0.01 || 
			Playedby_results[j] - Obtained_results[j] < 0.01;
}

void
all_report (FILE *csvf, FILE *textf)
{
	FILE *f;
	int i, j;
	size_t ml;
	char sdev_str_buffer[80];
	const char *sdev_str;

	calc_encounters(ENCOUNTERS_NOFLAGGED);
	calc_obtained_playedby_ENC();

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
			fprintf(f, "\n%s %-*s :%7s %9s %7s %6s\n", 
				"   #", 			
				(int)ml,
				"ENGINE", "RATING", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {
				j = Sorted[i];
				if (!Flagged[j]) {
				fprintf(f, "%4d %-*s:%7.1f %9.1f %7d %6.1f%s\n", 
					i+1,
					(int)ml+1,
					Name[j], Ratingof_results[j], Obtained_results[j], Playedby_results[j]
						, Playedby_results[j]==0? 0: 100.0*Obtained_results[j]/Playedby_results[j], "%");
				} else {
				fprintf(f, "%4d %-*s:%7s %9s %7s %6s%s\n", 
					i+1,
					(int)ml+1,
					Name[j], "----", "----", "----", "----","%");
				}
			}
		} else {
			fprintf(f, "\n%s %-*s :%7s %6s %8s %7s %6s\n", 
				"   #", 
				(int)ml, 
				"ENGINE", "RATING", "ERROR", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {
				j = Sorted[i];
				if (Sdev[j] > 0.00000001) {
					sprintf(sdev_str_buffer, "%6.1f", Sdev[j]);
					sdev_str = sdev_str_buffer;
				} else {
					sdev_str = "  ----";
				}
				if (!Flagged[j]) {
				fprintf(f, "%4d %-*s:%7.1f %s %8.1f %7d %6.1f%s\n", 
					i+1,
					(int)ml+1, 
					Name[j], Ratingof_results[j], sdev_str, Obtained_results[j], Playedby_results[j]
						, Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], "%");
				} else if (!is_super_player(j)) {
				fprintf(f, "%4d %-*s:%7.1f %s %8.1f %7d %6.1f%s\n", 
					i+1,
					(int)ml+1, 
					Name[j], Ratingof_results[j], "  ****", Obtained_results[j], Playedby_results[j]
						, Playedby_results[j]==0?0:100.0*Obtained_results[j]/Playedby_results[j], "%");
				} else {
				fprintf(f, "%4d %-*s:%7s %s %8s %7s %6s%s\n", 
					i+1,
					(int)ml+1,
					Name[j], "----", "  ----", "----", "----", "----","%");
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

void
calc_encounters (int selectivity)
{
	int i, e = 0;

	for (i = 0; i < N_games; i++) {

		if (Score[i] >= DISCARD) continue;

if (selectivity == ENCOUNTERS_NOFLAGGED) {
	if (Flagged[Whiteplayer[i]] || Flagged[Blackplayer[i]])
		continue;
}
		switch (Score[i]) {
			case WHITE_WIN: 	Encounter[e].wscore = 1.0; break;
			case RESULT_DRAW:	Encounter[e].wscore = 0.5; break;
			case BLACK_WIN:		Encounter[e].wscore = 0.0; break;
		}

		Encounter[e].wh = Whiteplayer[i];
		Encounter[e].bl = Blackplayer[i];
		Encounter[e].played = 1;
		e++;
	}
	N_encounters = e;

	shrink_ENC ();
	qsort (Encounter, (size_t)N_encounters, sizeof(struct ENC), compare_ENC);
	shrink_ENC ();
}


void
calc_obtained_playedby_ENC (void)
{
	int e, j, w, b;

	for (j = 0; j < N_players; j++) {
		Obtained[j] = 0.0;	
		Playedby[j] = 0;
	}	

	for (e = 0; e < N_encounters; e++) {
	
		w = Encounter[e].wh;
		b = Encounter[e].bl;

		Obtained[w] += Encounter[e].wscore;
		Obtained[b] += (double)Encounter[e].played - Encounter[e].wscore;

		Playedby[w] += Encounter[e].played;
		Playedby[b] += Encounter[e].played;
	}
}

void
calc_expected_ENC (void)
{
	int e, j, w, b;
	double f;
	double wperf,bperf;

	for (j = 0; j < N_players; j++) {
		Expected[j] = 0.0;	
	}	

	for (e = 0; e < N_encounters; e++) {
	
		w = Encounter[e].wh;
		b = Encounter[e].bl;

		f = xpect (Ratingof[w] + White_advantage, Ratingof[b]);

		wperf = Encounter[e].played * f;
		bperf = Encounter[e].played - wperf;

		Expected [w] += wperf; 
		Expected [b] += bperf; 

	}
}

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
		return r;
}

void
shrink_ENC (void)
{
	int e, j, g;

	for (j = 0; j < N_players; j++) {Expected[j] = 0.0;}	

	if (N_encounters == 0) return; 

	g = 0;
	for (e = 1; e < N_encounters; e++) {
	
		if (Encounter[e].wh == Encounter[g].wh && Encounter[e].bl == Encounter[g].bl) {
			Encounter[g] = encounter_merge (&Encounter[g], &Encounter[e]);
		}
		else {
			g++;
			Encounter[g] = Encounter[e];
		}
	}
	g++;
	N_encounters = g;
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

static void
purge_players(bool_t quiet)
{
	bool_t foundproblem;
	int j;

	do {
		calc_encounters(ENCOUNTERS_NOFLAGGED);
		calc_obtained_playedby_ENC();

		foundproblem = FALSE;
		for (j = 0; j < N_players; j++) {
			if (Flagged[j]) continue;
			if (Obtained[j] < 0.001 || Playedby[j] - Obtained[j] < 0.001) {
				Flagged[j]= TRUE;
				if (!quiet) printf ("purge --> %s\n", Name[j]);
				foundproblem = TRUE;
// No need to discard games.
// Flagging the players is enough
//
//				for (i = 0; i < N_games; i++) {
//					if (Whiteplayer[i] == j || Blackplayer[i] == j)
//						Score[i] = DISCARD;
//				}
			} 
		}
	} while (foundproblem);
}

void
calc_expected (void)
{
	calc_expected_ENC();
}

double
adjust_rating (double delta, double kappa)
{
	double y = 1.0;
	double d;
	double ymax = 0;
	double accum = 0;
	double excess, average;
	int j;
	int flagged;

	for (flagged = 0, j = 0; j < N_players; j++) {if (Flagged[j]) flagged++;}

	for (j = 0; j < N_players; j++) {
		if (Flagged[j]) continue;

		d = (Expected[j] - Obtained[j])/Playedby[j];
		d = d < 0? -d: d;
		y = d / (kappa+d);
		if (y > ymax) ymax = y;
		if (Expected[j] > Obtained [j]) {
			Ratingof[j] -= delta * y;
		} else {
			Ratingof[j] += delta * y;
		}
	}

	for (accum = 0, j = 0; j < N_players; j++) {
		accum += Ratingof[j];
	}		

	average = accum / (N_players-flagged);
	excess  = average - General_average;

	if (Anchor_use) {
		excess  = Ratingof[Anchor] - General_average;	
	}

	for (j = 0; j < N_players; j++) {
		if (!Flagged[j]) Ratingof[j] -= excess;
	}	
	
	return ymax * delta;
}

void
ratings_restore (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		Ratingof[j] = Ratingbk[j];
	}	
}

void
ratings_backup (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		Ratingbk[j] = Ratingof[j];
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

static void
simulate_scores(void)
{
	long int i, w, b, z, y;
	double f;
	double	*rating = Ratingof_results;

	for (i = 0; i < N_games; i++) {

		if (Score[i] == DISCARD) continue;

		w = Whiteplayer[i];
		b = Blackplayer[i];

		f = xpect (rating[w] + White_advantage, rating[b]);

		z = (long)((unsigned)(f * (0xffff+1)));
		y = randfast32() & 0xffff;

		if (y < z) {
			Score [i] = WHITE_WIN;
		} else {
			Score [i] = BLACK_WIN;
		}
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


//
//====
#if 1
#define CALCIND_SWSL
#endif

#ifdef CALCIND_SWSL

static double
calc_ind_rating(double cume_score, double *rtng, double *weig, int r);

static void
rate_super_players(bool_t quiet)
{
	int j, e;
//
	int myenc_n = 0;
	static struct ENC myenc[MAXENCOUNTERS];
//

	calc_encounters(ENCOUNTERS_FULL);
	calc_obtained_playedby_ENC();

	for (j = 0; j < N_players; j++) {

		if (Performance_type[j] != PERF_SUPERWINNER && Performance_type[j] != PERF_SUPERLOSER) 
			continue;

		myenc_n = 0; // reset

		if (Performance_type[j] == PERF_SUPERWINNER)
			if (!quiet) printf ("  all wins   --> %s\n", Name[j]);

		if (Performance_type[j] == PERF_SUPERLOSER) 
			if (!quiet) printf ("  all losses --> %s\n", Name[j]);

//printf ("N_encounters=%d\n",N_encounters);

		for (e = 0; e < N_encounters; e++) {
			int w = Encounter[e].wh;
			int b = Encounter[e].bl;
			if (j == w /*&& Performance_type[b] == PERF_NORMAL*/) {
//printf ("myenc_n=%d\n",myenc_n);
				myenc[myenc_n++] = Encounter[e];
			} else
			if (j == b /*&& Performance_type[w] == PERF_NORMAL*/) {
//printf ("myenc_n=%d\n",myenc_n);
				myenc[myenc_n++] = Encounter[e];
			}
		}
	
//printf ("myenc_n=%d\n",myenc_n);

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
		Ratingof[j] = calc_ind_rating (cume_score-0.25, rtng, weig, r);
}
if (Performance_type[j] == PERF_SUPERLOSER) {
		Ratingof[j] = calc_ind_rating (cume_score+0.25, rtng, weig, r);
}
		Flagged[j] = FALSE;
}

	}

	calc_encounters(ENCOUNTERS_NOFLAGGED);
	calc_obtained_playedby_ENC();
}
#endif

void
calc_rating (bool_t quiet)
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

	calc_encounters(ENCOUNTERS_NOFLAGGED);
	calc_obtained_playedby_ENC();
	calc_expected();
	olddev = curdev = deviation();

	if (!quiet) printf ("\nConvergence rating calculation\n\n");
	if (!quiet) printf ("%3s %4s %10s %10s\n", "phase", "iteration", "deviation","resolution");

	while (n-->0) {
		double kk = 1.0;
		for (i = 0; i < rounds; i++) {

			ratings_backup();
			olddev = curdev;

			resol = adjust_rating(delta,kappa*kk);
			calc_expected();
			curdev = deviation();

			if (curdev >= olddev) {
				ratings_restore();
				calc_expected();
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
rate_super_players(QUIET_MODE);
#endif

}


static double
xpect (double a, double b)
{
	return 1.0 / (1.0 + exp((b-a)*BETA));
}

/*==================================================================*/

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

//
static void
set_super_players(bool_t quiet)
{
	static double 	obt [MAXPLAYERS];
	static int		pla [MAXPLAYERS];

	int e, j, w, b;

	calc_encounters(ENCOUNTERS_FULL);

	for (j = 0; j < N_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	

	for (e = 0; e < N_encounters; e++) {
		w = Encounter[e].wh;
		b = Encounter[e].bl;

		obt[w] += Encounter[e].wscore;
		obt[b] += (double)Encounter[e].played - Encounter[e].wscore;

		pla[w] += Encounter[e].played;
		pla[b] += Encounter[e].played;

	}

	for (j = 0; j < N_players; j++) {
		Performance_type[j] = PERF_NORMAL;
		if (obt[j] < 0.001) {
			Performance_type[j] = PERF_SUPERLOSER;			
			if (!quiet) printf ("detected (all-losses player) --> %s\n", Name[j]);
		}	
		if (pla[j] - obt[j] < 0.001) {
			Performance_type[j] = PERF_SUPERWINNER;
			if (!quiet) printf ("detected (all-wins player)   --> %s\n", Name[j]);

		}
	}

	for (j = 0; j < N_players; j++) {
		obt[j] = 0.0;	
		pla[j] = 0;
	}	
}


//**************************************************************88
static double
ind_expected (double x, double *rtng, double *weig, int n)
{
	int i;
	double cume = 0;
	for (i = 0; i < n; i++) {
		cume += weig[i] * xpect (x, rtng[i]);
	}
//printf ("xp=%f\n",cume);
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

/*
printf ("cume score = %f, r=%d\n", cume_score,r);
for (i = 0; i < r; i++) {
	printf ("r=%d, rtng=%f, weig=%f\n", r, rtng[i], weig[i]);
}
printf ("\n");
*/
	D = cume_score - ind_expected(x,rtng,weig,r) ;
	curdev = D*D;
	olddev = curdev;

//printf ("D=%f\n",D);

	while (n-->0) {
		double kk = 1.0;
//printf ("n=%d\n",n);

		for (i = 0; i < rounds; i++) {
//printf ("i=%d\n",i);

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

//printf ("curdev=%f\n",curdev);

	return x;
}

//=========================================

typedef struct GROUP 		group_t;
typedef struct CONNECT 		connection_t;
typedef struct NODE 		node_t;
typedef struct PARTICIPANT 	participant_t;

struct GROUP {
	group_t 		*next;
	group_t 		*prev;
	group_t 		*combined;
	participant_t 	*pstart;
	participant_t 	*plast;
	connection_t	*cstart;
	connection_t	*clast;
	int				id;
};

struct CONNECT {
	connection_t 	*next;
	node_t			*node;
};

struct NODE {
	group_t 		*group;
};

struct PARTICIPANT {
	participant_t 	*next;
	char 			*name;
	int				id;
};

node_t				Gnode[MAXPLAYERS];

struct GROUP_BUFFER {
	group_t		list[MAXPLAYERS];
	int					n;
} group_buffer;

struct PARTICIPANT_BUFFER {
	participant_t		list[MAXPLAYERS];
	int					n;
} participant_buffer;

struct CONNECT_BUFFER {
	connection_t		list[MAXPLAYERS];
	int					n;
} connection_buffer;

static void connect_init (void) {connection_buffer.n = 0;}
static connection_t * connection_new (void) {return &connection_buffer.list[connection_buffer.n++];}

static void participant_init (void) {participant_buffer.n = 0;}
static participant_t * participant_new (void) {return &participant_buffer.list[participant_buffer.n++];}

static void group_init (void) {group_buffer.n = 0;}
static group_t * group_new  (void) {return &group_buffer.list[group_buffer.n++];}
static group_t * group_tail (void) {return group_buffer.n > 0? &group_buffer.list[group_buffer.n-1]:NULL;}
static group_t * group_head (void) {return group_buffer.n > 0? &group_buffer.list[0]:NULL;}
static group_t * group_reset(group_t *g)
{		if (g == NULL) return NULL;
		g->next = NULL;	g->pstart = NULL; g->plast = NULL; g->cstart = NULL; g->clast = NULL;
		g->prev = NULL; g->combined = NULL;
		g->id = -1;
		return g;
}
static group_t * group_combined(group_t *g)
{
	while (g->combined != NULL)
		g = g->combined;
	return g;
}

static void
add_participant (group_t *g, int i)
{
	participant_t *nw = participant_new();
	nw->next = NULL;
	nw->name = Name[i];
	nw->id = i;
	if (g->pstart == NULL) {
		g->pstart = nw; 
		g->plast = nw;	
	} else {
		g->plast->next = nw;	
		g->plast = nw;
	}
}

static void
add_connection (group_t *g, int i)
{
int group_id;
	connection_t *nw = connection_new();
	nw->next = NULL;
	nw->node = &Gnode[i];

group_id = -1;
if (Gnode[i].group) group_id = Gnode[i].group->id;

	if (g->cstart == NULL) {
		g->cstart = nw; 
		g->clast = nw;	
	} else {
		connection_t *l, *c;
		bool_t found = FALSE;
		for (c = g->cstart; !found && c != NULL; c = c->next) {
			node_t *nd = c->node;
			found = nd && nd->group && nd->group->id == group_id;
		}
		if (!found) {
			l = g->clast;
			l->next  = nw;
			g->clast = nw;
		}
	}		
}

static group_t * group_find(int id)
{
	group_t * s;
	for (s = group_head(); s != NULL; s = s->next) {
		if (id == s->id) return s;
	}
	return NULL;
}

//=========================

static bool_t encounter_is_SW(struct ENC *e) {return (e->played - e->wscore) < 0.0001;}
static bool_t encounter_is_SL(struct ENC *e) {return              e->wscore  < 0.0001;}

struct ENC 		SE[MAXENCOUNTERS];
static int 		N_se = 0;
struct ENC 		SE2[MAXENCOUNTERS];
static int 		N_se2 = 0;
static int 		group_belong[MAXPLAYERS];
static int		N_groups;

void
scan_encounters(void)
{
	int i,e,g;
	int c=1,j=1;
	struct ENC *pe;
	int gw, gb, lowerg, higherg;

	N_groups = N_players;
	for (i = 0; i < N_players; i++) {
		group_belong[i] = i;
	}

	for (e = 0; e < N_encounters; e++) {

		pe = &Encounter[e];
		if (encounter_is_SL(pe) || encounter_is_SW(pe)) {
			SE[N_se++] = *pe;
		} else {
			gw = group_belong[pe->wh];
			gb = group_belong[pe->bl];
			if (gw != gb) {
				lowerg   = gw < gb? gw : gb;
				higherg  = gw > gb? gw : gb;
				// join
				for (i = 0; i < N_players; i++) {
					if (group_belong[i] == higherg) {
						group_belong[i] = lowerg;
					}
				}
				N_groups--;
			}
		}
	} 

	for (e = 0, N_se2 = 0 ; e < N_se; e++) {
		int x,y;
		x = SE[e].wh;
		y = SE[e].bl;	
		if (group_belong[x] != group_belong[y]) {
			SE2[N_se2++] = SE[e];
		}
	}

	for (e = 0; e < N_se2; e++) {
		int x,y;
		x = SE2[e].wh;
		y = SE2[e].bl;	
		printf ("%s - %s = %.1f/%d\n",Name[x], Name[y], SE2[e].wscore, SE2[e].played);
	}

	printf ("Players=%d\n",N_players);
	printf ("Groups=%d\n",N_groups);
	printf ("Joining encounters=%d\n\n",N_se2);
	
	c = 1; j = 1;
	for (g = 0; g < N_players; g++) {
		bool_t found;
		for (i = 0, found = FALSE; !found && i < N_players; i++) {
			found = group_belong[i] == g;
		}
		if (found) { 
			printf ("GROUP=%d\n",j++);
			for (i = 0; i < N_players; i++) {
				if (group_belong[i] == g) {
					printf ("%3d    %s\n",c++,Name[i]);
				}
			}
			printf ("\n");
		}
	}
	return;
}

static void
convert_general_init(void)
{
	int i;
	connect_init();
	participant_init();
	group_init();
	for (i = 0; i < N_players; i++) {
		Gnode[i].group = NULL;
	}
	return;
}

static int get_iwin(struct ENC *pe) {return pe->wscore > 0.5? pe->wh: pe->bl;}
static int get_ilos(struct ENC *pe) {return pe->wscore > 0.5? pe->bl: pe->wh;}

static void
enc2groups (struct ENC *pe)
{
	int iwin, ilos;
	group_t *glw, *gll, *g, *f;

	iwin = get_iwin(pe);
	ilos = get_ilos(pe);

	if (Gnode[iwin].group == NULL) {

		g = group_find (group_belong[iwin]);
		if (g == NULL) {
			// creation
			f = group_tail();
			g = group_reset(group_new());		
			if (f != NULL) f->next = g; 
			g->id = group_belong[iwin];
			g->prev = f;
			Gnode[iwin].group = g;
		}
		glw = g;
	} else {
		glw = Gnode[iwin].group;
	}

	if (Gnode[ilos].group == NULL) {

		g = group_find (group_belong[ilos]);
		if (g == NULL) {
			// creation
			f = group_tail();
			g = group_reset(group_new());	
			if (f != NULL) f->next = g; 
			g->id = group_belong[ilos];
			g->prev = f;
			Gnode[ilos].group = g;
		}
		gll = g;
	} else {
		gll = Gnode[ilos].group;
	}
#if 1
	{	
		int i;
		printf ("Add: %3d %3d = %s // %s\n", iwin, ilos, Name[iwin], Name[ilos]);
		printf ("Grp: %3d %3d \n", group_belong[iwin], group_belong[ilos]);
		for (i = 0; i < group_buffer.n; i++) {
			printf("groups --> %3d\n",group_buffer.list[i].id);
		}
		printf("\n");
	}
#endif

//	add_participant(glw, iwin);
	Gnode[iwin].group = glw;

//	add_participant(gll, ilos);
	Gnode[ilos].group = gll;

	add_connection (glw, ilos);
}


static void
group_combine(group_t *g, group_t *h);

static void
convert_to_groups(void)
{
	group_t *s;
	participant_t *p;
	connection_t *c;

	int e;
	convert_general_init();

printf ("N_se2=%d\n",N_se2);

	for (e = 0 ; e < N_se2; e++) {
		enc2groups(&SE2[e]);
	}

#if 1
{
	int i;
	for (i = 0; i < N_players; i++) {
		int gb = group_belong[i];
		group_t *g = group_find(gb);
		add_participant(g, i);	
	}
}
#endif


group_combine(&group_buffer.list[1],&group_buffer.list[2]);
group_combine(&group_buffer.list[1],&group_buffer.list[3]);
group_combine(&group_buffer.list[0],&group_buffer.list[1]);

printf ("groups added=%d\n",group_buffer.n);

	for (s = group_head(); s != NULL; s = s->next) {

		printf ("\ngroup=%d\n",s->id);
		
		for (p = s->pstart; p != NULL; p = p->next) {
			printf ("name:%s\n",p->name);
		}
		
		for (c = s->cstart; c != NULL; c = c->next) {

			node_t *nd = c->node;
			if (nd) {
				group_t *gr = nd->group;
				if (gr) {
					gr = group_combined(gr);
					printf ("point to:%d\n",gr->id);
				} else {
					printf ("point to group NULL\n");
				}
			} else {
					printf ("point to node NULL\n");
			}
		}
	}
	return;
}

static void
group_combine(group_t *g, group_t *h)
{
	// unlink h
	group_t *pr = h->prev;
	group_t *ne = h->next;
	h->prev = NULL;
	h->next = NULL;
	assert(pr);
	pr->next = ne;
	if (ne) ne->prev = pr;
	
	h->combined = g;
	//
	g->plast->next = h->pstart;
	g->plast = h->plast;
	h->plast = NULL;
	h->pstart = NULL;	

	g->clast->next = h->cstart;
	g->clast = h->clast;
	h->clast = NULL;
	h->cstart = NULL;
}
