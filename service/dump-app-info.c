/*
Prints out application info for debugging and CLI tools

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

#include "dump-app-info.h"

static int
entry_cb (void * user_data, int columns, char ** values, char ** names)
{
	g_print("Entry: %s", values[0]);

	return SQLITE_OK;
}

void
print_tree (GList * tree, guint tab_depth)
{


	return;
}

void
dump_app_info (const gchar * app, const gchar * domain, sqlite3 * db)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(db != NULL);

	g_print("<hudappinfo>\n");

	g_print("\t<desktopfile>%s</desktopfile>\n", app);

	if (domain != NULL) {
		g_print("\t<gettext-domain>%s</gettext-domain>\n", domain);
	}

	gchar * statement = g_strdup_printf("select entry from usage where application = '%s';", app);

	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	GList * tree = NULL;
	exec_status = sqlite3_exec(db,
	                           statement,
	                           entry_cb, &tree, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to get entries: %s", failstring);
	}

	g_free(statement);

	if (tree != NULL) {
		g_print("\t<menus>\n");
		print_tree(tree, 1);
		g_print("\t</menus>\n");
	}

	g_print("</hudappinfo>\n");
	return;
}
