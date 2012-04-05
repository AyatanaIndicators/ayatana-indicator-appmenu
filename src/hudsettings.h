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

#ifndef __HUD_SETTINGS_H__
#define __HUD_SETTINGS_H__

#include <glib.h>

typedef struct _HudSettings HudSettings;

struct _HudSettings
{
  gboolean store_usage_data;

  guint indicator_penalty;
  guint max_distance;

  guint add_penalty;
  guint drop_penalty;
  guint end_drop_penalty;
  guint swap_penalty;
};

extern HudSettings hud_settings;

void                    hud_settings_init                               (void);

#endif /* __HUD_SETTINGS_H__ */
