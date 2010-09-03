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

#include <X11/Xlib.h>
#include <gdk/gdkx.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <dbus/dbus-gtype-specialized.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>

#include <libbamf/bamf-matcher.h>

#include "indicator-appmenu-marshal.h"
#include "window-menus.h"
#include "dbus-shared.h"
#include "gdk-get-func.h"

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

	void (*window_registered) (IndicatorAppmenu * iapp, guint wid, gchar * address, gpointer path, gpointer user_data);
	void (*window_unregistered) (IndicatorAppmenu * iapp, guint wid, gpointer user_data);
};

struct _IndicatorAppmenu {
	IndicatorObject parent;

	WindowMenus * default_app;
	GHashTable * apps;

	BamfMatcher * matcher;
	BamfWindow * active_window;

	gulong sig_entry_added;
	gulong sig_entry_removed;

	GtkMenuItem * close_item;

	GArray * window_menus;

	GHashTable * desktop_windows;
	WindowMenus * desktop_menu;

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
static GList * get_entries                                           (IndicatorObject * io);
static guint get_location                                            (IndicatorObject * io,
                                                                      IndicatorObjectEntry * entry);
static void switch_default_app                                       (IndicatorAppmenu * iapp,
                                                                      WindowMenus * newdef,
                                                                      BamfWindow * active_window);
static void find_desktop_windows                                     (IndicatorAppmenu * iapp);
static void new_window                                               (BamfMatcher * matcher,
                                                                      BamfView * view,
                                                                      gpointer user_data);
static void old_window                                               (BamfMatcher * matcher,
                                                                      BamfView * view,
                                                                      gpointer user_data);
static gboolean _application_menu_registrar_server_register_window   (IndicatorAppmenu * iapp,
                                                                      guint windowid,
                                                                      const gchar * objectpath,
                                                                      DBusGMethodInvocation * method);
static gboolean _application_menu_registrar_server_unregister_window (IndicatorAppmenu * iapp,
                                                                      guint windowid,
                                                                      GError ** error);
static gboolean _application_menu_registrar_server_get_menu_for_window (IndicatorAppmenu * iapp,
                                                                      guint windowid,
                                                                      gchar ** objectpath,
                                                                      gchar ** address,
                                                                      GError ** error);
static gboolean _application_menu_registrar_server_get_menus         (IndicatorAppmenu * iapp,
                                                                      GPtrArray ** entries,
                                                                      GError ** error);
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
static gboolean _application_menu_renderer_server_get_current_menu   (IndicatorAppmenuDebug * iappd,
                                                                      gchar ** objectpath,
                                                                      gchar ** address,
                                                                      GError ** error);
static gboolean _application_menu_renderer_server_activate_menu_item (IndicatorAppmenuDebug * iappd,
                                                                      GArray * menulist,
                                                                      GError ** error);
static gboolean _application_menu_renderer_server_dump_current_menu  (IndicatorAppmenuDebug * iappd,
                                                                      gchar ** jsondata,
                                                                      GError ** error);
static gboolean _application_menu_renderer_server_dump_menu          (IndicatorAppmenuDebug * iappd,
                                                                      guint windowid,
                                                                      gchar ** jsondata,
                                                                      GError ** error);
static GQuark error_quark                                            (void);

/* Unique error codes for debug interface */
enum {
	ERROR_NO_APPLICATIONS,
	ERROR_NO_DEFAULT_APP,
	ERROR_WINDOW_NOT_FOUND
};

/**********************
  DBus Interfaces
 **********************/
#include "application-menu-registrar-server.h"
#include "application-menu-renderer-server.h"

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
	                                      _indicator_appmenu_marshal_VOID__UINT_STRING_BOXED,
	                                      G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_STRING, DBUS_TYPE_G_OBJECT_PATH);
	signals[WINDOW_UNREGISTERED] =  g_signal_new("window-unregistered",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (IndicatorAppmenuClass, window_unregistered),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__UINT,
	                                      G_TYPE_NONE, 1, G_TYPE_UINT);

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
	self->close_item = NULL;

	/* Setup the entries for the fallbacks */
	self->window_menus = g_array_sized_new(FALSE, FALSE, sizeof(IndicatorObjectEntry), 2);

	/* Setup the cache of windows with possible desktop entries */
	self->desktop_windows = g_hash_table_new(g_direct_hash, g_direct_equal);
	self->desktop_menu = NULL; /* Starts NULL until found */

	build_window_menus(self);

	/* Get the default BAMF matcher */
	self->matcher = bamf_matcher_get_default();
	if (self->matcher == NULL) {
		/* we don't want to exit out of Unity -- but this
		   should really never happen */
		g_warning("Unable to get BAMF matcher, can not watch applications switch!");
	} else {
		g_signal_connect(G_OBJECT(self->matcher), "active-window-changed", G_CALLBACK(active_window_changed), self);

		/* Desktop window tracking */
		g_signal_connect(G_OBJECT(self->matcher), "view-opened", G_CALLBACK(new_window), self);
		g_signal_connect(G_OBJECT(self->matcher), "view-closed", G_CALLBACK(old_window), self);
	}

	find_desktop_windows(self);

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

	if (iapp->desktop_windows != NULL) {
		g_hash_table_destroy(iapp->desktop_windows);
		iapp->desktop_windows = NULL;
	}

	if (iapp->desktop_menu != NULL) {
		/* Wait, nothing here?  Yup.  We're not referencing the
		   menus here they're already attached to the window ID.
		   We're just keeping an efficient pointer to them. */
		iapp->desktop_menu = NULL;
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

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->finalize (object);
	return;
}

