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


#include <assert.h>
#include <stdlib.h>
#include "groups.h"
#include "mytypes.h"

struct BITARRAY {
	uint64_t pod[MAXPLAYERS/64];
	long int max;
};

//---------------------------------------------------------------------

static char 		**Namelist = NULL;

//---------------------------------------------------------------------

// supporting memory
static struct ENC 	*SE   = NULL;
static int 			N_se  = 0;
static struct ENC 	*SE2  = NULL;
static int 			N_se2 = 0;

static int 			group_belong[MAXPLAYERS];
static int			N_groups;

struct BITARRAY 	BA;
struct BITARRAY		BB;
static int 			Get_new_id[MAXPLAYERS];

static group_t *	Group_final_list[MAXPLAYERS];
static long			Group_final_list_n = 0;

static node_t		Gnode[MAXPLAYERS];

//----------------------------------------------------------------------

static void			simplify_all(void);
static void			finish_it(void);
static void 		connect_init (void) {connection_buffer.n = 0;}
static connection_t * 
					connection_new (void) {return &connection_buffer.list[connection_buffer.n++];}
static void 		participant_init (void) {participant_buffer.n = 0;}
static participant_t * 
					participant_new (void) {return &participant_buffer.list[participant_buffer.n++];}

// prototypes
static group_t * 	group_new(void);
static group_t * 	group_reset(group_t *g);
static group_t * 	group_combined(group_t *g);
static group_t * 	group_pointed(connection_t *c);
static group_t *	group_pointed_by_node(node_t *nd);
static void			final_list_output(FILE *f);
static void			group_output(FILE *f, group_t *s);

// groupset functions

#ifndef NDEBUG
static bool_t
groupset_sanity_check(void)
{ 
	group_t *c; 

	c = group_buffer.prehead;
	if (c != NULL)
		c = c->next;
	else
		return FALSE;

	for (; c != NULL; c = c->next) {
		if (c->prev == NULL) return FALSE;
	}
	return TRUE;
}
#endif

static void groupset_init (void) 
{
	group_buffer.tail    = &group_buffer.list[0];
	group_buffer.prehead = &group_buffer.list[0];
	group_reset(group_buffer.prehead);
	group_buffer.n = 1;
}

static group_t * groupset_tail (void) {return group_buffer.tail;}
static group_t * groupset_head (void) {return group_buffer.prehead->next;}

static void groupset_add (group_t *a) 
{
	group_t *t = groupset_tail();
	t->next = a;
	a->prev = t;
	group_buffer.tail = a;
}

static group_t * groupset_find(int id)
{
	group_t * s;
	for (s = groupset_head(); s != NULL; s = s->next) {
		if (id == s->id) return s;
	}
	return NULL;
}

//===

static group_t * group_new  (void) {return &group_buffer.list[group_buffer.n++];}

