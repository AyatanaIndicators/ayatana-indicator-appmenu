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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-gtype-specialized.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#include <libbamf/bamf-matcher.h>

#include "indicator-appmenu-marshal.h"
#include "window-menus.h"
#include "dbus-shared.h"

/**********************
  Indicator Object
 **********************/
#define INDICATOR_APPMENU_TYPE            (indicator_appmenu_get_type ())
#define INDICATOR_APPMENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_APPMENU_TYPE, IndicatorAppmenu))
#define INDICATOR_APPMENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_APPMENU_TYPE, IndicatorAppmenuClass))
#define IS_INDICATOR_APPMENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_APPMENU_TYPE))
#define IS_INDICATOR_APPMENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_APPMENU_TYPE))
#define INDICATOR_APPMENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_APPMENU_TYPE, IndicatorAppmenuClass))

GType indicator_appmenu_get_type (void);

INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_APPMENU_TYPE)

typedef struct _IndicatorAppmenu      IndicatorAppmenu;
typedef struct _IndicatorAppmenuClass IndicatorAppmenuClass;
typedef struct _IndicatorAppmenuDebug      IndicatorAppmenuDebug;
typedef struct _IndicatorAppmenuDebugClass IndicatorAppmenuDebugClass;

struct _IndicatorAppmenuClass {
	IndicatorObjectClass parent_class;

	void (*window_registered) (IndicatorAppmenu * iapp, guint wid, gchar * path, gpointer user_data);
	void (*window_unregistered) (IndicatorAppmenu * iapp, guint wid, gchar * path, gpointer user_data);
};

struct _IndicatorAppmenu {
	IndicatorObject parent;

	WindowMenus * default_app;
	GHashTable * apps;

	BamfMatcher * matcher;
	BamfWindow * active_window;

	gulong sig_entry_added;
	gulong sig_entry_removed;

	GArray * window_menus;
	GArray * desktop_menus;

	IndicatorAppmenuDebug * debug;
};


/**********************
  Debug Proxy
 **********************/
#define INDICATOR_APPMENU_DEBUG_TYPE            (indicator_appmenu_debug_get_type ())
#define INDICATOR_APPMENU_DEBUG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_APPMENU_DEBUG_TYPE, IndicatorAppmenuDebug))
#define INDICATOR_APPMENU_DEBUG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_APPMENU_DEBUG_TYPE, IndicatorAppmenuDebugClass))
#define IS_INDICATOR_APPMENU_DEBUG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_APPMENU_DEBUG_TYPE))
#define IS_INDICATOR_APPMENU_DEBUG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_APPMENU_DEBUG_TYPE))
#define INDICATOR_APPMENU_DEBUG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_APPMENU_DEBUG_TYPE, IndicatorAppmenuDebugClass))

GType indicator_appmenu_debug_get_type (void);

struct _IndicatorAppmenuDebugClass {
	GObjectClass parent_class;
};

struct _IndicatorAppmenuDebug {
	GObject parent;
	IndicatorAppmenu * appmenu;
};


/**********************
  Prototypes
 **********************/
static void indicator_appmenu_class_init                             (IndicatorAppmenuClass *klass);
static void indicator_appmenu_init                                   (IndicatorAppmenu *self);
static void indicator_appmenu_dispose                                (GObject *object);
static void indicator_appmenu_finalize                               (GObject *object);
static void indicator_appmenu_debug_class_init                       (IndicatorAppmenuDebugClass *klass);
static void indicator_appmenu_debug_init                             (IndicatorAppmenuDebug *self);
static void build_window_menus                                       (IndicatorAppmenu * iapp);
static void build_desktop_menus                                      (IndicatorAppmenu * iapp);
static GList * get_entries                                           (IndicatorObject * io);
static guint get_location                                            (IndicatorObject * io,
                                                                      IndicatorObjectEntry * entry);
static void switch_default_app                                       (IndicatorAppmenu * iapp,
                                                                      WindowMenus * newdef,
                                                                      BamfWindow * active_window);
static gboolean _application_menu_registrar_server_register_window   (IndicatorAppmenu * iapp,
                                                                      guint windowid,
                                                                      const gchar * objectpath,
                                                                      DBusGMethodInvocation * method);
static void request_name_cb                                          (DBusGProxy *proxy,
                                                                      guint result,
                                                                      GError *error,
                                                                      gpointer userdata);
