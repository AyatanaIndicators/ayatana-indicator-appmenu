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
#include <glib.h>
#include <gio/gio.h>
#include <libbamf/bamf-matcher.h>

#include "window-menus.h"
#include "indicator-appmenu-marshal.h"

/* Private parts */

typedef struct _WindowMenusPrivate WindowMenusPrivate;
struct _WindowMenusPrivate {
	guint windowid;
	DbusmenuGtkClient * client;
	DbusmenuMenuitem * root;
	GCancellable * props_cancel;
	GDBusProxy * props;
	GArray * entries;
	gboolean error_state;
	guint   retry_timer;
	gint    retry_id;
	gchar * retry_name;
	GVariant * retry_data;
	guint   retry_timestamp;
	BamfApplication *app;
	gulong window_removed_id;
};

typedef struct _WMEntry WMEntry;
struct _WMEntry {
	IndicatorObjectEntry ioentry;
	gboolean disabled;
	gboolean hidden;
	DbusmenuMenuitem * mi;
};

#define WINDOW_MENUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENUS_TYPE, WindowMenusPrivate))

/* Signals */

enum {
	ENTRY_ADDED,
	ENTRY_REMOVED,
	DESTROY,
	ERROR_STATE,
	SHOW_MENU,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Prototypes */

static void window_menus_class_init (WindowMenusClass *klass);
static void window_menus_init       (WindowMenus *self);
static void window_menus_dispose    (GObject *object);
static void window_menus_finalize   (GObject *object);
static void window_removed          (GObject * gobject, BamfView * view, gpointer user_data);
static void root_changed            (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data);
static void menu_entry_added        (DbusmenuMenuitem * root, DbusmenuMenuitem * newentry, guint position, gpointer user_data);
static void menu_entry_removed      (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data);
static void menu_entry_realized     (DbusmenuMenuitem * newentry, gpointer user_data);
static void menu_child_realized     (DbusmenuMenuitem * child, gpointer user_data);
static void props_cb (GObject * object, GAsyncResult * res, gpointer user_data);

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
	                                      g_cclosure_marshal_VOID__POINTER,
	                                      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ENTRY_REMOVED] =  g_signal_new(WINDOW_MENUS_SIGNAL_ENTRY_REMOVED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, entry_removed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__POINTER,
	                                      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[DESTROY] =       g_signal_new(WINDOW_MENUS_SIGNAL_DESTROY,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, destroy),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__VOID,
	                                      G_TYPE_NONE, 0, G_TYPE_NONE);
	signals[ERROR_STATE] =   g_signal_new(WINDOW_MENUS_SIGNAL_ERROR_STATE,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, error_state),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__BOOLEAN,
	                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN, G_TYPE_NONE);
	signals[SHOW_MENU] =     g_signal_new(WINDOW_MENUS_SIGNAL_SHOW_MENU,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenusClass, show_menu),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__POINTER_UINT,
	                                      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_NONE);

	return;
}

/* Initialize the per-instance data */
static void
window_menus_init (WindowMenus *self)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(self);

	priv->client = NULL;
	priv->props_cancel = NULL;
	priv->props = NULL;
	priv->root = NULL;
	priv->error_state = FALSE;

	priv->entries = g_array_new(FALSE, FALSE, sizeof(WMEntry *));

	return;
}

/* Destroy objects */
static void
window_menus_dispose (GObject *object)
{
	g_signal_emit(object, signals[DESTROY], 0, TRUE);

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	if (priv->app != NULL) {
		g_signal_handler_disconnect(priv->app, priv->window_removed_id);
		g_object_unref(G_OBJECT(priv->app));
		priv->app = NULL;
	}

	if (priv->root != NULL) {
		g_object_unref(G_OBJECT(priv->root));
		priv->root = NULL;
	}

	if (priv->client != NULL) {
		g_object_unref(G_OBJECT(priv->client));
		priv->client = NULL;
	}
	
	if (priv->props != NULL) {
		g_object_unref(G_OBJECT(priv->props));
		priv->props = NULL;
	}

	if (priv->props_cancel != NULL) {
		g_cancellable_cancel(priv->props_cancel);
		g_object_unref(priv->props_cancel);
		priv->props_cancel = NULL;
	}

	if (priv->retry_timer != 0) {
		g_source_remove(priv->retry_timer);
		priv->retry_timer = 0;
		g_variant_unref(priv->retry_data);
		priv->retry_data = NULL;
		g_free(priv->retry_name);
		priv->retry_name = NULL;
	}

	G_OBJECT_CLASS (window_menus_parent_class)->dispose (object);
	return;
}

