/*
DBus facing code for the HUD

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "shared-values.h"
#include "hud.interface.h"
#include "hud-dbus.h"
#include "hud-search.h"

struct _HudDbusPrivate {
	GDBusConnection * bus;
	GCancellable * bus_lookup;
	guint bus_registration;
	HudSearch * search;
};

#define HUD_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_DBUS_TYPE, HudDbusPrivate))

static void hud_dbus_class_init (HudDbusClass *klass);
static void hud_dbus_init       (HudDbus *self);
static void hud_dbus_dispose    (GObject *object);
static void hud_dbus_finalize   (GObject *object);

static void bus_got_cb          (GObject *object, GAsyncResult * res, gpointer user_data);
static void bus_method          (GDBusConnection *connection,
                                 const gchar *sender,
                                 const gchar *object_path,
                                 const gchar *interface_name,
                                 const gchar *method_name,
                                 GVariant *parameters,
                                 GDBusMethodInvocation *invocation,
                                 gpointer user_data);
static GVariant * get_suggestions (HudDbus * self, const gchar * query);
static void execute_query (HudDbus * self, GVariant * key, guint timestamp);


G_DEFINE_TYPE (HudDbus, hud_dbus, G_TYPE_OBJECT);
static GDBusNodeInfo * node_info = NULL;
static GDBusInterfaceInfo * iface_info = NULL;
static GDBusInterfaceVTable bus_vtable = {
	method_call: bus_method,
	get_property: NULL,
	set_property: NULL,
};

static void
hud_dbus_class_init (HudDbusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudDbusPrivate));

	object_class->dispose = hud_dbus_dispose;
	object_class->finalize = hud_dbus_finalize;

	if (node_info == NULL) {
		GError * error = NULL;

		node_info = g_dbus_node_info_new_for_xml(hud_interface, &error);
		if (error != NULL) {
			g_error("Unable to parse HUD interface: %s", error->message);
			g_error_free(error);
		}
	}

	if (node_info != NULL && iface_info == NULL) {
		iface_info = g_dbus_node_info_lookup_interface(node_info, DBUS_IFACE);
		if (iface_info == NULL) {
			g_error("Unable to find interface '" DBUS_IFACE "'");
		}
	}

	return;
}

static void
hud_dbus_init (HudDbus *self)
{
	self->priv = HUD_DBUS_GET_PRIVATE(self);

	self->priv->bus = NULL;
	self->priv->bus_lookup = NULL;
	self->priv->bus_registration = 0;
	self->priv->search = NULL;

	self->priv->bus_lookup = g_cancellable_new();
	g_bus_get(G_BUS_TYPE_SESSION, self->priv->bus_lookup, bus_got_cb, self);

	self->priv->search = hud_search_new();

	return;
}

static void
hud_dbus_dispose (GObject *object)
{
	HudDbus * self = HUD_DBUS(object);
	g_return_if_fail(self != NULL);

	if (self->priv->bus_lookup != NULL) {
		g_cancellable_cancel(self->priv->bus_lookup);
		g_object_unref(self->priv->bus_lookup);
		self->priv->bus_lookup = NULL;
	}

	if (self->priv->bus_registration != 0) {
		g_dbus_connection_unregister_object(self->priv->bus, self->priv->bus_registration);
		self->priv->bus_registration = 0;
	}

	if (self->priv->bus != NULL) {
		g_object_unref(self->priv->bus);
		self->priv->bus = NULL;
	}

	if (self->priv->search != NULL) {
		g_object_unref(self->priv->search);
		self->priv->search = NULL;
	}

	G_OBJECT_CLASS (hud_dbus_parent_class)->dispose (object);
	return;
}

static void
hud_dbus_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_dbus_parent_class)->finalize (object);
	return;
}

HudDbus *
hud_dbus_new (void)
{
	return g_object_new(HUD_DBUS_TYPE, NULL);
}

static void
bus_got_cb (GObject *object, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;
	HudDbus * self = HUD_DBUS(user_data);
	GDBusConnection * bus;

	bus = g_bus_get_finish(res, &error);
	if (error != NULL) {
		g_critical("Unable to get bus: %s", error->message);
		g_error_free(error);
		return;
	}

	self->priv->bus = bus;

	/* Register object */
	self->priv->bus_registration = g_dbus_connection_register_object(bus,
	                                                /* path */       DBUS_PATH,
	                                                /* interface */  iface_info,
	                                                /* vtable */     &bus_vtable,
	                                                /* userdata */   self,
	                                                /* destroy */    NULL,
	                                                /* error */      &error);

	return;
}

