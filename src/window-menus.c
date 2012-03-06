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

#include <gtk/gtk.h>

#include <libdbusmenu-gtk/menu.h>
#include <glib.h>
#include <gio/gio.h>

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
};

typedef struct _WMEntry WMEntry;
struct _WMEntry {
	IndicatorObjectEntry ioentry;
	gboolean disabled;
	gboolean hidden;
	DbusmenuMenuitem * mi;
	WindowMenus * wm;
};

#define WINDOW_MENUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENUS_TYPE, WindowMenusPrivate))

/* Prototypes */

static void window_menus_dispose    (GObject *object);
static void root_changed            (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data);
static void event_status            (DbusmenuClient * client, DbusmenuMenuitem * mi, gchar * event, GVariant * evdata, guint timestamp, GError * error, gpointer user_data);
static void item_activate           (DbusmenuClient * client, DbusmenuMenuitem * item, guint timestamp, gpointer user_data);
static void status_changed          (DbusmenuClient * client, GParamSpec * pspec, gpointer user_data);
static void menu_entry_added        (DbusmenuMenuitem * root, DbusmenuMenuitem * newentry, guint position, gpointer user_data);
static void menu_entry_removed      (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data);
static void menu_entry_realized     (DbusmenuMenuitem * newentry, gpointer user_data);
static void menu_entry_realized_child_added (DbusmenuMenuitem * parent, DbusmenuMenuitem * child, guint position, gpointer user_data);
static void menu_prop_changed       (DbusmenuMenuitem * item, const gchar * property, GVariant * value, gpointer user_data);
static void menu_child_realized     (DbusmenuMenuitem * child, gpointer user_data);
static void props_cb (GObject * object, GAsyncResult * res, gpointer user_data);
static GList *          get_entries      (WindowMenu * wm);
static guint            get_location     (WindowMenu * wm, IndicatorObjectEntry * entry);
static guint            get_xid          (WindowMenu * wm);
static gchar *          get_path         (WindowMenu * wm);
static gchar *          get_address      (WindowMenu * wm);
static gboolean         get_error_state  (WindowMenu * wm);
static WindowMenuStatus get_status       (WindowMenu * wm);
static void             entry_restore    (WindowMenu * wm, IndicatorObjectEntry * entry);
static void             entry_activate   (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp);

G_DEFINE_TYPE (WindowMenus, window_menus, WINDOW_MENU_TYPE);

/* Build the one-time class */
static void
window_menus_class_init (WindowMenusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenusPrivate));

	object_class->dispose = window_menus_dispose;

	WindowMenuClass * menu_class = WINDOW_MENU_CLASS(klass);
	menu_class->get_entries = get_entries;
	menu_class->get_location = get_location;
	menu_class->get_xid = get_xid;
	menu_class->get_path = get_path;
	menu_class->get_address = get_address;
	menu_class->get_error_state = get_error_state;
	menu_class->get_status = get_status;
	menu_class->entry_restore = entry_restore;
	menu_class->entry_activate = entry_activate;

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

static void
entry_free(IndicatorObjectEntry * entry)
{
	g_return_if_fail(entry != NULL);
	WMEntry * wmentry = (WMEntry *)entry;

	if (wmentry->mi != NULL) {
		g_signal_handlers_disconnect_by_func(wmentry->mi, G_CALLBACK(menu_prop_changed), &wmentry->ioentry);
		g_object_unref(G_OBJECT(wmentry->mi));
		wmentry->mi = NULL;
	}

	if (entry->label != NULL) {
		g_object_unref(entry->label);
		entry->label = NULL;
	}
	if (entry->accessible_desc != NULL) {
		entry->accessible_desc = NULL;
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

static void
free_entries(GObject *object, gboolean should_signal)
{
	g_return_if_fail(IS_WINDOW_MENUS(object));

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	if (priv->entries != NULL) {
		while (priv->entries->len > 0) {
			IndicatorObjectEntry * entry;
			entry = g_array_index(priv->entries, IndicatorObjectEntry *, 0);
			g_array_remove_index(priv->entries, 0);
			if (should_signal) {			
				g_signal_emit_by_name(object, WINDOW_MENU_SIGNAL_ENTRY_REMOVED, entry, TRUE);
			}
			entry_free(entry);
		}
	}
}

/* Destroy objects */
static void
window_menus_dispose (GObject *object)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(object);

	free_entries(object, FALSE);

	if (priv->entries != NULL) {
		g_array_free(priv->entries, TRUE);
		priv->entries = NULL;
	}

	if (priv->root != NULL) {
		root_changed(DBUSMENU_CLIENT(priv->client), NULL, object);
		g_warn_if_fail(priv->root == NULL);
	}

	if (priv->client != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->client), G_CALLBACK(root_changed),    object);
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->client), G_CALLBACK(event_status),    object);
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->client), G_CALLBACK(item_activate),   object);
		g_signal_handlers_disconnect_by_func(G_OBJECT(priv->client), G_CALLBACK(status_changed),  object);

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
	}

	G_OBJECT_CLASS (window_menus_parent_class)->dispose (object);
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

	dbusmenu_menuitem_handle_event(dbusmenu_client_get_root(DBUSMENU_CLIENT(priv->client)),
	                               "x-appmenu-retry-ping",
	                               NULL,
	                               0);

	priv->retry_timer = 0;

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
		g_signal_emit_by_name(G_OBJECT(user_data), WINDOW_MENU_SIGNAL_ERROR_STATE, priv->error_state, TRUE);

		for (i = 0; i < priv->entries->len; i++) {
			IndicatorObjectEntry * entry = g_array_index(priv->entries, IndicatorObjectEntry *, i);
			entry_restore(WINDOW_MENU(user_data), entry);
		}

		if (priv->retry_timer != 0) {
			g_source_remove(priv->retry_timer);
			priv->retry_timer = 0;
		}

		return;
	}

	/* Uhg, means that events are breaking, now we need to
	   try and handle that case. */
	priv->error_state = TRUE;
	g_signal_emit_by_name(G_OBJECT(user_data), WINDOW_MENU_SIGNAL_ERROR_STATE, priv->error_state, TRUE);

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
	}

	return;
}

