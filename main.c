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

	static const char *example_str =
		" Processes file.pgn and calculates ratings.\n"
		" The general pool will average 2500\n"
		" Output is in output.csv (to be imported with a spreadsheet program, excel etc.)\n"
		;

	static const char *help_str =
		" -h          print this help\n"
		" -v          print version number and exit\n"
		" -L          display the license information\n"
		" -q          quiet (no screen progress updates)\n"
		" -a <avg>    set general rating average\n"
		" -A <player> anchor: rating given by '-a' is fixed for <player>, if provided\n"
		" -w <value>  white advantage value (default=0.0)\n"
		" -W          white advantage, automatically adjusted\n"
		" -z <value>  scaling: rating that means a winning chance of 0.731 (default=173)\n"
		" -p <file>   input file in .pgn format\n"
		" -c <file>   output file (comma separated value format)\n"
		" -o <file>   output file (text format), goes to the screen if not present\n"
		" -s  #       perform # simulations to calculate errors\n"
		" -e <file>   saves an error matrix, if -s was used\n"
		"\n"
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		;

const char *OPTION_LIST = "vhp:qWLa:A:o:c:s:w:z:e:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/

enum RESULTS {
	WHITE_WIN = 0,
	BLACK_WIN = 2,
	RESULT_DRAW = 1,
	DISCARD = 3
};

static char		Labelbuffer[LABELBUFFERSIZE] = {'\0'};
static char 	*Labelbuffer_end = Labelbuffer;

/* players */
static char 	*Name   [MAXPLAYERS];
static int 		N_players = 0;

enum 			{MAX_ANCHORSIZE=256};
static bool_t	Anchor_use = FALSE;
static int		Anchor = 0;
static char		Anchor_name[MAX_ANCHORSIZE] = "";

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

#if 1
#define NEW_ENC
#endif

/**/
static double	general_average = 2300.0;
static int		sorted  [MAXPLAYERS]; /* sorted index by rating */
static double	obtained[MAXPLAYERS];
static double	expected[MAXPLAYERS];
static int		playedby[MAXPLAYERS]; /* N games played by player "i" */
static double	ratingof[MAXPLAYERS]; /* rating current */
static double	ratingbk[MAXPLAYERS]; /* rating backup  */

static double	Rating_results[MAXPLAYERS];
static double	Obtained_results[MAXPLAYERS];

static double	sum1[MAXPLAYERS]; 
static double	sum2[MAXPLAYERS]; 
static double	sdev[MAXPLAYERS]; 

static long 	Simulate = 0;

static double	White_advantage = 0;
static double	Inv_beta = 173.0;
static double	BETA = 1.0/173.0;

struct DEVIATION_ACC {
	double sum1;
	double sum2;
	double sdev;
};

struct DEVIATION_ACC *sim = NULL;

/*------------------------------------------------------------------------*/

void			calc_encounters (void);
void			calc_obtained_playedby_ENC (void);
void			calc_expected_ENC (void);
void			calc_expected_ORI (void);

void			all_report (FILE *csvf, FILE *textf);
void			calc_obtained_playedby_ORI (void);
void			init_rating (void);
void			calc_expected (void);
double			xpect (double a, double b);
void			adjust_rating (double delta);
void			calc_rating (bool_t quiet);
double 			deviation (void);
void			ratings_restore (void);
void			ratings_backup  (void);


static void 	ratings_results (void);

static void		simulate_scores(void);
static void		errorsout(const char *out);

/*------------------------------------------------------------------------*/

static void 	transform_DB(struct DATA *db);
static bool_t	find_anchor_player(int *anchor);

/*------------------------------------------------------------------------*/

