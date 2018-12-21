/*
 * libdpkg - Debian packaging suite library routines
 * fsys.h - filesystem nodes hash table
 *
 * Copyright © 1995 Ian Jackson <ijackson@chiark.greenend.org.uk>
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

#ifndef LIBDPKG_FSYS_H
#define LIBDPKG_FSYS_H

#include <dpkg/file.h>

/*
 * Data structure here is as follows:
 *
 * For each package we have a ‘struct fileinlist *’, the head of a list
 * of files in that package. They are in ‘forwards’ order. Each entry has
 * a pointer to the ‘struct filenamenode’.
 *
 * The struct filenamenodes are in a hash table, indexed by name.
 * (This hash table is not visible to callers.)
 *
 * Each filenamenode has a (possibly empty) list of ‘struct filepackage’,
 * giving a list of the packages listing that filename.
 *
 * When we read files contained info about a particular package we set the
 * ‘files’ member of the clientdata struct to the appropriate thing. When
 * not yet set the files pointer is made to point to ‘fileslist_uninited’
 * (this is available only internally, within filesdb.c - the published
 * interface is ensure_*_available).
 */

struct pkginfo;

/**
 * Flags to findnamenode().
 */
enum fnnflags {
	/** Do not need to copy filename. */
	fnn_nocopy			= DPKG_BIT(0),
	/** findnamenode may return NULL. */
	fnn_nonew			= DPKG_BIT(1),
};

enum filenamenode_flags {
	/** In the newconffiles list. */
	fnnf_new_conff			= DPKG_BIT(0),
	/** In the new filesystem archive. */
	fnnf_new_inarchive		= DPKG_BIT(1),
	/** In the old package's conffiles list. */
	fnnf_old_conff			= DPKG_BIT(2),
	/** Obsolete conffile. */
	fnnf_obs_conff			= DPKG_BIT(3),
	/** Must remove from other packages' lists. */
	fnnf_elide_other_lists		= DPKG_BIT(4),
	/** >= 1 instance is a dir, cannot rename over. */
	fnnf_no_atomic_overwrite	= DPKG_BIT(5),
	/** New file has been placed on the disk. */
	fnnf_placed_on_disk		= DPKG_BIT(6),
	fnnf_deferred_fsync		= DPKG_BIT(7),
	fnnf_deferred_rename		= DPKG_BIT(8),
	/** Path being filtered. */
	fnnf_filtered			= DPKG_BIT(9),
};

/**
 * Stores information to uniquely identify an on-disk file.
 */
struct file_ondisk_id {
	dev_t id_dev;
	ino_t id_ino;
};

struct filenamenode {
	struct filenamenode *next;
	const char *name;

	struct pkg_list *packages;
	struct diversion *divert;

	/** We allow the administrator to override the owner, group and mode
	 * of a file. If such an override is present we use that instead of
	 * the stat information stored in the archive.
	 *
	 * This functionality used to be in the suidmanager package. */
	struct file_stat *statoverride;

	struct trigfileint *trig_interested;

	/*
	 * Fields from here on are cleared by filesdbinit().
	 */

	/** Set to zero when a new node is created. */
	enum filenamenode_flags flags;

	/** Valid iff this namenode is in the newconffiles list. */
	const char *oldhash;
	/** Valid iff the file was unpacked and hashed on this run. */
	const char *newhash;

	struct file_ondisk_id *file_ondisk_id;
};

/**
 * When we deal with an ‘overridden’ file, every package except the
 * overriding one is considered to contain the other file instead. Both
 * files have entries in the filesdb database, and they refer to each other
 * via these diversion structures.
 *
 * The contested filename's filenamenode has an diversion entry with
 * useinstead set to point to the redirected filename's filenamenode; the
 * redirected filenamenode has camefrom set to the contested filenamenode.
 * Both sides' diversion entries will have pkg set to the package (if any)
 * which is allowed to use the contended filename.
 *
 * Packages that contain either version of the file will all refer to the
 * contested filenamenode in their per-file package lists (both in core and
 * on disk). References are redirected to the other filenamenode's filename
 * where appropriate.
 */
struct diversion {
	struct filenamenode *useinstead;
	struct filenamenode *camefrom;
	struct pkgset *pkgset;

	/** The ‘contested’ halves are in this list for easy cleanup. */
	struct diversion *next;
};

struct fileinlist {
	struct fileinlist *next;
	struct filenamenode *namenode;
};

/**
 * Queue of filenamenode entries.
 */
struct filenamenode_queue {
	struct fileinlist *head, **tail;
};

/**
 * Forward filesystem node iterator.
 */
struct fileiterator;

/**
 * Reverse filesystem node iterator.
 */
struct reversefilelistiter {
	struct fileinlist *todo;
};

/**
 * Forward filesystem node package owners iterator.
 */
struct filepackages_iterator;


void filesdbinit(void);
void files_db_reset(void);
int fsys_hash_entries(void);

struct filenamenode *findnamenode(const char *filename, enum fnnflags flags);

struct fileiterator *files_db_iter_new(void);
struct filenamenode *files_db_iter_next(struct fileiterator *iter);
void files_db_iter_free(struct fileiterator *iter);

void reversefilelist_init(struct reversefilelistiter *iterptr, struct fileinlist *files);
struct filenamenode *reversefilelist_next(struct reversefilelistiter *iterptr);
void reversefilelist_abort(struct reversefilelistiter *iterptr);

struct filepackages_iterator *filepackages_iter_new(struct filenamenode *fnn);
struct pkginfo *filepackages_iter_next(struct filepackages_iterator *iter);
void filepackages_iter_free(struct filepackages_iterator *iter);

const char *dpkg_fsys_set_dir(const char *dir);
const char *dpkg_fsys_get_dir(void);
char *dpkg_fsys_get_path(const char *pathpart);

#endif /* LIBDPKG_FSYS_H */
