/**
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
 **/

#ifndef __HUD_SOURCE_LIST_H__
#define __HUD_SOURCE_LIST_H__

#include "hudsource.h"

#define HUD_TYPE_SOURCE_LIST                                (hud_source_list_get_type ())
#define HUD_SOURCE_LIST(inst)                               (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_SOURCE_LIST, HudSourceList))
#define HUD_IS_SOURCE_LIST(inst)                            (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_SOURCE_LIST))

typedef struct _HudSourceList                                   HudSourceList;

GType                   hud_source_list_get_type                        (void);

HudSourceList *         hud_source_list_new                             (void);

void                    hud_source_list_add                             (HudSourceList *list,
                                                                         HudSource     *source);

#endif /* __HUD_SOURCE_LIST_H__ */