G_DEFINE_TYPE (IndicatorAppmenuDebug, indicator_appmenu_debug, G_TYPE_OBJECT);

/* One time init */
static void
indicator_appmenu_debug_class_init (IndicatorAppmenuDebugClass *klass)
{
	dbus_g_object_type_install_info(INDICATOR_APPMENU_DEBUG_TYPE, &dbus_glib__application_menu_renderer_server_object_info);

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

/* Close the current application using magic */
static void
close_current (GtkMenuItem * mi, gpointer user_data)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	if (iapp->active_window == NULL) {
		g_warning("Can't close a window we don't have.  NULL not cool.");
		return;
	}

	guint xid = bamf_window_get_xid(iapp->active_window);
	guint timestamp = gdk_event_get_time(NULL);

	XEvent xev;

	xev.xclient.type = ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = True;
	xev.xclient.display = gdk_x11_get_default_xdisplay ();
	xev.xclient.window = xid;
	xev.xclient.message_type = gdk_x11_atom_to_xatom (gdk_atom_intern ("_NET_CLOSE_WINDOW", TRUE));
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = timestamp;
	xev.xclient.data.l[1] = 2; /* Client type pager, so it listens to us */
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	gdk_error_trap_push ();
	XSendEvent (gdk_x11_get_default_xdisplay (),
	            gdk_x11_get_default_root_xwindow (),
	            False,
	            SubstructureRedirectMask | SubstructureNotifyMask,
	            &xev);
	gdk_error_trap_pop ();

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
	g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(close_current), iapp);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_append(entries[0].menu, GTK_WIDGET(mi));
	iapp->close_item = mi;

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

/* Determine which windows should be used as the desktop
   menus. */
static void
determine_new_desktop (IndicatorAppmenu * iapp)
{
	GList * keys = g_hash_table_get_keys(iapp->desktop_windows);
	GList * key;

	for (key = keys; key != NULL; key = g_list_next(key)) {
		guint xid = GPOINTER_TO_UINT(key->data);
		gpointer pwm = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(xid));
		if (pwm != NULL) {
			g_debug("Setting Desktop Menus to: %X", xid);
			iapp->desktop_menu = WINDOW_MENUS(pwm);
		}
	}

	g_list_free(keys);

	return;
}

/* Puts all the desktop windows into the hash table so that we
   can have a nice list of them. */
static void
find_desktop_windows (IndicatorAppmenu * iapp)
{
	GList * windows = bamf_matcher_get_windows(iapp->matcher);
	GList * lwindow;

	for (lwindow = windows; lwindow != NULL; lwindow = g_list_next(lwindow)) {
		BamfView * view = BAMF_VIEW(lwindow->data);
		new_window(iapp->matcher, view, iapp);
	}

	g_list_free(windows);

	return;
}

