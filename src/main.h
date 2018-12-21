/*
 * dpkg - main program for package management
 * main.h - external definitions for this program
 *
 * Copyright © 1995 Ian Jackson <ijackson@chiark.greenend.org.uk>
 * Copyright © 2006, 2008-2016 Guillem Jover <guillem@debian.org>
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

#ifndef MAIN_H
#define MAIN_H

#include <dpkg/debug.h>
#include <dpkg/pkg-list.h>

/* These two are defined in <dpkg/fsys.h>. */
struct fileinlist;
struct filenamenode;

enum pkg_istobe {
	/** Package is to be left in a normal state. */
	PKG_ISTOBE_NORMAL,
	/** Package is to be removed. */
	PKG_ISTOBE_REMOVE,
	/** Package is to be installed, configured or triggered. */
	PKG_ISTOBE_INSTALLNEW,
	/** Package is to be deconfigured. */
	PKG_ISTOBE_DECONFIGURE,
	/** Package is to be checked for Pre-Depends satisfiability. */
	PKG_ISTOBE_PREINSTALL,
};

enum pkg_cycle_color {
	PKG_CYCLE_WHITE,
	PKG_CYCLE_GRAY,
	PKG_CYCLE_BLACK,
};

struct perpackagestate {
  enum pkg_istobe istobe;

  /** Used during cycle detection. */
  enum pkg_cycle_color color;

  bool enqueued;

  int replacingfilesandsaid;
  int cmdline_seen;

  /** Non-NULL iff in trigproc.c:deferred. */
  struct pkg_list *trigprocdeferred;
};

enum action {
	act_unset,

	act_unpack,
	act_configure,
	act_install,
	act_triggers,
	act_remove,
	act_purge,
	act_verify,
	act_commandfd,

	act_status,
	act_listpackages,
	act_listfiles,
	act_searchfiles,
	act_controlpath,
	act_controllist,
	act_controlshow,

	act_cmpversions,

	act_arch_add,
	act_arch_remove,
	act_printarch,
	act_printforeignarches,

	act_assertpredep,
	act_assertepoch,
	act_assertlongfilenames,
	act_assertmulticonrep,
	act_assertmultiarch,
	act_assertverprovides,

	act_validate_pkgname,
	act_validate_trigname,
	act_validate_archname,
	act_validate_version,

	act_audit,
	act_unpackchk,
	act_predeppackage,

	act_getselections,
	act_setselections,
	act_clearselections,

	act_avail,
	act_printavail,
	act_avclear,
	act_avreplace,
	act_avmerge,
	act_forgetold,
};

extern const char *const statusstrings[];

extern int f_pending, f_recursive, f_alsoselect, f_skipsame, f_noact;
extern int f_autodeconf, f_nodebsig;
extern int f_triggers;
extern int fc_downgrade, fc_configureany, fc_hold, fc_removereinstreq, fc_overwrite;
extern int fc_removeessential, fc_conflicts, fc_depends, fc_dependsversion;
extern int fc_breaks, fc_badpath, fc_overwritediverted, fc_architecture;
extern int fc_nonroot, fc_overwritedir, fc_conff_new, fc_conff_miss;
extern int fc_conff_old, fc_conff_def;
extern int fc_conff_ask;
extern int fc_badverify;
extern int fc_badversion;
extern int fc_unsafe_io;
extern int fc_script_chrootless;

extern bool abort_processing;
extern int errabort;
extern const char *instdir;
extern struct pkg_list *ignoredependss;

struct invoke_hook {
	struct invoke_hook *next;
	char *command;
};

struct invoke_list {
	struct invoke_hook *head, **tail;
};

/* from perpkgstate.c */

void ensure_package_clientdata(struct pkginfo *pkg);

/* from archives.c */

int archivefiles(const char *const *argv);
void process_archive(const char *filename);
bool wanttoinstall(struct pkginfo *pkg);

/* from update.c */

int forgetold(const char *const *argv);
int updateavailable(const char *const *argv);

/* from enquiry.c */

int audit(const char *const *argv);
int unpackchk(const char *const *argv);
int assertepoch(const char *const *argv);
int assertpredep(const char *const *argv);
int assertlongfilenames(const char *const *argv);
int assertmulticonrep(const char *const *argv);
int assertmultiarch(const char *const *argv);
int assertverprovides(const char *const *argv);
int validate_pkgname(const char *const *argv);
int validate_trigname(const char *const *argv);
int validate_archname(const char *const *argv);
int validate_version(const char *const *argv);
int predeppackage(const char *const *argv);
int printarch(const char *const *argv);
int printinstarch(const char *const *argv);
int print_foreign_arches(const char *const *argv);
int cmpversions(const char *const *argv);

/* from verify.c */

bool verify_set_output(const char *name);
int verify(const char *const *argv);

/* from select.c */

int getselections(const char *const *argv);
int setselections(const char *const *argv);
int clearselections(const char *const *argv);

/* from packages.c, remove.c and configure.c */

