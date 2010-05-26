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
static void menu_entry_added        (GtkContainer * container, GtkWidget * widget, gpointer user_data);
static void menu_entry_removed      (GtkContainer * container, GtkWidget * widget, gpointer user_data);

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

static void
root_changed (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data)
{
	menu_entry_added(NULL, NULL, NULL);
	menu_entry_removed(NULL, NULL, NULL);

	return;
}

/* Respond to an entry getting added to the menu */
static void
menu_entry_added (GtkContainer * container, GtkWidget * widget, gpointer user_data)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	if (!GTK_IS_MENU_ITEM(widget)) {
		g_warning("Got an item added to the menu which isn't a menu item: %s", G_OBJECT_TYPE_NAME(widget));
		return;
	}

	IndicatorObjectEntry * entry = g_new0(IndicatorObjectEntry, 1);

	/* TODO: Should be a better way for this */
	entry->label = GTK_LABEL(gtk_label_new_with_mnemonic(gtk_menu_item_get_label(GTK_MENU_ITEM(widget))));
	gtk_widget_show(GTK_WIDGET(entry->label));
	/* TODO: Check for image item */
	entry->image = NULL;
	entry->menu = GTK_MENU(gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget)));

	if (entry->menu != NULL) {
		g_object_ref(G_OBJECT(entry->menu));
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), NULL);
	} else {
		g_warning("No menu!");
	}

	g_array_append_val(priv->entries, entry);
	return;
}

/* Respond to an entry getting removed from the menu */
static void
menu_entry_removed (GtkContainer * container, GtkWidget * widget, gpointer user_data)
{
	/* TODO */

	return;
}
