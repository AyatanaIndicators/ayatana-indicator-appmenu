/*
An implementation of indicator object showing menus from applications.

Copyright 2010 Canonical Ltd.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libdbusmenu-gtk/menu.h>

#include "window-menus.h"

/* Private parts */

typedef struct _WindowMenusPrivate WindowMenusPrivate;
struct _WindowMenusPrivate {
	guint windowid;
	DbusmenuGtkClient * client;
	GArray * entries;
};

#define WINDOW_MENUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENUS_TYPE, WindowMenusPrivate))

/* Signals */

enum {
	ENTRY_ADDED,
	ENTRY_REMOVED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Prototypes */

static void window_menus_class_init (WindowMenusClass *klass);
static void window_menus_init       (WindowMenus *self);
static void window_menus_dispose    (GObject *object);
static void window_menus_finalize   (GObject *object);
static void root_changed            (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data);
static void menu_entry_added        (DbusmenuMenuitem * root, DbusmenuMenuitem * newentry, guint position, gpointer user_data);
static void menu_entry_removed      (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data);
static void menu_entry_realized     (DbusmenuMenuitem * newentry, gpointer user_data);
static void menu_child_realized     (DbusmenuMenuitem * child, gpointer user_data);

G_DEFINE_TYPE (WindowMenus, window_menus, G_TYPE_OBJECT);

/* Build the one-time class */
static void
window_menus_class_init (WindowMenusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenusPrivate));

	object_class->dispose = window_menus_dispose;
	object_class->finalize = window_menus_finalize;

	/* Signals */
	signals[ENTRY_ADDED] =  g_signal_new(WINDOW_MENUS_SIGNAL_ENTRY_ADDED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, entry_added),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__OBJECT,
	                                      G_TYPE_NONE, 1, G_TYPE_OBJECT);
	signals[ENTRY_REMOVED] =  g_signal_new(WINDOW_MENUS_SIGNAL_ENTRY_REMOVED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, entry_removed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__OBJECT,
	                                      G_TYPE_NONE, 1, G_TYPE_OBJECT);

	return;
}

/* Initialize the per-instance data */
static void
window_menus_init (WindowMenus *self)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(self);

	priv->client = NULL;

	priv->entries = g_array_new(FALSE, FALSE, sizeof(IndicatorObjectEntry *));

	return;
}

/* Destroy objects */
static void
window_menus_dispose (GObject *object)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	if (priv->client != NULL) {
		g_object_unref(G_OBJECT(priv->client));
		priv->client = NULL;
	}

	G_OBJECT_CLASS (window_menus_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
window_menus_finalize (GObject *object)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	if (priv->entries != NULL) {
		g_array_free(priv->entries, TRUE);
		priv->entries = NULL;
	}

	G_OBJECT_CLASS (window_menus_parent_class)->finalize (object);
	return;
}

/* Build a new window menus object and attach to the signals to build
   up the representative menu. */
WindowMenus *
window_menus_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object)
{
	g_debug("Creating new windows menu: %X, %s, %s", windowid, dbus_addr, dbus_object);

	WindowMenus * newmenu = WINDOW_MENUS(g_object_new(WINDOW_MENUS_TYPE, NULL));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(newmenu);

	priv->client = dbusmenu_gtkclient_new((gchar *)dbus_addr, (gchar *)dbus_object);

	g_signal_connect(G_OBJECT(priv->client), DBUSMENU_GTKCLIENT_SIGNAL_ROOT_CHANGED, G_CALLBACK(root_changed),   newmenu);

	DbusmenuMenuitem * root = dbusmenu_client_get_root(DBUSMENU_CLIENT(priv->client));
	if (root != NULL) {
		root_changed(DBUSMENU_CLIENT(priv->client), root, newmenu);
	}

	return newmenu;
}

/* Get the location of this entry */
guint
window_menus_get_location (WindowMenus * wm, IndicatorObjectEntry * entry)
{
	if (entry == NULL) {
		return 0;
	}

	guint i;
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	for (i = 0; i < priv->entries->len; i++) {
		if (entry == g_array_index(priv->entries, IndicatorObjectEntry *, i)) {
			break;
		}
	}

	if (i == priv->entries->len) {
		return 0;
	}

	return i;
}

