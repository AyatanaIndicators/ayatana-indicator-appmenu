#ifndef __WINDOW_MENU_MODEL_H__
#define __WINDOW_MENU_MODEL_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define WINDOW_MENU_MODEL_TYPE            (window_menu_model_get_type ())
#define WINDOW_MENU_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENU_MODEL_TYPE, WindowMenuModel))
#define WINDOW_MENU_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENU_MODEL_TYPE, WindowMenuModelClass))
#define IS_WINDOW_MENU_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENU_MODEL_TYPE))
#define IS_WINDOW_MENU_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENU_MODEL_TYPE))
#define WINDOW_MENU_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENU_MODEL_TYPE, WindowMenuModelClass))

typedef struct _WindowMenuModel        WindowMenuModel;
typedef struct _WindowMenuModelClass   WindowMenuModelClass;
typedef struct _WindowMenuModelPrivate WindowMenuModelPrivate;

struct _WindowMenuModelClass {
	GObjectClass parent_class;
};

struct _WindowMenuModel {
	GObject parent;

	WindowMenuModelPrivate * priv;
};

GType window_menu_model_get_type (void);

G_END_DECLS

#endif
