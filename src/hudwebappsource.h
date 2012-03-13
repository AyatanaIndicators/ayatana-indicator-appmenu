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

#ifndef __HUD_WEBAPP_SOURCE_H__
#define __HUD_WEBAPP_SOURCE_H__

#include <glib-object.h>
#include "hudwindowsource.h"

#define HUD_TYPE_WEBAPP_SOURCE                           (hud_webapp_source_get_type ())
#define HUD_WEBAPP_SOURCE(inst)                          (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_WEBAPP_SOURCE, HudWebappSource))
#define HUD_IS_WEBAPP_SOURCE(inst)                       (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_WEBAPP_SOURCE))

typedef struct _HudWebappSource                          HudWebappSource;

GType                   hud_webapp_source_get_type                   (void);
HudWebappSource *    hud_webapp_source_new                        (HudWindowSource *window_source);

#endif /* __HUD_WEBAPP_SOURCE_H__ */
