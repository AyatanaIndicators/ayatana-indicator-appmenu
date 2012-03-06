#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-menu.h"

#define WINDOW_MENU_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_TYPE, WindowMenuPrivate))

static void window_menu_class_init (WindowMenuClass *klass);
static void window_menu_init       (WindowMenu *self);
static void window_menu_dispose    (GObject *object);
static void window_menu_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenu, window_menu, G_TYPE_OBJECT);

static void
window_menu_class_init (WindowMenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = window_menu_dispose;
	object_class->finalize = window_menu_finalize;

	return;
}

static void
window_menu_init (WindowMenu *self)
{

	return;
}

static void
window_menu_dispose (GObject *object)
{

	G_OBJECT_CLASS (window_menu_parent_class)->dispose (object);
	return;
}

static void
window_menu_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menu_parent_class)->finalize (object);
	return;
}