/* When new windows are born, we check to see if they're desktop
   windows. */
static void
new_window (BamfMatcher * matcher, BamfView * view, gpointer user_data)
{
	BamfWindow * window = BAMF_WINDOW(view);
	if (window == NULL) {
		return;
	}

	if (bamf_window_get_window_type(window) != BAMF_WINDOW_DESKTOP) {
		return;
	}

	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	guint xid = bamf_window_get_xid(window);
	g_hash_table_insert(iapp->desktop_windows, GUINT_TO_POINTER(xid), GINT_TO_POINTER(TRUE));

	g_debug("New Desktop Window: %X", xid);

	gpointer pwm = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(xid));
	if (pwm != NULL) {
		WindowMenus * wm = WINDOW_MENUS(pwm);
		iapp->desktop_menu = wm;
		g_debug("Setting Desktop Menus to: %X", xid);
		if (iapp->active_window == NULL && iapp->default_app == NULL) {
			switch_default_app(iapp, NULL, NULL);
		}
	}

	return;
}

/* When windows leave us, this function gets called */
static void
old_window (BamfMatcher * matcher, BamfView * view, gpointer user_data)
{
	BamfWindow * window = BAMF_WINDOW(view);
	if (window == NULL) {
		return;
	}

	if (bamf_window_get_window_type(window) != BAMF_WINDOW_DESKTOP) {
		return;
	}

	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	g_hash_table_remove(iapp->desktop_windows, GUINT_TO_POINTER(bamf_window_get_xid(window)));

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

	if (iapp->active_window == NULL) {
		if (iapp->desktop_menu == NULL) {
			return NULL;
		} else {
			return window_menus_get_entries(iapp->desktop_menu);
		}
	}

	GList * output = NULL;
	int i;

	for (i = 0; i < iapp->window_menus->len; i++) {
		output = g_list_append(output, &g_array_index(iapp->window_menus, IndicatorObjectEntry, i));
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
		/* Find the location in the desktop menu */
		if (iapp->desktop_menu != NULL) {
			count = window_menus_get_location(iapp->desktop_menu, entry);
		}
	}
	return count;
}

/* A helper for switch_default_app that takes care of the
   switching of the active window variable */