static void window_entry_added                                       (WindowMenus * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      gpointer user_data);
static void window_entry_removed                                     (WindowMenus * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      gpointer user_data);
static void active_window_changed                                    (BamfMatcher * matcher,
                                                                      BamfView * oldview,
                                                                      BamfView * newview,
                                                                      gpointer user_data);
static gboolean _application_menu_debug_server_current_menu          (IndicatorAppmenuDebug * iappd,
                                                                      guint * windowid,
                                                                      gchar ** objectpath,
                                                                      gchar ** address,
                                                                      GError ** error);
static gboolean _application_menu_debug_server_all_menus             (IndicatorAppmenuDebug * iappd,
                                                                      GPtrArray ** entries,
                                                                      GError ** error);
static gboolean _application_menu_debug_server_j_so_ndump            (IndicatorAppmenuDebug * iappd,
                                                                      guint windowid,
                                                                      gchar ** jsondata,
                                                                      GError ** error);

/**********************
  DBus Interfaces
 **********************/
#include "application-menu-registrar-server.h"
#include "application-menu-debug-server.h"

enum {
	WINDOW_REGISTERED,
	WINDOW_UNREGISTERED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (IndicatorAppmenu, indicator_appmenu, INDICATOR_OBJECT_TYPE);

/* One time init */
static void
indicator_appmenu_class_init (IndicatorAppmenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_appmenu_dispose;
	object_class->finalize = indicator_appmenu_finalize;

	IndicatorObjectClass * ioclass = INDICATOR_OBJECT_CLASS(klass);

	ioclass->get_entries = get_entries;
	ioclass->get_location = get_location;

	signals[WINDOW_REGISTERED] =  g_signal_new("window-registered",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (IndicatorAppmenuClass, window_registered),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__UINT_STRING,
	                                      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);
	signals[WINDOW_UNREGISTERED] =  g_signal_new("window-unregistered",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (IndicatorAppmenuClass, window_unregistered),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__UINT_STRING,
	                                      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	dbus_g_object_type_install_info(INDICATOR_APPMENU_TYPE, &dbus_glib__application_menu_registrar_server_object_info);

	return;
}

/* Per instance Init */
static void
indicator_appmenu_init (IndicatorAppmenu *self)
{
	self->default_app = NULL;
	self->debug = NULL;
	self->apps = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
	self->matcher = NULL;
	self->active_window = NULL;

	/* Setup the entries for the fallbacks */
	self->window_menus = g_array_sized_new(FALSE, FALSE, sizeof(IndicatorObjectEntry), 2);
	self->desktop_menus = g_array_sized_new(FALSE, FALSE, sizeof(IndicatorObjectEntry), 2);

	build_window_menus(self);
	build_desktop_menus(self);

	/* Get the default BAMF matcher */
	self->matcher = bamf_matcher_get_default();
	if (self->matcher == NULL) {
		/* we don't want to exit out of Unity -- but this
		   should really never happen */
		g_warning("Unable to get BAMF matcher, can not watch applications switch!");
	} else {
		g_signal_connect(G_OBJECT(self->matcher), "active-window-changed", G_CALLBACK(active_window_changed), self);
	}

	/* Register this object on DBus */
	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	dbus_g_connection_register_g_object(connection,
	                                    REG_OBJECT,
	                                    G_OBJECT(self));

	/* Request a name so others can find us */
	DBusGProxy * dbus_proxy = dbus_g_proxy_new_for_name_owner(connection,
	                                                   DBUS_SERVICE_DBUS,
	                                                   DBUS_PATH_DBUS,
	                                                   DBUS_INTERFACE_DBUS,
	                                                   NULL);
	org_freedesktop_DBus_request_name_async (dbus_proxy,
	                                         DBUS_NAME,
	                                         DBUS_NAME_FLAG_DO_NOT_QUEUE,
	                                         request_name_cb,
	                                         self);

	/* Setup debug interface */
	self->debug = g_object_new(INDICATOR_APPMENU_DEBUG_TYPE, NULL);
	self->debug->appmenu = self;

	return;
}

/* Object refs decrement */
static void
indicator_appmenu_dispose (GObject *object)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(object);

	/* bring down the matcher before resetting to no menu so we don't
	   get match signals */
	if (iapp->matcher != NULL) {
		g_object_unref(iapp->matcher);
		iapp->matcher = NULL;
	}

	/* No specific ref */
	switch_default_app (iapp, NULL, NULL);

	if (iapp->apps != NULL) {
		g_hash_table_destroy(iapp->apps);
		iapp->apps = NULL;
	}

	if (iapp->debug != NULL) {
		g_object_unref(iapp->debug);
		iapp->debug = NULL;
	}

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
indicator_appmenu_finalize (GObject *object)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(object);

	if (iapp->window_menus != NULL) {
		if (iapp->window_menus->len != 0) {
			g_warning("Window menus weren't free'd in dispose!");
		}
		g_array_free(iapp->window_menus, TRUE);
		iapp->window_menus = NULL;
	}

	if (iapp->desktop_menus != NULL) {
		if (iapp->desktop_menus->len != 0) {
			g_warning("Desktop menus weren't free'd in dispose!");
		}
		g_array_free(iapp->desktop_menus, TRUE);
		iapp->desktop_menus = NULL;
	}

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->finalize (object);
	return;
}

G_DEFINE_TYPE (IndicatorAppmenuDebug, indicator_appmenu_debug, G_TYPE_OBJECT);

/* One time init */
static void
indicator_appmenu_debug_class_init (IndicatorAppmenuDebugClass *klass)
{
	dbus_g_object_type_install_info(INDICATOR_APPMENU_DEBUG_TYPE, &dbus_glib__application_menu_debug_server_object_info);

	return;
}

/* Per instance Init */
static void
indicator_appmenu_debug_init (IndicatorAppmenuDebug *self)
{
	self->appmenu = NULL;

	/* Register this object on DBus */
	DBusGConnection * connection = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	dbus_g_connection_register_g_object(connection,
	                                    DEBUG_OBJECT,
	                                    G_OBJECT(self));

	return;
}

/* Create the default window menus */
static void
build_window_menus (IndicatorAppmenu * iapp)
{
	IndicatorObjectEntry entries[2] = {{0}, {0}};
	GtkAccelGroup * agroup = gtk_accel_group_new();
	GtkMenuItem * mi = NULL;
	GtkStockItem stockitem;

	/* File Menu */
	if (!gtk_stock_lookup(GTK_STOCK_FILE, &stockitem)) {
		g_warning("Unable to find the file menu stock item");
		stockitem.label = "_File";
	}
	entries[0].label = GTK_LABEL(gtk_label_new_with_mnemonic(stockitem.label));
	g_object_ref(G_OBJECT(entries[0].label));
	gtk_widget_show(GTK_WIDGET(entries[0].label));

	entries[0].menu = GTK_MENU(gtk_menu_new());
	g_object_ref(G_OBJECT(entries[0].menu));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[0].menu, GTK_WIDGET(mi));

