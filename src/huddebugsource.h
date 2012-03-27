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

#ifndef __HUD_DEBUG_SOURCE_H__
#define __HUD_DEBUG_SOURCE_H__

#include <glib-object.h>

#define HUD_TYPE_DEBUG_SOURCE                               (hud_debug_source_get_type ())
#define HUD_DEBUG_SOURCE(inst)                              (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_DEBUG_SOURCE, HudDebugSource))
#define HUD_IS_DEBUG_SOURCE(inst)                           (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_DEBUG_SOURCE))

typedef struct _HudDebugSource                              HudDebugSource;

GType                   hud_debug_source_get_type                   (void);

HudDebugSource *        hud_debug_source_new                        (void);

#endif /* __HUD_DEBUG_SOURCE_H__ */
