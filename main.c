#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "mystr.h"
#include "proginfo.h"

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

/* TYPES */

	typedef int bool_t;
	#define FALSE 0
	#define TRUE !FALSE

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
		" -q        quiet (no output except error messages)\n"
		" -a <avg>  set general rating average\n"
		" -i <file> input file\n"
		" -o <file> output file (comma separated value format)\n"
		"\n"
	/*	 ....5....|....5....|....5....|....5....|....5....|....5....|....5....|....5....|*/
		;

const char *OPTION_LIST = "vhi:qLa:o:";

/*
|
|	ORDO DEFINITIONS
|
\*--------------------------------------------------------------*/


#define PGNSTRSIZE 1024

enum {
	WHITE_WIN = 0,
	BLACK_WIN = 2,
	RESULT_DRAW = 1
};

char *Result_string[] = {"1-0", "=-=", "0-1"};

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

static bool_t	playeridx_from_str (const char *s, int *idx);
static bool_t	addplayer (const char *s, int *i);
void			pgn_all_report (const char *outputf);
void			calc_obtained_playedby (void);
void			init_rating (void);
void			calc_expected (void);
double			xpect (double a, double b);
void			adjust_rating (double delta);
void			calc_rating (void);
double 			deviation (void);
void			ratings_restore (void);
void			ratings_backup  (void);

/*------------------------------------------------------------------------*/

static void		report_error 	(int n);

static void		skip_comment 	(FILE *f, long int *counter);
static void		read_tagln 		(FILE *f, char s[], char t[], int sz, long int *counter);
static void		skip_variation 	(FILE *f, long int *counter);
static void		skip_string 	(FILE *f, long int *counter);
static void		read_stringln 	(FILE *f, char s[], int sz);

static int		res2int 		(const char *s);
#if 0
static bool_t	issep 			(int c);
static bool_t	isend 			(const char *token);
#endif
static bool_t	isresultcmd 	(const char *s);

static bool_t 	fpgnscan (FILE *fpgn);

static bool_t 	iswhitecmd (const char *s);
static bool_t 	isblackcmd (const char *s);


static bool_t	pgn_getresults (const char *pgn);

/*
|
|	MAIN
|
\*--------------------------------------------------------------*/