static void
switch_active_window (IndicatorAppmenu * iapp, BamfWindow * active_window)
{
	if (iapp->active_window == active_window) {
		return;
	}

	iapp->active_window = active_window;

	if (iapp->close_item == NULL) {
		g_warning("No close item!?!?!");
		return;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(iapp->close_item), FALSE);

	guint xid = bamf_window_get_xid(iapp->active_window);
	if (xid == 0) {
		return;
	}

	GdkWindow * window = gdk_window_foreign_new(xid);
	if (window == NULL) {
		g_warning("Unable to get foreign window for: %d", xid);
		return;
	}

	GdkWMFunction functions;
	if (!egg_window_get_functions(window, &functions)) {
		g_debug("Unable to get MWM functions for: %d", xid);
		functions = GDK_FUNC_ALL;
	}

	if (functions & GDK_FUNC_ALL || functions & GDK_FUNC_CLOSE) {
		gtk_widget_set_sensitive(GTK_WIDGET(iapp->close_item), TRUE);
	}

	g_object_unref(window);

	return;
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
		switch_active_window(iapp, active_window);
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
	switch_active_window(iapp, active_window);

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

		if (iapp->default_app != NULL) {
			window_menus_entry_restore(iapp->default_app, entry);
		} else {
			if (iapp->active_window == NULL) {
				/* Desktop Menus */
				window_menus_entry_restore(iapp->desktop_menu, entry);
			} else {
				/* Window Menus */
				if (entry->label != NULL) {
					gtk_widget_show(GTK_WIDGET(entry->label));
				}
			}
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

	if (window != NULL && bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP) {
		window = NULL;
	}

	IndicatorAppmenu * appmenu = INDICATOR_APPMENU(user_data);

	WindowMenus * menus = NULL;
	guint32 xid = 0;

	while (window != NULL && menus == NULL) {
		if (!bamf_view_user_visible(BAMF_VIEW(window))) {
			window = NULL;
		}

		xid = bamf_window_get_xid(window);
	
		menus = g_hash_table_lookup(appmenu->apps, GUINT_TO_POINTER(xid));

		if (menus == NULL) {
			window = bamf_window_get_transient(window);
		}
	}

	g_debug("Switching to windows from XID %d", xid);

	/* Note: This function can handle menus being NULL */
	if (xid == 0) {
		switch_default_app(appmenu, NULL, NULL);
	} else {
		switch_default_app(appmenu, menus, BAMF_WINDOW(newview));
	}

	return;
}

/* Respond to the menus being destroyed.  We need to deregister
   and make sure we weren't being shown.  */
static void
menus_destroyed (GObject * menus, gpointer user_data)
{
	gboolean reload_menus = FALSE;
	WindowMenus * wm = WINDOW_MENUS(menus);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	guint xid = window_menus_get_xid(wm);
	g_return_if_fail(xid != 0);

	g_hash_table_steal(iapp->apps, GUINT_TO_POINTER(xid));

	g_debug("Removing menus for %d", xid);

	if (iapp->desktop_menu == wm) {
		iapp->desktop_menu = NULL;
		determine_new_desktop(iapp);
		if (iapp->default_app == NULL && iapp->active_window == NULL) {
			reload_menus = TRUE;
		}
	}

	/* If we're it, let's remove ourselves and BAMF will probably
	   give us a new entry in a bit. */
	if (iapp->default_app == wm) {
		reload_menus = TRUE;
	}

	if (reload_menus) {
		switch_default_app(iapp, NULL, NULL);
	}

	return;
}

/* A new window wishes to register it's windows with us */
static gboolean
_application_menu_registrar_server_register_window (IndicatorAppmenu * iapp, guint windowid, const gchar * objectpath, DBusGMethodInvocation * method)
{
	const gchar * sender = dbus_g_method_get_sender(method);
	g_debug("Registering window ID %d with path %s from %s", windowid, objectpath, sender);

	if (g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)) == NULL && windowid != 0) {
		WindowMenus * wm = window_menus_new(windowid, sender, objectpath);
		g_return_val_if_fail(wm != NULL, FALSE);

		g_signal_connect(G_OBJECT(wm), WINDOW_MENUS_SIGNAL_DESTROY, G_CALLBACK(menus_destroyed), iapp);

		g_hash_table_insert(iapp->apps, GUINT_TO_POINTER(windowid), wm);

		g_signal_emit(G_OBJECT(iapp), signals[WINDOW_REGISTERED], 0, windowid, sender, objectpath, TRUE);

		gpointer pdesktop = g_hash_table_lookup(iapp->desktop_windows, GUINT_TO_POINTER(windowid));
		if (pdesktop != NULL) {
			determine_new_desktop(iapp);
		}

		/* Note: Does not cause ref */
		BamfWindow * win = bamf_matcher_get_active_window(iapp->matcher);

		active_window_changed(iapp->matcher, NULL, BAMF_VIEW(win), iapp);
	} else {
		if (windowid == 0) {
			g_warning("Can't build windows for a NULL window ID %d with path %s from %s", windowid, objectpath, sender);
		} else {
			g_warning("Already have a menu for window ID %d with path %s from %s", windowid, objectpath, sender);
		}
	}

	dbus_g_method_return(method);
	return TRUE;
}

/* Kindly remove an entry from our DB */
static gboolean
_application_menu_registrar_server_unregister_window (IndicatorAppmenu * iapp, guint windowid, GError ** error)
{
	/* TODO: Do it */

	return FALSE;
}

/* Grab the menu information for a specific window */
static gboolean
_application_menu_registrar_server_get_menu_for_window (IndicatorAppmenu * iapp, guint windowid, gchar ** objectpath, gchar ** address, GError ** error)
{
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

	*objectpath = window_menus_get_path(wm);
	*address = window_menus_get_address(wm);

	return TRUE;
}

