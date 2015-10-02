/*
An implementation of indicator object showing menus from applications.

Copyright 2010-2013 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>
    Marco Trevisan <marco@canonical.com>

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

#include <stdlib.h> /* exit() */

#include <X11/Xlib.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/client.h>

#include <libbamf/bamf-matcher.h>

#include "gen-application-menu-registrar.xml.h"
#include "gen-application-menu-renderer.xml.h"
#include "indicator-appmenu-marshal.h"
#include "window-menu.h"
#include "window-menu-dbusmenu.h"
#include "window-menu-model.h"
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

typedef enum _ActiveStubsState ActiveStubsState;
enum _ActiveStubsState {
	STUBS_UNKNOWN,
	STUBS_SHOW,
	STUBS_HIDE
};

typedef enum _AppmenuMode AppmenuMode;
enum _AppmenuMode {
	MODE_STANDARD,
	MODE_UNITY,
	MODE_UNITY_ALL_MENUS
};

struct _IndicatorAppmenuClass {
	IndicatorObjectClass parent_class;
};

struct _IndicatorAppmenu {
	IndicatorObject parent;

	AppmenuMode mode;

	WindowMenu * default_app;
	GHashTable * apps;

	BamfMatcher * matcher;
	BamfWindow * active_window;
	ActiveStubsState active_stubs;

	GtkMenuItem * close_item;
	GArray * window_menus;

	GHashTable * desktop_windows;
	WindowMenu * desktop_menu;

	GDBusConnection * bus;
	guint owner_id;
	guint dbus_registration;
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
	GCancellable * bus_cancel;
	GDBusConnection * bus;
	guint dbus_registration;
};


/**********************
  Prototypes
 **********************/
static gboolean indicator_appmenu_delayed_init                       (IndicatorAppmenu * iapp);
static void indicator_appmenu_dispose                                (GObject *object);
static void indicator_appmenu_finalize                               (GObject *object);
static void build_window_menus                                       (IndicatorAppmenu * iapp);
static GList * get_entries                                           (IndicatorObject * io);
static guint get_location                                            (IndicatorObject * io,
                                                                      IndicatorObjectEntry * entry);
static void entry_activate                                           (IndicatorObject * io,
                                                                      IndicatorObjectEntry * entry,
                                                                      guint timestamp);
static void entry_activate_window                                    (IndicatorObject * io,
                                                                      IndicatorObjectEntry * entry,
                                                                      guint windowid,
                                                                      guint timestamp);
static void switch_default_app                                       (IndicatorAppmenu * iapp,
                                                                      WindowMenu * newdef,
                                                                      BamfWindow * active_window);
static void find_relevant_windows                                    (IndicatorAppmenu * iapp);
static void new_window                                               (BamfMatcher * matcher,
                                                                      BamfView * view,
                                                                      gpointer user_data);
static void old_window                                               (BamfMatcher * matcher,
                                                                      BamfView * view,
                                                                      gpointer user_data);
static void window_entry_added                                       (WindowMenu * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      IndicatorAppmenu * iapp);
static void window_entry_removed                                     (WindowMenu * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      IndicatorAppmenu * iapp);
static void window_status_changed                                    (WindowMenu * mw,
                                                                      DbusmenuStatus status,
                                                                      IndicatorAppmenu * iapp);
static void window_show_menu                                         (WindowMenu * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      guint timestamp,
                                                                      gpointer user_data);
static void window_a11y_update                                       (WindowMenu * mw,
                                                                      IndicatorObjectEntry * entry,
                                                                      gpointer user_data);
static void active_window_changed                                    (BamfMatcher * matcher,
                                                                      BamfView * oldview,
                                                                      BamfView * newview,
                                                                      gpointer user_data);
static WindowMenu * update_active_window                             (IndicatorAppmenu * appmenu,
                                                                      BamfWindow *window);
static GQuark error_quark                                            (void);
static void bus_method_call                                          (GDBusConnection * connection,
                                                                      const gchar * sender,
                                                                      const gchar * object_path,
                                                                      const gchar * interface,
                                                                      const gchar * method,
                                                                      GVariant * params,
                                                                      GDBusMethodInvocation * invocation,
                                                                      gpointer user_data);
static void on_bus_acquired                                          (GDBusConnection * connection,
                                                                      const gchar * name,
                                                                      gpointer user_data);
static void on_name_lost                                             (GDBusConnection * connection,
                                                                      const gchar * name,
                                                                      gpointer user_data);
static WindowMenu * ensure_menus                                     (IndicatorAppmenu * iapp,
	                                                                  BamfWindow * window);
static GVariant * unregister_window                                  (IndicatorAppmenu * iapp,
                                                                      guint windowid);
static void connect_to_menu_signals                                  (IndicatorAppmenu * iapp,
	                                                                  WindowMenu * menus);