	gtk_widget_show(GTK_WIDGET(entries[0].menu));

	/* Edit Menu */
	if (!gtk_stock_lookup(GTK_STOCK_EDIT, &stockitem)) {
		g_warning("Unable to find the edit menu stock item");
		stockitem.label = "_Edit";
	}
	entries[1].label = GTK_LABEL(gtk_label_new_with_mnemonic(stockitem.label));
	g_object_ref(G_OBJECT(entries[1].label));
	gtk_widget_show(GTK_WIDGET(entries[1].label));

	entries[1].menu = GTK_MENU(gtk_menu_new());
	g_object_ref(G_OBJECT(entries[1].menu));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_UNDO, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_REDO, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_separator_menu_item_new());
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_CUT, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_COPY, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_PASTE, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_DELETE, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_separator_menu_item_new());
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_SELECT_ALL, agroup));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[1].menu, GTK_WIDGET(mi));

	gtk_widget_show(GTK_WIDGET(entries[1].menu));

	/* Copy the entries on the stack into the array */
	g_array_insert_vals(iapp->window_menus, 0, entries, 2);

	return;
}

/* Create the default desktop menus */
static void
build_desktop_menus (IndicatorAppmenu * iapp)
{
	IndicatorObjectEntry entries[1] = {{0}};

	/* File Menu */
	entries[0].label = GTK_LABEL(gtk_label_new("Desktop"));
	g_object_ref(G_OBJECT(entries[0].label));
	gtk_widget_show(GTK_WIDGET(entries[0].label));

	entries[0].menu = GTK_MENU(gtk_menu_new());
	g_object_ref(G_OBJECT(entries[0].menu));

	GtkMenuItem * mi = GTK_MENU_ITEM(gtk_menu_item_new_with_label("Desktop Menus will go here"));
	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[0].menu, GTK_WIDGET(mi));

	gtk_widget_show(GTK_WIDGET(entries[0].menu));

	/* Copy the entries on the stack into the array */
	g_array_insert_vals(iapp->desktop_menus, 0, entries, 1);

	return;
}

