#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "hud-search.h"

struct _HudSearchPrivate {
	int dummy;
};

#define HUD_SEARCH_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_SEARCH_TYPE, HudSearchPrivate))

static void hud_search_class_init (HudSearchClass *klass);
static void hud_search_init       (HudSearch *self);
static void hud_search_dispose    (GObject *object);
static void hud_search_finalize   (GObject *object);

G_DEFINE_TYPE (HudSearch, hud_search, G_TYPE_OBJECT);

static void
hud_search_class_init (HudSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudSearchPrivate));

	object_class->dispose = hud_search_dispose;
	object_class->finalize = hud_search_finalize;

	return;
}

static void
hud_search_init (HudSearch *self)
{
	self->priv = HUD_SEARCH_GET_PRIVATE(self);

	return;
}

static void
hud_search_dispose (GObject *object)
{

	G_OBJECT_CLASS (hud_search_parent_class)->dispose (object);
	return;
}

static void
hud_search_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_search_parent_class)->finalize (object);
	return;
}

HudSearch *
hud_search_new (void)
{
	HudSearch * ret = HUD_SEARCH(g_object_new(HUD_SEARCH_TYPE, NULL));
	return ret;
}
