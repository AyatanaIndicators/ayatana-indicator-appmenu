#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>

#include "hud-dbus.h"

struct _HudDbusPrivate {
	GDBusConnection * bus;
	GCancellable * bus_lookup;
	guint bus_registration;
};

#define HUD_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_DBUS_TYPE, HudDbusPrivate))

static void hud_dbus_class_init (HudDbusClass *klass);
static void hud_dbus_init       (HudDbus *self);
static void hud_dbus_dispose    (GObject *object);
static void hud_dbus_finalize   (GObject *object);

static void bus_got_cb          (GObject *object, GAsyncResult * res, gpointer user_data);

G_DEFINE_TYPE (HudDbus, hud_dbus, G_TYPE_OBJECT);

static void
hud_dbus_class_init (HudDbusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudDbusPrivate));

	object_class->dispose = hud_dbus_dispose;
	object_class->finalize = hud_dbus_finalize;

	return;
}

static void
hud_dbus_init (HudDbus *self)
{
	self->priv = HUD_DBUS_GET_PRIVATE(self);

	self->priv->bus = NULL;
	self->priv->bus_lookup = NULL;
	self->priv->bus_registration = 0;

	self->priv->bus_lookup = g_cancellable_new();
	g_bus_get(G_BUS_TYPE_SESSION, self->priv->bus_lookup, bus_got_cb, self);

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

	if (self->priv->bus != NULL) {
		g_object_unref(self->priv->bus);
		self->priv->bus = NULL;
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

	return;
}
