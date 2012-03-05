#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-menu-model.h"

struct _WindowMenuModelPrivate {
	gint dummy;
};

#define WINDOW_MENU_MODEL_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_MODEL_TYPE, WindowMenuModelPrivate))

static void window_menu_model_class_init (WindowMenuModelClass *klass);
static void window_menu_model_init       (WindowMenuModel *self);
static void window_menu_model_dispose    (GObject *object);
static void window_menu_model_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenuModel, window_menu_model, G_TYPE_OBJECT);

static void
window_menu_model_class_init (WindowMenuModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenuModelPrivate));

	object_class->dispose = window_menu_model_dispose;
	object_class->finalize = window_menu_model_finalize;

	return;
}

static void
window_menu_model_init (WindowMenuModel *self)
{
	self->priv = WINDOW_MENU_MODEL_GET_PRIVATE(self);

	return;
}

static void
window_menu_model_dispose (GObject *object)
{


	G_OBJECT_CLASS (window_menu_model_parent_class)->dispose (object);
	return;
}

static void
window_menu_model_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menu_model_parent_class)->finalize (object);
	return;
}
