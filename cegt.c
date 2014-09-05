/*
	Ordo is program for calculating ratings of engine or chess players
    Copyright 2013 Miguel A. Ballicora

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <assert.h>
#include <stddef.h>

#include "cegt.h"
#include "mytypes.h"

#include "ordolim.h"
#include "gauss.h"

struct OPP_LINE {
	long i;
	long w;
	long d;
	long l;
	double R;
};

static struct OPP_LINE oline[MAXPLAYERS];

// STATICS
static size_t find_maxlen (char *nm[], long int n);
static const char *SP_symbolstr[3] = {"<",">"," "};
static const char *get_super_player_symbolstr(int j, struct CEGT *p);
static void all_report_rat (FILE *textf, struct CEGT *p);
static void all_report_prg (FILE *textf, struct CEGT *p);
static void all_report_gen (FILE *textf, struct CEGT *p);

static void	all_report_indiv_stats (FILE *textf, struct CEGT *p, int simulate);

static bool_t 
output_report_individual_f (FILE *indf, struct CEGT *p, int simulate);

static bool_t 
output_cegt_style_f (FILE *genf, FILE *ratf, FILE *prgf, struct CEGT *p);

extern bool_t 
output_cegt_style (const char *general_name, const char *rating_name, const char *programs_name, struct CEGT *p)
{
	bool_t success;
	FILE *genf, *ratf, *prgf;

	printf ("\n");
	printf ("Output (Elostat format):\n");
	printf ("general file = %s\n",general_name);
	printf ("rating file = %s\n",rating_name);
	printf ("programs file = %s\n",programs_name);


	if (NULL ==	(genf = fopen (general_name, "w"))) {
			fprintf(stderr, "Error trying to write on file: %s\n",general_name);		
			return FALSE;	
	}

	if (NULL ==	(ratf = fopen (rating_name, "w"))) {
			fprintf(stderr, "Error trying to write on file: %s\n",rating_name);		
			fclose(genf);	
			return FALSE;	
	}

	if (NULL ==	(prgf = fopen (programs_name, "w"))) {
			fprintf(stderr, "Error trying to write on file: %s\n",programs_name);		
			fclose(genf);
			fclose(ratf);
			return FALSE;	
	}

	success = output_cegt_style_f (genf, ratf, prgf, p);

	fclose(genf);
	fclose(ratf);
	fclose(prgf);
	return success;
} 


static bool_t 
output_cegt_style_f (FILE *genf, FILE *ratf, FILE *prgf, struct CEGT *p)
{
	if (ratf)
		all_report_rat (ratf, p);

	if (prgf)
		all_report_prg (prgf, p);

	if (genf)
		all_report_gen (genf, p);

	return ratf != NULL && prgf != NULL && genf != NULL;
}


extern bool_t 
output_report_individual (const char *outindiv_name, struct CEGT *p, int simulate)
{
	bool_t success;
	FILE *indivf;

	if (NULL ==	(indivf = fopen (outindiv_name, "w"))) {
			fprintf(stderr, "Error trying to write on file: %s\n", outindiv_name);		
			return FALSE;	
	}

	success = output_report_individual_f (indivf, p, simulate);

	fclose(indivf);
	return success;
} 


static bool_t 
output_report_individual_f (FILE *indf, struct CEGT *p, int simulate)
{
	if (indf)
		all_report_indiv_stats (indf, p, simulate);

	return indf != NULL;
}


//

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

static const char *
get_super_player_symbolstr(int j, struct CEGT *p)
{
	if (p->obtained_results[j] < 0.01) {
		return SP_symbolstr[0];
	} else if (p->playedby_results[j] - p->obtained_results[j] < 0.01) {
		return SP_symbolstr[1];
	} else
		return SP_symbolstr[2];
}

static double av_opp(long int j, struct CEGT *p)
{
	long int e, opp;
	long target = j;

	double rsum = 0;
	long nsum = 0;

	struct ENC 	*enc = p->enc;
	long int 	n_enc = p->n_enc ;
	double		*ratingof_results = p->ratingof_results ;

	for (e = 0; e < n_enc; e++) {
		if (enc[e].wh == target || enc[e].bl == target) {
			opp = enc[e].wh == target? enc[e].bl: enc[e].wh;
			rsum += enc[e].played * ratingof_results[opp];
			nsum += enc[e].played;
		}	
	}

	return rsum/ (double) nsum;
}

static double draw_percentage(int j, struct ENC *enc, long int n_enc)
{
	long int e;
	long target = j;

	long draws = 0;
	long games = 0;

	for (e = 0; e < n_enc; e++) {
		if (enc[e].wh == target || enc[e].bl == target) {
			draws += enc[e].D;
			games += enc[e].played;
		}	
	}

	return 100.0 * (double) draws/ (double) games;
}


static void
all_report_rat (FILE *textf, struct CEGT *p)
{
	FILE *f;
	int j;
	int i;
	size_t ml;

	// Interface p with internal variables or pointers
	struct ENC 	*Enc = p->enc;
	long int 	N_enc = p->n_enc ;
	long int 	N_players = p->n_players ;
	int			*Sorted = p->sorted ;
	double		*Ratingof_results = p->ratingof_results ;
	double		*Obtained_results = p->obtained_results ;
	int			*Playedby_results = p->playedby_results ;
	double		*Sdev = p->sdev; 
	bool_t		*Flagged = p->flagged ;
	char 		**Name = p->name ;
	double		confidence = p->confidence_factor;

	/* output in text format */
	f = textf;
	if (f != NULL) {

		ml = find_maxlen (Name, N_players);
		//intf ("max length=%ld\n", ml);
		if (ml > 50) ml = 50;

		{
			fprintf(f, "\n%s %-*s     %4s %4s %4s %7s %7s %8s %6s\n\n", 
				"    ", 			
				(int)ml,
				"Program", 
				"Elo", "+", "-", "Games", "Score", "Av.Op.", "Draws");
	
			for (i = 0; i < N_players; i++) {
				j = Sorted[i];
				if (!Flagged[j]) {
					fprintf(f, "%4d %-*s %s :%5.0f%5.0f%5.0f %5d %7.1f%s %6.0f %6.1f%s\n", 
						i+1,
						(int)ml+1,
						Name[j],
						get_super_player_symbolstr(j,p),
	
						Ratingof_results[j],
						Sdev[j] * confidence,
						Sdev[j] * confidence,
						Playedby_results[j],
						Playedby_results[j]==0? 0: 100.0*Obtained_results[j]/Playedby_results[j],
						" %",
						av_opp(j, p),
						draw_percentage(j, Enc, N_enc),
						" %"
					);
				} else {
					fprintf(f, "%4d %-*s %s :%5s%5s%5s %5d %7.1f%s %6.0f %6.1f%s\n", 
						i+1,
						(int)ml+1,
						Name[j],
						" ",
	
						"----",
						"--",
						"--",
						Playedby_results[j],
						Playedby_results[j]==0? 0: 100.0*Obtained_results[j]/Playedby_results[j],
						" %",
						av_opp(j, p),
						draw_percentage(j, Enc, N_enc),
						" %"
					);
				}
			}
		}

	} /*if*/

	return;
}