static void
bus_method (GDBusConnection *connection, const gchar *sender, const gchar *object_path, const gchar *interface_name, const gchar *method_name, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data)
{
	HudDbus * self = HUD_DBUS(user_data);

	if (g_strcmp0(method_name, "GetSuggestions") == 0) {
		GVariant * ret = NULL;
		gchar * query = NULL;

		g_variant_get(parameters, "(s)", &query);

		ret = get_suggestions(self, query);

		g_dbus_method_invocation_return_value(invocation, ret);
		g_free(query);
	} else if (g_strcmp0(method_name, "ExecuteQuery") == 0) {
		GVariant * key = NULL;
		guint timestamp = 0;

		key = g_variant_get_child_value(parameters, 0);
		g_variant_get_child(parameters, 1, "u", &timestamp);

		execute_query(self, key, timestamp);
		
		g_dbus_method_invocation_return_value(invocation, NULL);
		g_variant_unref(key);
	}

	return;
}

/* Take a path to a desktop file and find its icon */
static gchar *
desktop2icon (gchar * desktop)
{
	g_return_val_if_fail(desktop != NULL, g_strdup(""));

	if (!g_file_test(desktop, G_FILE_TEST_EXISTS)) {
		return g_strdup("");
	}

	/* Try to build an app info from the desktop file
	   path */
	GDesktopAppInfo * appinfo = g_desktop_app_info_new_from_filename(desktop);

	if (!G_IS_DESKTOP_APP_INFO(appinfo)) {
		return g_strdup("");
	}

	/* Get the name out of the icon, note the icon is not
	   ref'd so it doesn't need to be free'd */
	gchar * retval = NULL;
	GIcon * icon = g_app_info_get_icon(G_APP_INFO(appinfo));
	if (icon != NULL) {
		retval = g_icon_to_string(icon);
	} else {
		retval = g_strdup("");
	}

	/* Drop the app info */
	g_object_unref(appinfo);
	appinfo = NULL;

	return retval;
}

/* Respond to the GetSuggestions command from DBus by looking
   in our HUD search object for suggestions from that query */
static GVariant *
get_suggestions (HudDbus * self, const gchar * query)
{
	/* Do the search */
	gchar * desktop = NULL;
	gchar * target = NULL;
	GList * suggestions = hud_search_suggestions(self->priv->search, query, &desktop, &target);

	gchar * icon = NULL;
	icon = desktop2icon(desktop);

	/* Build into into a variant */
	GVariantBuilder ret;
	g_variant_builder_init(&ret, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&ret, g_variant_new_string(target));

	/* Free the strings */
	g_free(target);
	g_free(desktop);

	if (suggestions != NULL) {
		GList * suggestion = suggestions;
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

		while (suggestion != NULL) {
			HudSearchSuggest * suggest = (HudSearchSuggest *)suggestion->data;

			GVariantBuilder tuple;
			g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
			g_variant_builder_add_value(&tuple, g_variant_new_string(hud_search_suggest_get_display(suggest)));
			g_variant_builder_add_value(&tuple, g_variant_new_string(icon));
			g_variant_builder_add_value(&tuple, g_variant_new_string(hud_search_suggest_get_icon(suggest)));
			g_variant_builder_add_value(&tuple, g_variant_new_string(""));
			g_variant_builder_add_value(&tuple, hud_search_suggest_get_key(suggest));

			g_variant_builder_add_value(&builder, g_variant_builder_end(&tuple));

			suggestion = g_list_next(suggestion);
		}

		g_variant_builder_add_value(&ret, g_variant_builder_end(&builder));
	} else {
		/* If we didn't get any suggestions we need to build
		   a null array to make the DBus interface happy */
		g_variant_builder_add_value(&ret, g_variant_new_array(G_VARIANT_TYPE("(ssssv)"), NULL, 0));
	}

	/* Free the remaining icon */
	g_free(icon);

	/* Clean up the list */
	g_list_free_full(suggestions, (GDestroyNotify)hud_search_suggest_free);

	return g_variant_builder_end(&ret);
}

static void
execute_query (HudDbus * self, GVariant * key, guint timestamp)
{
	hud_search_execute(self->priv->search, key, timestamp);
	return;
}
