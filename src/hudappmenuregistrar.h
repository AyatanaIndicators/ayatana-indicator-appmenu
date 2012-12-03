/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#ifndef __HUD_APP_MENU_REGISTRAR_H__
#define __HUD_APP_MENU_REGISTRAR_H__

#include <glib-object.h>

#define HUD_TYPE_APP_MENU_REGISTRAR                         (hud_app_menu_registrar_get_type ())
#define HUD_APP_MENU_REGISTRAR(inst)                        (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_APP_MENU_REGISTRAR,                            \
                                                             HudAppMenuRegistrar))
#define HUD_IS_APP_MENU_REGISTRAR(inst)                     (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_APP_MENU_REGISTRAR))

typedef struct _HudAppMenuRegistrar                         HudAppMenuRegistrar;

GType                   hud_app_menu_registrar_get_type                 (void);

HudAppMenuRegistrar *   hud_app_menu_registrar_get                      (void);

typedef void         (* HudAppMenuRegistrarObserverFunc)                (HudAppMenuRegistrar             *registrar,
                                                                         guint                            xid,
                                                                         const gchar                     *bus_name,
                                                                         const gchar                     *object_path,
                                                                         gpointer                         user_data);

void                    hud_app_menu_registrar_add_observer             (HudAppMenuRegistrar             *registrar,
                                                                         guint                            xid,
                                                                         HudAppMenuRegistrarObserverFunc  callback,
                                                                         gpointer                         user_data);

void                    hud_app_menu_registrar_remove_observer          (HudAppMenuRegistrar             *registrar,
                                                                         guint                            xid,
                                                                         HudAppMenuRegistrarObserverFunc  callback,
                                                                         gpointer                         user_data);

#endif /* __HUD_APP_MENU_REGISTRAR_H__ */
