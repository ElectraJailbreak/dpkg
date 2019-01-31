/*
 * libdpkg - Debian packaging suite library routines
 * fsys-hash.c - filesystem nodes hash table
 *
 * Copyright © 1995 Ian Jackson <ijackson@chiark.greenend.org.uk>
 * Copyright © 2000, 2001 Wichert Akkerman <wakkerma@debian.org>
 * Copyright © 2008-2014 Guillem Jover <guillem@debian.org>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <compat.h>

#include <string.h>
#include <stdlib.h>

#include <dpkg/dpkg.h>
#include <dpkg/dpkg-db.h>
#include <dpkg/string.h>
#include <dpkg/path.h>

#include "fsys.h"

/* This must always be a prime for optimal performance.
 * This is the closest one to 2^18 (262144). */
#define BINS 262139

static struct fsys_namenode *bins[BINS];
static int nfiles = 0;

void
fsys_hash_init(void)
{
	struct fsys_namenode *fnn;
	int i;

	for (i = 0; i < BINS; i++) {
		for (fnn = bins[i]; fnn; fnn = fnn->next) {
			fnn->flags = 0;
			fnn->oldhash = NULL;
			fnn->newhash = EMPTYHASHFLAG;
			fnn->file_ondisk_id = NULL;
		}
	}
}

void
fsys_hash_reset(void)
{
	int i;

	for (i = 0; i < BINS; i++)
		bins[i] = NULL;

	nfiles = 0;
}

int
fsys_hash_entries(void)
{
	return nfiles;
}

struct fsys_namenode *
fsys_hash_find_node(const char *name, enum fsys_hash_find_flags flags)
{
	struct fsys_namenode **pointerp, *newnode;
	const char *orig_name = name;

	/* We skip initial slashes and ‘./’ pairs, and add our own single
	 * leading slash. */
	name = path_skip_slash_dotslash(name);

	pointerp = bins + (str_fnv_hash(name) % (BINS));
	while (*pointerp) {
		/* XXX: This should not be needed, but it has been a constant
		 * source of assertions over the years. Hopefully with the
		 * internerr() we will get better diagnostics. */
		if ((*pointerp)->name[0] != '/')
			internerr("filename node '%s' does not start with '/'",
			          (*pointerp)->name);

		if (strcmp((*pointerp)->name + 1, name) == 0)
			break;
		pointerp = &(*pointerp)->next;
	}
	if (*pointerp)
		return *pointerp;

	if (flags & FHFF_NONE)
		return NULL;

	newnode = nfmalloc(sizeof(*newnode));
	newnode->packages = NULL;
	if ((flags & FHFF_NOCOPY) && name > orig_name && name[-1] == '/') {
		newnode->name = name - 1;
	} else {
		char *newname = nfmalloc(strlen(name) + 2);

		newname[0] = '/';
		strcpy(newname + 1, name);
		newnode->name = newname;
	}
	newnode->flags = 0;
	newnode->next = NULL;
	newnode->divert = NULL;
	newnode->statoverride = NULL;
	newnode->oldhash = NULL;
	newnode->newhash = EMPTYHASHFLAG;
	newnode->file_ondisk_id = NULL;
	newnode->trig_interested = NULL;
	*pointerp = newnode;
	nfiles++;

	return newnode;
}

/*
 * Forward iterator.
 */

struct fsys_hash_iter {
	struct fsys_namenode *namenode;
	int nbinn;
};

struct fsys_hash_iter *
fsys_hash_iter_new(void)
{
	struct fsys_hash_iter *iter;

	iter = m_malloc(sizeof(*iter));
	iter->namenode = NULL;
	iter->nbinn = 0;

	return iter;
}

struct fsys_namenode *
fsys_hash_iter_next(struct fsys_hash_iter *iter)
{
	struct fsys_namenode *fnn = NULL;

	while (!iter->namenode) {
		if (iter->nbinn >= BINS)
			return NULL;
		iter->namenode = bins[iter->nbinn++];
	}
	fnn = iter->namenode;
	iter->namenode = fnn->next;

	return fnn;
}

void
fsys_hash_iter_free(struct fsys_hash_iter *iter)
{
	free(iter);
}
