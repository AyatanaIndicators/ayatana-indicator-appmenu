/*
Small utility to dump application info in the HUD usage DB

Copyright 2011 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <glib.h>
#include "dump-app-info.h"

int
main (int argv, char * argc[])
{
	if (argv != 2 && argv != 3) {
		g_error("Usage: %s <desktop file path> [gettext domain]\n", argc[0]);
		return 1;
	}

	const gchar * basecachedir = g_getenv("HUD_CACHE_DIR");
	if (basecachedir == NULL) {
		basecachedir = g_get_user_cache_dir();
	}

	gchar * cachedir = g_build_filename(basecachedir, "indicator-appmenu", NULL);
	if (!g_file_test(cachedir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
		g_warning("Cache directory '%s' doesn't exist", cachedir);
		return 1;
	}
	g_free(cachedir);

	gchar * cachefile = g_build_filename(basecachedir, "indicator-appmenu", "hud-usage-log.sqlite", NULL);
	gboolean db_exists = g_file_test(cachefile, G_FILE_TEST_EXISTS);

	if (!db_exists) {
		g_warning("There is no HUD usage log: %s", cachefile);
		return 1;
	}

	sqlite3 * db;
	int open_status = sqlite3_open(cachefile, &db); 

	if (open_status != SQLITE_OK) {
		g_warning("Error opening usage DB: %s", cachefile);
		sqlite3_close(db);
		return 1;
	}

	gchar * domain = NULL;
	if (argv == 3) {
		domain = argc[2];
	}

	dump_app_info(argc[1], domain, db);

	sqlite3_close(db);
	g_free(cachefile);

	return 0;
}
