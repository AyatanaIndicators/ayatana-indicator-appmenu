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

#include "window-menu.h"

G_BEGIN_DECLS

#define WINDOW_MENUS_TYPE            (window_menus_get_type ())
#define WINDOW_MENUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENUS_TYPE, WindowMenus))
#define WINDOW_MENUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENUS_TYPE, WindowMenusClass))
#define IS_WINDOW_MENUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENUS_TYPE))
#define IS_WINDOW_MENUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENUS_TYPE))
#define WINDOW_MENUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENUS_TYPE, WindowMenusClass))

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

G_END_DECLS

#endif
