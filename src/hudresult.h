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

#ifndef __HUD_RESULT_H__
#define __HUD_RESULT_H__

#include "huditem.h"

#define HUD_TYPE_RESULT                                     (hud_result_get_type ())
#define HUD_RESULT(inst)                                    (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_RESULT, HudResult))
#define HUD_IS_RESULT(inst)                                 (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_RESULT))

typedef struct _HudResult                                   HudResult;

GType                   hud_result_get_type                             (void);

HudResult *             hud_result_new                                  (HudItem     *item,
                                                                         const gchar *search_string);

HudResult *             hud_result_get_if_matched                       (HudItem     *item,
                                                                         const gchar *search_string,
                                                                         guint        max_distance);

HudItem *               hud_result_get_item                             (HudResult   *result);
guint                   hud_result_get_distance                         (HudResult   *result,
                                                                         guint        max_usage);
const gchar *           hud_result_get_html_description                 (HudResult   *result);


#endif /* __HUD_RESULT_H__ */
