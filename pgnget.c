#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "mystr.h"
#include "pgnget.h"
#include "boolean.h"

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

const char *Result_string[] = {"1-0", "=-=", "0-1"};

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

struct DATA {	
	int 	n_players;
	int 	n_games;
	char	labels[LABELBUFFERSIZE];
	char *	labels_end;
	char *	name	[MAXPLAYERS];
	int 	white	[MAXGAMES];
	int 	black	[MAXGAMES];
	int 	score	[MAXGAMES];
};
struct DATA DB;
static void 	data_init (struct DATA *d);


static char		Labelbuffer[LABELBUFFERSIZE] = {'\0'};
static char 	*Labelbuffer_end = Labelbuffer;

/* players */
static char 	*Name   [MAXPLAYERS];
static int 		N_players = 0;

/* games */
static int 		Whiteplayer	[MAXGAMES];
static int 		Blackplayer	[MAXGAMES];
static int 		Score		[MAXGAMES];
static int 		N_games = 0;

/*------------------------------------------------------------------------*/

static bool_t	playeridx_from_str (const char *s, int *idx);
static bool_t	addplayer (const char *s, int *i);
static void		report_error 	(long int n);
static void		skip_comment 	(FILE *f, long int *counter);
static void		read_tagln 		(FILE *f, char s[], char t[], int sz, long int *counter);
static void		skip_variation 	(FILE *f, long int *counter);
static void		skip_string 	(FILE *f, long int *counter);
static void		read_stringln 	(FILE *f, char s[], int sz);
static int		res2int 		(const char *s);
static bool_t	isresultcmd 	(const char *s);
static bool_t 	fpgnscan (FILE *fpgn, bool_t quiet);
static bool_t 	iswhitecmd (const char *s);
static bool_t 	isblackcmd (const char *s);
static bool_t 	is_complete (struct pgn_result *p);

static void 	pgn_result_reset (struct pgn_result *p);
static bool_t 	pgn_result_collect (struct pgn_result *p);


/*
|
|
\*--------------------------------------------------------------*/

bool_t
pgn_getresults (const char *pgn, bool_t quiet)
{
	FILE *fpgn;
	bool_t ok = FALSE;

	data_init (&DB);

	if (NULL != (fpgn = fopen (pgn, "r"))) {
		ok = fpgnscan (fpgn, quiet);
		fclose(fpgn);
	}
	return ok;
}

/*--------------------------------------------------------------*\
|
|
\**/


static void
data_init (struct DATA *d)
{
	d->labels[0] = '\0';
	d->labels_end = d->labels;
	d->n_players = 0;
	d->n_games = 0;
}


static bool_t
playeridx_from_str (const char *s, int *idx)
{
	int i;
	bool_t found;
	for (i = 0, found = FALSE; !found && i < N_players; i++) {
		found = (0 == strcmp (s, Name[i]) );
		if (found) *idx = i;
	}
	return found;
}

static bool_t
addplayer (const char *s, int *idx)
{
	long int i;
	char *b = Labelbuffer_end;
	long int remaining = (long int) (&Labelbuffer[LABELBUFFERSIZE] - b - 1);
	long int len = (long int) strlen(s);
	bool_t success = len < remaining && N_players < MAXPLAYERS;

	if (success) {
		*idx = N_players;
		Name[N_players++] = b;
		for (i = 0; i < len; i++)  {
			*b++ = *s++;
		}
		*b++ = '\0';
	}

	Labelbuffer_end = b;
	return success;
}

static void report_error (long int n) 
{
	fprintf(stderr, "\nParsing error in line: %ld\n", n+1);
}

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


static void
pgn_result_reset (struct pgn_result *p)
{
	p->wtag_present   = FALSE;
	p->btag_present   = FALSE;
	p->result_present = FALSE;	
	p->wtag[0] = '\0';
	p->btag[0] = '\0';
	p->result = 0;
}

static bool_t
pgn_result_collect (struct pgn_result *p)
{
	int i, j;
	bool_t ok = TRUE;

	if (ok && !playeridx_from_str (p->wtag, &i)) {
		ok = addplayer (p->wtag, &i);
	}

	if (ok && !playeridx_from_str (p->btag, &j)) {
		ok = addplayer (p->btag, &j);
	}

	if (ok) {
		Whiteplayer [N_games] = i;
		Blackplayer [N_games] = j;
		Score       [N_games] = p->result;
		N_games++;
	}

	return ok;
}


static bool_t 
is_complete (struct pgn_result *p)
{
	return p->wtag_present && p->btag_present && p->result_present;
}

static bool_t
fpgnscan (FILE *fpgn, bool_t quiet)
{
	struct pgn_result 	result;
	int					c;
	long int			line_counter = 0;
	long int			game_counter = 0;

	if (NULL == fpgn)
		return FALSE;

	if (!quiet) 
		printf("\nimporting results (x1000): \n"); fflush(stdout);

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
			if (!pgn_result_collect (&result)) {
				fprintf (stderr, "out of memory\n");
				exit(EXIT_FAILURE);
			}
			pgn_result_reset  (&result);
			game_counter++;

			if (!quiet) {
				if ((game_counter%1000)==0) {
					printf ("*"); fflush(stdout);
				}
				if ((game_counter%40000)==0) {
					printf ("  %4ldk\n", game_counter/1000); fflush(stdout);
				}
			}
		}

	} /* while */

	if (!quiet) 
		printf("  total games: %7ld \n", game_counter); fflush(stdout);

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