static int compare_ENC2 (const void * a, const void * b)
{
	const struct ENC *ap = a;
	const struct ENC *bp = b;

	long int topa = ap->wh > ap->bl? ap->wh: ap->bl;
	long int bota = ap->wh < ap->bl? ap->wh: ap->bl;
	long int topb = bp->wh > bp->bl? bp->wh: bp->bl;
	long int botb = bp->wh < bp->bl? bp->wh: bp->bl;

	if (ap->wh == bp->wh && ap->bl == bp->bl) return 0;

	if (bota == botb) {
		return topa < topb? 1: -1;
	} else {	 
		return bota < botb? 1: -1;
	}
	
}

static void
all_report_prg (FILE *textf, struct CEGT *p)
{
	FILE *f;
	long i, j;
	size_t ml;

	// Interface p with internal variables or pointers
	struct ENC 	*Enc = p->enc;
	long int 	N_enc = p->n_enc ;
	long int 	N_players = p->n_players ;
	int			*Sorted = p->sorted ;
	double		*Ratingof_results = p->ratingof_results ;
	bool_t		*Flagged = p->flagged ;
	char 		**Name = p->name ;

	struct ENC *Temp_enc = NULL;
	size_t allocsize = sizeof(struct ENC) * (size_t)(N_enc+1);

	Temp_enc = malloc(allocsize);

	/* output in text format */
	f = textf;
	if (f != NULL && Temp_enc != NULL) {

		ml = find_maxlen (Name, N_players);

		if (ml > 50) 
			ml = 50;
		
		fprintf(f,"Individual statistics:\n\n");
	
		for (i = 0; i < N_players; i++) {
			j = Sorted[i];

			if (!Flagged[j]) {
				long int e;
				long t;
				long target = j;

				long won; 
				long dra; 
				long los;

				t = 0;
				for (e = 0; e < N_enc; e++) {
					if (Enc[e].wh == target || Enc[e].bl == target) {
						Temp_enc[t++] = Enc[e];
					}	
				}

				qsort (Temp_enc, (size_t)t, sizeof(struct ENC), compare_ENC2);
	
				won = 0;
				dra = 0;
				los = 0;

				for (e = 0; e < t; e++) {
					won += Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
					dra += Temp_enc[e].D;
					los += Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;
					assert (Temp_enc[e].wh == target || Temp_enc[e].bl == target);
				}

				fprintf(f, "%ld %-*s :%5.0f %ld (+%3ld,=%3ld,-%3ld), %4.1f %s\n\n"
					,i+1
					,(int)ml+1
					, Name[target]
					, Ratingof_results[target]
					, (won+dra+los)
					, won
					, dra
					, los
					, 100.0*((double)won+(double)dra/2)/(double)(won+dra+los)
					, "%" 
				);

				won = 0;
				dra = 0;
				los = 0;

				for (e = 0; e < t; ) {
					int oth = Temp_enc[e].wh == target? Temp_enc[e].bl: Temp_enc[e].wh;
	
					won = Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
					dra = Temp_enc[e].D;
					los = Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;

					// There is a next entry, and it is the "rematch" of the current one 
					// (same, different colors)
					if ((e+1) < t
						&& Temp_enc[e].wh == Temp_enc[e+1].bl 
						&& Temp_enc[e].bl == Temp_enc[e+1].wh
						) {
						e++;
						won += Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
						dra += Temp_enc[e].D;
						los += Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;
					}

					assert (Temp_enc[e].wh == target || Temp_enc[e].bl == target);

					fprintf(f, "%-*s : %ld (+%3ld,=%3ld,-%3ld), %4.1f %s\n"
						,(int)ml+5
						, Name[oth]
						, won+dra+los
						, won, dra, los
						, 100.0*((double)won+(double)dra/2)/(double)(won+dra+los), "%" 
					);
					e++;
				}

				fprintf(f, "\n");

			} else {
				fprintf(f, "%4ld %-*s :Removed from calculation\n", 
					i+1,
					(int)ml+1,
					Name[j]
				);
			}
		}

	} /*if*/

	if (Temp_enc != NULL) {
		free(Temp_enc);
	} else {
		fprintf(stderr,"Not enough memory to calculate and output program data.");
	}
	return;
}

