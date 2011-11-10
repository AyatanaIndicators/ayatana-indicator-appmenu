#ifndef __USAGE_TRACKER_H__
#define __USAGE_TRACKER_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define USAGE_TRACKER_TYPE            (usage_tracker_get_type ())
#define USAGE_TRACKER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), USAGE_TRACKER_TYPE, UsageTracker))
#define USAGE_TRACKER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), USAGE_TRACKER_TYPE, UsageTrackerClass))
#define IS_USAGE_TRACKER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), USAGE_TRACKER_TYPE))
#define IS_USAGE_TRACKER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), USAGE_TRACKER_TYPE))
#define USAGE_TRACKER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), USAGE_TRACKER_TYPE, UsageTrackerClass))

typedef struct _UsageTracker        UsageTracker;
typedef struct _UsageTrackerClass   UsageTrackerClass;
typedef struct _UsageTrackerPrivate UsageTrackerPrivate;

struct _UsageTrackerClass {
	GObjectClass parent_class;
};

struct _UsageTracker {
	GObject parent;

	UsageTrackerPrivate * priv;
};

GType usage_tracker_get_type (void);
UsageTracker * usage_tracker_new (void);
void usage_tracker_mark_usage (UsageTracker * self, const gchar * application, const gchar * entry);
guint usage_tracker_get_usage (UsageTracker * self, const gchar * application, const gchar * entry);

G_END_DECLS

#endif
