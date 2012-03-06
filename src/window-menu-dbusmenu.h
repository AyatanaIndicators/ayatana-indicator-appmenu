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

#ifndef __WINDOW_MENU_DBUSMENU_H__
#define __WINDOW_MENU_DBUSMENU_H__

#include "window-menu.h"

G_BEGIN_DECLS

#define WINDOW_MENU_DBUSMENU_TYPE            (window_menu_dbusmenu_get_type ())
#define WINDOW_MENU_DBUSMENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENU_DBUSMENU_TYPE, WindowMenuDbusmenu))
#define WINDOW_MENU_DBUSMENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENU_DBUSMENU_TYPE, WindowMenuDbusmenuClass))
#define IS_WINDOW_MENU_DBUSMENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENU_DBUSMENU_TYPE))
#define IS_WINDOW_MENU_DBUSMENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENU_DBUSMENU_TYPE))
#define WINDOW_MENU_DBUSMENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENU_DBUSMENU_TYPE, WindowMenuDbusmenuClass))

typedef struct _WindowMenuDbusmenu      WindowMenuDbusmenu;
typedef struct _WindowMenuDbusmenuClass WindowMenuDbusmenuClass;

struct _WindowMenuDbusmenuClass {
	WindowMenuClass parent_class;
};

struct _WindowMenuDbusmenu {
	WindowMenu parent;
};

GType window_menu_dbusmenu_get_type (void);
WindowMenuDbusmenu * window_menu_dbusmenu_new (const guint windowid, const gchar * dbus_addr, const gchar * dbus_object);

G_END_DECLS

#endif