/* Get the current set of entries */
static GList *
get_entries (IndicatorObject * io)
{
	g_return_val_if_fail(IS_INDICATOR_APPMENU(io), NULL);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);

	if (iapp->default_app != NULL) {
		return window_menus_get_entries(iapp->default_app);
	}

	GArray * entryarray;
	if (iapp->active_window == NULL) {
		entryarray = iapp->desktop_menus;
	} else {
		entryarray = iapp->window_menus;
	}

	GList * output = NULL;
	int i;

	for (i = 0; i < entryarray->len; i++) {
		output = g_list_append(output, &g_array_index(entryarray, IndicatorObjectEntry, i));
	}

	return output;
}

/* Grabs the location of the entry */
static guint
get_location (IndicatorObject * io, IndicatorObjectEntry * entry)
{
	guint count = 0;
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);
	if (iapp->default_app != NULL) {
		/* Find the location in the app */
		count = window_menus_get_location(iapp->default_app, entry);
	} else if (iapp->active_window != NULL) {
		/* Find the location in the window menus */
		for (count = 0; count < iapp->window_menus->len; count++) {
			if (entry == &g_array_index(iapp->window_menus, IndicatorObjectEntry, count)) {
				break;
			}
		}
		if (count == iapp->window_menus->len) {
			g_warning("Unable to find entry in default window menus");
			count = 0;
		}
	} else {
		/* Find the location in the desktop menus */
		for (count = 0; count < iapp->desktop_menus->len; count++) {
			if (entry == &g_array_index(iapp->desktop_menus, IndicatorObjectEntry, count)) {
				break;
			}
		}
		if (count == iapp->desktop_menus->len) {
			g_warning("Unable to find entry in default window menus");
			count = 0;
		}
	}
	return count;
}

/* Switch applications, remove all the entires for the previous
   one and add them for the new application */
static void
switch_default_app (IndicatorAppmenu * iapp, WindowMenus * newdef, BamfWindow * active_window)
{
	GList * entry_head, * entries;

	if (iapp->default_app == newdef && iapp->default_app != NULL) {
		/* We've got an app with menus and it hasn't changed. */

		/* Keep active window up-to-date, though we're probably not
		   using it much. */
		iapp->active_window = active_window;
		return;
	}
	if (iapp->default_app == NULL && iapp->active_window == active_window && newdef == NULL) {
		/* There's no application menus, but the active window hasn't
		   changed.  So there's no change. */
		return;
	}

	entry_head = indicator_object_get_entries(INDICATOR_OBJECT(iapp));

	for (entries = entry_head; entries != NULL; entries = g_list_next(entries)) {
		IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;

		if (entry->label != NULL) {
			gtk_widget_hide(GTK_WIDGET(entry->label));
		}

		if (entry->menu != NULL) {
			gtk_menu_detach(entry->menu);
		}

		g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED_ID, 0, entries->data, TRUE);
	}

	g_list_free(entry_head);
	
	/* Disconnect signals */
	if (iapp->sig_entry_added != 0) {
		g_signal_handler_disconnect(G_OBJECT(iapp->default_app), iapp->sig_entry_added);
		iapp->sig_entry_added = 0;
	}
	if (iapp->sig_entry_removed != 0) {
		g_signal_handler_disconnect(G_OBJECT(iapp->default_app), iapp->sig_entry_removed);
		iapp->sig_entry_removed = 0;
	}

	/* Default App is NULL, let's see if it needs replacement */
	iapp->default_app = NULL;

	/* Update the active window pointer -- may be NULL */
	iapp->active_window = active_window;

	/* If we're putting up a new window, let's do that now. */
	if (newdef != NULL) {
		/* Switch */
		iapp->default_app = newdef;

		/* Connect signals */
		iapp->sig_entry_added =   g_signal_connect(G_OBJECT(iapp->default_app),
		                                           WINDOW_MENUS_SIGNAL_ENTRY_ADDED,
		                                           G_CALLBACK(window_entry_added),
		                                           iapp);
		iapp->sig_entry_removed = g_signal_connect(G_OBJECT(iapp->default_app),
		                                           WINDOW_MENUS_SIGNAL_ENTRY_REMOVED,
		                                           G_CALLBACK(window_entry_removed),
		                                           iapp);
	}

	/* Get our new list of entries.  Now we can go ahead and signal
	   that each of them has been added */

	entry_head = indicator_object_get_entries(INDICATOR_OBJECT(iapp));

	for (entries = entry_head; entries != NULL; entries = g_list_next(entries)) {
		IndicatorObjectEntry * entry = (IndicatorObjectEntry *)entries->data;

		if (entry->label != NULL) {
			gtk_widget_show(GTK_WIDGET(entry->label));
		}

		g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED_ID, 0, entries->data, TRUE);
	}

	g_list_free(entry_head);

	return;
}

