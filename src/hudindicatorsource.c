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

#include "hudindicatorsource.h"

#include "indicator-tracker.h"
#include "hudsource.h"

/**
 * SECTION:hudindicatorsource
 * @title:HudIndicatorSource
 * @short_description: a #HudSource to search through the menus of
 *   indicators
 *
 * @HudIndicatorSource searches through the menu items of the
 * indicators.
 *
 * Actually, that's a lie.  Presently it does nothing at all.
 **/

/**
 * HudIndicatorSource:
 *
 * This is an opaque structure type.
 **/
struct _HudIndicatorSource
{
  GObject parent_instance;

  IndicatorTracker *tracker;
};

typedef GObjectClass HudIndicatorSourceClass;

static void hud_indicator_source_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudIndicatorSource, hud_indicator_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_indicator_source_iface_init))

static void
hud_indicator_source_search (HudSource   *hud_source,
                             GPtrArray   *results_array,
                             const gchar *search_string)
{
}

static void
hud_indicator_source_finalize (GObject *object)
{
  HudIndicatorSource *source = HUD_INDICATOR_SOURCE (object);

  g_object_unref (source->tracker);

  G_OBJECT_CLASS (hud_indicator_source_parent_class)
    ->finalize (object);
}

static void
hud_indicator_source_init (HudIndicatorSource *source)
{
}

static void
hud_indicator_source_iface_init (HudSourceInterface *iface)
{
  iface->search = hud_indicator_source_search;
}

static void
hud_indicator_source_class_init (HudIndicatorSourceClass *class)
{
  class->finalize = hud_indicator_source_finalize;
}

/**
 * hud_indicator_source_new:
 *
 * Creates a #HudIndicatorSource.
 *
 * Returns: a new #HudIndicatorSource
 **/
HudIndicatorSource *
hud_indicator_source_new (void)
{
  return g_object_new (HUD_TYPE_INDICATOR_SOURCE, NULL);
}