/* Unique error codes for debug interface */
enum {
	ERROR_NO_APPLICATIONS,
	ERROR_NO_DEFAULT_APP,
	ERROR_WINDOW_NOT_FOUND
};

/**********************
  DBus Interfaces
 **********************/
static GDBusNodeInfo *      node_info = NULL;
static GDBusInterfaceInfo * interface_info = NULL;
static GDBusInterfaceVTable interface_table = {
       method_call:    bus_method_call,
       get_property:   NULL, /* No properties */
       set_property:   NULL  /* No properties */
};

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
	ioclass->entry_activate = entry_activate;
	ioclass->entry_activate_window = entry_activate_window;

	/* Setting up the DBus interfaces */
	if (node_info == NULL) {
		GError * error = NULL;

		node_info = g_dbus_node_info_new_for_xml(_application_menu_registrar, &error);
		if (error != NULL) {
			g_critical("Unable to parse Application Menu Interface description: %s", error->message);
			g_error_free(error);
		}
	}

	if (interface_info == NULL) {
		interface_info = g_dbus_node_info_lookup_interface(node_info, REG_IFACE);

		if (interface_info == NULL) {
			g_critical("Unable to find interface '" REG_IFACE "'");
		}
	}

	return;
}

/* Per instance Init */
static void
indicator_appmenu_init (IndicatorAppmenu *self)
{
	self->apps = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
	self->mode = MODE_STANDARD;
	self->active_stubs = STUBS_UNKNOWN;

	/* Setup the cache of windows with possible desktop entries */
	self->desktop_windows = g_hash_table_new(g_direct_hash, g_direct_equal);

	g_idle_add((GSourceFunc) indicator_appmenu_delayed_init, self);
}

/* Delayed Init, this is done so it can happen after that the mode has been set */
static gboolean
indicator_appmenu_delayed_init (IndicatorAppmenu *self)
{
	if (indicator_object_check_environment(INDICATOR_OBJECT(self), "unity-all-menus")) {
		self->mode = MODE_UNITY_ALL_MENUS;
	} else if (indicator_object_check_environment(INDICATOR_OBJECT(self), "unity")) {
		self->mode = MODE_UNITY;
	}

	if (self->mode != MODE_STANDARD)
		self->active_stubs = STUBS_HIDE;

	if (self->active_stubs != STUBS_HIDE)
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

	find_relevant_windows(self);

	/* Request a name so others can find us */
	self->owner_id = g_bus_own_name (G_BUS_TYPE_SESSION,
	                                 DBUS_NAME,
	                                 G_BUS_NAME_OWNER_FLAGS_NONE,
	                                 on_bus_acquired,
	                                 NULL,
	                                 on_name_lost,
	                                 self,
	                                 NULL);

	return G_SOURCE_REMOVE;
}

static void
on_bus_acquired (GDBusConnection * connection, const gchar * name,
                 gpointer user_data)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	GError * error = NULL;

	if (connection == NULL) {
		g_critical ("Unable to get session bus.");
		exit (0);
	}

	iapp->bus = connection;

	/* Now register our object on our new connection */
	iapp->dbus_registration = g_dbus_connection_register_object(connection,
	                                                            REG_OBJECT,
	                                                            interface_info,
	                                                            &interface_table,
	                                                            user_data,
	                                                            NULL,
	                                                            &error);

	if (error != NULL) {
		g_critical("Unable to register the object to DBus: %s", error->message);
		g_error_free(error);
	}
}

static void
on_name_lost (GDBusConnection * connection, const gchar * name,
              gpointer user_data)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	if (connection == NULL) {
		g_critical("OMG! Unable to get a connection to DBus");
	}
	else {
		g_critical("Unable to claim the name %s", DBUS_NAME);
	}

	/* We can rest assured no one will register with us, but let's
	   just ensure we're not showing anything. */
	switch_default_app(iapp, NULL, NULL);
}

/* Object refs decrement */
static void
indicator_appmenu_dispose (GObject *object)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(object);

	if (iapp->dbus_registration != 0) {
		g_dbus_connection_unregister_object(iapp->bus, iapp->dbus_registration);
		/* Don't care if it fails, there's nothing we can do */
		iapp->dbus_registration = 0;
	}

	g_clear_object(&iapp->bus);

	if (iapp->owner_id != 0) {
		g_bus_unown_name(iapp->owner_id);
		iapp->owner_id = 0;
	}

	/* bring down the matcher before resetting to no menu so we don't
	   get match signals */
	g_clear_object(&iapp->matcher);

	/* No specific ref */
	switch_default_app(iapp, NULL, NULL);

	g_clear_pointer(&iapp->apps, g_hash_table_destroy);
	g_clear_pointer(&iapp->desktop_windows, g_hash_table_destroy);

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
		g_signal_handlers_disconnect_by_data(iapp->close_item, iapp);

		gint i;
		for (i = 0; i < iapp->window_menus->len; ++i) {
			IndicatorObjectEntry *entry = &g_array_index(iapp->window_menus, IndicatorObjectEntry, i);
			g_clear_object(&(entry->label));
			g_clear_object(&(entry->menu));
		}
		g_array_free(iapp->window_menus, TRUE);
	}

	g_signal_handlers_disconnect_by_data(iapp->matcher, iapp);

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->finalize (object);
	return;
}