/* Recieve the signal that the window being shown
   has now changed. */
static void
active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data)
{
	BamfWindow * window = NULL;

	if (newview != NULL) {
		window = BAMF_WINDOW(newview);
	}

	IndicatorAppmenu * appmenu = INDICATOR_APPMENU(user_data);

	WindowMenus * menus = NULL;
	guint32 xid = 0;

	while (window != NULL && menus == NULL) {
		xid = bamf_window_get_xid(window);
	
		menus = g_hash_table_lookup(appmenu->apps, GUINT_TO_POINTER(xid));

		if (menus == NULL) {
			window = bamf_window_get_transient(window);
		}
	}

	g_debug("Switching to windows from XID %d", xid);

	/* Note: This function can handle menus being NULL */
	switch_default_app(appmenu, menus, BAMF_WINDOW(newview));

	return;
}

/* Respond to the menus being destroyed.  We need to deregister
   and make sure we weren't being shown.  */
static void
menus_destroyed (GObject * menus, gpointer user_data)
{
	WindowMenus * wm = WINDOW_MENUS(menus);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	/* If we're it, let's remove ourselves and BAMF will probably
	   give us a new entry in a bit. */
	if (iapp->default_app == wm) {
		switch_default_app(iapp, NULL, NULL);
	}

	guint xid = window_menus_get_xid(wm);
	g_return_if_fail(xid != 0);

	g_hash_table_steal(iapp->apps, GUINT_TO_POINTER(xid));

	g_debug("Removing menus for %d", xid);

	return;
}

/* A new window wishes to register it's windows with us */
static gboolean
_application_menu_registrar_server_register_window (IndicatorAppmenu * iapp, guint windowid, const gchar * objectpath, DBusGMethodInvocation * method)
{
	g_debug("Registering window ID %d with path %s from %s", windowid, objectpath, dbus_g_method_get_sender(method));

	if (g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)) == NULL && windowid != 0) {
		WindowMenus * wm = window_menus_new(windowid, dbus_g_method_get_sender(method), objectpath);
		g_return_val_if_fail(wm != NULL, FALSE);

		g_signal_connect(G_OBJECT(wm), WINDOW_MENUS_SIGNAL_DESTROY, G_CALLBACK(menus_destroyed), iapp);

		g_hash_table_insert(iapp->apps, GUINT_TO_POINTER(windowid), wm);

		g_signal_emit(G_OBJECT(iapp), signals[WINDOW_REGISTERED], 0, windowid, objectpath, TRUE);

		/* Note: Does not cause ref */
		BamfWindow * win = bamf_matcher_get_active_window(iapp->matcher);

		active_window_changed(iapp->matcher, NULL, BAMF_VIEW(win), iapp);
	} else {
		if (windowid == 0) {
			g_warning("Can't build windows for a NULL window ID %d with path %s from %s", windowid, objectpath, dbus_g_method_get_sender(method));
		} else {
			g_warning("Already have a menu for window ID %d with path %s from %s", windowid, objectpath, dbus_g_method_get_sender(method));
		}
	}

	dbus_g_method_return(method);
	return TRUE;
}

