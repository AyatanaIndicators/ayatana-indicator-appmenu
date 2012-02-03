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

#ifndef __MENUITEM_COLLECTOR_H__
#define __MENUITEM_COLLECTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define MENUITEM_COLLECTOR_TYPE            (menuitem_collector_get_type ())
#define MENUITEM_COLLECTOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MENUITEM_COLLECTOR_TYPE, MenuitemCollector))
#define MENUITEM_COLLECTOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MENUITEM_COLLECTOR_TYPE, MenuitemCollectorClass))
#define IS_MENUITEM_COLLECTOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MENUITEM_COLLECTOR_TYPE))
#define IS_MENUITEM_COLLECTOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MENUITEM_COLLECTOR_TYPE))
#define MENUITEM_COLLECTOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MENUITEM_COLLECTOR_TYPE, MenuitemCollectorClass))

typedef struct _MenuitemCollector          MenuitemCollector;
typedef struct _MenuitemCollectorClass     MenuitemCollectorClass;
typedef struct _MenuitemCollectorPrivate   MenuitemCollectorPrivate;
typedef struct _MenuitemCollectorFound     MenuitemCollectorFound;

struct _MenuitemCollectorClass {
	GObjectClass parent_class;
};

struct _MenuitemCollector {
	GObject parent;

	MenuitemCollectorPrivate * priv;
};

GType menuitem_collector_get_type (void);
MenuitemCollector * menuitem_collector_new (void);
GList * menuitem_collector_search (MenuitemCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * prefix, const gchar * search);
void menuitem_collector_execute (MenuitemCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, gint id, guint timestamp);

guint menuitem_collector_found_get_distance (MenuitemCollectorFound * found);
void menuitem_collector_found_set_distance (MenuitemCollectorFound * found, guint distance);
const gchar * menuitem_collector_found_get_display (MenuitemCollectorFound * found);
const gchar * menuitem_collector_found_get_db (MenuitemCollectorFound * found);
void menuitem_collector_found_free (MenuitemCollectorFound * found);
void menuitem_collector_found_list_free (GList * found_list);
const gchar *  menuitem_collector_found_get_indicator (MenuitemCollectorFound * found);
void menuitem_collector_found_set_indicator (MenuitemCollectorFound * found, const gchar * indicator);
const gchar * menuitem_collector_found_get_dbus_addr (MenuitemCollectorFound * found);
const gchar * menuitem_collector_found_get_dbus_path (MenuitemCollectorFound * found);
gint menuitem_collector_found_get_dbus_id (MenuitemCollectorFound * found);
const gchar * menuitem_collector_found_get_app_icon (MenuitemCollectorFound * found);
void menuitem_collector_found_set_app_icon (MenuitemCollectorFound * found, const gchar * app_icon);

G_END_DECLS

#endif