static void
emit_signal (IndicatorAppmenu * iapp, const gchar * name, GVariant * variant)
{
	GError * error = NULL;

	g_dbus_connection_emit_signal (iapp->bus,
		                       NULL,
		                       REG_OBJECT,
		                       REG_IFACE,
		                       name,
		                       variant,
		                       &error);

	if (error != NULL) {
		g_critical("Unable to send %s signal: %s", name, error->message);
		g_error_free(error);
		return;
	}

	return;
}

/* Close the current application using magic */
static void
close_current (GtkMenuItem * mi, gpointer user_data)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	if (!BAMF_IS_WINDOW (iapp->active_window) || bamf_view_is_closed (BAMF_VIEW (iapp->active_window))) {
		g_warning("Can't close a window we don't have. Window is either non-existent or recently closed.");
		return;
	}

	guint32 xid = bamf_window_get_xid(iapp->active_window);
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
	gdk_flush ();
	gdk_error_trap_pop_ignored ();

	return;
}

/* Create the default window menus */
static void
build_window_menus (IndicatorAppmenu * iapp)
{
	IndicatorObjectEntry entries[1] = {{0}};
	GtkAccelGroup * agroup = gtk_accel_group_new();
	GtkMenuItem * mi = NULL;
	GtkStockItem stockitem;

	/* Setup the entries for the fallbacks */
	iapp->window_menus = g_array_sized_new(FALSE, FALSE, sizeof(IndicatorObjectEntry), 2);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	/* File Menu */
	if (!gtk_stock_lookup(GTK_STOCK_FILE, &stockitem)) {
		g_warning("Unable to find the file menu stock item");
		stockitem.label = "_File";
	}
G_GNUC_END_IGNORE_DEPRECATIONS

	entries[0].label = GTK_LABEL(gtk_label_new_with_mnemonic(stockitem.label));
	g_object_ref(G_OBJECT(entries[0].label));
	gtk_widget_show(GTK_WIDGET(entries[0].label));

	entries[0].menu = GTK_MENU(gtk_menu_new());
	g_object_ref(G_OBJECT(entries[0].menu));

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	mi = GTK_MENU_ITEM(gtk_image_menu_item_new_from_stock(GTK_STOCK_CLOSE, agroup));
G_GNUC_END_IGNORE_DEPRECATIONS

	gtk_widget_set_sensitive(GTK_WIDGET(mi), FALSE);
	g_signal_connect(G_OBJECT(mi), "activate", G_CALLBACK(close_current), iapp);
	gtk_widget_show(GTK_WIDGET(mi));
	gtk_menu_shell_append(GTK_MENU_SHELL(entries[0].menu), GTK_WIDGET(mi));
	iapp->close_item = mi;

	gtk_widget_show(GTK_WIDGET(entries[0].menu));

	/* Copy the entries on the stack into the array */
	g_array_insert_vals(iapp->window_menus, 0, entries, 1);
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
			iapp->desktop_menu = WINDOW_MENU(pwm);
			break;
		}
	}

	g_list_free(keys);

	return;
}

/* Puts all the windows we care about into the hash table so that we
   can have a nice list of them. */
static void
find_relevant_windows (IndicatorAppmenu * iapp)
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
	if (!BAMF_IS_WINDOW(view)) {
		return;
	}

	BamfWindow * window = BAMF_WINDOW(view);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	guint32 xid = bamf_window_get_xid(window);

	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		ensure_menus(iapp, window);
		return;
	}

	if (bamf_window_get_window_type(window) != BAMF_WINDOW_DESKTOP) {
		return;
	}

	g_hash_table_insert(iapp->desktop_windows, GUINT_TO_POINTER(xid), GINT_TO_POINTER(TRUE));

	g_debug("New Desktop Window: %X", xid);

	gpointer pwm = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(xid));
	if (pwm != NULL) {
		WindowMenu * wm = WINDOW_MENU(pwm);
		iapp->desktop_menu = wm;
		g_debug("Setting Desktop Menus to: %X", xid);
		if (iapp->active_window == NULL && iapp->default_app == NULL) {
			switch_default_app(iapp, NULL, NULL);
		}
	}
}

