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

typedef enum _tree_type_t tree_type_t;
enum _tree_type_t {
	MENU_TYPE,
	ITEM_TYPE
};

typedef struct _menu_t menu_t;
struct _menu_t {
	tree_type_t tree_type;
	gchar * name;
	int count;
	GList * subitems;
};

GList *
place_on_tree (GList * tree_in, gchar ** entries)
{
	menu_t * menu;

	if (entries[0] == NULL) {
		return tree_in;
	}

	GList * entry = tree_in;
	while (entry != NULL) {
		menu = (menu_t *)entry->data;

		if (g_strcmp0(menu->name, entries[0]) == 0) {
			break;
		}

		entry = g_list_next(entry);
	}

	if (entry != NULL) {
		if (menu->tree_type == ITEM_TYPE) {
			if (entries[1] != NULL) {
				g_warning("Error parsing on entry '%s'", entries[0]);
			} else {
				menu->count++;
			}
		} else {
			menu->subitems = place_on_tree(menu->subitems, &entries[1]);
		}

		return tree_in;
	}

	menu = g_new0(menu_t, 1);
	menu->name = g_strdup(entries[0]);

	if (entries[1] == NULL) {
		/* This is an item */
		menu->tree_type = ITEM_TYPE;
		menu->count = 1;
		menu->subitems = NULL;
	} else {
		/* This is a menu */
		menu->tree_type = MENU_TYPE;
		menu->subitems = place_on_tree(NULL, &entries[1]);
	}

	return g_list_append(tree_in, menu);
}

static int
entry_cb (void * user_data, int columns, char ** values, char ** names)
{
	g_print("Entry: %s", values[0]);
	GList ** tree = (GList **)user_data;

	gchar ** entries = g_strsplit(values[0], " > ", -1); // TODO: Get from: _("%s > %s")

	*tree = place_on_tree(*tree, entries);

	g_strfreev(entries);

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
