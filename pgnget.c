#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "mystr.h"
#include "pgnget.h"
#include "boolean.h"
#include "ordolim.h"

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

struct DATA DB;
static void 	data_init (struct DATA *d);

/*------------------------------------------------------------------------*/

static bool_t	playeridx_from_str (const char *s, int *idx);
static bool_t	addplayer (const char *s, int *i);
static void		report_error 	(long int n);
static int		res2int 		(const char *s);
static bool_t 	fpgnscan (FILE *fpgn, bool_t quiet);
static bool_t 	is_complete (struct pgn_result *p);
static void 	pgn_result_reset (struct pgn_result *p);
static bool_t 	pgn_result_collect (struct pgn_result *p);

/*
static void		skip_string 	(FILE *f, long int *counter);
static void		read_stringln 	(FILE *f, char s[], int sz);
static void		skip_comment 	(FILE *f, long int *counter);
static void		read_tagln 		(FILE *f, char s[], char t[], int sz, long int *counter);
static void		skip_variation 	(FILE *f, long int *counter);
static bool_t	isresultcmd 	(const char *s);
static bool_t 	iswhitecmd (const char *s);
static bool_t 	isblackcmd (const char *s);
*/

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
	d->labels_end_idx = 0;
	d->n_players = 0;
	d->n_games = 0;
}


static bool_t
playeridx_from_str (const char *s, int *idx)
{
	int i;
	bool_t found;
	for (i = 0, found = FALSE; !found && i < DB.n_players; i++) {
		ptrdiff_t x = DB.name[i];
		char * l = DB.labels + x;	
		found = (0 == strcmp (s, l) );
		if (found) *idx = i;
	}
	return found;
}

static bool_t
addplayer (const char *s, int *idx)
{
	long int i;
	char *b = DB.labels + DB.labels_end_idx;
	long int remaining = (long int) (&DB.labels[LABELBUFFERSIZE] - b - 1);
	long int len = (long int) strlen(s);
	bool_t success = len < remaining && DB.n_players < MAXPLAYERS;

	if (success) {
		int x = DB.n_players++;
		*idx = x;
		DB.name[x] = b - DB.labels;
		for (i = 0; i < len; i++)  {
			*b++ = *s++;
		}
		*b++ = '\0';
	}

	DB.labels_end_idx = b - DB.labels;
	return success;
}

static void report_error (long int n) 
{
	fprintf(stderr, "\nParsing error in line: %ld\n", n+1);
}

#if 0
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
#endif

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

	ok = ok && DB.n_games < MAXGAMES;
	if (ok) {
		DB.white [DB.n_games] = i;
		DB.black [DB.n_games] = j;
		DB.score [DB.n_games] = p->result;
		DB.n_games++;
	}

	return ok;
}


static bool_t 
is_complete (struct pgn_result *p)
{
	return p->wtag_present && p->btag_present && p->result_present;
}


static void
parsing_error(long line_counter)
{
	report_error (line_counter);
	fprintf(stderr, "Parsing problems\n");
	exit(EXIT_FAILURE);
}


static bool_t
fpgnscan (FILE *fpgn, bool_t quiet)
{
#define MAX_MYLINE 40000

	char myline[MAX_MYLINE];

	const char *whitesep = "[White \"";
	const char *whiteend = "\"]";
	const char *blacksep = "[Black \"";
	const char *blackend = "\"]";
	const char *resulsep = "[Result \"";
	const char *resulend = "\"]";

	size_t whitesep_len, blacksep_len, resulsep_len;
	char *x, *y;

	struct pgn_result 	result;
//	int					c;
	long int			line_counter = 0;
	long int			game_counter = 0;

	if (NULL == fpgn)
		return FALSE;

	if (!quiet) 
		printf("\nimporting results (x1000): \n"); fflush(stdout);

	pgn_result_reset  (&result);

	whitesep_len = strlen(whitesep); 
	blacksep_len = strlen(blacksep); 
	resulsep_len = strlen(resulsep); 

	while (NULL != fgets(myline, MAX_MYLINE, fpgn)) {

		line_counter++;

		if (NULL != (x = strstr (myline, whitesep))) {
			x += whitesep_len;
			if (NULL != (y = strstr (myline, whiteend))) {
				*y = '\0';
						strcpy (result.wtag, x);
						result.wtag_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (NULL != (x = strstr (myline, blacksep))) {
			x += blacksep_len;
			if (NULL != (y = strstr (myline, blackend))) {
				*y = '\0';
						strcpy (result.btag, x);
						result.btag_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (NULL != (x = strstr (myline, resulsep))) {
			x += resulsep_len;
			if (NULL != (y = strstr (myline, resulend))) {
				*y = '\0';
						result.result = res2int (x);
						result.result_present = TRUE;
			} else {
				parsing_error(line_counter);
			}
		}

		if (is_complete (&result)) {
			if (!pgn_result_collect (&result)) {
				fprintf (stderr, "\nCould not collect more games: Limits reached\n");
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


#if 0
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
#endif

#if 0
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
#endif

static int
res2int (const char *s)
{
	if (!strcmp(s, "1-0")) {
		return WHITE_WIN;
	} else if (!strcmp(s, "0-1")) {
		return BLACK_WIN;
	} else if (!strcmp(s, "1/2-1/2")) {
		return RESULT_DRAW;
	} else {

printf("result problems: %s\n",s);
exit(0);
		return RESULT_DRAW;
}
}

/************************************************************************/