/* When windows leave us, this function gets called */
static void
old_window (BamfMatcher * matcher, BamfView * view, gpointer user_data)
{
	if (!BAMF_IS_WINDOW(view)) {
		return;
	}

	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	BamfWindow * window = BAMF_WINDOW(view);
	guint32 xid = bamf_window_get_xid(window);

	unregister_window(iapp, xid);

	return;
}

/* List of desktop files that shouldn't have menu stubs. */
const static gchar * stubs_blacklist[] = {
	/* Firefox */
	"/firefox.desktop",
	/* Thunderbird */
	"/thunderbird.desktop",
	/* Open Office */
	"/openoffice.org-base.desktop",
	"/openoffice.org-impress.desktop",
	"/openoffice.org-calc.desktop",
	"/openoffice.org-math.desktop",
	"/openoffice.org-draw.desktop",
	"/openoffice.org-writer.desktop",
	/* Blender */
	"/blender-fullscreen.desktop",
	"/blender-windowed.desktop",
	/* Eclipse */
	"/eclipse.desktop",

	NULL
};

/* Check with BAMF, and then check the blacklist of desktop files
   to see if any are there.  Otherwise, show the stubs. */
gboolean
show_menu_stubs (BamfApplication * app)
{
	if (bamf_application_get_show_menu_stubs(app) == FALSE) {
		return FALSE;
	}

	const gchar * desktop_file = bamf_application_get_desktop_file(app);
	if (desktop_file == NULL || desktop_file[0] == '\0') {
		return TRUE;
	}

	int i;
	for (i = 0; stubs_blacklist[i] != NULL; i++) {
		if (g_str_has_suffix(desktop_file, stubs_blacklist[i]) == 0) {
			return FALSE;
		}
	}

	return TRUE;
}

/* Get the current set of entries */
static GList *
get_entries (IndicatorObject * io)
{
	g_return_val_if_fail(IS_INDICATOR_APPMENU(io), NULL);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);
	GHashTableIter iter;
	gpointer value;
	GList* entries = NULL;

	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		g_hash_table_iter_init(&iter, iapp->apps);
		while (g_hash_table_iter_next(&iter, NULL, &value)) {
			GList *app_entries = window_menu_get_entries(WINDOW_MENU (value));
			entries = g_list_concat(app_entries, entries);
		}

		return entries;
	}

	/* If we have a focused app with menus, use it's windows */
	if (iapp->default_app != NULL) {
		return window_menu_get_entries(iapp->default_app);
	}

	/* Else, let's go with desktop windows if there isn't a focused window */
	if (iapp->active_window == NULL) {
		if (iapp->desktop_menu == NULL) {
			return NULL;
		} else {
			return window_menu_get_entries(iapp->desktop_menu);
		}
	}

	/* Oh, now we're looking at stubs. */

	if (iapp->active_stubs == STUBS_UNKNOWN) {
		iapp->active_stubs = STUBS_SHOW;

		BamfApplication * app = bamf_matcher_get_application_for_window(iapp->matcher, iapp->active_window);
		if (app != NULL) {
			/* First check to see if we can find an app, then if we can
			   check to see if it has an opinion on whether we should
			   show the stubs or not. */
			if (show_menu_stubs(app) == FALSE) {
				/* If it blocks them, fall out. */
				iapp->active_stubs = STUBS_HIDE;
			}
		}
	}

	if (iapp->active_stubs == STUBS_HIDE) {
		return NULL;
	}

	/* There is only one item in window_menus now, but there
	   was more, and there is likely to be more in the future
	   so we're leaving this here to avoid a possible bug. */
	int i;
	for (i = 0; i < iapp->window_menus->len; i++) {
		entries = g_list_append(entries, &g_array_index(iapp->window_menus, IndicatorObjectEntry, i));
	}

	return entries;
}

/* Grabs the location of the entry */
static guint
get_location (IndicatorObject * io, IndicatorObjectEntry * entry)
{
	guint count = 0;
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);

	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		GHashTableIter iter;
		gpointer value;

		g_hash_table_iter_init(&iter, iapp->apps);
		while (g_hash_table_iter_next(&iter, NULL, &value)) {
			count = window_menu_get_location(WINDOW_MENU (value), entry);

			if (count != G_MAXUINT)
				return count;
		}

		return 0;
	}

	if (iapp->default_app != NULL) {
		/* Find the location in the app */
		count = window_menu_get_location(iapp->default_app, entry);
	} else if (iapp->active_window != NULL && iapp->window_menus) {
		/* Find the location in the window menus */
		for (count = 0; count < iapp->window_menus->len; count++) {
			if (entry == &g_array_index(iapp->window_menus, IndicatorObjectEntry, count)) {
				break;
			}
		}
		if (count == iapp->window_menus->len) {
			g_warning("Unable to find entry in default window menus");
			count = G_MAXUINT;
		}
	} else {
		/* Find the location in the desktop menu */
		if (iapp->desktop_menu != NULL) {
			count = window_menu_get_location(iapp->desktop_menu, entry);
		}
	}

	return (count == G_MAXUINT) ? 0 : count;
}

