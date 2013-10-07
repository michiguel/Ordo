#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>
#include "boolean.h"
#include "csv.h"

#define MAXSIZETOKEN 1024

static bool_t issep(int c) { return c == ',';}
static bool_t isquote(int c) { return c == '"';}
static char *skipblanks(char *p) {while (isspace(*p)) p++; return p;}

//===================================================================

static void
rm_blank_tails(char *s)
{
	char *t = s + strlen(s);
	while (t-- > s && isspace(*t))
		*t = '\0';
}

static char *
csv_gettoken(char *p, char *s, size_t max)
{
	char *s_ori;
	s_ori = s;

	if (*p == '\0') return NULL; // no more tokens

	p = skipblanks(p);

	if (issep(*p)) {
		s[0] = '\0'; //return empty string
		return ++p;
	}

	if (isquote(*p)) {
		p++;
		while (*p != '\0' && s < max + s_ori && !isquote(*p)) {*s++ = *p++;}
		*s = '\0';
		if (isquote(*p++)) {
			p = skipblanks(p);
			if (issep(*p)) p++;
		}
		return p;
	}

	while (*p != '\0' && s < max + s_ori && !issep(*p)) {*s++ = *p++;}
	*s = '\0';

	rm_blank_tails(s_ori);

	if (issep(*p)) p++;
	
	return p;
}

//================================

bool_t
csv_line_init(csv_line_t *c, char *p)
{
	char *s;
	bool_t success;

	assert(c != NULL);

	c->n = 0;	
	c->s[0] = NULL;
	c->mem = malloc(4096);
	if (c->mem == NULL) return FALSE;

	s = c->mem;
	p = skipblanks(p);

	success = (NULL != (p = csv_gettoken(p, s, MAXSIZETOKEN)));

	while (success) {

		p = skipblanks(p);

		c->s[c->n++] = s;

		s += strlen(s) + 1;

		success = (NULL != (p = csv_gettoken(p, s, MAXSIZETOKEN)));
	}
	return c->mem != NULL;
}

void
csv_line_done(csv_line_t *c)
{
	assert(c != NULL);
	if (c->mem)	free(c->mem);
	c->n = 0;
	c->mem = NULL;
	c->s[0] = NULL;
}



