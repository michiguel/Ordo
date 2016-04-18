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
#include <string.h>

#include "groups.h"
#include "mytypes.h"
#include "mymem.h"
#include "bitarray.h"

#include "mytimer.h"

#define NO_ID -1

static bool_t	group_buffer_init (struct GROUP_BUFFER *g, player_t n);
static void		group_buffer_done (struct GROUP_BUFFER *g);
static bool_t	participant_buffer_init (struct PARTICIPANT_BUFFER *x, player_t n);
static void		participant_buffer_done (struct PARTICIPANT_BUFFER *x);
static bool_t	connection_buffer_init (struct CONNECT_BUFFER *x, gamesnum_t n);
static void		connection_buffer_done (struct CONNECT_BUFFER *x);

//---------------------------------------------------------------------

// supporting memory

	// SE2: list of "Super" encounters that do not belong to the same group

struct SUPER {
	struct ENC *	SE2;
	gamesnum_t		N_se2;
};

typedef struct SUPER super_t;

//----------------------------------------------------------------------

static void				groupvar_simplify (group_var_t *gv);
static void				groupvar_finish (group_var_t *gv);
static void 			connect_init (group_var_t *gv) {gv->connectionbuffer.n = 0;}
static connection_t *	connection_new (group_var_t *gv) 
{
	assert (gv->connectionbuffer.n < gv->connectionbuffer.max);
	return &gv->connectionbuffer.list[gv->connectionbuffer.n++];
}

static void 			participant_init (group_var_t *gv) {gv->participantbuffer.n = 0;}
static participant_t *	participant_new (group_var_t *gv) 
{
	assert (gv->participantbuffer.n < gv->participantbuffer.max);	
	return &gv->participantbuffer.list[gv->participantbuffer.n++];
}

// prototypes

static group_t * 	group_new (group_var_t *gv);
static group_t * 	group_reset (group_t *g);
static group_t * 	group_combined (group_t *g);
static group_t * 	group_pointed_by_conn (connection_t *c);
static group_t *	group_pointed_by_node (node_t *nd);
static void			groupvar_assign_newid (group_var_t *gv);
static void			groupvar_list_sort (group_var_t *gv);
static void			groupvar_list_output (group_var_t *gv, FILE *f);
static void			group_output (group_t *s, group_var_t *gv, FILE *f, bool_t showlinks);

// groupset functions

#ifndef NDEBUG
static bool_t
groupset_sanity_check(group_var_t *gv)
{ 
	// verify g is properly double-linked
	group_t *g = gv->groupbuffer.prehead;
	if (g == NULL) return FALSE;
	for (g = g->next; g != NULL; g = g->next) {
		if (g->prev == NULL) return FALSE;
	}
	return TRUE;
}

static bool_t
groupset_sanity_check_nocombines(group_var_t *gv)
{ 
	// verify g is properly double-linked
	group_t *g = gv->groupbuffer.prehead;
	if (g == NULL) return FALSE;
	for (g = g->next; g != NULL; g = g->next) {
		if (g->combined != NULL) return FALSE;
	}
	return TRUE;
}
#endif

static void 
groupset_init (group_var_t *gv) 
{
	gv->groupbuffer.tail    = &gv->groupbuffer.list[0];
	gv->groupbuffer.prehead = &gv->groupbuffer.list[0];
	group_reset(gv->groupbuffer.prehead);
	gv->groupbuffer.n = 1;
}

static group_t * groupset_tail (group_var_t *gv) {return gv->groupbuffer.tail;}
static group_t * groupset_head (group_var_t *gv) {return gv->groupbuffer.prehead->next;}

#if 0
// for debuggin purposes
static void groupset_print(group_var_t *gv)
{
	group_t * s;
	printf("ID added {\n");
	for (s = groupset_head(gv); s != NULL; s = s->next) {
		printf("  id=%ld\n", (long)s->id);
	}
	printf("}\n");
	return;
}
#endif

static void 
groupset_add (group_var_t *gv, group_t *a) 
{
	group_t *t = groupset_tail(gv);
	t->next = a;
	a->prev = t;
	gv->groupbuffer.tail = a;
}

static void
groupset_reset_finding (group_var_t *gv)
{
	group_t * s;
	player_t i;

	for (i = 0; i < gv->nplayers; i++) {
		gv->tofindgroup[i] = NULL;
	}
	for (s = groupset_head(gv); s != NULL; s = s->next) {
		gv->tofindgroup[s->id] = s;
	}
}

static group_t * 
groupset_find (group_var_t *gv, player_t id)
{
#if 1
	return gv->tofindgroup[id];
#else
	group_t * s;
	for (s = groupset_head(gv); s != NULL; s = s->next) {
		if (id == s->id) return s;
	}
	return NULL;
#endif
}

//---------------------------------- INITIALIZATION --------------------------

typedef struct SELINK selink_t;

struct SELINK {
	player_t iwin;
	player_t ilos;
};

static bool_t
supporting_encmem_init (gamesnum_t nenc, super_t *sp, selink_t **pse)
{
	struct ENC 	*a;
	selink_t 	*b;

	if (NULL == (a = memnew (sizeof(struct ENC) * (size_t)nenc))) {
		return FALSE;
	} else
	if (NULL == (b = memnew (sizeof(selink_t)   * (size_t)nenc))) {
		memrel(a);
		return FALSE;
	}	
	sp->SE2 = a;
	*pse = b;

	return TRUE;
}

