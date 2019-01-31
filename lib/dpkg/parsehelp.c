/*
 * libdpkg - Debian packaging suite library routines
 * parsehelp.c - helpful routines for parsing and writing
 *
 * Copyright © 1995 Ian Jackson <ijackson@chiark.greenend.org.uk>
 * Copyright © 2006-2012 Guillem Jover <guillem@debian.org>
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

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <dpkg/i18n.h>
#include <dpkg/c-ctype.h>
#include <dpkg/dpkg.h>
#include <dpkg/dpkg-db.h>
#include <dpkg/string.h>
#include <dpkg/error.h>
#include <dpkg/parsedump.h>

static const char *
parse_error_msg(struct parsedb_state *ps, const char *fmt)
{
  static char msg[1024];
  char filename[256];

  str_escape_fmt(filename, ps->filename, sizeof(filename));

  if (ps->pkg && ps->pkg->set->name) {
    char pkgname[256];

    str_escape_fmt(pkgname, pkgbin_name(ps->pkg, ps->pkgbin, pnaw_nonambig),
                   sizeof(pkgname));
    sprintf(msg, _("parsing file '%.255s' near line %d package '%.255s':\n"
                   " %.255s"), filename, ps->lno, pkgname, fmt);
  } else
    sprintf(msg, _("parsing file '%.255s' near line %d:\n"
                   " %.255s"), filename, ps->lno, fmt);

  return msg;
}

void
parse_error(struct parsedb_state *ps, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  ohshitv(parse_error_msg(ps, fmt), args);
}

void
parse_warn(struct parsedb_state *ps, const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  warningv(parse_error_msg(ps, fmt), args);
  va_end(args);
}

const struct fieldinfo *
find_field_info(const struct fieldinfo *fields, const char *fieldname)
{
  const struct fieldinfo *field;

  for (field = fields; field->name; field++)
    if (strcasecmp(field->name, fieldname) == 0)
      return field;

  return NULL;
}

const struct arbitraryfield *
find_arbfield_info(const struct arbitraryfield *arbs, const char *fieldname)
{
  const struct arbitraryfield *arbfield;

  for (arbfield = arbs; arbfield; arbfield = arbfield->next)
    if (strcasecmp(arbfield->name, fieldname) == 0)
      return arbfield;

  return NULL;
}

const char *
pkg_name_is_illegal(const char *p)
{
  /* FIXME: _ is deprecated, remove sometime. */
  static const char alsoallowed[] = "-+._";
  static char buf[150];
  int c;

  if (!*p) return _("may not be empty string");
  if (!c_isalnum(*p))
    return _("must start with an alphanumeric character");
  while ((c = *p++) != '\0')
    if (!c_isalnum(c) && !strchr(alsoallowed, c))
      break;
  if (!c) return NULL;

  snprintf(buf, sizeof(buf), _(
	   "character '%c' not allowed (only letters, digits and characters '%s')"),
	   c, alsoallowed);
  return buf;
}

void varbufversion
(struct varbuf *vb,
 const struct dpkg_version *version,
 enum versiondisplayepochwhen vdew)
{
  switch (vdew) {
  case vdew_never:
    break;
  case vdew_nonambig:
    if (!version->epoch &&
        (!version->version || !strchr(version->version,':')) &&
        (!version->revision || !strchr(version->revision,':'))) break;
    /* Fall through. */
  case vdew_always:
    varbuf_printf(vb, "%u:", version->epoch);
    break;
  default:
    internerr("unknown versiondisplayepochwhen '%d'", vdew);
  }
  if (version->version)
    varbuf_add_str(vb, version->version);
  if (str_is_set(version->revision)) {
    varbuf_add_char(vb, '-');
    varbuf_add_str(vb, version->revision);
  }
}

const char *versiondescribe
(const struct dpkg_version *version,
 enum versiondisplayepochwhen vdew)
{
  static struct varbuf bufs[10];
  static int bufnum=0;

  struct varbuf *vb;

  if (!dpkg_version_is_informative(version))
    return C_("version", "<none>");

  vb= &bufs[bufnum]; bufnum++; if (bufnum == 10) bufnum= 0;
  varbuf_reset(vb);
  varbufversion(vb,version,vdew);
  varbuf_end_str(vb);

  return vb->buf;
}