int main (int argc, char *argv[])
{
	int op;
	char *inputf, *outputf;
	int version_mode, help_mode, license_mode, input_mode, output_mode;
	double general_average;

	/* defaults */
	version_mode = FALSE;
	license_mode = FALSE;
	help_mode    = FALSE;
	input_mode   = FALSE;
	output_mode  = FALSE;
	QUIET_MODE   = FALSE;
	inputf       = NULL;
	outputf      = NULL;

	while (END_OF_OPTIONS != (op = options (argc, argv, OPTION_LIST))) {
		switch (op) {
			case 'v':	version_mode = TRUE; 	break;
			case 'L':	version_mode = TRUE; 	
						license_mode = TRUE;
						break;
			case 'h':	help_mode = TRUE;		break;
			case 'i': 	input_mode = TRUE;
					 	inputf = opt_arg;
						break;
			case 'o': 	output_mode = TRUE;
					 	outputf = opt_arg;
						break;
			case 'a': 	
						if (1 == sscanf(opt_arg,"%lf", &general_average)) {
							printf("Average rating = %lf\n",general_average);
						} else {
							printf("wrong average parameter\n");
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
		fprintf (stderr, "Extra parameters parameters present\n");
		exit(EXIT_FAILURE);
	}
	if (input_mode && argc != opt_index) {
		fprintf (stderr, "Extra parameters parameters present\n");
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

	if (pgn_getresults(inputf)) {
		printf ("Success reading\n");
	} else {
		printf ("Problems reading\n");
		return EXIT_FAILURE; 
	}
	init_rating();

	printf ("====================================\n");

	calc_rating();
	pgn_all_report (outputf);

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
	char *example_options = "-a 2500 -i file.pgn -o output.csv";
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
	char *usage_options = "[-OPTION]";
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

static bool_t
playeridx_from_str (const char *s, int *idx)
{
	int i;
	bool_t found;

	if (N_players == 0)
		return FALSE;

	for (i = 0, found = FALSE; !found && i < N_players; i++) {
		found = (0 == strcmp (s, Name[i]) );
		if (found) *idx = i;
	}
	return found;
}

static bool_t
addplayer (const char *s, int *idx)
{
	int i;
	int remaining = &Labelbuffer[LABELBUFFERSIZE] - Labelbuffer_end - 1;
	int len = strlen(s);


	if (len < remaining && N_players < MAXPLAYERS) {
		*idx = N_players;
		Name[N_players++] = Labelbuffer_end;
		for (i = 0; i < len; i++)  {
			*Labelbuffer_end++ = *s++;
		}
		*Labelbuffer_end++ = '\0';
		return TRUE;
	}
	else
		return FALSE;
}

static bool_t
pgn_getresults (const char *pgn)
{

	FILE *fpgn;
	bool_t ok = TRUE;

	if (NULL == (fpgn = fopen (pgn, "r"))) {
		printf ("Cannot open %s\n", pgn);
		return FALSE;
	}

	ok = fpgnscan (fpgn);

	fclose(fpgn);
	return ok;

}



static void 
report_error (int n)
{
	fprintf(stderr, "\nParsing error in line: %d\n", n+1);

}


/**********************************************************************************************************************/

static void
skip_comment (FILE *f, long int *counter) 
{
	int c;
	while (EOF != (c = fgetc(f))) {
		
		if (c == '\n')
			*counter += 1;

		if (c == '\"') {
			skip_string (f, counter);
			continue;
		}
		if (c == '}') 
			break;
	}	
}


void
pgn_result_reset (struct pgn_result *p)
{
	p->wtag_present   = FALSE;
	p->btag_present   = FALSE;
	p->result_present = FALSE;	
	p->wtag[0] = '\0';
	p->btag[0] = '\0';
	p->result = 0;
}

void
pgn_result_report (struct pgn_result *p)
{
	int i, j;
	bool_t ok;

/*	printf ("%12s vs. %12s : %s\n", p->wtag, p->btag, Result_string[p->result]);*/


	if (!playeridx_from_str (p->wtag, &i)) {
		ok = addplayer (p->wtag, &i);
		if (!ok) {
			fprintf (stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	if (!playeridx_from_str (p->btag, &j)) {
		ok = addplayer (p->btag, &j);
		if (!ok) {
			fprintf (stderr, "out of memory\n");
			exit(EXIT_FAILURE);
		}
	}

	Whiteplayer [N_games] = i;
	Blackplayer [N_games] = j;
	Score       [N_games] = p->result;
	N_games++;
	return;
}

	int
	compareit (const void *a, const void *b)
	{
		const int *ja = (const int *) a;
		const int *jb = (const int *) b;

		const double da = ratingof[*ja];
		const double db = ratingof[*jb];
     
		return (da < db) - (da > db);
	}


void
pgn_all_report (const char *outputf)
{
	int i, j;

	calc_obtained_playedby ();

	#if 0
	for (j = 0; j < N_players; j++) {
		printf("%30s: %6.1f / %5d, %4.1f%s   rating = %4.1f\n", 
			Name[j], obtained[j], playedby[j], 100.0*obtained[j]/playedby[j], "%", ratingof[j]);
	}
	#endif

	for (j = 0; j < N_players; j++) {
		sorted[j] = j;
	}

	qsort (sorted, N_players, sizeof (sorted[0]), compareit);

	printf ("\nSORTED by RATING\n");

	for (i = 0; i < N_players; i++) {
		j = sorted[i];
		printf("%30s: %6.1f / %5d, %4.1f%s   rating = %4.1f\n", 
			Name[j], obtained[j], playedby[j], 100.0*obtained[j]/playedby[j], "%", ratingof[j]);
	}

	/* output in a comma separated value file */
	if (outputf != NULL)
	{
		FILE *f = fopen (outputf, "w");
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
		fclose (f);
		}
	}

	return;
}



bool_t 
is_complete (struct pgn_result *p)
{
	return p->wtag_present && p->btag_present && p->result_present;
}

static bool_t
fpgnscan (FILE *fpgn)
{

	struct pgn_result result;

	int			c;

//	bool_t		skiprest = FALSE;
	long int	line_counter = 0;
	long int	game_counter = 0;

//	int			endresult = 0;

	if (NULL == fpgn)
		return FALSE;

	printf("\nscanning games (x1000): \n"); fflush(stdout);

	pgn_result_reset  (&result);

	while (EOF != (c = fgetc(fpgn))) {

		if (c == '\n') {
			line_counter++;
			continue;
		}

		if (isspace(c) || c == '.') {
			continue;
		}

		switch (c) {

			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '0':

				break;

			case 'R':
			case 'N':
			case 'B':
			case 'K':
			case 'Q':
			case 'a':
			case 'b':
			case 'c':
			case 'd':
			case 'e':
			case 'f':
			case 'g':
			case 'h':
			case 'O':

			case '+': case '#': case 'x': case '/': case '-': case '=':

				break;

			case '*':
			
				break;

			case '[': 
				{	char cmmd[PGNSTRSIZE], prm [PGNSTRSIZE];
					read_tagln(fpgn, cmmd, prm, sizeof(cmmd), &line_counter); 
					if (isresultcmd(cmmd)) {
						result.result = res2int (prm);
						result.result_present = TRUE;
					}
					if (iswhitecmd(cmmd)) {
						strcpy (result.wtag, prm);
						result.wtag_present = TRUE;
					}
					if (isblackcmd(cmmd)) {
						strcpy (result.btag, prm);
						result.btag_present = TRUE;
					}
				}
				break;

			case '{':
				skip_comment(fpgn, &line_counter);
				break;

			case '(':
				skip_variation(fpgn, &line_counter);
				break;

			default:
				report_error (line_counter);
				printf("unrecognized character: %c :%d\n",c,c);
				break;

		} /* switch */

		if (is_complete (&result)) {
			pgn_result_report (&result);
			pgn_result_reset  (&result);
			game_counter++;
			if ((game_counter%1000)==0) {
				printf ("*"); fflush(stdout);
			}
		}

	} /* while */

	printf("\ntotal games: %7ld \n", game_counter); fflush(stdout);

	return TRUE;

}


static void  
read_tagln (FILE *f, char s[], char t[], int sz, long int *counter)
{
	int c;
	char prmstr [PGNSTRSIZE];
	char cmmd   [PGNSTRSIZE];
	char buf    [PGNSTRSIZE];
	char *p = cmmd;	
	char *limit = cmmd + PGNSTRSIZE - 1;

	while (p < limit && EOF != (c = fgetc(f))) {

		if (c == '\"') {
			read_stringln (f, prmstr, sizeof(prmstr));
			continue;
		}

		if (c == ']')
			break;
		
		if (c == '\n') { 
			*counter += 1;
		}

		*p++ = (char) c;
	}
	*p = '\0';

	sscanf (cmmd,"%s", buf);
	mystrncpy (s, buf, sz);
	mystrncpy (t, prmstr, sz);

}


static void
skip_variation (FILE *f, long int *counter)
{
	int level = 0;
	int c;

	while (EOF != (c = fgetc(f))) {
		
		if (c == '\n')
			*counter += 1;

		if (c == '(')
			level++;

		if (c == ')') { 
			if (level == 0)
				break;
			else
				level--;
		}
	}
}

static void
skip_string (FILE *f, long int *counter)
{
	int c;
	while (EOF != (c = fgetc(f))) {
		if (c == '\n')
			*counter += 1;
		if (c == '\"') 
			break;
	}
}


static void
read_stringln (FILE *f, char s[], int sz)

/* read the string until " unless there is an EOF of EOL */

{
	int		c;
	char	buffer[PGNSTRSIZE];
	char	*p     = buffer;
	char	*limit = buffer + PGNSTRSIZE - 1;

	while (p < limit && EOF != (c = fgetc(f))) {
		
		/* end */
		if (c == '\"') 
			break;
		
		/* end */
		if (c == '\n') { 
			ungetc(c, f);
			break;
		}

		/* escape sequence */
		if (c == '\\') {
			int cc = fgetc(f);
			if (EOF == cc)
				break;
			if ('\"' == cc || '\\' == cc) {
				*p++ = (char) cc;
			} else {
				*p++ = (char) c;
				if (p < limit)
					*p++ = (char) cc;				
			}
			continue;
		} 
		
		*p++ = (char) c;

	}
	*p = '\0';
	mystrncpy (s, buffer, sz);
}

#if 0
static bool_t
issep (int c) 
{
	return 	isspace(c) || c == '.' || c == '{' || c == '(';
}

static bool_t
isend (const char *token)
{
	return	   !strcmp(token,"1-0") 
			|| !strcmp(token,"0-1") 
			|| !strcmp(token,"1/2-1/2");
}
#endif

static bool_t
isresultcmd (const char *s)
{
	return !strcmp(s,"Result") || !strcmp(s,"result");
}

static bool_t
iswhitecmd (const char *s)
{
	return !strcmp(s,"White") || !strcmp(s,"white");
}

static bool_t
isblackcmd (const char *s)
{
	return !strcmp(s,"Black") || !strcmp(s,"black");
}


static int
res2int (const char *s)
{
	if (!strcmp(s, "1-0")) {
		return WHITE_WIN;
	} else if (!strcmp(s, "0-1")) {
		return BLACK_WIN;
	} else if (!strcmp(s, "1/2-1/2")) {
		return RESULT_DRAW;
	} else
		return RESULT_DRAW;
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

void
show_backup (void)
{
	int j;
	printf ("backup\n");
	for (j = 0; j < N_players; j++) {
		printf ("%d: %f\n",j,ratingbk[j]);
	}	
}

void
show_current (void)
{
	int j;
	printf ("current\n");
	for (j = 0; j < N_players; j++) {
		printf ("%d: %f\n",j,ratingof[j]);
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
show_partial(void)
{
	int j;
	for (j = 0; j < N_players; j++) {
		printf("%21s: p=%d, o=%4.1f,  e=%4.1f, rating = %4.1f\n", 
			Name[j], playedby[j], obtained[j], expected[j], ratingof[j]);
	}
}


void
calc_rating (void)
{
	int i, rounds;
	double delta, denom;
	double olddev, curdev;

	int n = 20;

	delta = 100.0;
	denom = 2;
	rounds = 10000;

	calc_obtained_playedby();

	init_rating();
	calc_expected();
	olddev = curdev = deviation();

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

		printf ("rounds: %d, deviation: %f\n", i, outputdev);
	}

	printf ("done\n");
}


double
xpect (double a, double b)
{
	double k = 173;
	double diff = a - b;
	return 1.0 / (1.0 + exp(-diff/k));
}