static void
entry_free(IndicatorObjectEntry * entry)
{
	g_return_if_fail(entry != NULL);

	if (entry->label != NULL) {
		g_object_unref(entry->label);
		entry->label = NULL;
	}
	if (entry->image != NULL) {
		g_object_unref(entry->image);
		entry->image = NULL;
	}
	if (entry->menu != NULL) {
		g_signal_handlers_disconnect_by_func(entry->menu, G_CALLBACK(gtk_widget_destroyed), &entry->menu);
		g_object_unref(entry->menu);
		entry->menu = NULL;
	}

	g_free(entry);
}

/* Free memory */
static void
window_menus_finalize (GObject *object)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	g_debug("Window Menus Object finalizing for: %d", priv->windowid);

	if (priv->entries != NULL) {
		int i;
		for (i = 0; i < priv->entries->len; i++) {
			IndicatorObjectEntry * entry;
			entry = g_array_index(priv->entries, IndicatorObjectEntry *, i);
			entry_free(entry);
		}
		g_array_free(priv->entries, TRUE);
		priv->entries = NULL;
	}

	G_OBJECT_CLASS (window_menus_parent_class)->finalize (object);
	return;
}

/* Retry the event sending to the server to see if we can get things
   working again. */
static gboolean
retry_event (gpointer user_data)
{
	g_debug("Retrying event");
	g_return_val_if_fail(IS_WINDOW_MENUS(user_data), FALSE);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	dbusmenu_client_send_event(DBUSMENU_CLIENT(priv->client), priv->retry_id, priv->retry_name, priv->retry_data, priv->retry_timestamp);

	priv->retry_timer = 0;
	g_variant_unref(priv->retry_data);
	priv->retry_data = NULL;
	g_free(priv->retry_name);
	priv->retry_name = NULL;

	return FALSE;
}

/* Listen to whether our events are successfully sent */
static void
event_status (DbusmenuClient * client, DbusmenuMenuitem * mi, gchar * event, GVariant * evdata, guint timestamp, GError * error, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	/* We don't care about status where there are no errors
	   when we're in a happy state, just let them go. */
	if (error == NULL && priv->error_state == FALSE) {
		return;
	}
	int i;

	/* Oh, things are working now! */
	if (error == NULL) {
		g_debug("Error state repaired");
		priv->error_state = FALSE;
		g_signal_emit(G_OBJECT(user_data), signals[ERROR_STATE], 0, priv->error_state, TRUE);

		for (i = 0; i < priv->entries->len; i++) {
			IndicatorObjectEntry * entry = g_array_index(priv->entries, IndicatorObjectEntry *, i);
			window_menus_entry_restore(WINDOW_MENUS(user_data), entry);
		}

		if (priv->retry_timer != 0) {
			g_source_remove(priv->retry_timer);
			priv->retry_timer = 0;
			g_variant_unref(priv->retry_data);
			priv->retry_data = NULL;
			g_free(priv->retry_name);
			priv->retry_name = NULL;
		}

		return;
	}

	/* Uhg, means that events are breaking, now we need to
	   try and handle that case. */
	priv->error_state = TRUE;
	g_signal_emit(G_OBJECT(user_data), signals[ERROR_STATE], 0, priv->error_state, TRUE);

	for (i = 0; i < priv->entries->len; i++) {
		IndicatorObjectEntry * entry = g_array_index(priv->entries, IndicatorObjectEntry *, i);

		if (entry->label != NULL) {
			gtk_widget_set_sensitive(GTK_WIDGET(entry->label), FALSE);
		}
		if (entry->image != NULL) {
			gtk_widget_set_sensitive(GTK_WIDGET(entry->image), FALSE);
		}
	}

	if (priv->retry_timer == 0) {
		g_debug("Setting up retry timer");
		priv->retry_timer = g_timeout_add_seconds(1, retry_event, user_data);

		priv->retry_id = dbusmenu_menuitem_get_id(mi);
		priv->retry_name = g_strdup(event);
		priv->retry_data = g_variant_ref(evdata);
		priv->retry_timestamp = timestamp;
	}

	return;
}

