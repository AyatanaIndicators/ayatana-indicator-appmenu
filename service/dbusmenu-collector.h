#ifndef __DBUSMENU_COLLECTOR_H__
#define __DBUSMENU_COLLECTOR_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define DBUSMENU_COLLECTOR_TYPE            (dbusmenu_collector_get_type ())
#define DBUSMENU_COLLECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DBUSMENU_COLLECTOR_TYPE, DbusmenuCollector))
#define DBUSMENU_COLLECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DBUSMENU_COLLECTOR_TYPE, DbusmenuCollectorClass))
#define IS_DBUSMENU_COLLECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DBUSMENU_COLLECTOR_TYPE))
#define IS_DBUSMENU_COLLECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DBUSMENU_COLLECTOR_TYPE))
#define DBUSMENU_COLLECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DBUSMENU_COLLECTOR_TYPE, DbusmenuCollectorClass))

typedef struct _DbusmenuCollector          DbusmenuCollector;
typedef struct _DbusmenuCollectorClass     DbusmenuCollectorClass;
typedef struct _DbusmenuCollectorPrivate   DbusmenuCollectorPrivate;

struct _DbusmenuCollectorClass {
	GObjectClass parent_class;
};

struct _DbusmenuCollector {
	GObject parent;

	DbusmenuCollectorPrivate * priv;
};

GType dbusmenu_collector_get_type (void);
DbusmenuCollector * dbusmenu_collector_new (void);
GStrv dbusmenu_collector_search (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, GStrv tokens);
gboolean dbusmenu_collector_exec (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, GStrv tokens);

G_END_DECLS

#endif