static group_t * group_reset(group_t *g)
{		if (g == NULL) return NULL;
		g->next = NULL;	
		g->prev = NULL; 
		g->combined = NULL;
		g->pstart = NULL; g->plast = NULL; 	
		g->cstart = NULL; g->clast = NULL;
		g->lstart = NULL; g->llast = NULL;
		g->id = -1;
		g->isolated = FALSE;
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
	nw->name = Namelist[i];
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

static void
add_revconn (group_t *g, int i)
{
	int group_id;
	connection_t *nw = connection_new();
	nw->next = NULL;
	nw->node = &Gnode[i];

	group_id = -1;
	if (Gnode[i].group) group_id = Gnode[i].group->id;

	if (g->lstart == NULL) {
		g->lstart = nw; 
		g->llast = nw;	
	} else {
		connection_t *l, *c;
		bool_t found = FALSE;
		for (c = g->lstart; !found && c != NULL; c = c->next) {
			node_t *nd = c->node;
			found = nd && nd->group && nd->group->id == group_id;
		}
		if (!found) {
			l = g->llast;
			l->next  = nw;
			g->llast = nw;
		}
	}		
}

static int get_iwin(struct ENC *pe) {return pe->wscore > 0.5? pe->wh: pe->bl;}
static int get_ilos(struct ENC *pe) {return pe->wscore > 0.5? pe->bl: pe->wh;}

static void
enc2groups (struct ENC *pe)
{
	int iwin, ilos;
	group_t *glw, *gll, *g;

	assert(pe);

	iwin = get_iwin(pe);
	ilos = get_ilos(pe);

	if (Gnode[iwin].group == NULL) {

		g = groupset_find (group_belong[iwin]);
		if (g == NULL) {
			// creation
			g = group_reset(group_new());	
			g->id = group_belong[iwin];
			groupset_add(g);
			Gnode[iwin].group = g;
		}
		glw = g;
	} else {
		glw = Gnode[iwin].group;
	}

	if (Gnode[ilos].group == NULL) {

		g = groupset_find (group_belong[ilos]);
		if (g == NULL) {
			// creation
			g = group_reset(group_new());	
			g->id = group_belong[ilos];
			groupset_add(g);
			Gnode[ilos].group = g;
		}
		gll = g;
	} else {
		gll = Gnode[ilos].group;
	}

	Gnode[iwin].group = glw;
	Gnode[ilos].group = gll;
	add_connection (glw, ilos);
	add_revconn (gll, iwin);
}

static void
group_gocombine(group_t *g, group_t *h);

static void
ifisolated2group (int x)
{
	group_t *g;

	if (Gnode[x].group == NULL) {
		g = groupset_find (group_belong[x]);
		if (g == NULL) {
			// creation
			g = group_reset(group_new());	
			g->id = group_belong[x];
			groupset_add(g);
			Gnode[x].group = g;
		}
		Gnode[x].group = g;
	} 
}

static void
convert_general_init(int N_plyrs)
{
	int i;
	connect_init();
	participant_init();
	groupset_init();
	for (i = 0; i < N_plyrs; i++) {
		Gnode[i].group = NULL;
	}
	return;
}

void
convert_to_groups(FILE *f, int N_plyrs, char **name)
{
	int i;
	int e;

	Namelist = name;

	convert_general_init(N_plyrs);

	for (e = 0 ; e < N_se2; e++) {
		enc2groups(&SE2[e]);
	}

	for (i = 0; i < N_plyrs; i++) {
		ifisolated2group(i);
	}

	for (i = 0; i < N_plyrs; i++) {
		int gb; 
		group_t *g;
		gb = group_belong[i];
		g = groupset_find(gb);
		assert(g);
		add_participant(g, i);	
	}

	simplify_all();
	finish_it();
	final_list_output(f);

	return;
}

static int
group_belonging(int plyr)
{
	group_t *ggg = group_pointed_by_node(&Gnode[plyr]);
	if (ggg) 
		return ggg->id;
	else
		return -1;
}

void
sieve_encounters(const struct ENC *enc, long N_enc, struct ENC *enca, long *N_enca, struct ENC *encb, long *N_encb)
{
	int e,w,b;
	long na = 0, nb = 0;

	*N_enca = 0;
	*N_encb = 0;

	for (e = 0; e < N_enc; e++) {
		w = enc[e].wh;
		b = enc[e].bl;
		if (group_belonging(w) == group_belonging(b)) {
			enca[na] = enc[e]; na += 1;
		} else {
			encb[nb] = enc[e]; nb += 1;
		}
	} 

	*N_enca = na;
	*N_encb = nb;

	return;
}

static void
group_gocombine(group_t *g, group_t *h)
{
	// unlink h
	group_t *pr = h->prev;
	group_t *ne = h->next;


if (h->combined == g) {
	return;
}

	h->prev = NULL;
	h->next = NULL;

//if (pr == NULL) { printf ("g=%d, h=%d\n",g->id,h->id);}

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

	g->llast->next = h->lstart;
	g->llast = h->llast;
	h->llast = NULL;
	h->lstart = NULL;
}

//======================


static void
ba_put(struct BITARRAY *ba, long int x)
{
	if (x < ba->max) {
		ba->pod[x/64] |= (uint64_t)1 << (x & 63);
	}
}

static bool_t
ba_ison(struct BITARRAY *ba, long int x)
{
	uint64_t y;
	bool_t ret;
	y = (uint64_t)1 & (ba->pod[x/64] >> (x & 63));	
	ret = y == 1;
	return ret;
}

static void
ba_init(struct BITARRAY *ba, long int max)
{
	long int i;
	long int max_p = max/64;
	ba->max = max;
	for (i = 0; i < max_p; i++) ba->pod[i] = 0;
}

static void
ba_done(struct BITARRAY *ba)
{
	long int i;
	long int max_p = ba->max/64;
	for (i = 0; i < max_p; i++) ba->pod[i] = 0;
	ba->max = 0;
}


static group_t *
group_pointed(connection_t *c)
{
	node_t *nd; 
	if (c == NULL) return NULL;
	nd = c->node;
	if (nd) {
		group_t *gr = nd->group;
		if (gr) {
			gr = group_combined(gr);
		} 
		return gr;
	} else {
		return NULL;
	}
}


static group_t *
group_pointed_by_node(node_t *nd)
{
	if (nd) {
		group_t *gr = nd->group;
		if (gr) {
			gr = group_combined(gr);
		} 
		return gr;
	} else {
		return NULL;
	}
}

static void simplify (group_t *g);

static group_t *group_next(group_t *g)
{
	assert(g);
	return g->next;
}

static void
simplify_all(void)
{
	group_t *g;

	g = groupset_head();
	assert(g);
	while(g) {
		simplify(g);
		g = group_next(g);
	}
	return;
}

#if 0
static void
beat_lost_output (group_t *g)
{
	group_t 		*beat_to, *lost_to;
	connection_t 	*c;

	printf ("G=%d, beat_to: ",g->id);

		// loop connections, examine id if repeated or self point (delete them)
		beat_to = NULL;
		c = g->cstart; 
		do {
			if (c && NULL != (beat_to = group_pointed(c))) {
				printf ("%d, ",beat_to->id);
				c = c->next;
			}
		} while (c && beat_to);

	printf ("\n");
	printf ("G=%d, lost_to: ",g->id);

		lost_to = NULL;
		c = g->lstart; 
		do {
			if (c && NULL != (lost_to = group_pointed(c))) {
				printf ("%d, ",lost_to->id);
				c = c->next;
			}
		} while (c && lost_to);
	printf ("\n");
}
#endif

static void
simplify_shrink (group_t *g)
{
	group_t 		*beat_to, *lost_to;
	connection_t 	*c, *p;
	int 			id, oid;

	id=-1;

		ba_init(&BA, MAXPLAYERS-1);
		ba_init(&BB, MAXPLAYERS-2);

		oid = g->id; // own id

		// loop connections, examine id if repeated or self point (delete them)
		beat_to = NULL;
		do {
			c = g->cstart; 
			if (c && NULL != (beat_to = group_pointed(c))) {
				id = beat_to->id;
				if (id == oid) { 
					// remove connection
					g->cstart = c->next; //FIXME mem leak? free(c)
				}
			}
		} while (c && beat_to && id == oid);


		if (c && beat_to) {

			ba_put(&BA, id);
			p = c;
			c = c->next;

			while (c != NULL) {
				beat_to = group_pointed(c);
				id = beat_to->id;
				if (id == oid || ba_ison(&BA, id)) {
					// remove connection and advance
					c = c->next; //FIXME mem leak? free(c)
					p->next = c; 
				}
				else {
					// remember and advance
					ba_put(&BA, id);
					p = c;
					c = c->next;
				}
			}

		}

		// loop connections, examine id if repeated or self point (delete them)

		lost_to = NULL;

		do {
			c = g->lstart; 
			if (c && NULL != (lost_to = group_pointed(c))) {
				id = lost_to->id;
				if (id == oid) { 
					// remove connection
					g->lstart = c->next; //FIXME mem leak?
				}
			}
		} while (c && lost_to && id == oid);


		if (c && lost_to) {

			ba_put(&BB, id);
			p = c;
			c = c->next;

			while (c != NULL) {
				lost_to = group_pointed(c);
				id = lost_to->id;
				if (id == oid || ba_ison(&BB, id)) {
					// remove connection and advance
					c = c->next;		
					p->next = c; //FIXME mem leak?
				}
				else {
					// remember and advance
					ba_put(&BB, id);
					p = c;
					c = c->next;
				}
			}
		}

		ba_done(&BA);
		ba_done(&BB);

	return;
}


static void
simplify (group_t *g)
{
	group_t 		*beat_to, *lost_to, *combine_with=NULL;
	connection_t 	*c, *p;
	int 			id, oid;
	bool_t 			gotta_combine = FALSE;
	bool_t			combined = FALSE;

	id=-1;

	do {

//printf("-------------\n");
//printf("before shrink\n");
//beat_lost_output (g);
simplify_shrink (g);
//printf("after  shrink\n");
//beat_lost_output (g);
//printf("-------------\n");

assert(groupset_sanity_check());

		ba_init(&BA, MAXPLAYERS-1);
		ba_init(&BB, MAXPLAYERS-2);

		oid = g->id; // own id

gotta_combine = FALSE;

	// loop connections, examine id if repeated or self point (delete them)
		beat_to = NULL;
		do {
			c = g->cstart; 
			if (c && NULL != (beat_to = group_pointed(c))) {
				id = beat_to->id;
				if (id == oid) { 
					// remove connection
					g->cstart = c->next; //FIXME mem leak? free(c)
				}
			}
		} while (c && beat_to && id == oid);


		if (c && beat_to) {

			ba_put(&BA, id);
			p = c;
			c = c->next;

			while (c != NULL) {
				beat_to = group_pointed(c);
				id = beat_to->id;
				if (id == oid || ba_ison(&BA, id)) {
					// remove connection and advance
					c = c->next; //FIXME mem leak? free(c)
					p->next = c; 
				}
				else {
					// remember and advance
					ba_put(&BA, id);
					p = c;
					c = c->next;
				}
			}

		}

	//=====
	// loop connections, examine id if repeated or self point (delete them)

		lost_to = NULL;

assert(groupset_sanity_check());

		do {
			c = g->lstart; 
			if (c && NULL != (lost_to = group_pointed(c))) {
				id = lost_to->id;
				if (id == oid) { 
					// remove connection
					g->lstart = c->next; //FIXME mem leak?
				}
			}
		} while (c && lost_to && id == oid);

//if (lost_to) printf ("found=%d\n",lost_to->id); else printf("no lost to found\n");

assert(groupset_sanity_check());

		if (c && lost_to) {

			// GOTTACOMBINE?
			if (ba_ison(&BA, id)) {

assert(groupset_sanity_check());

				gotta_combine = TRUE;
				combine_with = lost_to;
			}
			else gotta_combine = FALSE;

			ba_put(&BB, id);
			p = c;
			c = c->next;

assert(groupset_sanity_check());

			while (c != NULL && !gotta_combine) {
				lost_to = group_pointed(c);
				id = lost_to->id;
				if (id == oid || ba_ison(&BB, id)) {
					// remove connection and advance
					c = c->next;		
					p->next = c; //FIXME mem leak?
				}
				else {
					// remember and advance
					ba_put(&BB, id);
					p = c;
					c = c->next;
	
					// GOTTACOMBINE?
					if (ba_ison(&BA, id)) {
						gotta_combine = TRUE;
						combine_with = lost_to;
					}
					else gotta_combine = FALSE;
				}
			}


assert(groupset_sanity_check());
		}

		ba_done(&BA);
		ba_done(&BB);

		if (gotta_combine) {
//printf("combine g=%d combine_with=%d\n",g->id, combine_with->id);
assert(groupset_sanity_check());

			group_gocombine(g,combine_with);

			combined = TRUE;
		} else {
			combined = FALSE;
		}
	
	} while (combined);


//printf("----final----\n");
//printf("before shrink\n");
//beat_lost_output (g);
simplify_shrink (g);
//printf("after  shrink\n");
//beat_lost_output (g);
//printf("-------------\n");


	return;
}

//======================

static connection_t *group_beathead(group_t *g) {return g->cstart;} 
static connection_t *beat_next(connection_t *b) {return b->next;} 

static group_t *
group_unlink(group_t *g)
{
	group_t *a, *b; 
	assert(g);
	a = g->prev;
	b = g->next;
	if (a) a->next = b;
	if (b) b->prev = a;
	g->prev = NULL;
	g->next = NULL;
	g->isolated = TRUE;
	return g;
}

static group_t *
group_next_pointed_by_beat(group_t *g)
{
	group_t *gp;
	connection_t *b;
	int own_id;
	if (g == NULL)	return NULL; 
	own_id = g->id;
	b = group_beathead(g);
	if (b == NULL)	return NULL; 

	gp = group_pointed(b);
	while (gp == NULL || gp->isolated || gp->id == own_id) {
		b = beat_next(b);
		if (b == NULL) return NULL;
		gp = group_pointed(b);
	} 

	return gp;
}

static void
finish_it(void)
{
	int *chain_end;
	group_t *g, *h, *gp;
	connection_t *b;
	int own_id, bi;
	static int CHAIN[MAXPLAYERS];
	int *chain;
	bool_t startover;
	bool_t combined;

	do {
		startover = FALSE;
		combined = FALSE;

		chain = CHAIN;

		ba_init(&BA, MAXPLAYERS);

		g = groupset_head();
		if (g == NULL) break;

		own_id = g->id; // own id

		do {

			ba_put(&BA, own_id);
			*chain++ = own_id;

			h = group_next_pointed_by_beat(g);

			if (h == NULL) {

				Group_final_list[Group_final_list_n++] = group_unlink(g);

				ba_done(&BA);
				startover = TRUE;

			} else {
			
				g = h;

				own_id = g->id;

				for (b = group_beathead(g); b != NULL; b = beat_next(b)) {

					gp = group_pointed(b);
					bi = gp->id;
	
					if (ba_ison(&BA, bi)) {
						//	findprevious bi, combine... remember to include own id;
						int *p;
						chain_end = chain;
						while (chain-->CHAIN) {
							if (*chain == bi) break;
						}
						
						for (p = chain; p < chain_end; p++) {
							group_t *x, *y;
//printf("combine x=%d y=%d\n",own_id, *p);
							x = group_pointed_by_node(Gnode + own_id);
							y = group_pointed_by_node(Gnode + *p);
							group_gocombine(x,y);
							combined = TRUE;
							startover = TRUE;
							break;
						}

						break;
					}
				}	
			}
		} while (!combined && !startover);
	} while (startover);
	return;
}

static void
final_list_output(FILE *f)
{
	group_t *g;
	int i;
	int new_id;
	
	for (i = 0; i < MAXPLAYERS; i++) {
		Get_new_id[i] = -1;
	}

	new_id = 0;
	for (i = 0; i < Group_final_list_n; i++) {
		g = Group_final_list[i];
		new_id++;
		Get_new_id[g->id] = new_id;
	}

	for (i = 0; i < Group_final_list_n; i++) {
		g = Group_final_list[i];
		fprintf (f,"\nGroup %d\n",Get_new_id[g->id]);

//printf("-post-final--\n");
//printf("before shrink\n");
//beat_lost_output (g);
simplify_shrink (g);
//printf("after  shrink\n");
//beat_lost_output (g);
//printf("-------------\n");

		group_output(f,g);
	}
	fprintf(f,"\n");
}


static void
group_output(FILE *f, group_t *s)
{		
	participant_t *p;
	connection_t *c;
	int own_id;
	int winconnections = 0, lossconnections = 0;
	assert(s);
	own_id = s->id;
	for (p = s->pstart; p != NULL; p = p->next) {
		fprintf (f," | %s\n",p->name);
	}
	for (c = s->cstart; c != NULL; c = c->next) {
		group_t *gr = group_pointed(c);
		if (gr != NULL) {
			if (gr->id != own_id) {
				fprintf (f," \\---> there are (only) wins against group: %d\n",Get_new_id[gr->id]);
				winconnections++;
			}
		} else
			fprintf (f,"point to node NULL\n");
	}
	for (c = s->lstart; c != NULL; c = c->next) {
		group_t *gr = group_pointed(c);
		if (gr != NULL) {
			if (gr->id != own_id) {
				fprintf (f," \\---> there are (only) losses against group: %d\n",Get_new_id[gr->id]);
				lossconnections++;
			}
		} else
			fprintf (f,"pointed by node NULL\n");
	}

		if (winconnections == 0 && lossconnections == 0) {
			fprintf (f," \\---> this group is isolated from the rest\n");
		} 

}

static bool_t encounter_is_SW(const struct ENC *e) {return (e->played - e->wscore) < 0.0001;}
static bool_t encounter_is_SL(const struct ENC *e) {return              e->wscore  < 0.0001;}

void
scan_encounters(const struct ENC *enc, long N_enc, int N_plyrs)
{
	int i,e;

	const struct ENC *pe;
	int gw, gb, lowerg, higherg;

	N_groups = N_plyrs;
	for (i = 0; i < N_plyrs; i++) {
		group_belong[i] = i;
	}

	for (e = 0; e < N_enc; e++) {

		pe = &enc[e];
		if (encounter_is_SL(pe) || encounter_is_SW(pe)) {
			SE[N_se++] = *pe;
		} else {
			gw = group_belong[pe->wh];
			gb = group_belong[pe->bl];
			if (gw != gb) {
				lowerg   = gw < gb? gw : gb;
				higherg  = gw > gb? gw : gb;
				// join
				for (i = 0; i < N_plyrs; i++) {
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

	return;
}


//
/*
struct ENC 			*SE   = NULL;
static int 			N_se  = 0;
struct ENC 			*SE2  = NULL;
static int 			N_se2 = 0;
*/

bool_t
supporting_encmem_init (size_t nenc)
{
	struct ENC *a;
	struct ENC *b;

	if (NULL == (a = malloc (sizeof(struct ENC) * nenc))) {
		return FALSE;
	} else 
	if (NULL == (b = malloc (sizeof(struct ENC) * nenc))) {
		free(a);
		return FALSE;
	}

	SE = a;
	SE = b;

	return TRUE;
}

void
supporting_encmem_done (void)
{
	if (SE) free(SE);
	if (SE2) free (SE2);
	N_se = 0;
	N_se2 = 0;
	return;
}


