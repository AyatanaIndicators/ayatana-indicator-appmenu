
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#include "window-menus.h"
#include "dbus-shared.h"

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

struct _IndicatorAppmenuClass {
	IndicatorObjectClass parent_class;

	void (*window_registered) (IndicatorAppmenu * iapp, guint wid, gchar * path, gpointer user_data);
	void (*window_unregistered) (IndicatorAppmenu * iapp, guint wid, gchar * path, gpointer user_data);
};

struct _IndicatorAppmenu {
	IndicatorObject parent;

	WindowMenus * default_app;
	GHashTable * apps;
};

static void indicator_appmenu_class_init (IndicatorAppmenuClass *klass);
static void indicator_appmenu_init       (IndicatorAppmenu *self);
static void indicator_appmenu_dispose    (GObject *object);
static void indicator_appmenu_finalize   (GObject *object);
static GList * get_entries (IndicatorObject * io);
static void switch_default_app (IndicatorAppmenu * iapp, WindowMenus * newdef);
static gboolean _application_menu_registrar_server_window_register (IndicatorAppmenu * iapp, guint windowid, const GValue * objectpath, DBusGMethodInvocation * method);
static gboolean _application_menu_registrar_server_window_unregister (IndicatorAppmenu * iapp, guint windowid, const GValue * objectpath, DBusGMethodInvocation * method);
static void request_name_cb (DBusGProxy *proxy, guint result, GError *error, gpointer userdata);

#include "application-menu-registrar-server.h"

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

	signals[WINDOW_REGISTERED] =  g_signal_new("window-registered",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (IndicatorAppmenuClass, window_registered),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__UINT, // XXX
	                                      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);
	signals[WINDOW_UNREGISTERED] =  g_signal_new("window-unregistered",
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (IndicatorAppmenuClass, window_unregistered),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__UINT, // XXX
	                                      G_TYPE_NONE, 2, G_TYPE_UINT, G_TYPE_STRING);

	dbus_g_object_type_install_info(INDICATOR_APPMENU_TYPE, &dbus_glib__application_menu_registrar_server_object_info);

	return;
}

/* Per instance Init */
static void
indicator_appmenu_init (IndicatorAppmenu *self)
{
	self->default_app = NULL;
	self->apps = g_hash_table_new_full(NULL, NULL, NULL, g_object_unref);

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

	return;
}

/* Object refs decrement */
static void
indicator_appmenu_dispose (GObject *object)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(object);

	/* No specific ref */
	switch_default_app (iapp, NULL);

	if (iapp->apps != NULL) {
		g_hash_table_destroy(iapp->apps);
		iapp->apps = NULL;
	}

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
indicator_appmenu_finalize (GObject *object)
{

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->finalize (object);
	return;
}

/* Get the current set of entries */
static GList *
get_entries (IndicatorObject * io)
{
	g_return_val_if_fail(IS_INDICATOR_APPMENU(io), NULL);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);

	if (iapp->default_app == NULL) {
		return NULL;
	}

	return window_menus_get_entries(iapp->default_app);
}

/* Switch applications, remove all the entires for the previous
   one and add them for the new application */
static void
switch_default_app (IndicatorAppmenu * iapp, WindowMenus * newdef)
{
	if (iapp->default_app == newdef) {
		return;
	}

	GList * entries;

	/* Remove old */
	if (iapp->default_app != NULL) {
		for (entries = window_menus_get_entries(iapp->default_app); entries != NULL; entries = g_list_next(entries)) {
			g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED_ID, 0, entries->data, TRUE);
		}
	}

	/* Switch */
	iapp->default_app = newdef;

	/* Add new */
	if (iapp->default_app != NULL) {
		for (entries = window_menus_get_entries(iapp->default_app); entries != NULL; entries = g_list_next(entries)) {
			g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED_ID, 0, entries->data, TRUE);
		}
	}

	return;
}

/* A new window wishes to register it's windows with us */
static gboolean
_application_menu_registrar_server_window_register (IndicatorAppmenu * iapp, guint windowid, const GValue * objectpath, DBusGMethodInvocation * method)
{
	if (g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid)) == NULL) {
		WindowMenus * wm = window_menus_new(windowid, (gchar *)g_value_get_boxed(objectpath), dbus_g_method_get_sender(method));
		g_hash_table_insert(iapp->apps, GUINT_TO_POINTER(windowid), wm);

		/* TODO: Check to see if it's the visible window */

		g_signal_emit(G_OBJECT(iapp), signals[WINDOW_REGISTERED], 0, windowid, objectpath, TRUE);
	} else {
		g_warning("Already have a menu for window ID %X with path %s from %s", windowid, (gchar *)g_value_get_boxed(objectpath), dbus_g_method_get_sender(method));
	}

	dbus_g_method_return(method);
	return TRUE;
}

/* Oh, this one is done playing with us. */
static gboolean
_application_menu_registrar_server_window_unregister (IndicatorAppmenu * iapp, guint windowid, const GValue * objectpath, DBusGMethodInvocation * method)
{
	gpointer lookup = g_hash_table_lookup(iapp->apps, GUINT_TO_POINTER(windowid));
	if (lookup != NULL) {
		WindowMenus * wm = (WindowMenus *)lookup;
		if (iapp->default_app == wm) {
			switch_default_app(iapp, NULL);
		}
		g_hash_table_remove(iapp->apps, GUINT_TO_POINTER(windowid));

		g_signal_emit(G_OBJECT(iapp), signals[WINDOW_UNREGISTERED], 0, windowid, objectpath, TRUE);
	} else {
		g_warning("Unable to unregister window ID %X as I don't have it.", windowid);
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
		switch_default_app(INDICATOR_APPMENU(userdata), NULL);
	}

	g_object_unref(proxy);

	return;
}
