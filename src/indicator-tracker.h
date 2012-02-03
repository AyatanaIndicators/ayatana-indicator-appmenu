/*
Tracks the various indicators to know when they come on and off
the bus for searching their menus.

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
	gchar * icon;
};

GType indicator_tracker_get_type (void);
IndicatorTracker * indicator_tracker_new (void);
GList * indicator_tracker_get_indicators (IndicatorTracker * tracker);

G_END_DECLS

#endif
