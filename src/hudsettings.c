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

#define G_LOG_DOMAIN "hudsettings"

#include "hudsettings.h"

#include <gio/gio.h>

static void
hud_settings_refresh (GSettings *settings)
{
  g_debug ("refreshing hud settings");

  hud_settings.indicator_penalty = g_settings_get_uint (settings, "indicator-penalty");
  hud_settings.max_distance = g_settings_get_uint (settings, "max-distance");
  hud_settings.add_penalty = g_settings_get_uint (settings, "add-penalty");
  hud_settings.add_penalty_pre = g_settings_get_uint (settings, "add-penalty-pre");
  hud_settings.drop_penalty = g_settings_get_uint (settings, "drop-penalty");
  hud_settings.drop_penalty_end = g_settings_get_uint (settings, "drop-penalty-end");
  hud_settings.transpose_penalty = g_settings_get_uint (settings, "transpose-penalty");
  hud_settings.swap_penalty = g_settings_get_uint (settings, "swap-penalty");
  hud_settings.swap_penalty_case = g_settings_get_uint (settings, "swap-penalty-case");
}

void
hud_settings_init (void)
{
  GSettings *settings;

  settings = g_settings_new ("com.canonical.indicator.appmenu.hud.search");
  g_signal_connect (settings, "change-event", G_CALLBACK (hud_settings_refresh), NULL);
  hud_settings_refresh (settings);
}