/* Get all the menus we have */
static gboolean
_application_menu_registrar_server_get_menus (IndicatorAppmenu * iapp, GPtrArray ** entries, GError ** error)
{
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

/* Looks to see if we can find an accel label to steal the
   closure from */
static void
find_closure (GtkWidget * widget, gpointer user_data)
{
	GClosure ** closure = (GClosure **)user_data;

	/* If we have one quit */
	if (*closure != NULL) {
		return;
	}
	
	/* If we've got a label, steal its */
	if (GTK_IS_ACCEL_LABEL(widget)) {
		g_object_get (widget,
		              "accel-closure", closure,
		              NULL);
		return;
	}

	/* If we've got a container, dig deeper */
	if (GTK_IS_CONTAINER(widget)) {
		gtk_container_foreach(GTK_CONTAINER(widget), find_closure, user_data);
	}

	return;
}

/* Look at the closures in an accel group and find
   the one that matches the one we've been passed */
static gboolean
find_group_closure (GtkAccelKey * key, GClosure * closure, gpointer user_data)
{
	return closure == user_data;
}

/* Turn the key codes into a string for the JSON output */
static void
key2string (GtkAccelKey * key, GArray * strings)
{
	gchar * temp = NULL;

	temp = g_strdup(", \"" DBUSMENU_MENUITEM_PROP_SHORTCUT "\": [[");
	g_array_append_val(strings, temp);

	if (key->accel_mods & GDK_CONTROL_MASK) {
		temp = g_strdup_printf("\"%s\", ", DBUSMENU_MENUITEM_SHORTCUT_CONTROL);
		g_array_append_val(strings, temp);
	}
	if (key->accel_mods & GDK_MOD1_MASK) {
		temp = g_strdup_printf("\"%s\", ", DBUSMENU_MENUITEM_SHORTCUT_ALT);
		g_array_append_val(strings, temp);
	}
	if (key->accel_mods & GDK_SHIFT_MASK) {
		temp = g_strdup_printf("\"%s\", ", DBUSMENU_MENUITEM_SHORTCUT_SHIFT);
		g_array_append_val(strings, temp);
	}
	if (key->accel_mods & GDK_SUPER_MASK) {
		temp = g_strdup_printf("\"%s\", ", DBUSMENU_MENUITEM_SHORTCUT_SUPER);
		g_array_append_val(strings, temp);
	}

	temp = g_strdup_printf("\"%s\"]]", gdk_keyval_name(key->accel_key));
	g_array_append_val(strings, temp);

	return;
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
		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_TYPE "\": \"%s\"", DBUSMENU_CLIENT_TYPES_SEPARATOR);
		g_array_append_val(strings, temp);
	} else {
		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_TYPE "\": \"%s\"", DBUSMENU_CLIENT_TYPES_DEFAULT);
		g_array_append_val(strings, temp);
	}

	temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_ENABLED "\": %s", gtk_widget_get_sensitive(GTK_WIDGET(widget)) ? "true" : "false");
	g_array_append_val(strings, temp);

	temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_VISIBLE "\": %s", gtk_widget_get_visible(GTK_WIDGET(widget)) ? "true" : "false");
	g_array_append_val(strings, temp);

	const gchar * label = gtk_menu_item_get_label(GTK_MENU_ITEM(widget));
	if (label != NULL) {
		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_LABEL "\": \"%s\"", label);
		g_array_append_val(strings, temp);
	}

	/* Deal with shortcuts, find the accel closure if we can and then
	   turn that into a string */
	GClosure * closure = NULL;
	find_closure(widget, &closure);

	if (closure != NULL) {
		GtkAccelGroup * group = gtk_accel_group_from_accel_closure(closure);
		if (group != NULL) {
			GtkAccelKey * key = gtk_accel_group_find(group, find_group_closure, closure);
			if (key != NULL) {
				key2string(key, strings);
			}
		}
	}

	/* TODO: Handle check/radio items */

	GtkWidget * submenu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(widget));
	if (submenu != NULL) {
		temp = g_strdup(", \"" DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY "\": \"" DBUSMENU_MENUITEM_CHILD_DISPLAY_SUBMENU "\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [ ");
		g_array_append_val(strings, temp);

		guint old_len = strings->len;

		gtk_container_foreach(GTK_CONTAINER(submenu), menu_iterator, strings);

		if (old_len == strings->len) {
			temp = g_strdup("]");
			g_array_append_val(strings, temp);
		} else {
			gchar * last_one = g_array_index(strings, gchar *, strings->len - 1);
			guint lastlen = g_utf8_strlen(last_one, -1);

			if (last_one[lastlen - 1] != ',') {
				g_warning("Huh, this seems impossible.  Should be a comma at the end.");
			}

			last_one[lastlen - 1] = ']';
		}
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
		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_LABEL "\": \"%s\"", gtk_label_get_label(entry->label));
		g_array_append_val(strings, temp);

		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_ENABLED "\": %s", gtk_widget_get_sensitive(GTK_WIDGET(entry->label)) ? "true" : "false");
		g_array_append_val(strings, temp);

		temp = g_strdup_printf(", \"" DBUSMENU_MENUITEM_PROP_VISIBLE "\": %s", gtk_widget_get_visible(GTK_WIDGET(entry->label)) ? "true" : "false");
		g_array_append_val(strings, temp);
	}

	if (entry->menu != NULL) {
		temp = g_strdup(", \"" DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY "\": \"" DBUSMENU_MENUITEM_CHILD_DISPLAY_SUBMENU "\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [ ");
		g_array_append_val(strings, temp);

		guint old_len = strings->len;

		gtk_container_foreach(GTK_CONTAINER(entry->menu), menu_iterator, strings);

		if (old_len == strings->len) {
			temp = g_strdup("]");
			g_array_append_val(strings, temp);
		} else {
			gchar * last_one = g_array_index(strings, gchar *, strings->len - 1);
			guint lastlen = g_utf8_strlen(last_one, -1);

			if (last_one[lastlen - 1] != ',') {
				g_warning("Huh, this seems impossible.  Should be a comma at the end.");
			}

			last_one[lastlen - 1] = ']';
		}
	}


	temp = g_strdup("},");
	g_array_append_val(strings, temp);

	return;
}