/* Responds to a menuitem being activated on the panel. */
static void
entry_activate (IndicatorObject * io, IndicatorObjectEntry * entry, guint timestamp)
{
	return entry_activate_window(io, entry, 0, timestamp);
}

/* Find the BAMF Window that is associated with that XID.  Unfortunately
   this requires a bit of searching, don't do it too often */
static BamfWindow *
xid_to_bamf_window (IndicatorAppmenu * iapp, guint xid)
{
	BamfApplication *application = bamf_matcher_get_application_for_xid(iapp->matcher, xid);
	GList * children = bamf_view_get_children (BAMF_VIEW (application));
	GList * l;
	BamfWindow * newwindow = NULL;

	for (l = children; l; l = l->next) {
		if (!BAMF_IS_WINDOW(l->data)) {
			continue;
		}

		BamfWindow * testwindow = BAMF_WINDOW(l->data);

		if (xid == bamf_window_get_xid(testwindow)) {
			newwindow = testwindow;
			break;
		}
	}
	g_list_free(children);

	return newwindow;
}

/* Responds to a menuitem being activated on the panel. */
static void
entry_activate_window (IndicatorObject * io, IndicatorObjectEntry * entry, guint windowid, guint timestamp)
{
	WindowMenu * menus = NULL;
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);

	/* We need to force a focus change in this case as we probably
	   just haven't gotten the signal from BAMF yet */
	if (windowid != 0) {
		BamfWindow * newwindow = xid_to_bamf_window(iapp, windowid);

		if (newwindow != NULL) {
			menus = update_active_window(iapp, newwindow);
		}
	}

	if (iapp->mode != MODE_UNITY_ALL_MENUS && iapp->default_app != NULL) {
		menus = iapp->default_app;

		if (!menus && iapp->active_window == NULL) {
			menus = iapp->desktop_menu;
		}
	}

	if (menus) {
		window_menu_entry_activate(menus, entry, timestamp);
	}
}

/* Checks to see we cared about a window that's going
   away, so that we can deal with that */
static void
window_finalized_is_active (gpointer user_data, GObject * old_window)
{
	g_return_if_fail(IS_INDICATOR_APPMENU(user_data));
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);

	/* Pointer comparison as we can't really trust any of the
	   pointers to do any dereferencing */
	if ((gpointer)iapp->active_window != (gpointer)old_window) {
		/* Ah, no issue, we weren't caring about this one
		   anyway. */
		return;
	}

	/* We're going to a state where we don't know what the active
	   window is, hopefully BAMF will save us */
	active_window_changed(iapp->matcher, NULL, NULL, iapp);

	return;
}

/* A helper for switch_default_app that takes care of the
   switching of the active window variable */
static void
switch_active_window (IndicatorAppmenu * iapp, BamfWindow * active_window)
{
	if (iapp->active_window == active_window || iapp->mode == MODE_UNITY_ALL_MENUS) {
		return;
	}

	if (iapp->active_window != NULL) {
		g_object_weak_unref(G_OBJECT(iapp->active_window), window_finalized_is_active, iapp);
	}

	iapp->active_window = active_window;

	if (iapp->mode == MODE_STANDARD)
		iapp->active_stubs = STUBS_UNKNOWN;

	/* Close any existing open menu by showing a null entry */
	window_show_menu(iapp->default_app, NULL, gtk_get_current_event_time(), iapp);

	if (active_window != NULL) {
		g_object_weak_ref(G_OBJECT(active_window), window_finalized_is_active, iapp);
	}

	if (iapp->close_item == NULL) {
		g_warning("No close item!?!?!");
		return;
	}

	gtk_widget_set_sensitive(GTK_WIDGET(iapp->close_item), FALSE);

	if (iapp->active_window == NULL) {
		return;
	}

	guint32 xid = bamf_window_get_xid(iapp->active_window);
	if (xid == 0 || bamf_view_is_closed (BAMF_VIEW (iapp->active_window))) {
		return;
	}

	GdkWMFunction functions;
	if (!egg_xid_get_functions(xid, &functions)) {
		g_debug("Unable to get MWM functions for: %d", xid);
		functions = GDK_FUNC_ALL;
	}

	if (functions & GDK_FUNC_ALL || functions & GDK_FUNC_CLOSE) {
		gtk_widget_set_sensitive(GTK_WIDGET(iapp->close_item), TRUE);
	}

	return;
}

