#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hud-dbus.h"

struct _HudDbusPrivate {
	int dummy;
};

#define HUD_DBUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_DBUS_TYPE, HudDbusPrivate))

static void hud_dbus_class_init (HudDbusClass *klass);
static void hud_dbus_init       (HudDbus *self);
static void hud_dbus_dispose    (GObject *object);
static void hud_dbus_finalize   (GObject *object);

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

	self->priv->dummy = 0;

	return;
}

static void
hud_dbus_dispose (GObject *object)
{

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
