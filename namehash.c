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
#include <string.h>

#include "namehash.h"
#include "pgnget.h"
#include "mymem.h"

#ifdef NDEBUG
	// normal values
	#define PEAXPOD 8  
	#define PODBITS 12 
	#define PEA_REM_MAX (256*256)
#else
	// forces extremely low values
	#define PEAXPOD 1
	#define PODBITS 1
	#define PEA_REM_MAX 1
#endif

#define PODMASK ((1<<PODBITS)-1)
#define PODMAX   (1<<PODBITS)

struct NAMEPEA {
	player_t pidx; // player index
	uint32_t hash; // name hash
};

static bool_t name_tree_init(void);
static void   name_tree_done(void);
static bool_t name_ispresent_hashtable (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index);
static bool_t name_register_hashtable (uint32_t hash, player_t i);
static bool_t name_ispresent_tree (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index);
static bool_t name_register_tree (uint32_t hash, player_t i);

//*************************** GENERAL **************************************

bool_t
name_storage_init(void)
{
	return TRUE;
}

void
name_storage_done(void)
{
	name_tree_done();
	return;
}

bool_t
name_ispresent (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index)
{
	return	name_ispresent_hashtable (d, s, hash, out_index)
		||	name_ispresent_tree (d, s, hash, out_index);
}


bool_t
name_register (uint32_t hash, player_t i)
{
	return	name_register_hashtable (hash, i)
		||	name_register_tree (hash, i);
}

//************************* HASHED STORAGE *********************************

struct NAMEPOD {
	struct NAMEPEA pea[PEAXPOD];
	int n;
};

// ----------------- PRIVATE DATA---------------
static struct NAMEPOD Namehashtab[PODMAX];
//----------------------------------------------


static bool_t
name_ispresent_hashtable (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index)
{
	struct NAMEPOD *ppod = &Namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;
	bool_t 			found= FALSE;
	int i;

	ppea = ppod->pea;
	n = ppod->n;
	for (i = 0; i < n; i++) {
		if (ppea[i].hash == hash) {
			const char *name_str = database_getname(d, ppea[i].pidx);
			assert(name_str);
			if (!strcmp(s, name_str)) {
				found = TRUE;
				*out_index = ppea[i].pidx;
				break;
			}
		}
	}
	return found;
}

bool_t
name_register_hashtable (uint32_t hash, player_t i)
{
	struct NAMEPOD *ppod = &Namehashtab[hash & PODMASK];
	struct NAMEPEA *ppea;
	int 			n;

	ppea = ppod->pea;
	n = ppod->n;	

	if (n < PEAXPOD) {
		ppea[n].pidx = i;
		ppea[n].hash = hash;
		ppod->n++;
		return TRUE;
	}
	else {
		return FALSE;
	}
}

//**************************************************************************
#define MAX_NODESxBUFFER PEA_REM_MAX

struct NODETREE {
	struct NODETREE *hi;
	struct NODETREE *lo;
	struct NAMEPEA p;
};

struct BUFFERBLOCK {
	struct NODETREE buffer[MAX_NODESxBUFFER];
	struct BUFFERBLOCK *next;
};

// ----------------- PRIVATE DATA---------------
static struct BUFFERBLOCK *Buffer_head = NULL;
static struct BUFFERBLOCK *Buffer_curr = NULL;
static player_t Treemembers = 0;
static struct NODETREE *Troot = NULL;
static struct NODETREE *T_end = NULL;
static struct NODETREE *Tstop = NULL;
//----------------------------------------------

static void nodetree_connect (struct NODETREE *root, struct NODETREE *pnew);
static int	nodetree_cmp (struct NODETREE *a, struct NODETREE *b);
static bool_t nodetree_is_hit (const struct DATA *d, const char *s, uint32_t hash, const struct NODETREE *pnode);

static bool_t
name_tree_init (void)
{
	struct BUFFERBLOCK *q = memnew (sizeof (struct BUFFERBLOCK));
	if (q == NULL) return FALSE;
	q->next = NULL;
	Buffer_head = q;
	Buffer_curr = q;
	Troot = &q->buffer[0];
	T_end = Troot;
	Tstop = T_end + MAX_NODESxBUFFER;
	Treemembers = 0;
	return TRUE;
}

static void
name_tree_done (void)
{
	struct BUFFERBLOCK *p = Buffer_head;
	struct BUFFERBLOCK *n = NULL;
	while (p) {
		n = p->next;
		p->next = NULL;
		memrel(p);
		p = n;
	}
	return;
}

static bool_t
name_tree_addmem (void)
{
	struct BUFFERBLOCK *q = memnew (sizeof (struct BUFFERBLOCK));
	if (q == NULL) return FALSE;
	q->next = NULL;
	Buffer_curr->next = q;
	Buffer_curr = q;
	T_end = &q->buffer[0];
	Tstop = T_end + MAX_NODESxBUFFER;
	return TRUE;	
}

static bool_t
name_register_tree (uint32_t hash, player_t i)
{
	if (Treemembers == 0)
		name_tree_init();

	if (T_end == Tstop) {
		if (!name_tree_addmem())
			return FALSE;
	}

	T_end->hi = NULL;
	T_end->lo = NULL;		
	T_end->p.pidx = i;
	T_end->p.hash = hash;

	if (Treemembers > 0)
		nodetree_connect (Troot, T_end);
	Treemembers++;
	T_end++;
	return TRUE;
}


static bool_t
name_ispresent_tree (struct DATA *d, const char *s, uint32_t hash, /*out*/ player_t *out_index)
{
	bool_t hit = FALSE;
	struct NODETREE *pnode;
	for (pnode = Troot; !hit && pnode != NULL;) {
		hit = nodetree_is_hit (d, s, hash, pnode);
		if (hit) {
			*out_index = pnode->p.pidx;
		} else if (hash < pnode->p.hash) {
			pnode = pnode->lo;
		} else {
			pnode = pnode->hi;			
		}	
	}
	return hit;
}


static int
nodetree_cmp (struct NODETREE *a, struct NODETREE *b)
{
	if (a->p.hash < b->p.hash) return -1;
	if (a->p.hash > b->p.hash) return +1;
	if (a->p.pidx < b->p.pidx) return -1;
	if (a->p.pidx > b->p.pidx) return +1;
	return 0;	
}

static void
nodetree_connect (struct NODETREE *root, struct NODETREE *pnew)
{
	struct NODETREE *r = root;
	bool_t done = FALSE;
	while (!done) {
		if (0 > nodetree_cmp(pnew,r)) {
			if (r->lo == NULL) {
				r->lo = pnew;
				done = TRUE;
			} else {
				r = r->lo;
			}	
		} else {
			if (r->hi == NULL) {
				r->hi = pnew;
				done = TRUE;
			} else {
				r = r->hi;
			}	
		}
	}
	return;
}

static bool_t
nodetree_is_hit (const struct DATA *d, const char *s, uint32_t hash, const struct NODETREE *pnode)
{
	return hash == pnode->p.hash && !strcmp(s, database_getname(d, pnode->p.pidx));
}

/************************************************************************/

/*http://www.cse.yorku.ca/~oz/hash.html*/

uint32_t
namehash(const char *str)
{
	uint32_t hash = 5381;
	char chr;
	unsigned int c;
	while ('\0' != *str) {
		chr = *str++;
		c = (unsigned int) ((unsigned char)(chr));
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	}
	return hash;
}

/************************************************************************/