static void
connect_to_menu_signals (IndicatorAppmenu * iapp, WindowMenu * menus)
{
	g_return_if_fail(G_IS_OBJECT(menus));

	/* Connect signals */
	g_signal_connect(menus,
	                 WINDOW_MENU_SIGNAL_ENTRY_ADDED,
	                 G_CALLBACK(window_entry_added),
	                 iapp);
	g_signal_connect(menus,
	                 WINDOW_MENU_SIGNAL_ENTRY_REMOVED,
	                 G_CALLBACK(window_entry_removed),
	                 iapp);
	g_signal_connect(menus,
	                 WINDOW_MENU_SIGNAL_STATUS_CHANGED,
	                 G_CALLBACK(window_status_changed),
	                 iapp);
	g_signal_connect(menus,
	                 WINDOW_MENU_SIGNAL_SHOW_MENU,
	                 G_CALLBACK(window_show_menu),
	                 iapp);
	g_signal_connect(menus,
	                 WINDOW_MENU_SIGNAL_A11Y_UPDATE,
	                 G_CALLBACK(window_a11y_update),
	                 iapp);
}

/* Switch applications, remove all the entires for the previous
   one and add them for the new application */
static void
switch_default_app (IndicatorAppmenu * iapp, WindowMenu * newdef, BamfWindow * active_window)
{
	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		return;
	}

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

	/* hide the entries that we're swapping out */
	indicator_object_set_visible (INDICATOR_OBJECT(iapp), FALSE);

	if (iapp->default_app)
	{
		/* Disconnect signals */
		g_signal_handlers_disconnect_by_data(iapp->default_app, iapp);

		/* Default App is NULL, let's see if it needs replacement */
		iapp->default_app = NULL;
	}

	/* Update the active window pointer -- may be NULL */
	switch_active_window(iapp, active_window);

	/* If we're putting up a new window, let's do that now. */
	if (newdef != NULL) {
		/* Switch */
		iapp->default_app = newdef;
		connect_to_menu_signals(iapp, iapp->default_app);
	}

	/* show the entries that we're swapping in */
	indicator_object_set_visible (INDICATOR_OBJECT(iapp), TRUE);

	/* Set up initial state for new entries if needed */
	if (iapp->default_app != NULL &&
            window_menu_get_status (iapp->default_app) != WINDOW_MENU_STATUS_NORMAL) {
		window_status_changed(iapp->default_app,
		                      window_menu_get_status (iapp->default_app),
		                      iapp);
	}

	return;
}

static void
track_menus (IndicatorAppmenu * iapp, guint xid, WindowMenu * menus)
{
	g_return_if_fail(IS_WINDOW_MENU(menus));

	g_hash_table_insert(iapp->apps, GUINT_TO_POINTER(xid), menus);

	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		GList *entries, *l;
		WindowMenuStatus status;

		connect_to_menu_signals(iapp, menus);
		entries = window_menu_get_entries(menus);
		status = window_menu_get_status(menus);

		for (l = entries; l; l = l->next) {
			window_entry_added(menus, l->data, iapp);
		}

		if (status != WINDOW_MENU_STATUS_ACTIVE) {
			window_status_changed(menus, status, iapp);
		}

		g_list_free(entries);
	}
}

static WindowMenu *
ensure_menus (IndicatorAppmenu * iapp, BamfWindow * window)
{
	WindowMenu * menus = NULL;
	guint32 xid = 0;

	while (window != NULL && menus == NULL) {
		xid = bamf_window_get_xid(window);

		menus = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(xid));

		/* First look to see if we can get these from the
		   GMenuModel access */
		if (menus == NULL) {
			gchar * uniquename = bamf_window_get_utf8_prop (window, "_GTK_UNIQUE_BUS_NAME");

			if (uniquename != NULL) {
				BamfApplication * app = bamf_matcher_get_application_for_window(iapp->matcher, window);

				menus = WINDOW_MENU(window_menu_model_new(app, window));
				track_menus(iapp, xid, menus);
			}

			g_free(uniquename);
		}

		if (menus == NULL) {
			g_debug("Looking for parent window on XID %d", xid);
			window = bamf_window_get_transient(window);
		}
	}

	return menus;
}

/* Recieve the signal that the window being shown
   has now changed. */
static void
active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data)
{
	update_active_window(INDICATOR_APPMENU(user_data), (BamfWindow *) newview);
}

static WindowMenu *
update_active_window (IndicatorAppmenu * appmenu, BamfWindow *window)
{
	WindowMenu * menus = NULL;

	if (window != NULL) {
		if (!BAMF_IS_WINDOW(window)) {
			window = NULL;
			g_warning("Active window changed to View thats not a window.");
		}
	} else {
		g_debug("Active window is: NULL");
	}

	if (appmenu->mode == MODE_UNITY_ALL_MENUS) {
		if (window != NULL) {
			menus = ensure_menus(appmenu, window);
		}
		return menus;
	}

	if (window != NULL && bamf_window_get_window_type(window) == BAMF_WINDOW_DESKTOP) {
		g_debug("Switching to menus from desktop");
		switch_default_app(appmenu, NULL, NULL);
		return menus;
	}

	g_debug("Switching to menus from XID %d", window ? bamf_window_get_xid(window) : 0);
	menus = ensure_menus(appmenu, window);
	switch_default_app(appmenu, menus, window);

	return menus;
}

