#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-menus.h"

typedef struct _WindowMenusPrivate WindowMenusPrivate;
struct _WindowMenusPrivate {
	guint windowid;
};

#define WINDOW_MENUS_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENUS_TYPE, WindowMenusPrivate))

static void window_menus_class_init (WindowMenusClass *klass);
static void window_menus_init       (WindowMenus *self);
static void window_menus_dispose    (GObject *object);
static void window_menus_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenus, window_menus, G_TYPE_OBJECT);

/* Build the one-time class */
static void
window_menus_class_init (WindowMenusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenusPrivate));

	object_class->dispose = window_menus_dispose;
	object_class->finalize = window_menus_finalize;

	return;
}

/* Initialize the per-instance data */
static void
window_menus_init (WindowMenus *self)
{

	return;
}

/* Destroy objects */
static void
window_menus_dispose (GObject *object)
{

	G_OBJECT_CLASS (window_menus_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
window_menus_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menus_parent_class)->finalize (object);
	return;
}

WindowMenus *
window_menus_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object)
{

	return NULL;
}
