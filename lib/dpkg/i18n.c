/*
 * libdpkg - Debian packaging suite library routines
 * i18n.c - i18n support
 *
 * Copyright © 2013 Guillem Jover <guillem@debian.org>
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

#include <dpkg/i18n.h>

void
dpkg_locales_init(const char *package)
{
	setlocale(LC_ALL, "");
	bindtextdomain(package, LOCALEDIR);
	textdomain(package);

#if defined(__APPLE__) && defined(__MACH__)
	/*
	 * On Mac OS X, the libintl code needs to call into the CoreFoundation
	 * framework, which is internally threaded, to initialize some caches.
	 * This is a problem when that first call is done after a fork(3),
	 * because per POSIX, only one thread will survive, leaving the
	 * process in a very inconsistent state, leading to crashes.
	 *
	 * XXX: To workaround this, we try to force the caches initialization
	 * at program startup time, by performing a dummy gettext() call.
	 */
	gettext("");
#endif
}