void md5hash(struct pkginfo *pkg, char *hashbuf, const char *fn);
void enqueue_package(struct pkginfo *pkg);
void enqueue_package_mark_seen(struct pkginfo *pkg);
void process_queue(void);
int packages(const char *const *argv);
void removal_bulk(struct pkginfo *pkg);
int conffderef(struct pkginfo *pkg, struct varbuf *result, const char *in);

enum dep_check {
  DEP_CHECK_HALT = 0,
  DEP_CHECK_DEFER = 1,
  DEP_CHECK_OK = 2,
};

enum dep_check dependencies_ok(struct pkginfo *pkg, struct pkginfo *removing,
                               struct varbuf *aemsgs);
enum dep_check breakses_ok(struct pkginfo *pkg, struct varbuf *aemsgs);

void deferred_remove(struct pkginfo *pkg);
void deferred_configure(struct pkginfo *pkg);

extern int sincenothing, dependtry;

/* from cleanup.c (most of these are declared in archives.h) */

void cu_prermremove(int argc, void **argv);

/* from errors.c */

void print_error_perpackage(const char *emsg, const void *data);
void print_error_perarchive(const char *emsg, const void *data);
void forcibleerr(int forceflag, const char *format, ...) DPKG_ATTR_PRINTF(2);
int forcible_nonroot_error(int rc);
int reportbroken_retexitstatus(int ret);
bool skip_due_to_hold(struct pkginfo *pkg);

/* from help.c */

struct stat;

bool ignore_depends(struct pkginfo *pkg);
bool force_breaks(struct deppossi *possi);
bool force_depends(struct deppossi *possi);
bool force_conflicts(struct deppossi *possi);
void conffile_mark_obsolete(struct pkginfo *pkg, struct filenamenode *namenode);
void pkg_conffiles_mark_old(struct pkginfo *pkg);
bool find_command(const char *prog);
void checkpath(void);

struct filenamenode *namenodetouse(struct filenamenode *namenode,
                                   struct pkginfo *pkg, struct pkgbin *pkgbin);

int maintscript_installed(struct pkginfo *pkg, const char *scriptname,
                          const char *desc, ...) DPKG_ATTR_SENTINEL;
int maintscript_new(struct pkginfo *pkg,
                    const char *scriptname, const char *desc,
                    const char *cidir, char *cidirrest, ...)
	DPKG_ATTR_SENTINEL;
int maintscript_fallback(struct pkginfo *pkg,
                         const char *scriptname, const char *desc,
                         const char *cidir, char *cidirrest,
                         const char *ifok, const char *iffallback);

/* Callers wanting to run the postinst use these two as they want to postpone
 * trigger incorporation until after updating the package status. The effect
 * is that a package can trigger itself. */
int maintscript_postinst(struct pkginfo *pkg, ...) DPKG_ATTR_SENTINEL;
void post_postinst_tasks(struct pkginfo *pkg, enum pkgstatus new_status);

void clear_istobes(void);
bool dir_is_used_by_others(struct filenamenode *namenode, struct pkginfo *pkg);
bool dir_is_used_by_pkg(struct filenamenode *namenode, struct pkginfo *pkg,
                        struct fileinlist *list);
bool dir_has_conffiles(struct filenamenode *namenode, struct pkginfo *pkg);

void log_action(const char *action, struct pkginfo *pkg, struct pkgbin *pkgbin);

/* From selinux.c */

void dpkg_selabel_load(void);
void dpkg_selabel_set_context(const char *matchpath, const char *path, mode_t mode);
void dpkg_selabel_close(void);

/* from trigproc.c */

enum trigproc_type {
	/** Opportunistic trigger processing. */
	TRIGPROC_TRY,
	/** Required trigger processing. */
	TRIGPROC_REQUIRED,
};

void trigproc_install_hooks(void);
void trigproc_populate_deferred(void);
void trigproc_run_deferred(void);
void trigproc_reset_cycle(void);

void trigproc(struct pkginfo *pkg, enum trigproc_type type);

void trig_activate_packageprocessing(struct pkginfo *pkg);

/* from depcon.c */

enum which_pkgbin {
  wpb_installed,
  wpb_available,
  wpb_by_istobe,
};

struct deppossi_pkg_iterator;

struct deppossi_pkg_iterator *
deppossi_pkg_iter_new(struct deppossi *possi, enum which_pkgbin wpb);
struct pkginfo *
deppossi_pkg_iter_next(struct deppossi_pkg_iterator *iter);
void
deppossi_pkg_iter_free(struct deppossi_pkg_iterator *iter);

bool depisok(struct dependency *dep, struct varbuf *whynot,
             struct pkginfo **fixbyrm, struct pkginfo **fixbytrigaw,
             bool allowunconfigd);
struct cyclesofarlink;
bool findbreakcycle(struct pkginfo *pkg);
void describedepcon(struct varbuf *addto, struct dependency *dep);

#endif /* MAIN_H */
