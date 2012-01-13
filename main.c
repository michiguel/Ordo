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
		" -h        print this help\n"
		" -v        print version number and exit\n"
		" -L        display the license information\n"
		" -q        quiet (no screen progress updates)\n"
		" -a <avg>  set general rating average\n"
		" -p <file> input file in .pgn format\n"
		" -c <file> output file (comma separated value format)\n"
		" -o <file> output file (text format), goes to the screen if not present\n"
		"\n"
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		;

const char *OPTION_LIST = "vhp:qLa:o:c:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/


#define PGNSTRSIZE 1024

enum RESULTS {
	WHITE_WIN = 0,
	BLACK_WIN = 2,
	RESULT_DRAW = 1
};

struct pgn_result {	
	int 	wtag_present;
	int 	btag_present;
	int 	result_present;	
	int 	result;
	char 	wtag[PGNSTRSIZE];
	char 	btag[PGNSTRSIZE];
};


#define MAXGAMES 1000000
#define LABELBUFFERSIZE 100000
#define MAXPLAYERS 10000

static char		Labelbuffer[LABELBUFFERSIZE] = {'\0'};
static char 	*Labelbuffer_end = Labelbuffer;

/* players */
static char 	*Name   [MAXPLAYERS];
static double	obtained[MAXPLAYERS];
static double	expected[MAXPLAYERS];
static int		playedby[MAXPLAYERS]; /* N games played by player "i" */
static double	ratingof[MAXPLAYERS]; /* rating current */
static double	ratingbk[MAXPLAYERS]; /* rating backup  */
static int 		N_players = 0;

static double	general_average = 2300.0;

static int		sorted  [MAXPLAYERS]; /* sorted index by rating */

/* games */
static int 		Whiteplayer	[MAXGAMES];
static int 		Blackplayer	[MAXGAMES];
static int 		Score		[MAXGAMES];
static int 		N_games = 0;

/*------------------------------------------------------------------------*/

void			all_report (FILE *csvf, FILE *textf);
void			calc_obtained_playedby (void);
void			init_rating (void);
void			calc_expected (void);
double			xpect (double a, double b);
void			adjust_rating (double delta);
void			calc_rating (bool_t quiet);
double 			deviation (void);
void			ratings_restore (void);
void			ratings_backup  (void);

/*------------------------------------------------------------------------*/

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
	const char *inputf, *textstr, *csvstr;
	int version_mode, help_mode, license_mode, input_mode;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode    = FALSE;
	input_mode   = FALSE;
	QUIET_MODE   = FALSE;
	inputf       = NULL;
	textstr 	 = NULL;
	csvstr       = NULL;

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
			case 'a': 	
						if (1 != sscanf(opt_arg,"%lf", &general_average)) {
							fprintf(stderr, "wrong average parameter\n");
							exit(EXIT_FAILURE);
						}
						break;
			case 'q':	QUIET_MODE = TRUE;	break;
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
		fprintf (stderr, "Extra parameters present\n");
		exit(EXIT_FAILURE);
	}
	if (input_mode && argc != opt_index) {
		fprintf (stderr, "Extra parameters present\n");
		exit(EXIT_FAILURE);
	}
	if (!input_mode && argc == opt_index) {
		fprintf (stderr, "Need file name to proceed\n");
		exit(EXIT_FAILURE);
	}
	/* get folder, should be only one at this point */
	while (opt_index < argc ) {
		inputf = argv[opt_index++];
	}

	/*==== CALCULATIONS ====*/

	if (!pgn_getresults(inputf, QUIET_MODE)) {
		printf ("Problems reading results from: %s\n", inputf);
		return EXIT_FAILURE; 
	}
	
	transform_DB(&DB); /* convert DB to global variables */

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

	calc_rating(QUIET_MODE);

	all_report (csvf, textf);
	
	if (textf_opened) fclose (textf);
	if (csvf_opened)  fclose (csvf); 

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
	const char *example_options = "-a 2500 -i file.pgn -o output.csv";
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


static int
compareit (const void *a, const void *b)
{
	const int *ja = (const int *) a;
	const int *jb = (const int *) b;

	const double da = ratingof[*ja];
	const double db = ratingof[*jb];
    
	return (da < db) - (da > db);
}


void
all_report (FILE *csvf, FILE *textf)
{
	FILE *f;
	int i, j;

	calc_obtained_playedby ();

	for (j = 0; j < N_players; j++) {
		sorted[j] = j;
	}

	qsort (sorted, (size_t)N_players, sizeof (sorted[0]), compareit);

	/* output in text format */
	f = textf;
	if (f != NULL) {

		fprintf(f, "\n%30s: %7s %9s %7s %6s\n", 
			"ENGINE", "RATING", "POINTS", "PLAYED", "(%)");

		for (i = 0; i < N_players; i++) {
			j = sorted[i];
			fprintf(f, "%30s:  %5.1f    %6.1f   %5d   %4.1f%s\n", 
				Name[j], ratingof[j], obtained[j], playedby[j], 100.0*obtained[j]/playedby[j], "%");
		}
	}


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
calc_obtained_playedby (void)
{
	int i, j, w, b, s;

	for (j = 0; j < N_players; j++) {
		obtained[j] = 0.0;	
		playedby[j] = 0;
	}	

	for (i = 0; i < N_games; i++) {
	
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
	int i, j, w, b;
	double f, rw, rb;

	for (j = 0; j < N_players; j++) {
		expected[j] = 0.0;	
	}	

	for (i = 0; i < N_games; i++) {
	
		w = Whiteplayer[i];
		b = Blackplayer[i];

		rw = ratingof[w];
		rb = ratingof[b];

		f = xpect (rw, rb);
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
	int i, rounds;
	double delta, denom;
	double olddev, curdev;
	int phase = 0;

	int n = 20;

	delta = 100.0;
	denom = 2;
	rounds = 10000;

	calc_obtained_playedby();

	init_rating();
	calc_expected();
	olddev = curdev = deviation();

	if (!quiet) 
		printf ("%3s %4s %10s\n", "phase", "iteration", "deviation");

	while (n-->0) {
		double outputdev;

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

		delta = delta /denom;

		outputdev = sqrt(curdev)/N_players;

		if (!quiet) 
			printf ("%3d %7d %14.5f\n", phase, i, outputdev);
		phase++;
	}

	if (!quiet) 
		printf ("done\n");
}


double
xpect (double a, double b)
{
	double k = 173;
	double diff = a - b;
	return 1.0 / (1.0 + exp(-diff/k));
}
