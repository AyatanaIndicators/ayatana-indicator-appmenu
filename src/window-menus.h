#ifndef __WINDOW_MENUS_H__
#define __WINDOW_MENUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define WINDOW_MENUS_TYPE            (window_menus_get_type ())
#define WINDOW_MENUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENUS_TYPE, WindowMenus))
#define WINDOW_MENUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENUS_TYPE, WindowMenusClass))
#define IS_WINDOW_MENUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENUS_TYPE))
#define IS_WINDOW_MENUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENUS_TYPE))
#define WINDOW_MENUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENUS_TYPE, WindowMenusClass))

typedef struct _WindowMenus      WindowMenus;
typedef struct _WindowMenusClass WindowMenusClass;

struct _WindowMenusClass {
	GObjectClass parent_class;
};

struct _WindowMenus {
	GObject parent;
};

GType window_menus_get_type (void);
WindowMenus * window_menus_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object);
GList * window_menus_get_entries (WindowMenus * wm);

G_END_DECLS

#endif