/* Response to whether we got our name or not */
static void
request_name_cb (DBusGProxy *proxy, guint result, GError * inerror, gpointer userdata)
{
	gboolean error = FALSE;

	if (inerror != NULL) {
		g_warning("Unable to get name request: %s", inerror->message);
		error = TRUE;
	}

	if (!error && result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER && result != DBUS_REQUEST_NAME_REPLY_ALREADY_OWNER) {
		g_warning("The dbus name we want is already taken");
		error = TRUE;
	}

	if (error) {
		/* We can rest assured no one will register with us, but let's
		   just ensure we're not showing anything. */
		switch_default_app(INDICATOR_APPMENU(userdata), NULL, NULL);
	}

	g_object_unref(proxy);

	return;
}

/* Pass up the entry added event */
static void
window_entry_added (WindowMenus * mw, IndicatorObjectEntry * entry, gpointer user_data)
{
	g_signal_emit_by_name(G_OBJECT(user_data), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED, entry);
	return;
}

/* Pass up the entry removed event */
static void
window_entry_removed (WindowMenus * mw, IndicatorObjectEntry * entry, gpointer user_data)
{
	g_signal_emit_by_name(G_OBJECT(user_data), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED, entry);
	return;
}

/**********************
  DEBUG INTERFACE
 **********************/

/* Builds the error quark if we need it, otherwise just
   returns the same value */
static GQuark
error_quark (void)
{
	static GQuark error_quark = 0;

	if (error_quark == 0) {
		error_quark = g_quark_from_static_string("indicator-appmenu");
	}

	return error_quark;
}

/* Unique error codes for debug interface */
enum {
	ERROR_NO_APPLICATIONS,
	ERROR_NO_DEFAULT_APP,
	ERROR_WINDOW_NOT_FOUND
};

/* Get the current menu */
static gboolean
_application_menu_debug_server_current_menu (IndicatorAppmenuDebug * iappd, guint * windowid, gchar ** objectpath, gchar ** address, GError ** error)
{
	IndicatorAppmenu * iapp = iappd->appmenu;

	if (iapp->default_app == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_NO_DEFAULT_APP, "Not currently showing an application");
		return FALSE;
	}

	*windowid = window_menus_get_xid(iapp->default_app);
	*objectpath = window_menus_get_path(iapp->default_app);
	*address = window_menus_get_address(iapp->default_app);

	return TRUE;
}

/* Get all the menus we have */
static gboolean
_application_menu_debug_server_all_menus(IndicatorAppmenuDebug * iappd, GPtrArray ** entries, GError ** error)
{
	IndicatorAppmenu * iapp = iappd->appmenu;

	if (iapp->apps == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_NO_APPLICATIONS, "No applications are registered");
		return FALSE;
	}

	*entries = g_ptr_array_new();

	GList * appkeys = NULL;
	for (appkeys = g_hash_table_get_keys(iapp->apps); appkeys != NULL; appkeys = g_list_next(appkeys)) {
		GValueArray * structval = g_value_array_new(3);
		gpointer hash_val = g_hash_table_lookup(iapp->apps, appkeys->data);

		if (hash_val == NULL) { continue; }

		GValue winid = {0};
		g_value_init(&winid, G_TYPE_UINT);
		g_value_set_uint(&winid, window_menus_get_xid(WINDOW_MENUS(hash_val)));
		g_value_array_append(structval, &winid);
		g_value_unset(&winid);

		GValue path = {0};
		g_value_init(&path, DBUS_TYPE_G_OBJECT_PATH);
		g_value_take_boxed(&path, window_menus_get_path(WINDOW_MENUS(hash_val)));
		g_value_array_append(structval, &path);
		g_value_unset(&path);

		GValue address = {0};
		g_value_init(&address, G_TYPE_STRING);
		g_value_take_string(&address, window_menus_get_address(WINDOW_MENUS(hash_val)));
		g_value_array_append(structval, &address);
		g_value_unset(&address);

		g_ptr_array_add(*entries, structval);
	}

	return TRUE;
}

