#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>

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

static GVariant *
get_suggestions (HudDbus * self, const gchar * query)
{
	GVariantBuilder ret;

	g_variant_builder_init(&ret, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&ret, g_variant_new_string("New Document"));

	GList * suggestions = hud_search_suggestions(self->priv->search, query);

	if (suggestions != NULL) {
		GList * suggestion = suggestions;
		GVariantBuilder builder;
		g_variant_builder_init(&builder, G_VARIANT_TYPE_ARRAY);

		while (suggestion != NULL) {
			HudSearchSuggest * suggest = (HudSearchSuggest *)suggestion->data;

			GVariantBuilder tuple;
			g_variant_builder_init(&tuple, G_VARIANT_TYPE_TUPLE);
			g_variant_builder_add_value(&tuple, g_variant_new_string(hud_search_suggest_get_display(suggest)));
			g_variant_builder_add_value(&tuple, g_variant_new_string(hud_search_suggest_get_icon(suggest)));
			g_variant_builder_add_value(&tuple, hud_search_suggest_get_key(suggest));

			g_variant_builder_add_value(&builder, g_variant_builder_end(&tuple));

			suggestion = g_list_next(suggestion);
		}

		g_variant_builder_add_value(&ret, g_variant_builder_end(&builder));
	} else {
		g_variant_builder_add_value(&ret, g_variant_new_array(G_VARIANT_TYPE("(ssv)"), NULL, 0));
	}

	g_list_free_full(suggestions, (GDestroyNotify)hud_search_suggest_free);

	return g_variant_builder_end(&ret);
}

static void
execute_query (HudDbus * self, GVariant * key, guint timestamp)
{
	hud_search_execute(self->priv->search, key, timestamp);
	return;
}