//

static int compare_oline (const void * a, const void * b)
{
	const struct OPP_LINE *ap = a;
	const struct OPP_LINE *bp = b;
	double aR = ap->R;
	double bR = bp->R;
	if (aR < bR) {
		return 1;
	} else if (aR > bR) {
		return -1;
	} else {
		if (ap->i < bp->i) {
			return -1;
		} else if (ap->i > bp->i) {
			return 1;
		} else {
			return 0;
		}
	}
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

static size_t
calclen (long int x)
{
	char s[80];
	sprintf(s, "%ld", x);
	return strlen(s);	
}

static void
all_report_indiv_stats (FILE *textf, struct CEGT *p, int simulate)
{
	FILE *f;
	long i, j;
	size_t ml;
	size_t gl = 7;

	int gwlen = 3;
	int gdlen = 5;
	int gllen = 7;

	long int maxgames, maxwon, maxdra, maxlos;

	int	indent = 0;

	// Interface p with internal variables or pointers
	struct ENC 	*Enc = p->enc;
	long int 	N_enc = p->n_enc ;
	long int 	N_players = p->n_players ;
	int			*Sorted = p->sorted ;
	double		*Ratingof_results = p->ratingof_results ;
	bool_t		*Flagged = p->flagged ;
	char 		**Name = p->name ;

	struct ENC *Temp_enc = NULL;
	size_t allocsize = sizeof(struct ENC) * (size_t)(N_enc+1);

	indent = (int) calclen (N_players+1);

	Temp_enc = malloc(allocsize);

	/* output in text format */
	f = textf;
	if (f != NULL && Temp_enc != NULL) {

		ml = find_maxlen (Name, N_players);

		if (ml > 50) 
			ml = 50;
		
		fprintf(f,"Head to head statistics:\n\n");
	
		for (i = 0; i < N_players; i++) {
			j = Sorted[i];

			if (!Flagged[j]) {
				long e;
				long t;
				long target = j;

				long won; 
				long dra; 
				long los;
				long nl;

				t = 0;
				for (e = 0; e < N_enc; e++) {
					if (Enc[e].wh == target || Enc[e].bl == target) {
						Temp_enc[t++] = Enc[e];
					}	
				}
	
				qsort (Temp_enc, (size_t)t, sizeof(struct ENC), compare_ENC2);
	
				won = 0;
				dra = 0;
				los = 0;

				for (e = 0; e < t; e++) {
					won += Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
					dra += Temp_enc[e].D;
					los += Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;
					assert (Temp_enc[e].wh == target || Temp_enc[e].bl == target);
				}

				gl = 1 + calclen (won+dra+los);
				gl = gl < 6? 6: gl; //strlen("games");

				gwlen = (int)calclen(won);
				gdlen = (int)calclen(dra);
				gllen = (int)calclen(los);

				fprintf(f, "%*ld) %-*s %5.0f : %*ld (+%*ld,=%*ld,-%*ld), %5.1f %s\n\n"
					, indent, i+1
					, (int)ml, Name[target]
					, Ratingof_results[target]
					, (int)gl, (won+dra+los)
					, gwlen, won
					, gdlen, dra
					, gllen, los
					, 100.0*((double)won+(double)dra/2)/(double)(won+dra+los)
					, "%" 
				);

				won = 0;
				dra = 0;
				los = 0;

				maxgames = maxwon = maxdra = maxlos = 0;
				nl = 0;
				for (e = 0; e < t; ) {
					int oth = Temp_enc[e].wh == target? Temp_enc[e].bl: Temp_enc[e].wh;
	
					won = Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
					dra = Temp_enc[e].D;
					los = Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;

					// There is a next entry, and it is the "rematch" of the current one 
					// (same, different colors)
					if ((e+1) < t
						&& Temp_enc[e].wh == Temp_enc[e+1].bl 
						&& Temp_enc[e].bl == Temp_enc[e+1].wh
						) {
						e++;
						won += Temp_enc[e].wh == target? Temp_enc[e].W: Temp_enc[e].L;
						dra += Temp_enc[e].D;
						los += Temp_enc[e].wh == target? Temp_enc[e].L: Temp_enc[e].W;
					}

					assert (Temp_enc[e].wh == target || Temp_enc[e].bl == target);

					oline[nl].i = oth;
					oline[nl].w = won;
					oline[nl].d = dra;
					oline[nl].l = los;
					oline[nl].R = Ratingof_results[oth];
					nl++;

					maxgames = maxgames < (won+dra+los)? won+dra+los: maxgames;
					maxwon = maxwon < won? won: maxwon;
					maxdra = maxdra < dra? dra: maxdra;
					maxlos = maxlos < los? los: maxlos;

					e++;
				}

				fprintf(f, 
					"%*s"	
					"%-*s"
					"%s"
					"%*s"

					"%s"
					"%*s"
					"%s"
					"%*s"
					"%s"
					"%*s"

					"%s"
					"%6s"
					"%2s"
					" %6s"

					//
					, indent+2, ""
					,(int)ml+6, "vs."
					, " : "
					,(int)gl, "games"

					," ( "

					,gwlen,"+"
					,", "
					,gdlen,"="
					,", "
					,gllen,"-"

					, "),"
					, "(%)"
					, ":" 
					, "Diff"
				);

				if (simulate > 1 && p->sim != NULL) {
				fprintf(f, 
					", %3s, %6s"
					, "SD"
					, "CFS (%)"

				);
				}

				fprintf(f,"\n");

				qsort (oline, (size_t)nl, sizeof(struct OPP_LINE), compare_oline);

				// output lines
				for (e = 0; e < nl; e++) {
					long int oth = oline[e].i;

					double dr = Ratingof_results[target] - oline[e].R;

					won = oline[e].w;
					dra = oline[e].d;
					los = oline[e].l;

					fprintf(f, 
						"%*s"
						"%-*s : %*ld ( %*ld, %*ld, %*ld), %5.1f"
						, indent+2, ""
						,(int)ml+6
						, Name[oth]
						,(int)gl
						, won+dra+los
						, gwlen, won
						, gdlen, dra
						, gllen, los
						, 100.0*((double)won+(double)dra/2)/(double)(won+dra+los) 
					);

					fprintf(f, " : %+6.0f", dr);

					if (simulate > 1 && p->sim != NULL) {
						ptrdiff_t idx;
						double ctrs;
						double sd;

						idx = head2head_idx_sdev (target, oth);
						sd = p->sim[idx].sdev;
						ctrs = 100*gauss_integral(dr/sd);

						fprintf(f, ", %3.0f, %6.1f", sd, ctrs);

					} 
					fprintf(f, "\n");
				}
				fprintf(f, "\n");

			} else {
				fprintf(f, "%4ld %-*s :Removed from calculation\n", 
					i+1,
					(int)ml+6,
					Name[j]
				);
			}
		}

	} /*if*/

	if (Temp_enc != NULL) {
		free(Temp_enc);
	} else {
		fprintf(stderr,"Not enough memory to calculate and output program data.");
	}

	return;
}
//

static void
all_report_gen (FILE *textf, struct CEGT *p)
{
	// Interface p with internal variables or pointers
	struct ENC 	*Enc = p->enc;
	long int 	N_enc = p->n_enc ;

	long e;

	long totalgames = 0;
	long white_wins = 0;
	double white_perc = 0.0;
	long black_wins  = 0;
	double black_perc = 0.0;
	long draw  = 0;
	double draw_perc = 0.0;
	double white_performance = 0.0;
	double black_performance = 0.0;

	long won, dra, los;

	won = 0;
	dra = 0;
	los = 0;
	for (e = 0; e < N_enc; e++) {
		won += Enc[e].W;
		dra += Enc[e].D;
		los += Enc[e].L; 
	}

	totalgames = won + dra + los;

	white_wins = won;
	white_perc = (double) white_wins/ (double) totalgames;

	black_wins  = los;
	black_perc = (double) black_wins/ (double) totalgames;

	draw  = dra;
	draw_perc = (double) draw/ (double) totalgames;

	white_performance = white_perc + draw_perc/2;
	black_performance = black_perc + draw_perc/2;


	fprintf (textf,
		"\n"
		"Games        : %ld (finished)\n\n"
		"White Wins   : %ld (%.1f %s)\n"
		"Black Wins   : %ld (%.1f %s)\n"
		"Draws        : %ld (%.1f %s)\n"
		"Unfinished   : %ld\n\n"
		"White Score  : %.1f %s\n"
		"Black Score  : %.1f %s\n"
		"\n"

		,totalgames
		,white_wins, 100*white_perc, "%"
		,black_wins, 100*black_perc, "%"
		,draw, 100*draw_perc, "%"
		,(long)p->gstat->noresult
		,100*white_performance, "%"
		,100*black_performance, "%"
	);

	return;
}