/* Do something for each menu item */
static void
menu_iterator (GtkWidget * widget, gpointer user_data)
{
	GArray * strings = (GArray *)user_data;

	gchar * temp = g_strdup("{");
	g_array_append_val(strings, temp);

	/* TODO: We need some sort of useful ID, but for now  we're
	   just ensuring it's unique. */
	temp = g_strdup_printf("\"id\": %d", strings->len);
	g_array_append_val(strings, temp);

	if (GTK_IS_SEPARATOR_MENU_ITEM(widget)) {
		temp = g_strdup_printf(", \"type\": \"%s\"", "separator");
		g_array_append_val(strings, temp);
	} else {
		temp = g_strdup_printf(", \"type\": \"%s\"", "standard");
		g_array_append_val(strings, temp);
	}

	temp = g_strdup_printf(", \"enabled\": %s", gtk_widget_get_sensitive(GTK_WIDGET(widget)) ? "true" : "false");
	g_array_append_val(strings, temp);

	temp = g_strdup_printf(", \"visible\": %s", gtk_widget_get_visible(GTK_WIDGET(widget)) ? "true" : "false");
	g_array_append_val(strings, temp);

	const gchar * label = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));
	if (label != NULL) {
		temp = g_strdup_printf(", \"label\": \"%s\"", label);
		g_array_append_val(strings, temp);
	}

	GtkWidget * submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
	if (submenu != NULL) {
		temp = g_strdup(", \"children-display\": \"submenu\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [ ");
		g_array_append_val(strings, temp);

		gtk_container_foreach(GTK_CONTAINER(submenu), menu_iterator, strings);

		temp = g_strdup("]");
		g_array_append_val(strings, temp);
	}

	temp = g_strdup("},");
	g_array_append_val(strings, temp);

	return;
}

/* Takes an entry and outputs it into the ptrarray */
static void
entry2json (IndicatorObjectEntry * entry, GArray * strings)
{
	gchar * temp = g_strdup("{");
	g_array_append_val(strings, temp);

	/* TODO: We need some sort of useful ID, but for now  we're
	   just ensuring it's unique. */
	temp = g_strdup_printf("\"id\": %d", strings->len);
	g_array_append_val(strings, temp);

	if (entry->label != NULL) {
		temp = g_strdup_printf(", \"label\": \"%s\"", gtk_label_get_label(entry->label));
		g_array_append_val(strings, temp);

		temp = g_strdup_printf(", \"enabled\": %s", gtk_widget_get_sensitive(GTK_WIDGET(entry->label)) ? "true" : "false");
		g_array_append_val(strings, temp);

		temp = g_strdup_printf(", \"visible\": %s", gtk_widget_get_visible(GTK_WIDGET(entry->label)) ? "true" : "false");
		g_array_append_val(strings, temp);
	}

	if (entry->menu != NULL) {
		temp = g_strdup(", \"children-display\": \"submenu\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [ ");
		g_array_append_val(strings, temp);

		gtk_container_foreach(GTK_CONTAINER(entry->menu), menu_iterator, strings);

		temp = g_strdup("]");
		g_array_append_val(strings, temp);
	}


	temp = g_strdup("},");
	g_array_append_val(strings, temp);

	return;
}

/* Make JSON out of our menus */
static gboolean
_application_menu_debug_server_j_so_ndump (IndicatorAppmenuDebug * iappd, guint windowid, gchar ** jsondata, GError ** error)
{
	IndicatorAppmenu * iapp = iappd->appmenu;
	WindowMenus * wm = NULL;

	if (windowid == 0) {
		wm = iapp->default_app;
	} else {
		wm = WINDOW_MENUS(g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)));
	}

	if (wm == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_WINDOW_NOT_FOUND, "Window not found");
		return FALSE;
	}

	GArray * strings = g_array_new(TRUE, FALSE, sizeof(gchar *));
	GList * entries = window_menus_get_entries(wm);
	GList * entry;

	gchar * temp = g_strdup("{");
	g_array_append_val(strings, temp);

	temp = g_strdup("\"id\": 0");
	g_array_append_val(strings, temp);

	if (entries != NULL) {
		temp = g_strdup(", \"children-display\": \"submenu\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [");
		g_array_append_val(strings, temp);
	}

	for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
		entry2json(entry->data, strings);
	}

	if (entries != NULL) {
		temp = g_strdup("]");
		g_array_append_val(strings, temp);
	}

	g_list_free(entries);

	temp = g_strdup("}");
	g_array_append_val(strings, temp);

	*jsondata = g_strjoinv(NULL, (gchar **)strings->data);
	g_strfreev((gchar **)strings->data);
	g_array_free(strings, FALSE);

	return TRUE;
}