/* Get the entries that we have */
GList *
window_menus_get_entries (WindowMenus * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), NULL);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);

	int i;
	GList * output = NULL;
	for (i = 0; i < priv->entries->len; i++) {
		output = g_list_prepend(output, g_array_index(priv->entries, IndicatorObjectEntry *, i));
	}
	if (output != NULL) {
		output = g_list_reverse(output);
	}

	return output;
}

/* Goes through the items in the root node and adds them
   to the flock */
static void
new_root_helper (DbusmenuMenuitem * item, gpointer user_data)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);
	menu_entry_added(dbusmenu_client_get_root(DBUSMENU_CLIENT(priv->client)), item, priv->entries->len, user_data);
	return;
}

/* Respond to the root menu item on our client changing */
static void
root_changed (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	/* Remove the old entries */
	while (priv->entries->len != 0) {
		menu_entry_removed(NULL, NULL, NULL);
	}

	/* See if we've got new entries */
	if (new_root == NULL) {
		return;
	}

	/* Set up signals */
	g_signal_connect(G_OBJECT(new_root), DBUSMENU_MENUITEM_SIGNAL_CHILD_ADDED,   G_CALLBACK(menu_entry_added),   user_data);
	g_signal_connect(G_OBJECT(new_root), DBUSMENU_MENUITEM_SIGNAL_CHILD_REMOVED, G_CALLBACK(menu_entry_removed), user_data);
	/* TODO: Child Moved */

	/* Add the new entries */
	GList * children = dbusmenu_menuitem_get_children(new_root);
	while (children != NULL) {
		new_root_helper(DBUSMENU_MENUITEM(children->data), user_data);
		children = g_list_next(children);
	}

	return;
}

/* Respond to an entry getting added to the menu */
static void
menu_entry_added (DbusmenuMenuitem * root, DbusmenuMenuitem * newentry, guint position, gpointer user_data)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	g_signal_connect(G_OBJECT(newentry), DBUSMENU_MENUITEM_SIGNAL_REALIZED, G_CALLBACK(menu_entry_realized), user_data);

	GtkMenuItem * mi = dbusmenu_gtkclient_menuitem_get(priv->client, newentry);
	if (mi != NULL) {
		menu_entry_realized(newentry, user_data);
	}
	return;
}

/* React to the menuitem when we know that it's got all the data
   that we really need. */
static void
menu_entry_realized (DbusmenuMenuitem * newentry, gpointer user_data)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);
	GtkMenu * menu = dbusmenu_gtkclient_menuitem_get_submenu(priv->client, newentry);

	if (menu == NULL) {
		GList * children = dbusmenu_menuitem_get_children(newentry);
		if (children != NULL) {
			gpointer * data = g_new(gpointer, 2);
			data[0] = user_data;
			data[1] = newentry;

			g_signal_connect(G_OBJECT(children->data), DBUSMENU_MENUITEM_SIGNAL_REALIZED, G_CALLBACK(menu_child_realized), data);
		} else {
			g_warning("Entry has no children!");
		}
	} else {
		gpointer * data = g_new(gpointer, 2);
		data[0] = user_data;
		data[1] = newentry;

		menu_child_realized(NULL, data);
	}
	
	return;
}

/* We can't go until we have some kids.  Really, it's important. */
static void
menu_child_realized (DbusmenuMenuitem * child, gpointer user_data)
{
	DbusmenuMenuitem * newentry = (DbusmenuMenuitem *)(((gpointer *)user_data)[1]);

	/* Only care about the first */
	g_signal_handlers_disconnect_by_func(G_OBJECT(child), menu_child_realized, user_data);

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE((((gpointer *)user_data)[0]));
	IndicatorObjectEntry * entry = g_new0(IndicatorObjectEntry, 1);

	entry->label = GTK_LABEL(gtk_label_new(dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL)));
	entry->menu = dbusmenu_gtkclient_menuitem_get_submenu(priv->client, newentry);

	if (entry->menu == NULL) {
		g_debug("Submenu for %s is NULL", dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL));
	}

	gtk_widget_show(GTK_WIDGET(entry->label));

	g_array_append_val(priv->entries, entry);
	return;
}

/* Respond to an entry getting removed from the menu */
static void
menu_entry_removed (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data)
{
	//WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);
	
	/* Need to find the menuitem */

	return;
}
