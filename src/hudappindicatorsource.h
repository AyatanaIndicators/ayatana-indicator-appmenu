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

#ifndef __HUD_APP_INDICATOR_SOURCE_H__
#define __HUD_APP_INDICATOR_SOURCE_H__

#include <glib-object.h>

#define HUD_TYPE_APP_INDICATOR_SOURCE                       (hud_app_indicator_source_get_type ())
#define HUD_APP_INDICATOR_SOURCE(inst)                      (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_APP_INDICATOR_SOURCE, HudAppIndicatorSource))
#define HUD_IS_APP_INDICATOR_SOURCE(inst)                   (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_APP_INDICATOR_SOURCE))

typedef struct _HudAppIndicatorSource                       HudAppIndicatorSource;

GType                   hud_app_indicator_source_get_type                   (void);

HudAppIndicatorSource * hud_app_indicator_source_new                        (void);

#endif /* __HUD_APP_INDICATOR_SOURCE_H__ */