static void
supporting_encmem_done (super_t *sp, selink_t *se)
{
	if (sp->SE2) memrel (sp->SE2);
	sp->N_se2 = 0;
	if (se) memrel (se);
	return;
}

bool_t
groupvar_init (group_var_t *gv, player_t nplayers, gamesnum_t nenc)
{
	player_t 	*a;
	player_t	*b;
	groupcell_t *c;
	node_t 		*d;
	player_t	*e;
	group_t	*	*f;

	size_t		sa = sizeof(player_t);
	size_t		sb = sizeof(player_t);
	size_t		sc = sizeof(groupcell_t); 
	size_t		sd = sizeof(node_t);
	size_t		se = sizeof(player_t);
	size_t		sf = sizeof(group_t *);

	if (NULL == (a = memnew (sa * (size_t)nplayers))) {
		return FALSE;
	} else 
	if (NULL == (b = memnew (sb * (size_t)nplayers))) {
		memrel(a);
		return FALSE;
	} else 
	if (NULL == (c = memnew (sc * (size_t)nplayers))) {
		memrel(a);
		memrel(b);
		return FALSE;
	} else 
	if (NULL == (d = memnew (sd * (size_t)nplayers))) {
		memrel(a);
		memrel(b);
		memrel(c);
		return FALSE;
	} else 
	if (NULL == (e = memnew (se * (size_t)nplayers))) {
		memrel(a);
		memrel(b);
		memrel(c);
		memrel(d);
		return FALSE;
	} else 
	if (NULL == (f = memnew (sf * (size_t)nplayers))) {
		memrel(a);
		memrel(b);
		memrel(c);
		memrel(d);
		memrel(e);
		return FALSE;
	}

	gv->groupbelong = a;
	gv->getnewid = b;
	gv->groupfinallist = c;
	gv->node = d;
	gv->gchain = e;
	gv->tofindgroup = f;

	gv->nplayers = nplayers;
	gv->groupfinallist_n = 0;

	if (!group_buffer_init (&gv->groupbuffer, nplayers)) {
		return FALSE;
	}
	if (!participant_buffer_init (&gv->participantbuffer, nplayers)) {
		 group_buffer_done (&gv->groupbuffer);
		return FALSE;
	}
	if (!connection_buffer_init (&gv->connectionbuffer, nenc*2)) {
		 group_buffer_done (&gv->groupbuffer);
		 participant_buffer_done (&gv->participantbuffer);
		return FALSE;
	}

	return TRUE;
}

void
groupvar_done (group_var_t *gv)
{
	assert(gv);

	if (gv->groupbelong)	memrel (gv->groupbelong );
	if (gv->getnewid) 		memrel (gv->getnewid);
	if (gv->groupfinallist) memrel (gv->groupfinallist);
	if (gv->node) 			memrel (gv->node);
	if (gv->gchain) 		memrel (gv->gchain);
	if (gv->tofindgroup) 	memrel (gv->tofindgroup);


	gv->groupbelong = NULL;
	gv->getnewid = NULL;
	gv->groupfinallist = NULL;
	gv->node = NULL;
	gv->gchain = NULL;
	gv->tofindgroup = NULL;

	gv->groupfinallist_n = 0;
	gv->nplayers = 0;

	group_buffer_done (&gv->groupbuffer);
	participant_buffer_done (&gv->participantbuffer);
	connection_buffer_done (&gv->connectionbuffer);

	return;
}

//==

static bool_t
group_buffer_init (struct GROUP_BUFFER *g, player_t n)
{
	group_t *p;
	size_t elements = (size_t)n + 1; //one extra added for the head
	if (NULL != (p = memnew (sizeof(group_t) * elements))) {
		g->list = p;		
		g->tail = NULL;
		g->prehead = NULL;
		g->n = 0;
		g->max = n;
	} 
	return p != NULL;
}

static void
group_buffer_done (struct GROUP_BUFFER *g)
{
	if (g->list != NULL) {
		memrel(g->list);
		g->list = NULL;
	}
	g->tail = NULL;
	g->prehead = NULL;
	g->n = 0;
}


static bool_t
participant_buffer_init (struct PARTICIPANT_BUFFER *x, player_t n)
{
	participant_t *p;
	if (NULL != (p = memnew (sizeof(participant_t) * (size_t)n))) {
		x->list = p;		
		x->n = 0;
		x->max = n;
	} 
	return p != NULL;
}

static void
participant_buffer_done (struct PARTICIPANT_BUFFER *x)
{
	if (x->list != NULL) {
		memrel(x->list);
		x->list = NULL;
	}
	x->n = 0;
}

static bool_t
connection_buffer_init (struct CONNECT_BUFFER *x, gamesnum_t n)
{
	connection_t *p;
	if (NULL != (p = memnew (sizeof(connection_t) * (size_t)n))) {
		x->list = p;		
		x->n = 0;
		x->max = n;
	} 
	return p != NULL;
}

static void
connection_buffer_done (struct CONNECT_BUFFER *x)
{
	if (x->list != NULL) {
		memrel(x->list);
		x->list = NULL;
	}
	x->n = 0;
}

//----------------------------------------------------------------------------

static group_t * group_new (group_var_t *gv) 
{
	return &gv->groupbuffer.list[gv->groupbuffer.n++];
}

