/*
An implementation of indicator object showing menus from applications.

Copyright 2010 Canonical Ltd.

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

#ifndef __WINDOW_MENUS_H__
#define __WINDOW_MENUS_H__

#include <glib-object.h>
#include <libindicator/indicator-object.h>
#include <libdbusmenu-glib/client.h>
#include "window-menu.h"

G_BEGIN_DECLS

#define WINDOW_MENUS_TYPE            (window_menus_get_type ())
#define WINDOW_MENUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENUS_TYPE, WindowMenus))
#define WINDOW_MENUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENUS_TYPE, WindowMenusClass))
#define IS_WINDOW_MENUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENUS_TYPE))
#define IS_WINDOW_MENUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENUS_TYPE))
#define WINDOW_MENUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENUS_TYPE, WindowMenusClass))

#define WINDOW_MENUS_SIGNAL_ENTRY_ADDED    "entry-added"
#define WINDOW_MENUS_SIGNAL_ENTRY_REMOVED  "entry-removed"
#define WINDOW_MENUS_SIGNAL_ERROR_STATE    "error-state"
#define WINDOW_MENUS_SIGNAL_STATUS_CHANGED "status-changed"
#define WINDOW_MENUS_SIGNAL_SHOW_MENU      "show-menu"
#define WINDOW_MENUS_SIGNAL_A11Y_UPDATE    "a11y-update"

typedef struct _WindowMenus      WindowMenus;
typedef struct _WindowMenusClass WindowMenusClass;

struct _WindowMenusClass {
	WindowMenuClass parent_class;
};

struct _WindowMenus {
	WindowMenu parent;
};

GType window_menus_get_type (void);
WindowMenus * window_menus_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object);
GList * window_menus_get_entries (WindowMenus * wm);
guint window_menus_get_location (WindowMenus * wm, IndicatorObjectEntry * entry);

guint window_menus_get_xid (WindowMenus * wm);
gchar * window_menus_get_path (WindowMenus * wm);
gchar * window_menus_get_address (WindowMenus * wm);

gboolean window_menus_get_error_state (WindowMenus * wm);
DbusmenuStatus window_menus_get_status (WindowMenus * wm);
void window_menus_entry_restore (WindowMenus * wm, IndicatorObjectEntry * entry);

void window_menus_entry_activate (WindowMenus * wm, IndicatorObjectEntry * entry, guint timestamp);

G_END_DECLS

#endif