static IndicatorObjectEntry *
get_entry(WindowMenus *wm, DbusmenuMenuitem * item, guint *index)
{
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);

	guint position = 0;
	for (position = 0; position < priv->entries->len; ++position) {
		WMEntry * entry = g_array_index(priv->entries, WMEntry *, position);
		if (entry->mi == item) {
			if (index != NULL) {
				*index = position;
			}
			return &entry->ioentry;
		}
	}

	/* Not found */
	return NULL;
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

	IndicatorObjectEntry * entry = get_entry(WINDOW_MENUS(user_data), item, NULL);
	if (entry == NULL) {
		/* Not found */
		return;
	}

	g_signal_emit_by_name(G_OBJECT(user_data), WINDOW_MENU_SIGNAL_SHOW_MENU, entry, timestamp, TRUE);

	return;
}

/* Called when the client changes its status.  Used to show panel if requested.
   (Say, by an Alt press.) */
static void
status_changed (DbusmenuClient * client, GParamSpec * pspec, gpointer user_data)
{
	g_signal_emit_by_name(G_OBJECT(user_data), WINDOW_MENU_SIGNAL_STATUS_CHANGED, dbusmenu_client_get_status (client));
}

WindowMenuStatus dbusmenu_status_table[] = {
	[DBUSMENU_STATUS_NORMAL] = WINDOW_MENU_STATUS_NORMAL,
	[DBUSMENU_STATUS_NOTICE] = WINDOW_MENU_STATUS_ACTIVE
};

static WindowMenuStatus
get_status (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), DBUSMENU_STATUS_NORMAL);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);

	return dbusmenu_status_table[dbusmenu_client_get_status (DBUSMENU_CLIENT (priv->client))];
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
	g_object_ref(newmenu); /* Take a ref for the async callback */
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
	g_signal_connect(G_OBJECT(priv->client), "notify::" DBUSMENU_CLIENT_PROP_STATUS, G_CALLBACK(status_changed), newmenu);

	DbusmenuMenuitem * root = dbusmenu_client_get_root(DBUSMENU_CLIENT(priv->client));
	if (root != NULL) {
		root_changed(DBUSMENU_CLIENT(priv->client), root, newmenu);
	}

	return newmenu;
}

/* Callback from trying to create the proxy for the service, this
   could include starting the service. */
static void
props_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(res, &error);

	if (g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED)) {
		g_error_free (error);
		return; // Must exit before accessing freed memory
	}

	WindowMenus * self = WINDOW_MENUS(user_data);
	g_return_if_fail(self != NULL);

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(self);

	if (priv->props_cancel != NULL) {
		g_object_unref(priv->props_cancel);
		priv->props_cancel = NULL;
	}

	if (error != NULL) {
		g_warning("Could not grab DBus proxy for window %u: %s", priv->windowid, error->message);
		g_error_free(error);
		goto out;
	}

	/* Okay, we're good to grab the proxy at this point, we're
	sure that it's ours. */
	priv->props = proxy;

out:
	g_object_unref(G_OBJECT(self));

	return;
}

/* Get the location of this entry */
static guint
get_location (WindowMenu * wm, IndicatorObjectEntry * entry)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), 0);

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
static GList *
get_entries (WindowMenu * wm)
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