/* Called when a menu item wants to be displayed.  We need to see if
   it's one of our root items and pass it up if so. */
static void
item_activate (DbusmenuClient * client, DbusmenuMenuitem * item, guint timestamp, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	if (priv->root == NULL) {
		return;
	}

	GList * children = dbusmenu_menuitem_get_children(priv->root);
	guint position = 0;
	GList * child;

	for (child = children; child != NULL; position++, child = g_list_next(child)) {
		DbusmenuMenuitem * childmi = DBUSMENU_MENUITEM(child->data);

		/* We're only putting items with children on the panel, so
		   they're the only one with entires. */
		if (dbusmenu_menuitem_get_children(childmi) == NULL) {
			position--;
			continue;
		}

		if (childmi == item) {
			break;
		}
	}

	/* Not found */
	if (child == NULL) {
		return;
	}

	g_return_if_fail(position < priv->entries->len);

	IndicatorObjectEntry * entry = g_array_index(priv->entries, IndicatorObjectEntry *, position);
	g_signal_emit(G_OBJECT(user_data), signals[SHOW_MENU], 0, entry, timestamp, TRUE);

	return;
}

/* Build a new window menus object and attach to the signals to build
   up the representative menu. */
WindowMenus *
window_menus_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object)
{
	g_debug("Creating new windows menu: %X, %s, %s", windowid, dbus_addr, dbus_object);

	g_return_val_if_fail(windowid != 0, NULL);
	g_return_val_if_fail(dbus_addr != NULL, NULL);
	g_return_val_if_fail(dbus_object != NULL, NULL);

	WindowMenus * newmenu = WINDOW_MENUS(g_object_new(WINDOW_MENUS_TYPE, NULL));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(newmenu);

	priv->windowid = windowid;

	/* Build the service proxy */
	priv->props_cancel = g_cancellable_new();
	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
			         G_DBUS_PROXY_FLAGS_NONE,
			         NULL,
	                         dbus_addr,
	                         dbus_object,
	                         "org.freedesktop.DBus.Properties",
			         priv->props_cancel,
			         props_cb,
		                 newmenu);

	priv->client = dbusmenu_gtkclient_new((gchar *)dbus_addr, (gchar *)dbus_object);
	GtkAccelGroup * agroup = gtk_accel_group_new();
	dbusmenu_gtkclient_set_accel_group(priv->client, agroup);
	g_object_unref(agroup);

	g_signal_connect(G_OBJECT(priv->client), DBUSMENU_GTKCLIENT_SIGNAL_ROOT_CHANGED, G_CALLBACK(root_changed),   newmenu);
	g_signal_connect(G_OBJECT(priv->client), DBUSMENU_CLIENT_SIGNAL_EVENT_RESULT, G_CALLBACK(event_status), newmenu);
	g_signal_connect(G_OBJECT(priv->client), DBUSMENU_CLIENT_SIGNAL_ITEM_ACTIVATE, G_CALLBACK(item_activate), newmenu);

	DbusmenuMenuitem * root = dbusmenu_client_get_root(DBUSMENU_CLIENT(priv->client));
	if (root != NULL) {
		root_changed(DBUSMENU_CLIENT(priv->client), root, newmenu);
	}

	priv->app = bamf_matcher_get_application_for_xid(bamf_matcher_get_default(), windowid);
	if (priv->app) {
		g_object_ref(priv->app);
		priv->window_removed_id = g_signal_connect(G_OBJECT(priv->app), "window-removed", G_CALLBACK(window_removed), newmenu);
	}

	return newmenu;
}