/* Respond to the menus being destroyed.  We need to deregister
   and make sure we weren't being shown.  */
static void
menus_destroyed (IndicatorAppmenu * iapp, guint windowid)
{
	gboolean reload_menus = FALSE;
	WindowMenu * wm = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid));
	g_return_if_fail (IS_WINDOW_MENU(wm));

	g_hash_table_steal(iapp->apps, GUINT_TO_POINTER(windowid));
	g_signal_handlers_disconnect_by_data(wm, iapp);

	g_debug("Removing menus for %d", windowid);

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

	if (iapp->mode == MODE_UNITY_ALL_MENUS) {
		GList * entries, * l;
		entries = window_menu_get_entries(wm);
		for (l = entries; l; l = l->next) {
			window_entry_removed(wm, l->data, iapp);
		}
		g_list_free(entries);
	}

	g_object_unref(wm);
}

/* A new window wishes to register it's windows with us */
static GVariant *
register_window (IndicatorAppmenu * iapp, guint windowid, const gchar * objectpath,
                 const gchar * sender)
{
	g_debug("Registering window ID %d with path %s from %s", windowid, objectpath, sender);

	if (g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)) == NULL && windowid != 0) {
		WindowMenu * wm = WINDOW_MENU(window_menu_dbusmenu_new(windowid, sender, objectpath));
		g_return_val_if_fail(wm != NULL, FALSE);

		track_menus(iapp, windowid, wm);

		emit_signal(iapp, "WindowRegistered",
		            g_variant_new("(uso)", windowid, sender, objectpath));

		gpointer pdesktop = g_hash_table_lookup(iapp->desktop_windows, GUINT_TO_POINTER(windowid));
		if (pdesktop != NULL) {
			determine_new_desktop(iapp);
		}

		/* Note: Does not cause ref */
		BamfWindow * win = bamf_matcher_get_active_window(iapp->matcher);
		update_active_window(iapp, win);
	} else {
		if (windowid == 0) {
			g_warning("Can't build windows for a NULL window ID %d with path %s from %s", windowid, objectpath, sender);
		} else {
			g_warning("Already have a menu for window ID %d with path %s from %s, unregistering that one", windowid, objectpath, sender);
			unregister_window(iapp, windowid);

			/* NOTE: So we're doing a lookup here.  That seems pretty useless
			   now doesn't it.  It's for a good reason.  We're going recursive
			   with a pretty complex set of functions we want to ensure that
			   we're not going to end up infinitely recursive otherwise things
			   could go really bad. */
			if (g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)) == NULL) {
				return register_window(iapp, windowid, objectpath, sender);
			}

			g_warning("Unable to unregister window!");
		}
	}

	return g_variant_new("()");
}

/* Kindly remove an entry from our DB */
static GVariant *
unregister_window (IndicatorAppmenu * iapp, guint windowid)
{
	g_debug("Unregistering: %d", windowid);
	g_return_val_if_fail(IS_INDICATOR_APPMENU(iapp), NULL);
	g_return_val_if_fail(iapp->matcher != NULL, NULL);

	/* If it's a desktop window remove it from that table as well */
	g_hash_table_remove(iapp->desktop_windows, GUINT_TO_POINTER(windowid));

	emit_signal(iapp, "WindowUnregistered", g_variant_new ("(u)", windowid));

	menus_destroyed(iapp, windowid);

	return NULL;
}

/* Grab the menu information for a specific window */
static GVariant *
get_menu_for_window (IndicatorAppmenu * iapp, guint windowid, GError ** error)
{
	WindowMenu * wm = NULL;

	if (windowid == 0) {
		wm = iapp->default_app;
	} else {
		wm = WINDOW_MENU(g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)));
	}

	if (wm == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_WINDOW_NOT_FOUND, "Window not found");
		return NULL;
	}

	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);

	if (IS_WINDOW_MENU_DBUSMENU(wm)) {
		gchar * address = window_menu_dbusmenu_get_address(WINDOW_MENU_DBUSMENU(wm));
		gchar * path = window_menu_dbusmenu_get_path(WINDOW_MENU_DBUSMENU(wm));
		g_variant_builder_add_value(&builder, g_variant_new_string(address));
		g_variant_builder_add_value(&builder, g_variant_new_object_path(path));
		g_free(path);
		g_free(address);
	} else {
		g_variant_builder_add_value(&builder, g_variant_new_string(""));
		g_variant_builder_add_value(&builder, g_variant_new_object_path("/"));
	}

	return g_variant_builder_end(&builder);
}