/* Remove the various signals that we attach to menuitems to
   ensure they don't pop up later. */
static void
remove_menuitem_signals (DbusmenuMenuitem * mi, gpointer user_data)
{
	g_signal_handlers_disconnect_by_func(G_OBJECT(mi), G_CALLBACK(menu_entry_realized), user_data);
	g_signal_handlers_disconnect_by_func(G_OBJECT(mi), G_CALLBACK(menu_entry_realized_child_added), user_data);
	g_signal_handlers_disconnect_matched (mi, G_SIGNAL_MATCH_FUNC, 0, 0, 0, menu_child_realized, NULL);
	g_signal_handlers_disconnect_matched (mi, G_SIGNAL_MATCH_FUNC, 0, 0, 0, menu_prop_changed, NULL);

	return;
}

/* Respond to the root menu item on our client changing */
static void
root_changed (DbusmenuClient * client, DbusmenuMenuitem * new_root, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	/* Remove the old entries */
	free_entries(G_OBJECT(user_data), TRUE);

	if (priv->root != NULL) {
		dbusmenu_menuitem_foreach(priv->root, remove_menuitem_signals, user_data);

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
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	g_signal_connect(G_OBJECT(newentry), DBUSMENU_MENUITEM_SIGNAL_REALIZED, G_CALLBACK(menu_entry_realized), user_data);

	GtkMenuItem * mi = dbusmenu_gtkclient_menuitem_get(priv->client, newentry);
	if (mi != NULL) {
		menu_entry_realized(newentry, user_data);
	}
	return;
}

/* A small clean up function to ensure that the data
   gets free'd and the ref lost in all cases. */
static void
child_realized_data_cleanup (gpointer user_data, GClosure * closure)
{
	gpointer * data = (gpointer *)user_data;
	g_object_unref(data[1]);
	g_free(user_data);
	return;
}

/* This is the callback for when we're done waiting for a new entry to have
   children */
static void
menu_entry_realized_child_added (DbusmenuMenuitem * parent,
                                 DbusmenuMenuitem * child,
                                 guint position, gpointer user_data)
{
	/* Child added may be called more than once, so make sure we only
	   handle it once by disconnecting. */
	g_signal_handlers_disconnect_by_func(G_OBJECT(parent), G_CALLBACK(menu_entry_realized_child_added), user_data);

	menu_entry_realized (parent, user_data);
}

/* React to the menuitem when we know that it's got all the data
   that we really need. */
static void
menu_entry_realized (DbusmenuMenuitem * newentry, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	GtkMenu * menu = dbusmenu_gtkclient_menuitem_get_submenu(priv->client, newentry);

	/* Check to see if we have any children, if we don't let's see if
	   we can scare some up for fun. */
	GList * children = dbusmenu_menuitem_get_children(newentry);
	if (children == NULL && g_strcmp0(DBUSMENU_MENUITEM_CHILD_DISPLAY_SUBMENU, dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY)) == 0) {
		dbusmenu_menuitem_send_about_to_show(newentry, NULL, NULL);
	}

	if (menu == NULL) {
		if (children != NULL) {
			gpointer * data = g_new(gpointer, 2);
			data[0] = user_data;
			data[1] = g_object_ref(newentry);

			g_signal_connect_data(G_OBJECT(children->data), DBUSMENU_MENUITEM_SIGNAL_REALIZED, G_CALLBACK(menu_child_realized), data, child_realized_data_cleanup, 0);
		} else {
			g_signal_connect(G_OBJECT(newentry), DBUSMENU_MENUITEM_SIGNAL_CHILD_ADDED, G_CALLBACK(menu_entry_realized_child_added), user_data);
		}
	} else {
		gpointer * data = g_new(gpointer, 2);
		data[0] = user_data;
		data[1] = newentry;

		menu_child_realized(NULL, data);
		g_free (data);
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
		entry->accessible_desc = g_variant_get_string(value, NULL);

		if (wmentry->wm != NULL) {
			g_signal_emit_by_name(G_OBJECT(wmentry->wm), WINDOW_MENU_SIGNAL_A11Y_UPDATE, entry, TRUE);
		}
	}

	return;
}

