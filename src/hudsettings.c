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

/**
 * SECTION:hudsettings
 * @title: HudSettings
 * @short_description: tunable parameters
 *
 * #HudSettings is a structure that contains the value of several
 * tunable parameters that affect the behaviour of various components of
 * the HUD.
 *
 * This structure exists for two reasons.
 *
 * The first reason is that caching these values in local variables
 * eliminates the need to look them up from #GSettings on each use.
 * This vastly improves the performance of the matching algorithms (as
 * many of these values are used quite a lot from within them).
 *
 * The second reason is to improve testability.  The testcases are able
 * to hardcode sane values for the settings without worrying about
 * changes that the user may have made to their local configuration
 * (which could otherwise cause spurious test failures).
 **/

/**
 * HudSettings:
 * @store_usage_data: if usage tracking should be performed
 * @indicator_penalty: the percentage by which to increase the distance
 *   of indicators when sorting the results list
 * @max_distance: the maximum distance value we consider as being a
 *   matching result
 * @add_penalty: the penalty incurred by a character in the search term
 *   that does not exist in the item being matched
 * @add_penalty_pre: the penalty incurred by a character in the search
 *   term that does not exist in the item being matched when that
 *   character comes at the beginning of the term
 * @drop_penalty: the penalty incurred by a character missing from the
 *   search string as compared to the item being matched
 * @drop_penalty_end: the penalty incurred by a character missing from
 *   the search string as compared to the item being matched when the
 *   character is at the end of the item (ie: the search term is a
 *   prefix of the item)
 * @transpose_penalty: the penalty incurred for transposed characters
 * @swap_penalty: the penalty incurred for the substitution of one
 *   character for another
 * @swap_penalty_case: the penalty incurred for the substitution of one
 *   character for another when the characters differ only in case
 *
 * This structure contains the value of several tunable parameters that
 * affect the behaviour of various components of the HUD.
 **/

/**
 * hud_settings:
 *
 * The #HudSettings in effect.
 *
 * hud_settings_init() can be used to keep these values in sync with
 * #GSettings.  For testing, it may make sense to set these values
 * directly.
 **/
HudSettings hud_settings;

static void
hud_settings_refresh (GSettings *settings)
{
  g_debug ("refreshing hud settings");

  hud_settings.store_usage_data = g_settings_get_boolean (settings, "store-usage-data");

  g_debug ("store-usage-data: %s", hud_settings.store_usage_data ? "true" : "false");
}

static void
hud_search_settings_refresh (GSettings *settings)
{
  g_debug ("refreshing search settings");

  hud_settings.indicator_penalty = g_settings_get_uint (settings, "indicator-penalty");
  hud_settings.max_distance = g_settings_get_uint (settings, "max-distance");
  hud_settings.add_penalty = g_settings_get_uint (settings, "add-penalty");
  hud_settings.add_penalty_pre = g_settings_get_uint (settings, "add-penalty-pre");
  hud_settings.drop_penalty = g_settings_get_uint (settings, "drop-penalty");
  hud_settings.drop_penalty_end = g_settings_get_uint (settings, "drop-penalty-end");
  hud_settings.transpose_penalty = g_settings_get_uint (settings, "transpose-penalty");
  hud_settings.swap_penalty = g_settings_get_uint (settings, "swap-penalty");
  hud_settings.swap_penalty_case = g_settings_get_uint (settings, "swap-penalty-case");

  g_debug ("indicator penalty: %u, max distance: %u",
           hud_settings.indicator_penalty, hud_settings.max_distance);
  g_debug ("penalties: add:%u add-pre:%u drop:%u drop-end:%u trans:%u swap:%u swap-case:%u",
           hud_settings.add_penalty, hud_settings.add_penalty_pre, hud_settings.drop_penalty,
           hud_settings.drop_penalty_end, hud_settings.transpose_penalty, hud_settings.swap_penalty,
           hud_settings.swap_penalty_case);
}

/**
 * hud_settings_init:
 *
 * Initialises the #HudSettings using #GSettings and keeps it in sync.
 *
 * If #GSettings indicates that the settings have changed, they will be
 * updated.
 **/
void
hud_settings_init (void)
{
  GSettings *settings;

  settings = g_settings_new ("com.canonical.indicator.appmenu.hud");
  g_signal_connect (settings, "change-event", G_CALLBACK (hud_settings_refresh), NULL);
  hud_settings_refresh (settings);

  settings = g_settings_new ("com.canonical.indicator.appmenu.hud.search");
  g_signal_connect (settings, "change-event", G_CALLBACK (hud_search_settings_refresh), NULL);
  hud_search_settings_refresh (settings);
}