static double 	overallerror_fwadv (double wadv);
static double 	adjust_wadv (double start_wadv);

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
	int version_mode, help_mode, license_mode, input_mode;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode    = FALSE;
	input_mode   = FALSE;
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
			case 'p': 	input_mode = TRUE;
					 	inputf = opt_arg;
						break;
			case 'c': 	csvstr = opt_arg;
						break;
			case 'o': 	textstr = opt_arg;
						break;
			case 'e': 	ematstr = opt_arg;
						break;
			case 'a': 	if (1 != sscanf(opt_arg,"%lf", &general_average)) {
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
			case 'z': 	if (1 != sscanf(opt_arg,"%lf", &Inv_beta)) {
							fprintf(stderr, "wrong scaling parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
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
	
	transform_DB(&DB); /* convert DB to global variables */

	if (Anchor_use) {
		if (!find_anchor_player(&Anchor)) {
			fprintf (stderr, "ERROR: No games of anchor player, mispelled, wrong capital letters, or extra spaces = \"%s\"\n", Anchor_name);
			fprintf (stderr, "Surround the name with \"quotes\" if it contains spaces");
			return EXIT_FAILURE; 			
		} else {
			printf("anchor selected: %d, %s\n",Anchor, Anchor_name);
		}
	}

	init_rating();

	if (!QUIET_MODE) {
		printf("\nset average rating = %lf\n\n",general_average);
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

	/*==== CALCULATIONS ====*/

	randfast_init (1324561);

	BETA = 1.0/Inv_beta;
	
	{	long i;
		for (i = 0; i < N_players; i++) {
			sum1[i] = 0;
			sum2[i] = 0;
			sdev[i] = 0;
		}
	}

	calc_rating(QUIET_MODE);
	ratings_results();

	if (ADJUST_WHITE_ADVANTAGE) {
		double new_wadv = adjust_wadv (White_advantage);
		printf ("\nAdjusted White Advantage = %f\n\n",new_wadv);
		White_advantage = new_wadv;
	
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
					if (!QUIET_MODE) printf ("Simulation=%ld/%ld\n",Simulate-z,Simulate);
					simulate_scores();
					calc_rating(QUIET_MODE);
					for (i = 0; i < N_players; i++) {
						sum1[i] += ratingof[i];
						sum2[i] += ratingof[i]*ratingof[i];
					}
	
					for (i = 0; i < (N_players); i++) {
						for (j = 0; j < i; j++) {
							idx = (i*i-i)/2+j;
							assert(idx < est || !printf("idx=%ld est=%ld\n",idx,est));
							diff = ratingof[i] - ratingof[j];	
							sim[idx].sum1 += diff; 
							sim[idx].sum2 += diff * diff;
						}
					}
				}

				for (i = 0; i < N_players; i++) {
					sdev[i] = sqrt( sum2[i]/n - (sum1[i]/n) * (sum1[i]/n));
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
	const char *example_options = "-a 2500 -p file.pgn -o output.csv";
	fprintf (stderr, "\n"
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
	const char *usage_options = "[-OPTION]";
	fprintf (stderr, "\n"
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
transform_DB(struct DATA *db)
{
	int i;
	ptrdiff_t x;
	for (x = 0; x < db->labels_end_idx; x++) {
		Labelbuffer[x] = db->labels[x];
	}
	Labelbuffer_end = Labelbuffer + db->labels_end_idx;
	N_players = db->n_players;
	N_games   = db->n_games;

	for (i = 0; i < db->n_players; i++) {
		Name[i] = Labelbuffer + db->name[i];
	}

	for (i = 0; i < db->n_games; i++) {
		Whiteplayer[i] = db->white[i];
		Blackplayer[i] = db->black[i]; 
		Score[i]       = db->score[i];
	}
}

static bool_t
find_anchor_player(int *anchor)
{
	int i;
	bool_t found = FALSE;
	for (i = 0; i < N_players; i++) {
		//printf ("%d, %s\n",i,Name[i]);
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

	const double da = Rating_results[*ja];
	const double db = Rating_results[*jb];
    
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
			y = sorted[i];

			fprintf(f, "%ld,\"%21s\"",i,Name[y]);

			for (j = 0; j < i; j++) {
				x = sorted[j];

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

void
all_report (FILE *csvf, FILE *textf)
{
	FILE *f;
	int i, j;
	size_t ml;

#ifdef NEW_ENC
	calc_encounters();
	calc_obtained_playedby_ENC();
#else
	calc_obtained_playedby_ORI();
#endif

	for (j = 0; j < N_players; j++) {
		sorted[j] = j;
	}

	qsort (sorted, (size_t)N_players, sizeof (sorted[0]), compareit);

	/* output in text format */
	f = textf;
	if (f != NULL) {

		ml = find_maxlen (Name, N_players);
		//intf ("max length=%ld\n", ml);
		if (ml > 50) ml = 50;

		if (Simulate < 2) {
			fprintf(f, "\n%s %-*s : %7s %9s %7s %6s\n", 
				"   #", 			
				(int)ml,
				"ENGINE", "RATING", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {
				j = sorted[i];
				fprintf(f, "%4d %-*s: %7.1f %9.1f   %5d   %4.1f%s\n", 
					i+1,
					(int)ml+1,
					Name[j], Rating_results[j], Obtained_results[j], playedby[j], 100.0*Obtained_results[j]/playedby[j], "%");
			}
		} else {
			fprintf(f, "\n%s %-*s : %7s %6s %8s %7s %6s\n", 
				"   #", 
				(int)ml, 
				"ENGINE", "RATING", "ERROR", "POINTS", "PLAYED", "(%)");
	
			for (i = 0; i < N_players; i++) {
				j = sorted[i];
				fprintf(f, "%4d %-*s: %7.1f %6.1f %8.1f   %5d   %4.1f%s\n", 
					i+1,
					(int)ml+1, 
					Name[j], Rating_results[j], sdev[j], Obtained_results[j], playedby[j], 100.0*Obtained_results[j]/playedby[j], "%");
			}
		}

	} /*if*/

	/* output in a comma separated value file */
	f = csvf;
	if (f != NULL) {
		for (i = 0; i < N_players; i++) {
			j = sorted[i];
			fprintf(f, "\"%21s\", %6.1f,"
			",%.2f"
			",%d"
			",%.2f"
			"\n"		
			,Name[j]
			,ratingof[j] 
			,obtained[j]
			,playedby[j]
			,100.0*obtained[j]/playedby[j] 
			);
		}
	}

	return;
}

/************************************************************************/

void
calc_encounters (void)
{
	int i, e = 0;

	for (i = 0; i < N_games; i++) {

		if (Score[i] >= DISCARD) continue;

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
}


void
calc_obtained_playedby_ENC (void)
{
	int e, j, w, b;

	for (j = 0; j < N_players; j++) {
		obtained[j] = 0.0;	
		playedby[j] = 0;
	}	

	for (e = 0; e < N_encounters; e++) {
	
		w = Encounter[e].wh;
		b = Encounter[e].bl;

		obtained[w] += Encounter[e].wscore;
		obtained[b] += (double)Encounter[e].played - Encounter[e].wscore;

		playedby[w] += Encounter[e].played;
		playedby[b] += Encounter[e].played;

	}
}

void
calc_expected_ENC (void)
{
	int e, j, w, b;
	double f, rw, rb;

	for (j = 0; j < N_players; j++) {
		expected[j] = 0.0;	
	}	

	for (e = 0; e < N_encounters; e++) {
	
		w = Encounter[e].wh;
		b = Encounter[e].bl;

		rw = ratingof[w];
		rb = ratingof[b];

		f = xpect (rw + White_advantage, rb);
		expected [w] += Encounter[e].played * f;
		expected [b] += Encounter[e].played * (1.0 - f);	
	}
}

void
calc_obtained_playedby_ORI (void)
{
	int i, j, w, b, s;

	for (j = 0; j < N_players; j++) {
		obtained[j] = 0.0;	
		playedby[j] = 0;
	}	

	for (i = 0; i < N_games; i++) {
	
		if (Score[i] == DISCARD) continue;

		w = Whiteplayer[i];
		b = Blackplayer[i];
		s = Score[i];		

		if (s == WHITE_WIN) {
			obtained[w] += 1.0;
		}
		if (s == BLACK_WIN) {
			obtained[b] += 1.0;
		}
		if (s == RESULT_DRAW) {
			obtained[w] += 0.5;
			obtained[b] += 0.5;
		}

		playedby[w] += 1;
		playedby[b] += 1;

	}
}


void
init_rating (void)
{
	int i;
	for (i = 0; i < N_players; i++) {
		ratingof[i] = general_average;
	}
	for (i = 0; i < N_players; i++) {
		ratingbk[i] = general_average;
	}
}

void
calc_expected (void)
{
#ifdef NEW_ENC
	calc_expected_ENC();
#else
	calc_expected_ORI();
#endif
}

void
calc_expected_ORI (void)
{
	int i, j, w, b;
	double f, rw, rb;

	for (j = 0; j < N_players; j++) {
		expected[j] = 0.0;	
	}	

	for (i = 0; i < N_games; i++) {
	
		if (Score[i] == DISCARD) continue;

		w = Whiteplayer[i];
		b = Blackplayer[i];

		rw = ratingof[w];
		rb = ratingof[b];

		f = xpect (rw + White_advantage, rb);
		expected [w] += f;
		expected [b] += 1.0 - f;	
	}
}


void
adjust_rating (double delta)
{
	double accum = 0;
	double excess, average;
	int j;

	for (j = 0; j < N_players; j++) {
		if (expected[j] > obtained [j]) {
			ratingof[j] -= delta;
		} else {
			ratingof[j] += delta;
		}
	}	

	for (accum = 0, j = 0; j < N_players; j++) {
		accum += ratingof[j];
	}		

	average = accum / N_players;
	excess  = average - general_average;

	if (Anchor_use) {
		excess  = ratingof[Anchor] - general_average;	
	}

	for (j = 0; j < N_players; j++) {
		ratingof[j] -= excess;
	}	

}

void
ratings_restore (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		ratingof[j] = ratingbk[j];
	}	
}

void
ratings_backup (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		ratingbk[j] = ratingof[j];
	}	
}

#if 1
void
ratings_results (void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		Rating_results[j] = ratingof[j];
	}	
	for (j = 0; j < N_players; j++) {
		Obtained_results[j] = obtained[j];
	}
}

static void
simulate_scores(void)
{
	long int i, w, b, z, y;
	double f;
	double	*rating = Rating_results;

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
		diff = expected[j] - obtained [j];
		accum += diff * diff;
	}		
	return accum;
}

void
calc_rating (bool_t quiet)
{
	double 	olddev, curdev, outputdev;
	int 	i;

	int		rounds = 10000;
	double 	delta = 100.0;
	double 	denom = 2;
	int 	phase = 0;
	int 	n = 20;

#ifdef NEW_ENC
	calc_encounters();
	calc_obtained_playedby_ENC();
#else
	calc_obtained_playedby_ORI();
#endif

	calc_expected();
	olddev = curdev = deviation();

	if (!quiet) printf ("%3s %4s %10s\n", "phase", "iteration", "deviation");

	while (n-->0) {

		for (i = 0; i < rounds; i++) {

			ratings_backup();
			olddev = curdev;

			adjust_rating(delta);
			calc_expected();
			curdev = deviation();

			if (curdev >= olddev) {
				ratings_restore();
				calc_expected();
				curdev = deviation();	
				assert (curdev == olddev);
				break;
			};	
		}

		delta = delta/denom;

		outputdev = sqrt(curdev)/N_players;

		if (!quiet) printf ("%3d %7d %14.5f\n", phase, i, outputdev);
		phase++;
	}

	if (!quiet) printf ("done\n\n");
}


double
xpect (double a, double b)
{
	double diff = a - b;
	return 1.0 / (1.0 + exp(-diff*BETA));
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

		rw = ratingof[Whiteplayer[i]];
		rb = ratingof[Blackplayer[i]];

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



