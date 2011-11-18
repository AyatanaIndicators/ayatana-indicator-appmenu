/*
An object to collect the various DBusmenu objects that exist
on dbus.

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
typedef struct _DbusmenuCollectorFound     DbusmenuCollectorFound;

struct _DbusmenuCollectorClass {
	GObjectClass parent_class;
};

struct _DbusmenuCollector {
	GObject parent;

	DbusmenuCollectorPrivate * priv;
};

GType dbusmenu_collector_get_type (void);
DbusmenuCollector * dbusmenu_collector_new (void);
GList * dbusmenu_collector_search (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * search);
void dbusmenu_collector_execute (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, gint id, guint timestamp);

guint dbusmenu_collector_found_get_distance (DbusmenuCollectorFound * found);
const gchar * dbusmenu_collector_found_get_display (DbusmenuCollectorFound * found);
void dbusmenu_collector_found_free (DbusmenuCollectorFound * found);
void dbusmenu_collector_found_list_free (GList * found_list);
const gchar *  dbusmenu_collector_found_get_indicator (DbusmenuCollectorFound * found);
const gchar * dbusmenu_collector_found_get_dbus_addr (DbusmenuCollectorFound * found);
const gchar * dbusmenu_collector_found_get_dbus_path (DbusmenuCollectorFound * found);
gint dbusmenu_collector_found_get_dbus_id (DbusmenuCollectorFound * found);

G_END_DECLS

#endif