/* Get all the menus we have */
static GVariant *
get_menus (IndicatorAppmenu * iapp, GError ** error)
{
	if (iapp->apps == NULL) {
		g_set_error_literal(error, error_quark(), ERROR_NO_APPLICATIONS, "No applications are registered");
		return NULL;
	}

	GVariantBuilder builder;
	GHashTableIter hash_iter;
	gpointer value;

	g_variant_builder_init (&builder, G_VARIANT_TYPE("a(uso)"));
	g_hash_table_iter_init (&hash_iter, iapp->apps);
	while (g_hash_table_iter_next (&hash_iter, NULL, &value)) {
		if (value != NULL) {
			WindowMenu * wm = WINDOW_MENU(value);
			if (IS_WINDOW_MENU_DBUSMENU(wm)) {
				gchar * address = window_menu_dbusmenu_get_address(WINDOW_MENU_DBUSMENU(wm));
				gchar * path = window_menu_dbusmenu_get_path(WINDOW_MENU_DBUSMENU(wm));
				g_variant_builder_add (&builder, "(uso)",
				                       window_menu_get_xid(wm),
				                       address,
				                       path);
				g_free(path);
				g_free(address);
			} else {
				g_variant_builder_add (&builder, "(uso)",
				                       window_menu_get_xid(wm),
				                       "",
				                       "/");
			}
		}
	}

	return g_variant_new ("(a(uso))", &builder);
}

/* A method has been called from our dbus inteface.  Figure out what it
   is and dispatch it. */
static void
bus_method_call (GDBusConnection * connection, const gchar * sender,
                 const gchar * object_path, const gchar * interface,
                 const gchar * method, GVariant * params,
                 GDBusMethodInvocation * invocation, gpointer user_data)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(user_data);
	GVariant * retval = NULL;
	GError * error = NULL;

	if (g_strcmp0(method, "RegisterWindow") == 0) {
		guint32 xid;
		const gchar * path;
		g_variant_get(params, "(u&o)", &xid, &path);
		retval = register_window(iapp, xid, path, sender);
	} else if (g_strcmp0(method, "UnregisterWindow") == 0) {
		guint32 xid;
		g_variant_get(params, "(u)", &xid);
		retval = unregister_window(iapp, xid);
	} else if (g_strcmp0(method, "GetMenuForWindow") == 0) {
		guint32 xid;
		g_variant_get(params, "(u)", &xid);
		retval = get_menu_for_window(iapp, xid, &error);
	} else if (g_strcmp0(method, "GetMenus") == 0) {
		retval = get_menus(iapp, &error);
	} else {
		g_warning("Calling method '%s' on the indicator service and it's unknown", method);
	}

	if (error != NULL) {
		g_dbus_method_invocation_return_dbus_error(invocation,
		                                           "com.canonical.AppMenu.Error",
		                                           error->message);
		g_error_free(error);
	} else {
		g_dbus_method_invocation_return_value(invocation, retval);
	}
	return;
}

/* Pass up the entry added event */
static void
window_entry_added (WindowMenu * mw, IndicatorObjectEntry * entry, IndicatorAppmenu * iapp)
{
	entry->parent_object = INDICATOR_OBJECT(iapp);
	g_signal_emit_by_name(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED, entry);
}

/* Pass up the entry removed event */
static void
window_entry_removed (WindowMenu * mw, IndicatorObjectEntry * entry, IndicatorAppmenu * iapp)
{
	entry->parent_object = INDICATOR_OBJECT(iapp);
	g_signal_emit_by_name(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED, entry);
}

/* Pass up the status changed event */
static void
window_status_changed (WindowMenu * mw, DbusmenuStatus status, IndicatorAppmenu * iapp)
{
	gboolean show_now = (status == DBUSMENU_STATUS_NOTICE);
	GList * l, * window_entries = window_menu_get_entries(mw);

	for (l = window_entries; l; l = l->next) {
		IndicatorObjectEntry * entry = l->data;
		g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_SHOW_NOW_CHANGED_ID, 0, entry, show_now);
	}
	g_list_free (window_entries);
}

/* Pass up the show menu event */
static void
window_show_menu (WindowMenu * mw, IndicatorObjectEntry * entry, guint timestamp, gpointer user_data)
{
	g_signal_emit_by_name(G_OBJECT(user_data), INDICATOR_OBJECT_SIGNAL_MENU_SHOW, entry, timestamp);
}

/* Pass up the accessible string update */
static void
window_a11y_update (WindowMenu * mw, IndicatorObjectEntry * entry, gpointer user_data)
{
	g_signal_emit_by_name(G_OBJECT(user_data), INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE, entry);
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

