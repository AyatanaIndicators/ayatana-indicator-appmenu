/*
DBus facing code for the HUD

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

#ifndef __HUD_DBUS_H__
#define __HUD_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HUD_DBUS_TYPE            (hud_dbus_get_type ())
#define HUD_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HUD_DBUS_TYPE, HudDbus))
#define HUD_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HUD_DBUS_TYPE, HudDbusClass))
#define IS_HUD_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HUD_DBUS_TYPE))
#define IS_HUD_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HUD_DBUS_TYPE))
#define HUD_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HUD_DBUS_TYPE, HudDbusClass))

typedef struct _HudDbus        HudDbus;
typedef struct _HudDbusClass   HudDbusClass;
typedef struct _HudDbusPrivate HudDbusPrivate;

struct _HudDbusClass {
	GObjectClass parent_class;
};

struct _HudDbus {
	GObject parent;
	HudDbusPrivate * priv;
};

GType hud_dbus_get_type (void);
HudDbus * hud_dbus_new (void);

G_END_DECLS

#endif