/* Callback from trying to create the proxy for the service, this
   could include starting the service. */
static void
props_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;

	WindowMenus * self = WINDOW_MENUS(user_data);
	g_return_if_fail(self != NULL);

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(self);
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (priv->props_cancel != NULL) {
		g_object_unref(priv->props_cancel);
		priv->props_cancel = NULL;
	}

	if (error != NULL) {
		g_error("Could not grab DBus proxy for window %u: %s", priv->windowid, error->message);
		g_error_free(error);
		return;
	}

	/* Okay, we're good to grab the proxy at this point, we're
	sure that it's ours. */
	priv->props = proxy;

	return;
}

static void
window_removed (GObject * gobject, BamfView * view, gpointer user_data)
{
	WindowMenus * wm = WINDOW_MENUS(user_data);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);

	if (!BAMF_IS_WINDOW(view)) {
		return;
	}

	BamfWindow * window = BAMF_WINDOW(view);

	if (bamf_window_get_xid(window) == priv->windowid) {
		g_debug("Window removed for window: %d", priv->windowid);
		g_object_unref(G_OBJECT(wm));
	}
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
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	/* Remove the old entries */
	while (priv->entries->len != 0) {
		menu_entry_removed(NULL, NULL, user_data);
	}

	if (priv->root != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->root), G_CALLBACK(menu_entry_added), user_data);
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->root), G_CALLBACK(menu_entry_removed), user_data);
		g_object_unref(priv->root);
	}

	priv->root = new_root;

	/* See if we've got new entries */
	if (new_root == NULL) {
		return;
	}

	g_object_ref(priv->root);

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

/* Respond to properties changing on the menu item so that we can
   properly hide and show them. */
static void
menu_prop_changed (DbusmenuMenuitem * item, const gchar * property, GVariant * value, gpointer user_data)
{
	IndicatorObjectEntry * entry = (IndicatorObjectEntry *)user_data;
	WMEntry * wmentry = (WMEntry *)user_data;

	if (!g_strcmp0(property, DBUSMENU_MENUITEM_PROP_VISIBLE)) {
		if (g_variant_get_boolean(value)) {
			gtk_widget_show(GTK_WIDGET(entry->label));
			wmentry->hidden = FALSE;
		} else {
			gtk_widget_hide(GTK_WIDGET(entry->label));
			wmentry->hidden = TRUE;
		}
	} else if (!g_strcmp0(property, DBUSMENU_MENUITEM_PROP_ENABLED)) {
		gtk_widget_set_sensitive(GTK_WIDGET(entry->label), g_variant_get_boolean(value));
		wmentry->disabled = !g_variant_get_boolean(value);
	} else if (!g_strcmp0(property, DBUSMENU_MENUITEM_PROP_LABEL)) {
		gtk_label_set_text_with_mnemonic(entry->label, g_variant_get_string(value, NULL));
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
	WMEntry * wmentry = g_new0(WMEntry, 1);
	IndicatorObjectEntry * entry = &wmentry->ioentry;

	wmentry->mi = newentry;

	entry->label = GTK_LABEL(gtk_label_new_with_mnemonic(dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL)));

	if (entry->label != NULL) {
		g_object_ref(entry->label);
	}

	entry->menu = dbusmenu_gtkclient_menuitem_get_submenu(priv->client, newentry);

	if (entry->menu == NULL) {
		g_debug("Submenu for %s is NULL", dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL));
	} else {
		g_object_ref(entry->menu);
		gtk_menu_detach(entry->menu);
		g_signal_connect(entry->menu, "destroy", G_CALLBACK(gtk_widget_destroyed), &entry->menu);
	}

	g_signal_connect(G_OBJECT(newentry), DBUSMENU_MENUITEM_SIGNAL_PROPERTY_CHANGED, G_CALLBACK(menu_prop_changed), entry);

	if (dbusmenu_menuitem_property_get_variant(newentry, DBUSMENU_MENUITEM_PROP_VISIBLE) != NULL
		&& dbusmenu_menuitem_property_get_bool(newentry, DBUSMENU_MENUITEM_PROP_VISIBLE) == FALSE) {
		gtk_widget_hide(GTK_WIDGET(entry->label));
		wmentry->hidden = TRUE;
	} else {
		gtk_widget_show(GTK_WIDGET(entry->label));
		wmentry->hidden = FALSE;
	}

	if (dbusmenu_menuitem_property_get_variant (newentry, DBUSMENU_MENUITEM_PROP_ENABLED) != NULL) {
		gboolean sensitive = dbusmenu_menuitem_property_get_bool(newentry, DBUSMENU_MENUITEM_PROP_ENABLED);
		gtk_widget_set_sensitive(GTK_WIDGET(entry->label), sensitive);
		wmentry->disabled = !sensitive;
	}

	g_array_append_val(priv->entries, wmentry);

	g_signal_emit(G_OBJECT((((gpointer *)user_data)[0])), signals[ENTRY_ADDED], 0, entry, TRUE);

	g_free(user_data);

	return;
}