/* We can't go until we have some kids.  Really, it's important. */
static void
menu_child_realized (DbusmenuMenuitem * child, gpointer user_data)
{
	/* Grab our values out to stack variables */
	DbusmenuMenuitem * newentry = DBUSMENU_MENUITEM(((gpointer *)user_data)[1]);
	WindowMenus * wm = WINDOW_MENUS(((gpointer *)user_data)[0]);

	g_return_if_fail(newentry != NULL);
	g_return_if_fail(wm != NULL);

	/* Disconnection below will drop the ref for this signal
	   handler, let's make sure that's not a problem */
	g_object_ref(G_OBJECT(newentry));

	/* Only care about the first */
	/* This will cause the cleanup function attached to the signal
	   handler to be run. */
	if (child != NULL) {
		g_signal_handlers_disconnect_by_func(G_OBJECT(child), menu_child_realized, user_data);
	}

	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	WMEntry * wmentry = g_new0(WMEntry, 1);
	wmentry->wm = wm;
	IndicatorObjectEntry * entry = &wmentry->ioentry;

	wmentry->mi = newentry;
	g_object_ref(G_OBJECT(wmentry->mi));

	entry->label = GTK_LABEL(gtk_label_new_with_mnemonic(dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL)));

	if (entry->label != NULL) {
		g_object_ref_sink(entry->label);
	}

	entry->accessible_desc = dbusmenu_menuitem_property_get(newentry, DBUSMENU_MENUITEM_PROP_LABEL);

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

	g_signal_emit_by_name(G_OBJECT(wm), WINDOW_MENU_SIGNAL_ENTRY_ADDED, entry, TRUE);

	g_object_unref(newentry);

	guint pos = dbusmenu_menuitem_get_position (newentry,
	                                            dbusmenu_menuitem_get_parent (newentry));
	if (pos < priv->entries->len - 1) {
		/* Our entry added signals that we pass up don't have position
		   information, so we can only add to the end of the list of
		   entries.  So when we get an inserted-in-the-middle entry,
		   we fake a root change to remove all entries and add them
		   back up again */
		g_object_ref(priv->root);
		root_changed(DBUSMENU_CLIENT(priv->client), priv->root, wm);
		g_object_unref(priv->root);
	}

	return;
}

/* Respond to an entry getting removed from the menu */
static void
menu_entry_removed (DbusmenuMenuitem * root, DbusmenuMenuitem * oldentry, gpointer user_data)
{
	g_return_if_fail(IS_WINDOW_MENUS(user_data));
	g_return_if_fail(DBUSMENU_IS_MENUITEM(oldentry));
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(user_data);

	if (priv->entries == NULL || priv->entries->len == 0) {
		return;
	}

	guint position;
	IndicatorObjectEntry * entry = get_entry(WINDOW_MENUS(user_data), oldentry, &position);

	if (entry != NULL) {
		g_array_remove_index(priv->entries, position);
		g_signal_emit_by_name(G_OBJECT(user_data), WINDOW_MENU_SIGNAL_ENTRY_REMOVED, entry, TRUE);
		entry_free(entry);
	} else {
		/* We've been called before menu_child_realized fired,
		 * so there isn't a WMEntry yet. Don't be going ahead
		 * and creating one if this menu item continues to live! */
		g_signal_handlers_disconnect_by_func(G_OBJECT(oldentry), G_CALLBACK(menu_entry_realized), user_data);
		g_signal_handlers_disconnect_by_func(G_OBJECT(oldentry), G_CALLBACK(menu_entry_realized_child_added), user_data);
	}

	return;
}

/* Get the XID of this window */
static guint
get_xid (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), 0);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	return priv->windowid;
}

/* Get the path for this object */
static gchar *
get_path (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), NULL);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	GValue obj = {0};
	g_value_init(&obj, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(priv->client), DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &obj);
	gchar * retval = g_value_dup_string(&obj);
	g_value_unset(&obj);
	return retval;
}

/* Get the address of this object */
static gchar *
get_address (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), NULL);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	GValue obj = {0};
	g_value_init(&obj, G_TYPE_STRING);
	g_object_get_property(G_OBJECT(priv->client), DBUSMENU_CLIENT_PROP_DBUS_NAME, &obj);
	gchar * retval = g_value_dup_string(&obj);
	g_value_unset(&obj);
	return retval;
}

/* Return whether we're in an error state or not */
static gboolean
get_error_state (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENUS(wm), TRUE);
	WindowMenusPrivate * priv = WINDOW_MENUS_GET_PRIVATE(wm);
	return priv->error_state;
}

/* Regain whether we're supposed to be hidden or disabled, we
   want to keep that if that's the case, otherwise bring back
   to the base state */
static void
entry_restore (WindowMenu * wm, IndicatorObjectEntry * entry)
{
	g_return_if_fail(IS_WINDOW_MENUS(wm));
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
static void
entry_activate (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp)
{
	g_return_if_fail(IS_WINDOW_MENUS(wm));
	WMEntry * wme = (WMEntry *)entry;
	dbusmenu_menuitem_send_about_to_show(wme->mi, NULL, NULL);
	return;
}