/**
 * Parse a version string and check for invalid syntax.
 *
 * Distinguish between lax (warnings) and strict (error) parsing.
 *
 * @param rversion The parsed version.
 * @param string The version string to parse.
 * @param err The warning or error message if any.
 *
 * @retval  0 On success.
 * @retval -1 On failure, and err is set accordingly.
 */
int
parseversion(struct dpkg_version *rversion, const char *string,
             struct dpkg_error *err)
{
  char *hyphen, *colon, *eepochcolon;
  const char *end, *ptr;

  /* Trim leading and trailing space. */
  while (*string && c_isblank(*string))
    string++;

  if (!*string)
    return dpkg_put_error(err, _("version string is empty"));

  /* String now points to the first non-whitespace char. */
  end = string;
  /* Find either the end of the string, or a whitespace char. */
  while (*end && !c_isblank(*end))
    end++;
  /* Check for extra chars after trailing space. */
  ptr = end;
  while (*ptr && c_isblank(*ptr))
    ptr++;
  if (*ptr)
    return dpkg_put_error(err, _("version string has embedded spaces"));

  colon= strchr(string,':');
  if (colon) {
    long epoch;

    errno = 0;
    epoch = strtol(string, &eepochcolon, 10);
    if (string == eepochcolon)
      return dpkg_put_error(err, _("epoch in version is empty"));
    if (colon != eepochcolon)
      return dpkg_put_error(err, _("epoch in version is not number"));
    if (epoch < 0)
      return dpkg_put_error(err, _("epoch in version is negative"));
    if (epoch > INT_MAX || errno == ERANGE)
      return dpkg_put_error(err, _("epoch in version is too big"));
    if (!*++colon)
      return dpkg_put_error(err, _("nothing after colon in version number"));
    string= colon;
    rversion->epoch= epoch;
  } else {
    rversion->epoch= 0;
  }
  rversion->version= nfstrnsave(string,end-string);
  hyphen= strrchr(rversion->version,'-');
  if (hyphen) {
    *hyphen++ = '\0';

    if (*hyphen == '\0')
      return dpkg_put_error(err, _("revision number is empty"));
  }
  rversion->revision= hyphen ? hyphen : "";

  /* XXX: Would be faster to use something like cisversion and cisrevision. */
  ptr = rversion->version;
  if (!*ptr)
    return dpkg_put_error(err, _("version number is empty"));
  if (*ptr && !c_isdigit(*ptr++))
    return dpkg_put_warn(err, _("version number does not start with digit"));
  for (; *ptr; ptr++) {
    if (!c_isdigit(*ptr) && !c_isalpha(*ptr) && strchr(".-+~:", *ptr) == NULL)
      return dpkg_put_warn(err, _("invalid character in version number"));
  }
  for (ptr = rversion->revision; *ptr; ptr++) {
    if (!c_isdigit(*ptr) && !c_isalpha(*ptr) && strchr(".+~", *ptr) == NULL)
      return dpkg_put_warn(err, _("invalid character in revision number"));
  }

  return 0;
}

/**
 * Parse a version string coming from a database file.
 *
 * It parses a version string, and prints a warning or an error depending
 * on the parse options.
 *
 * @param ps The parsedb state.
 * @param version The version to parse into.
 * @param value The version string to parse from.
 * @param fmt The error format string.
 */
void
parse_db_version(struct parsedb_state *ps, struct dpkg_version *version,
                 const char *value, const char *fmt, ...)
{
  struct dpkg_error err;
  va_list args;
  char buf[1000];

  if (parseversion(version, value, &err) == 0)
    return;

  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);

  if (err.type == DPKG_MSG_WARN && (ps->flags & pdb_lax_version_parser))
    parse_warn(ps, "%s: %.250s", buf, err.str);
  else
    parse_error(ps, "%s: %.250s", buf, err.str);

  dpkg_error_destroy(&err);
}

void
parse_must_have_field(struct parsedb_state *ps,
                      const char *value, const char *what)
{
  if (!value)
    parse_error(ps, _("missing %s"), what);
  else if (!*value)
    parse_error(ps, _("empty value for %s"), what);
}

void
parse_ensure_have_field(struct parsedb_state *ps,
                        const char **value, const char *what)
{
  static const char empty[] = "";

  if (!*value) {
    parse_warn(ps, _("missing %s"), what);
    *value = empty;
  } else if (!**value) {
    parse_warn(ps, _("empty value for %s"), what);
  }
}
