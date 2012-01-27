/*
Tracks which menu items get used by users and works to promote those
higher in the search rankings than others.

Copyright 2011 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __USAGE_TRACKER_H__
#define __USAGE_TRACKER_H__

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
