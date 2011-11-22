/*
Loads application info for initial app usage and verification

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

#include "load-app-info.h"
#include <gio/gio.h>

static GMarkupParser app_info_parser = {
	start_element:  NULL,
	end_element:    NULL,
	text:           NULL,
	passthrough:    NULL,
	error:          NULL
};

typedef struct _menu_data_t menu_data_t;
struct _menu_data_t {
	sqlite3 * db;
	gchar * desktopfile;
	gchar * domain;
};

gboolean
load_app_info (const gchar * filename, sqlite3 * db)
{
	/* verify */
	g_return_val_if_fail(filename != NULL, FALSE);
	g_return_val_if_fail(db != NULL, FALSE);

	if (!g_file_test(filename, G_FILE_TEST_EXISTS)) {
		return FALSE;
	}

	/* get data */
	GFile * file = g_file_new_for_path(filename);
	gchar * data = NULL;
	gsize len = 0;
	GError * error = NULL;

	gboolean load = g_file_load_contents(file,
	                                     NULL, /* cancelable */
	                                     &data,
	                                     &len,
	                                     NULL, /* end tag */
	                                     &error);

	if (error != NULL) {
		g_warning("Unable to load file '%s': %s", filename, error->message);
		g_error_free(error);
		g_object_unref(file);
		return FALSE;
	}

	if (!load) {
		g_warning("Unable to load file '%s'", filename);
		g_object_unref(file);
		return FALSE;
	}

	/* parse it */
	menu_data_t menu_data = {
		db: db,
		desktopfile: NULL,
		domain: NULL
	};

	GMarkupParseContext * context = g_markup_parse_context_new(&app_info_parser,
	                                                           0, /* flags */
	                                                           &menu_data,
	                                                           NULL /* destroy func */);

	gboolean parsed = g_markup_parse_context_parse(context, data, len, &error);

	if (error != NULL) {
		g_warning("Unable to parse file '%s': %s", filename, error->message);
		g_error_free(error);
		error = NULL;
	}

	if (!parsed) {
		g_warning("Unable to parse file '%s'", filename);
	}

	g_markup_parse_context_free(context);

	g_free(menu_data.desktopfile);
	g_free(menu_data.domain);

	/* Free data */
	g_free(data);
	g_object_unref(file);

	return parsed;
}