static group_t * group_reset (group_t *g)
{		
	if (g == NULL) return NULL;
	g->next = NULL;	
	g->prev = NULL; 
	g->combined = NULL;
	g->pstart = NULL; g->plast = NULL; 	
	g->cstart = NULL; g->clast = NULL;
	g->lstart = NULL; g->llast = NULL;
	g->id = NO_ID;
	g->isolated = FALSE;
	return g;
}

static group_t * group_combined (group_t *g)
{
	while (g->combined != NULL)
		g = g->combined;
	return g;
}

//no globals
static void
add_participant (group_var_t *gv, group_t *g, player_t i, const char *name)
{
	participant_t *nw;
	nw = participant_new(gv);
	nw->next = NULL;
	nw->name = name;
	nw->id = i; 

	if (g->pstart == NULL) {
		g->pstart = nw; 
		g->plast = nw;	
	} else {
		g->plast->next = nw;	
		g->plast = nw;
	}
}

//no globals
static void
add_beat_connection (group_var_t *gv, group_t *g, struct NODE *pnode)
{
	// Assume scan counters properly sorted and eliminated duplicates
	// otherwise, duplicates could be added	

	connection_t *nw;

	assert(g && pnode && pnode->group);

	nw = connection_new(gv);
	nw->next = NULL;
	nw->node = pnode;
	if (g->cstart == NULL) {
		g->cstart = nw; 
	} else {
		g->clast->next  = nw;
	}		
	g->clast = nw;
}

// no globals
static void
add_lost_connection (group_var_t *gv, group_t *g, struct NODE *pnode)
{
	// Assume scan counters properly sorted and eliminated duplicates
	// otherwise, duplicates could be added	

	connection_t *nw;

	assert(g && pnode && pnode->group);

	nw = connection_new(gv);
	nw->next = NULL;
	nw->node = pnode;
	if (g->lstart == NULL) {
		g->lstart = nw; 
	} else {
		g->llast->next  = nw;
	}		
	g->llast = nw;
}

//


typedef struct PNODE pnode_t;

struct PNODE {
	player_t 	indx;
	pnode_t *	next;
	pnode_t	*	last;		
};

static void
pnode_clear (pnode_t *pn, player_t n)
{
	player_t i;
	for (i = 0; i < n; i++) {
		pn[i].indx = i;
		pn[i].next = NULL;
		pn[i].last = NULL;
	}
}

static void
pnode_connect (pnode_t *pn, player_t i, player_t j)
{
	player_t z;
	pnode_t *a, *b, *c, *l;

	a = pn + i;
	b = pn + j;
	z = a->indx;

	for (c = b; c != NULL; c = c->next) c->indx = z; // fill

	l = b->last? b->last: b;

	l->next = a->next;
	a->next = b;	
	if (a->last == NULL) a->last = l;
	b->last = NULL;
	
}

static int compare_selink (const void * a, const void * b)
{
	const selink_t *sa = a;
	const selink_t *sb = b;

	if (sa->iwin < sb->iwin) return -1;
	if (sa->iwin > sb->iwin) return 1;

	if (sa->ilos < sb->ilos) return -1;
	if (sa->ilos > sb->ilos) return 1;

	return 0;
}

static player_t get_iwin (struct ENC *pe);
static player_t get_ilos (struct ENC *pe);

static bool_t encounter_is_SW (const struct ENC *e) {return e->W  > 0 && e->D == 0 && e->L == 0;}
static bool_t encounter_is_SL (const struct ENC *e) {return e->W == 0 && e->D == 0 && e->L  > 0;}

static bool_t
same_SE_link (selink_t *a, selink_t *b)
{
	return 	a->iwin == b->iwin && 
			a->ilos == b->ilos;
}

// no globals
static void
scan_encounters ( const struct ENC *enc, gamesnum_t n_enc
				, player_t *belongto, player_t n_plyrs
				, struct ENC *sup_enc, gamesnum_t *pn_enc
				 
				, selink_t *sel
				, gamesnum_t *psel_n
				)
{
	// sup_enc: list of "Super" encounters that do not belong to the same group

 	pnode_t *xx;

	player_t i;
	gamesnum_t e;
	const struct ENC *pe;
	gamesnum_t n_a, n_b;
	player_t gw, gb, lowerg, higherg;

	assert (sup_enc != NULL);
	assert (pn_enc != NULL);
	assert (belongto != NULL);

	for (i = 0; i < n_plyrs; i++) {
		belongto[i] = i;
	}

//
	if (NULL != (xx = memnew( (size_t)n_plyrs * sizeof(pnode_t) ))) {

		pnode_clear (xx, n_plyrs);

		for (e = 0, n_a = 0; e < n_enc; e++) {
			pe = &enc[e];
			if (encounter_is_SL(pe) || encounter_is_SW(pe)) {
				sup_enc[n_a++] = *pe;
			} else {
				gw = xx[pe->wh].indx;
				gb = xx[pe->bl].indx;
				if (gw != gb) {
					lowerg   = gw < gb? gw : gb;
					higherg  = gw > gb? gw : gb;
					// join
					pnode_connect(xx, lowerg, higherg);
				}
			}
		} 

		for (i = 0; i < n_plyrs; i++) {
			belongto[i] = xx[i].indx;
		}

		memrel(xx);

	} else {
		fprintf(stderr,"Not enough memory available, FILE=%s, LINE=%d\n",__FILE__, __LINE__);
		exit(EXIT_FAILURE);
	}
//
	for (e = 0, n_b = 0 ; e < n_a; e++) {
		player_t x,y;
		x = sup_enc[e].wh;
		y = sup_enc[e].bl;	
		if (belongto[x] != belongto[y]) {
			if (e != n_b)
				sup_enc[n_b] = sup_enc[e];
			n_b++;
		}
	}

	*pn_enc = n_b; 	// number of "Super" encounters that do not belong to the same group

{
	gamesnum_t r;

	for (e = 0; e < n_b; e++) {
		sel[e].iwin = belongto [ get_iwin(&sup_enc[e]) ];
		sel[e].ilos = belongto [ get_ilos(&sup_enc[e]) ];
	}

	qsort (sel, (size_t)n_b, sizeof(selink_t), compare_selink);

	*psel_n = n_b;

	if (n_b > 0) {
		for (e = 1, r = 0 ; e < n_b; e++) {
			if (!same_SE_link (&sel[r], &sel[e]) ) {
				r++;
				if (r != e) sel[r] = sel[e];
			}
		}
		*psel_n = r + 1;
	} else {
		*psel_n = 0;
	}


}

	return;
}