/* Grab the location of the dbusmenu of the current menu */
static gboolean
_application_menu_renderer_server_get_current_menu (IndicatorAppmenuDebug * iappd, gchar ** objectpath, gchar ** address, GError ** error)
{
	IndicatorAppmenu * iapp = iappd->appmenu;

	if (iapp->default_app == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_NO_DEFAULT_APP, "Not currently showing an application");
		return FALSE;
	}

	*objectpath = window_menus_get_path(iapp->default_app);
	*address = window_menus_get_address(iapp->default_app);

	return TRUE;
}

/* Activate menu items through a script given as a parameter */
static gboolean
_application_menu_renderer_server_activate_menu_item (IndicatorAppmenuDebug * iappd, GArray * menulist, GError ** error)
{
	/* TODO: Do it */

	return FALSE;
}

/* Dump the current menu to a JSON file */
static gboolean
_application_menu_renderer_server_dump_current_menu  (IndicatorAppmenuDebug * iappd, gchar ** jsondata, GError ** error)
{
	return _application_menu_renderer_server_dump_menu(iappd, 0, jsondata, error);
}

/* Dump a specific window's menus to a JSON file */
static gboolean
_application_menu_renderer_server_dump_menu (IndicatorAppmenuDebug * iappd, guint windowid, gchar ** jsondata, GError ** error)
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
		temp = g_strdup(", \"" DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY "\": \"" DBUSMENU_MENUITEM_CHILD_DISPLAY_SUBMENU "\"");
		g_array_append_val(strings, temp);

		temp = g_strdup(", \"submenu\": [");
		g_array_append_val(strings, temp);
	}

	guint old_len = strings->len;

	for (entry = entries; entry != NULL; entry = g_list_next(entry)) {
		entry2json(entry->data, strings);
	}

	if (entries != NULL) {
		if (old_len == strings->len) {
			temp = g_strdup("]");
			g_array_append_val(strings, temp);
		} else {
			gchar * last_one = g_array_index(strings, gchar *, strings->len - 1);
			guint lastlen = g_utf8_strlen(last_one, -1);

			if (last_one[lastlen - 1] != ',') {
				g_warning("Huh, this seems impossible.  Should be a comma at the end.");
			}

			last_one[lastlen - 1] = ']';
		}
	}

	g_list_free(entries);

	temp = g_strdup("}");
	g_array_append_val(strings, temp);

	*jsondata = g_strjoinv(NULL, (gchar **)strings->data);
	g_strfreev((gchar **)strings->data);
	g_array_free(strings, FALSE);

	return TRUE;
}

