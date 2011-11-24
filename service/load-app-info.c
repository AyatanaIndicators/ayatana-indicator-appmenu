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
#include <glib/gi18n.h>

static void new_element (GMarkupParseContext *context, const gchar * name, const gchar ** attribute_names, const gchar ** attribute_values, gpointer user_data, GError **error);
static void end_element (GMarkupParseContext  *context, const gchar * name, gpointer user_data, GError ** error);

static GMarkupParser app_info_parser = {
	start_element:  new_element,
	end_element:    end_element,
	text:           NULL,
	passthrough:    NULL,
	error:          NULL
};

typedef struct _menu_data_t menu_data_t;
struct _menu_data_t {
	sqlite3 * db;
	gchar * desktopfile;
	gchar * domain;
	gboolean seen_header;
	GQueue queue;
	GString * statement;
};

typedef enum _menu_errors_t menu_errors_t;
enum _menu_errors_t {
	DUPLICATE_HEADERS,
	DUPLICATE_DESKTOPFILE,
	MISSING_HEADER,
	MISSING_DESKTOP,
	ERROR_LAST
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
		seen_header: FALSE,
		desktopfile: NULL,
		domain: NULL,
		queue: G_QUEUE_INIT,
		statement: NULL
	};

	menu_data.statement = g_string_new("");

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
	g_debug("SQL: %s", menu_data.statement->str);
	g_string_free(menu_data.statement, TRUE);

	/* Free data */
	g_free(data);
	g_object_unref(file);

	return parsed;
}

static GQuark
error_domain (void)
{
	static GQuark domain = 0;
	if (domain == 0) {
		domain = g_quark_from_static_string("hud-app-info-parser");
	}
	return domain;
}

#define COLLECT(first, ...) \
  g_markup_collect_attributes (name,                                         \
                               attribute_names, attribute_values, error,     \
                               first, __VA_ARGS__, G_MARKUP_COLLECT_INVALID)
#define OPTIONAL   G_MARKUP_COLLECT_OPTIONAL
#define STRDUP     G_MARKUP_COLLECT_STRDUP
#define STRING     G_MARKUP_COLLECT_STRING
#define NO_ATTRS() COLLECT (G_MARKUP_COLLECT_INVALID, NULL)

static void
new_element (GMarkupParseContext *context, const gchar * name, const gchar ** attribute_names, const gchar ** attribute_values, gpointer user_data, GError **error)
{
	menu_data_t * menu_data = (menu_data_t *)user_data;

	if (g_strcmp0(name, "hudappinfo") == 0) {
		if (menu_data->seen_header) {
			g_set_error(error, error_domain(), DUPLICATE_HEADERS, "Recieved second header");
		}

		menu_data->seen_header = TRUE;
		return;
	}

	if (!menu_data->seen_header) {
		g_set_error(error, error_domain(), MISSING_HEADER, "Missing the header when we got to element '%s'", name);
		return;
	}

	if (g_strcmp0(name, "desktopfile") == 0) {
		const gchar * desktopfile;

		if (!COLLECT(STRING, "path", &desktopfile)) {
			return;
		}

		if (menu_data->desktopfile != NULL) {
			g_set_error(error, error_domain(), DUPLICATE_DESKTOPFILE, "Two desktop file definitions.  First as '%s' then as '%s'.", menu_data->desktopfile, desktopfile);
			return;
		}

		menu_data->desktopfile = g_strdup(desktopfile);
		return;
	}

	if (g_strcmp0(name, "menus") == 0) {
		if (menu_data->desktopfile == NULL) {
			g_set_error(error, error_domain(), MISSING_DESKTOP, "No desktop file is defined");
		}

		return;
	}

	if (g_strcmp0(name, "menu") == 0) {
		const gchar * mname;

		if (!COLLECT(STRING, "name", &mname)) {
			return;
		}

		if (g_queue_is_empty(&menu_data->queue)) {
			g_queue_push_head(&menu_data->queue, g_strdup(mname));
		} else {
			g_queue_push_head(&menu_data->queue, g_strdup_printf(_("%s > %s"), (gchar *)g_queue_peek_head(&menu_data->queue), mname));
		}

		return;
	}

	if (g_strcmp0(name, "item") == 0) {
		const gchar * iname;
		const gchar * scount;

		if (!COLLECT(STRING, "name", &iname,
		             STRING, "count", &scount)) {
			return;
		}

		gchar * finalitem = g_strdup_printf(_("%s > %s"), (gchar *)g_queue_peek_head(&menu_data->queue), iname);
		gint64 count = g_ascii_strtoll(scount, NULL, 10);

		g_debug("Adding '%s' %s times", finalitem, scount);
		int i;
		for (i = 0; i < count; i++) {
			if (i == 0) {
				g_string_append_printf(menu_data->statement, "insert into usage (application, entry, timestamp) values ('%s', '%s', date('now', 'utc'));", menu_data->desktopfile, finalitem);
			} else {
				g_string_append_printf(menu_data->statement, "insert into usage (application, entry, timestamp) values ('%s', '%s', date('now', 'utc', '-%d days'));", menu_data->desktopfile, finalitem, i);
			}
		}

		g_free(finalitem);
		return;
	}

	return;
}

static void
end_element (GMarkupParseContext  *context, const gchar * name, gpointer user_data, GError ** error)
{
	menu_data_t * menu_data = (menu_data_t *)user_data;

	if (g_strcmp0(name, "menu") == 0) {
		if (g_queue_is_empty(&menu_data->queue)) {
			g_warning("Menu stack is empty!");
		} else {
			g_free(g_queue_pop_head(&menu_data->queue));
		}

		return;
	}

	return;
}