static bool_t node_is_occupied (group_var_t *gv, player_t x) 
{
	assert(gv);
	assert(gv->node);
	return gv->node[x].group != NULL;
}

static void
node_add_group (group_var_t *gv, player_t x)
{
	group_t *g;
	assert (!node_is_occupied(gv,x)); 
	if (NULL == (g = groupset_find (gv, gv->groupbelong[x]))) {
		g = group_reset(group_new(gv));	
		g->id = gv->groupbelong[x];
		groupset_add(gv,g);

		gv->tofindgroup[gv->groupbelong[x]] = g;
	}
	assert (g->id == gv->groupbelong[x]);
	gv->node[x].group = g;
}

static player_t
group_belonging (group_var_t *gv, player_t x)
{
	group_t *g = group_pointed_by_node (&gv->node[x]);
	return NULL == g? NO_ID: g->id;
}

static group_t *
group_pointed_by_node (node_t *nd)
{
	if (nd && nd->group) {
		return group_combined(nd->group);
	} else {
		return NULL;
	}
}

static group_t *
group_pointed_by_conn (connection_t *c)
{
	return c == NULL? NULL: group_pointed_by_node (c->node);
}

// no globals
static void
selink2group (player_t iwin, player_t ilos, group_var_t *gv)
{	
	if (!node_is_occupied (gv,iwin)) node_add_group (gv,iwin);
	if (!node_is_occupied (gv,ilos)) node_add_group (gv,ilos);
	add_beat_connection	(gv, gv->node[iwin].group, &gv->node[ilos]);
	add_lost_connection	(gv, gv->node[ilos].group, &gv->node[iwin]);
}

static player_t get_iwin (struct ENC *pe) {return pe->W > 0? pe->wh: pe->bl;}
static player_t get_ilos (struct ENC *pe) {return pe->W > 0? pe->bl: pe->wh;}

#if 0
// no globals
static void
sup_enc2group (struct ENC *pe, group_var_t *gv)
{	
	// sort super encounter
	// into their respective groups	
	player_t iwin, ilos;

	assert(pe);
	assert(encounter_is_SL(pe) || encounter_is_SW(pe));

	iwin = get_iwin(pe);
	ilos = get_ilos(pe);
	selink2group (iwin, ilos, gv);
}
#endif

static void
group_gocombine (group_t *g, group_t *h);



static void
convert_general_init (group_var_t *gv, player_t n_plyrs)
{
	player_t i;

	connect_init(gv);
	participant_init(gv);
	groupset_init(gv);
	for (i = 0; i < n_plyrs; i++) {
		gv->node[i].group = NULL;
	}
	return;
}

static player_t
groupvar_counter (group_var_t *gv)
{
	return gv->groupfinallist_n;
}


static void
groupvar_finallist_sort(group_var_t *gv);

player_t
groupvar_build (group_var_t *gv, player_t n_plyrs, const char **name, const struct PLAYERS *players, const struct ENCOUNTERS *encounters)
{
	selink_t *SElnk;
	gamesnum_t SElnk_n;

	player_t ret;
	player_t i;
	gamesnum_t e;
	super_t SP;
	super_t *sp = &SP;

	timelog("memory initialization...");

	if (supporting_encmem_init (encounters->n, sp, &SElnk)) {

		timelog("scan games...");
		scan_encounters (encounters->enc, encounters->n, gv->groupbelong, players->n, sp->SE2, &sp->N_se2, SElnk , &SElnk_n); 

		timelog("general initialization...");
		convert_general_init (gv, n_plyrs);

		// Initiate groups from critical "super" encounters
		timelog_ld("incorporate links into groups...      N=", (long)sp->N_se2);
		timelog_ld("incorporate links into groups... Unique=", (long)SElnk_n);

		groupset_reset_finding (gv);

		for (e = 0 ; e < SElnk_n; e++) {
			selink2group (SElnk[e].iwin, SElnk[e].ilos, gv);
		}

		supporting_encmem_done (sp, SElnk);

	} else {
		fprintf (stderr, "No memory available\n");
		exit(EXIT_FAILURE);
	}

	timelog("start groups for each player with no links...");

	// Initiate groups for each player present in the database that did not have 
	// critical super encounters
	for (i = 0; i < n_plyrs; i++) {
		if (players->present_in_games[i] && !node_is_occupied (gv,i)) 
			node_add_group (gv,i);
	}

	timelog("add list of participants...");
	// for all the previous added groups, add respective list of participants
	for (i = 0; i < n_plyrs; i++) {
		if (node_is_occupied(gv,i)) {
			group_t *g = groupset_find(gv, gv->groupbelong[i]);
			if (g) add_participant(gv, g, i, name[i]);	
		}
	}

	assert(groupset_sanity_check_nocombines(gv));

	timelog("group joining and simplification...");
	groupvar_simplify(gv);

	timelog("finish complex circuits...");
	groupvar_finish(gv);

	timelog("sort...");
	groupvar_list_sort (gv);
	groupvar_finallist_sort(gv);

	timelog("assign new indexes...");
	groupvar_assign_newid (gv);

	ret = groupvar_counter(gv) ;

	return ret;
}