/* Respond to an entry getting removed from the menu */
static void
menu_entry_removed (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	if (priv->entries == NULL || priv->entries->len == 0) {
		return;
	}
	
	/* TODO: find the menuitem */
	IndicatorObjectEntry * entry = g_array_index(priv->entries, IndicatorObjectEntry *, priv->entries->len - 1);
	g_array_remove_index(priv->entries, priv->entries->len - 1);

	g_signal_emit(G_OBJECT(user_data), signals[ENTRY_REMOVED], 0, entry, TRUE);

	entry_free(entry);

	return;
}

/* Get the XID of this window */
guint
window_menus_get_xid (WindowMenus * wm)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	return priv->windowid;
}

/* Get the path for this object */
gchar *
window_menus_get_path (WindowMenus * wm)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	GValue obj = {0};
	g_value_init(&obj, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(priv->client), DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &obj);
	gchar * retval = g_value_dup_string(&obj);
	g_value_unset(&obj);
	return retval;
}

/* Get the address of this object */
gchar *
window_menus_get_address (WindowMenus * wm)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	GValue obj = {0};
	g_value_init(&obj, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(priv->client), DBUSMENU_CLIENT_PROP_DBUS_NAME, &obj);
	gchar * retval = g_value_dup_string(&obj);
	g_value_unset(&obj);
	return retval;
}

/* Return whether we're in an error state or not */
gboolean
window_menus_get_error_state (WindowMenus * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), TRUE);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	return priv->error_state;
}

/* Regain whether we're supposed to be hidden or disabled, we
   want to keep that if that's the case, otherwise bring back
   to the base state */
void
window_menus_entry_restore (WindowMenus * wm, IndicatorObjectEntry * entry)
{
	WMEntry * wmentry = (WMEntry *)entry;

	if (entry->label != NULL) {
		gtk_widget_set_sensitive(GTK_WIDGET(entry->label), !wmentry->disabled);
		if (wmentry->hidden) {
			gtk_widget_hide(GTK_WIDGET(entry->label));
		} else {
			gtk_widget_show(GTK_WIDGET(entry->label));
		}
	}

	if (entry->image != NULL) {
		gtk_widget_set_sensitive(GTK_WIDGET(entry->image), !wmentry->disabled);
		if (wmentry->hidden) {
			gtk_widget_hide(GTK_WIDGET(entry->image));
		} else {
			gtk_widget_show(GTK_WIDGET(entry->image));
		}
	}

	return;
}

/* Signaled when the menu item is activated on the panel so we
   can pass it down the stack. */
void
window_menus_entry_activate (WindowMenus * wm, IndicatorObjectEntry * entry, guint timestamp)
{
	WMEntry * wme = (WMEntry *)entry;
	dbusmenu_menuitem_send_about_to_show(wme->mi, NULL, NULL);
	return;
}
