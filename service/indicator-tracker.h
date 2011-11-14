#ifndef __INDICATOR_TRACKER_H__
#define __INDICATOR_TRACKER_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define INDICATOR_TRACKER_TYPE            (indicator_tracker_get_type ())
#define INDICATOR_TRACKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_TRACKER_TYPE, IndicatorTracker))
#define INDICATOR_TRACKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_TRACKER_TYPE, IndicatorTrackerClass))
#define IS_INDICATOR_TRACKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_TRACKER_TYPE))
#define IS_INDICATOR_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_TRACKER_TYPE))
#define INDICATOR_TRACKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_TRACKER_TYPE, IndicatorTrackerClass))

typedef struct _IndicatorTracker          IndicatorTracker;
typedef struct _IndicatorTrackerClass     IndicatorTrackerClass;
typedef struct _IndicatorTrackerPrivate   IndicatorTrackerPrivate;
typedef struct _IndicatorTrackerIndicator IndicatorTrackerIndicator;

struct _IndicatorTrackerClass {
	GObjectClass parent_class;
};

struct _IndicatorTracker {
	GObject parent;
	IndicatorTrackerPrivate * priv;
};

struct _IndicatorTrackerIndicator {
	gchar * name;
	gchar * dbus_name;
	gchar * dbus_name_wellknown;
	gchar * dbus_object;
	gchar * prefix;
};

GType indicator_tracker_get_type (void);
IndicatorTracker * indicator_tracker_new (void);
GArray * indicator_tracker_get_indicators (IndicatorTracker * tracker);

G_END_DECLS

#endif