static void
groupvar_sieveenc	( group_var_t *gv
					, const struct ENC *enc
					, gamesnum_t n_enc
					, gamesnum_t *N_enca
					, gamesnum_t *N_encb
)
{
	gamesnum_t e;
	player_t w,b;
	gamesnum_t na = 0, nb = 0;

	for (e = 0; e < n_enc; e++) {
		w = enc[e].wh; 
		b = enc[e].bl; 
		if (group_belonging(gv,w) == group_belonging(gv,b)) {
			na += 1;
		} else {
			nb += 1;
		}
	} 
	*N_enca = na;
	*N_encb = nb;
	return;
}

static void
group_gocombine (group_t *g, group_t *h)
{
	// unlink h
	group_t *pr = h->prev;
	group_t *ne = h->next;

	assert(pr);

	if (h->combined == g) {
		return;
	}	

	// unlink h
	h->prev = NULL;
	h->next = NULL;
	pr->next = ne;
	if (ne) ne->prev = pr;
	
	// if h is called, go to h
	h->combined = g;
	
	// inherit information from h to g
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

//-----------------------------------------------


static void simplify (group_t *g, group_t **cmbbuf);

static group_t *group_next (group_t *g)
{
	assert(g);
	return g->next;
}

static void
groupvar_simplify (group_var_t *gv)
{
	group_t 	*g;
	group_t *	*cmbbuf;
	size_t mem = (size_t)gv->nplayers * sizeof (group_t *);

	if (NULL != (cmbbuf = memnew (mem))) {

		g = groupset_head(gv);
		assert(g);

		while(g) {
			simplify(g,cmbbuf);
			g = group_next(g);
		}
		assert(groupset_sanity_check(gv));

		memrel(cmbbuf);
	} else {
		fprintf (stderr, "No memory available for group detection\n");
		exit (EXIT_FAILURE);
	}

	return;
}

#if 0
static void
beat_lost_output (group_t *g)
{
	group_t 		*beat_to, *lost_to;
	connection_t 	*c;

	printf ("  G=%ld, beat_to: ",g->id);

		// loop connections, examine id if repeated or self point (delete them)
		beat_to = NULL;
		c = g->cstart; 
		do {
			if (c && NULL != (beat_to = group_pointed_by_conn(c))) {
				printf ("%ld, ",beat_to->id);
				c = c->next;
			}
		} while (c && beat_to);

	printf ("\n");
	printf ("  G=%ld, lost_to: ",g->id);

		lost_to = NULL;
		c = g->lstart; 
		do {
			if (c && NULL != (lost_to = group_pointed_by_conn(c))) {
				printf ("%ld, ",lost_to->id);
				c = c->next;
			}
		} while (c && lost_to);
	printf ("\n");
}
#endif

static player_t
id_pointed_by_conn(connection_t *c)
{
	group_t *gg;
	return NULL != c && NULL != (gg = group_pointed_by_conn(c))? gg->id: NO_ID;
}

static player_t
find_top_id (group_t *g)
{
	connection_t *c;
	player_t id;
	player_t topid = g->id;
	for (c = g->cstart; NULL != c; c = c->next) {
		id = id_pointed_by_conn(c);
		if (id > topid) topid = id;
	}
	for (c = g->lstart; NULL != c; c = c->next) {
		id = id_pointed_by_conn(c);
		if (id > topid) topid = id;
	}
	return topid;
}

static void
simplify_shrink__ (group_t *g)
{
	bitarray_t 		bA;
	connection_t 	*c, *p, *pre;
	player_t		id;
	connection_t 	pre_connection = {NULL, NULL};
	player_t		max_player;
	
	pre = &pre_connection;
	assert(g);

	if (g->combined) {
		assert(g->cstart == NULL);
		assert(g->lstart == NULL);
		assert(g->pstart == NULL);
		return;
	}

	max_player = 1 + find_top_id(g); // used to be max_player = gv->nplayers;

	if (!ba_init(&bA, max_player)) {
		fprintf(stderr, "No memory to initialize internal arrays\n");
		exit(EXIT_FAILURE);			  
	}

	assert (g->id < max_player);

	// beat list
	pre->next = g->cstart;
	
	ba_clear(&bA);
	ba_put(&bA, g->id);
	for (p = pre, c = p->next; NULL != c; c = c->next) {
		if (NO_ID != (id = id_pointed_by_conn(c)) && ba_ison(&bA, id)) {
			p->next = c->next;
		 } else {
			p = c;
			if (id != NO_ID) ba_put(&bA, id);
		}	
		assert(id < max_player);
	}

	g->cstart = pre->next;
	g->clast  = p;

	// lost list
	pre->next = g->lstart;

	ba_clear(&bA);
	ba_put(&bA, g->id);
	for (p = pre, c = p->next; NULL != c; c = c->next) {
		if (NO_ID != (id = id_pointed_by_conn(c)) && ba_ison(&bA, id)) {
			p->next = c->next;
		 } else {
			p = c;
			if (id != NO_ID) ba_put(&bA, id);
		}	
		assert(id < max_player);
	}

	g->lstart = pre->next;
	g->llast  = p;

	ba_done(&bA);

	return;
}


static void
simplify_shrink_redundancy (group_t *g)
{
	#if 0
	printf("-------------\n");
	printf("before shrink\n");
	beat_lost_output (g);
	#endif

	simplify_shrink__ (g);
	
	#if 0
	printf("after  shrink\n");
	beat_lost_output (g);
	printf("-------------\n");
	#endif
}


static player_t
find_combine_candidate (group_t *g, group_t *comb[])
{
	group_t 		*comb_candidate = NULL;
	bitarray_t 		bA;
	connection_t 	*c;
	player_t		id;
	player_t		max_player;
	player_t 		cmb = 0;

	assert(g);

	max_player = 1 + find_top_id(g); // used to be max_player = gv->nplayers;

	if (!ba_init(&bA, max_player)) {
		fprintf(stderr, "No memory to initialize internal arrays\n");
		exit(EXIT_FAILURE);			  
	}

	// beat list
	for (c = g->cstart; NULL != c; c = c->next) {
		id = id_pointed_by_conn(c);
		if (id != NO_ID) ba_put(&bA, id);
	}

	// lost list
	for (c = g->lstart; NULL != c; c = c->next) {
		id = id_pointed_by_conn(c);
		if (id != NO_ID && ba_ison(&bA, id)) {
			comb_candidate = group_pointed_by_conn(c);			
			comb[cmb++] = comb_candidate;
		}
	}

	ba_done(&bA);

	return cmb;
}

static void
simplify (group_t *g, group_t **combbuf)
{
	player_t cmb;
	player_t n;
	group_t	*combine_with = NULL;

	do {
		simplify_shrink_redundancy (g);
		n = find_combine_candidate (g, combbuf);
		if (n > 0) {
			// printf ("comb: %5ld \n", g->id);
			for (cmb = 0; cmb < n; cmb++) {
				combine_with = combbuf[cmb];
				group_gocombine (g, combine_with);
			}
		} 
	} while (n);
	simplify_shrink_redundancy (g);
	return;
}

//======================

static connection_t *group_beat_head (group_t *g) {return g->cstart;} 
static connection_t *beat_next (connection_t *b) {return b->next;} 

static group_t *
group_unlink (group_t *g)
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
group_next_pointed_by_beat (group_t *g)
{
	group_t *gp;
	connection_t *c;
	player_t own_id;
	if (NULL == g || NULL == (c = group_beat_head(g)))	
		return NULL;
	own_id = g->id;
	while ( c && (NULL == (gp = group_pointed_by_conn(c)) || gp->isolated || gp->id == own_id)) {
		c = beat_next(c);
	} 
	return c == NULL? NULL: gp;
}

static player_t participants_list_population (participant_t *pstart);

static player_t
group_population (group_t *s)
{
	assert(s);
	return participants_list_population(s->pstart);
}

static bool_t
sorted (participant_t * a, participant_t * b);

static int compare_cell (const void * a, const void * b)
{
	int r;
	const groupcell_t *ap = a;
	const groupcell_t *bp = b;

	r = ap->count < bp->count? 1: (ap->count > bp->count? -1:0);

	if (r == 0) {
		r = sorted (ap->group->pstart, bp->group->pstart)? -1: 1;  
	}

	return r;
}


static void
groupvar_finallist_sort(group_var_t *gv)
{
	qsort (gv->groupfinallist, (size_t)gv->groupfinallist_n, sizeof(groupcell_t), compare_cell);
}

static void
groupvar_finish (group_var_t *gv)
{
	player_t i;

	bitarray_t 	bA;
	player_t *chain, *chain_end;
	group_t *g, *gp, *prev_g, *x;
	connection_t *b;
	player_t own_id, bi;
	bool_t startover, combined;

	assert(gv);

	if (!ba_init(&bA, gv->nplayers)) {
		fprintf(stderr, "No memory to initialize internal arrays\n");
		exit(EXIT_FAILURE);		
	}

	assert(gv->groupfinallist_n == 0); // should have been initialized
	gv->groupfinallist_n = 0;

	do {
		startover = FALSE;
		combined = FALSE;

		chain = gv->gchain;

		g = groupset_head(gv);
		if (g == NULL) break;
		own_id = g->id; // own id

		do {
			ba_put(&bA, own_id);
			*chain++ = own_id;
			prev_g = g;
			g = group_next_pointed_by_beat(g);

			if (prev_g->lstart == NULL || g == NULL) { // no losses or no victories, quit now to optimize
				gv->groupfinallist[gv->groupfinallist_n++].group = group_unlink(prev_g);
				startover = TRUE;
			} else {
				assert (g != NULL);
				own_id = g->id;
				for (b = group_beat_head(g); b != NULL; b = beat_next(b)) {
					gp = group_pointed_by_conn(b);
					bi = gp->id;
					if (ba_ison(&bA, bi)) {
						//	findprevious bi, combine... remember to include own id;
						for (chain_end = chain; *chain != bi && chain > gv->gchain; )
							chain--;
						x = group_pointed_by_node(gv->node + own_id);
						for (; chain < chain_end; chain++) { 
							group_gocombine (x, group_pointed_by_node(gv->node + *chain));
							combined = TRUE;
						}
						simplify_shrink_redundancy(x);
						break;
					}
				}
			}

		} while (!combined && !startover);

		ba_clear(&bA);			

	} while (startover || combined);

	ba_done(&bA);	

	// build groupcell_t
	for (i = 0; i < gv->groupfinallist_n; i++) {
		player_t pop = group_population(gv->groupfinallist[i].group);
		gv->groupfinallist[i].count = pop;
	}

	groupvar_finallist_sort(gv);

//	qsort (gv->groupfinallist, (size_t)gv->groupfinallist_n, sizeof(groupcell_t), compare_cell);

	return;
}

// no globals
static void
groupvar_assign_newid (group_var_t *gv)
{
	group_t *g;
	player_t i;
	long new_id;

	for (i = 0; i < gv->nplayers; i++) {
		gv->getnewid[i] = NO_ID;
	}

	new_id = 0;
	for (i = 0; i < gv->groupfinallist_n; i++) {
		g = gv->groupfinallist[i].group;
		new_id++;
		gv->getnewid[g->id] = new_id;
	}
}

// no globals
static void
groupvar_list_output (group_var_t *gv, FILE *f)
{
	group_t *g;
	player_t i;

	for (i = 0; i < gv->groupfinallist_n; i++) {
		g = gv->groupfinallist[i].group;
		fprintf (f,"\nGroup %ld\n",(long)gv->getnewid[g->id]);
		simplify_shrink_redundancy (g);
		group_output (g, gv, f, FALSE);
	}

	fprintf(f,"\n");
}



static player_t
participants_list_population (participant_t *pstart)
{
	player_t group_n;
	participant_t *p;

	for (p = pstart, group_n = 0; p != NULL; p = p->next) {
		group_n++;
	}
	return group_n;
}


/*
static int compare_str (const void * a, const void * b)
{
	const char * const *ap = a;
	const char * const *bp = b;
	return strcmp(*ap,*bp);
}
*/



static participant_t *
plist_go_last (participant_t *pstart)
{
	participant_t *s, *p;
	s = pstart;
	p = NULL;
	while (s) {
		p = s;
		s = s->next;
	}
	return p;
}

static bool_t
sorted (participant_t * a, participant_t * b)
{
	assert(a && b);
	return 0 > strcmp(a->name, b->name);	
}

static participant_t *
plist_inssort (participant_t *pstart)
{
	participant_t head;
	participant_t *h, *i, *j, *q, *p;

	head.next = NULL;
	head.name = NULL;
	head.id   = NO_ID;
	h = &head;

	i = pstart;

	while (i) {
		j = i->next;

		p = h;
		while (p->next && !sorted (i, p->next))
			p = p->next;

		q = p->next;
		p->next = i;
		i->next = q;

		i = j;
	}

	return h->next;
}

//--------------

static bool_t
too_short(participant_t *a)
{
	participant_t *p;
	long unsigned i = 0;
	for (p = a; p != NULL && i < 3; p = p->next) {
		i++;
	}
	return i < 3;
}

#if 1
static participant_t *
get_half(participant_t *a)
{
	participant_t *p, *s, *b;
	long unsigned i = 0;

	if (a == NULL) return NULL;

	for (p = a, s = a; p != NULL; p = p->next) {
		if ((i & 1) == 1)
			s = s->next;
		i++;
	}

	b = s->next;
	s->next = NULL;

	return b;
}
#else
static participant_t *
get_half(participant_t *a)
{
	participant_t *r, *b;

	if (a == NULL) return NULL;

	r = b = a->next;

	for (;;) {
		if (NULL == b) break;
		a->next = b->next;
		a = b->next;
		if (NULL == a) break;
		b->next = a->next;
		b = a->next;
	}

	return r;
}
#endif

static participant_t *
merge (participant_t *a, participant_t *b)
{
	participant_t head;
	participant_t *l, *t;

	assert(a && b);

	head.next = NULL;
	l = &head;

	while (a && b) {
		if (sorted(a,b)) {
			t = a;
			a = a->next;
		} else {
			t = b;
			b = b->next;
		}
		l->next = t;
		t->next = NULL;
		l = t;	
	}

	if (a == NULL) {
		l->next = b;
	}
	if (b == NULL) {
		l->next = a;
	}
	return head.next;	
	
}


static participant_t *
plist_mrgsort (participant_t *a)
{
	participant_t *b, *r;

	if (too_short(a))
		return plist_inssort(a);

	b = get_half(a);
	a = plist_mrgsort(a);
	b = plist_mrgsort(b);
	r = merge(a,b);
	return r;
}

//--------------

static void
groupvar_list_sort (group_var_t *gv)
{
	group_t *g;
	player_t i;

	for (i = 0; i < gv->groupfinallist_n; i++) {
		g = gv->groupfinallist[i].group;
		g->pstart = plist_mrgsort(g->pstart);
		g->plast  = plist_go_last(g->pstart);
	}
}


static void
participants_list_print (FILE *f, participant_t *pstart)
{
	participant_t *p;
	for (p = pstart; p != NULL; p = p->next) {
		fprintf (f," | %s\n",p->name);
	}
}


static void
group_output (group_t *s, group_var_t *gv, FILE *f, bool_t showlinks)
{		
	connection_t *c;
	player_t own_id;
	int winconnections = 0, lossconnections = 0;
	assert(s);
	own_id = s->id;

	participants_list_print (f, s->pstart);

	for (c = s->cstart; c != NULL; c = c->next) {
		group_t *gr = group_pointed_by_conn(c);
		if (gr != NULL) {
			if (gr->id != own_id) {
				if (showlinks)
					fprintf (f," \\---> there are (only) wins against group: %ld\n",(long)gv->getnewid[gr->id]);
				winconnections++;
			}
		} else
			fprintf (f,"point to node NULL\n");
	}
	for (c = s->lstart; c != NULL; c = c->next) {
		group_t *gr = group_pointed_by_conn(c);
		if (gr != NULL) {
			if (gr->id != own_id) {
				if (showlinks)
					fprintf (f," \\---> there are (only) losses against group: %ld\n",(long)gv->getnewid[gr->id]);
				lossconnections++;
			}
		} else
			fprintf (f,"pointed by node NULL\n");
	}
	if (winconnections == 0 && lossconnections == 0) {
		fprintf (f," \\---> this group is isolated from the rest\n");
	} else {
		if (!showlinks)
			fprintf (f," \\---> this group has incomplete links with the rest (only wins or losses)\n");
	}
}


//-----------------------------------------------------------------------------------

static void
groupvar_output_info (group_var_t *gv, FILE *groupf)
{
	if (NULL != groupf) {
		if (groupvar_counter(gv) > 1) {
			fprintf (groupf,"Group connectivity: **FAILED**\n");
			groupvar_list_output(gv, groupf);
		} else {
			assert (1 == groupvar_counter(gv));
			fprintf (groupf,"Group connectivity: **PASSED**\n");
			fprintf (groupf,"All players are connected into only one group.\n");
		}	
	}
}

static void
groupvar_to_groupid(group_var_t *gv, player_t *groupid_out)
{
	if (groupid_out) {
		player_t i;
		for (i = 0; i < gv->nplayers; i++) {
			groupid_out[i] = gv->getnewid[group_belonging(gv,i)];
		}
	}
}

//----------------------------------------------------------

group_var_t *
GV_make
		( const struct ENCOUNTERS *encounters
		, const struct PLAYERS *players
		)
{
	player_t 	n = 0;
	bool_t 		ok = FALSE;
	group_var_t *gv;

	assert (encounters && players);
	assert (encounters->n > 0);

	if (NULL != (gv = memnew(sizeof(group_var_t)))) {
		if (groupvar_init (gv, players->n, encounters->n)) {
			n = groupvar_build (gv, players->n, players->name, players, encounters);
			ok = n > 0 && TRUE;
		} else {
			ok = FALSE;
		}
		if (!ok) memrel(gv);
	}

	return ok? gv: NULL;
}

group_var_t *
GV_kill (group_var_t *gv)
{
	assert(gv);
	groupvar_done(gv);
	memrel(gv);
	return NULL;
}

void
GV_out (group_var_t *gv, FILE *f)
{
	assert(f);
	if (f)	groupvar_output_info (gv, f);
}

void
GV_sieve (group_var_t *gv, const struct ENCOUNTERS *encounters, gamesnum_t * pN_intra, gamesnum_t * pN_inter)
{
	assert(gv && encounters && pN_intra && pN_inter);
	groupvar_sieveenc (gv, encounters->enc, encounters->n, pN_intra, pN_inter);
}


player_t
GV_counter (group_var_t *gv)
{
	assert(gv);
	return groupvar_counter(gv);
}

void
GV_groupid (group_var_t *gv, player_t *groupid_out)
{
	assert(gv && groupid_out);
	groupvar_to_groupid(gv, groupid_out);
}

static player_t
participants_list_actives (participant_t *pstart, const struct PLAYERS *players)
{
	participant_t *p;
	player_t j;
	player_t accum; 

	for (p = pstart, accum = 0; p != NULL; p = p->next) {
		j = p->id;
		accum += players->present_in_games[j]? (player_t)1: (player_t)0;
	}
	return accum;
}

static player_t
group_number_of_actives (group_t *s, const struct PLAYERS *players)
{		
	return	participants_list_actives (s->pstart, players);
}

player_t
GV_non_empty_groups_pop (group_var_t *gv, const struct PLAYERS *players)
{
	group_t *g;
	player_t i;
	player_t x;
	player_t counter = 0;

	for (i = 0; i < gv->groupfinallist_n; i++) {
		g = gv->groupfinallist[i].group;
		simplify_shrink_redundancy (g);
		x = group_number_of_actives (g, players);
		if (x > 0) counter++;
	}
	return counter;
}

bool_t
well_connected (const struct ENCOUNTERS *pEncounters, const struct PLAYERS *pPlayers)
{
	bool_t ok = FALSE;
	group_var_t *gv;

	if (NULL != (gv = GV_make (pEncounters, pPlayers))) {
		ok = GV_counter(gv) == 1 && GV_non_empty_groups_pop (gv, pPlayers) == 1;
		GV_kill(gv);
	} else {
		fprintf (stderr, "not enough memory for encounters allocation\n");
		exit(EXIT_FAILURE);
	}
	return ok;
}
