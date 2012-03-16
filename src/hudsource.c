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

#define G_LOG_DOMAIN "hudsource"

#include "hudsource.h"

/**
 * SECTION:hudsource
 * @title: HudSource
 * @short_description: a source of #HudResults
 *
 * A #HudSource is a very simple interface with only two APIs.
 *
 * First, a #HudSource may be searched, with a string.  The search
 * results in a list of #HudResults being returned.  This is
 * hud_source_search().
 *
 * Second, a #HudSource has a simple "changed" signal that is emitted
 * whenever the result of calling hud_source_search() may have changed.
 *
 * A #HudSource is stateless with respect to active queries.
 *
 * FUTURE DIRECTIONS:
 *
 * #HudSource should probably have an API to indicate if there is an
 * active query in progress.  There are two reasons for this.  First is
 * because the implementations could be more lazy with respect to
 * watching for changes.  Second is because dbusmenu has a concept of if
 * a menu is being shown or not and some applications may find this
 * information to be useful.
 *
 * It may also make sense to handle queries in a more stateful way,
 * possibly replacing the "change" signal with something capable of
 * expressing more fine-grained changes (eg: a single item was added or
 * removed).
 **/

/**
 * HudSource:
 *
 * This is an opaque structure type.
 **/

/**
 * HudSourceInterface:
 * @g_iface: the #GTypeInterface
 * @search: virtual function pointer for hud_source_search()
 *
 * This is the interface vtable for #HudSource.
 **/

G_DEFINE_INTERFACE (HudSource, hud_source, G_TYPE_OBJECT)

static gulong hud_source_changed_signal;

static void
hud_source_default_init (HudSourceInterface *iface)
{
  /**
   * HudSource::changed:
   * @source: a #HudSource
   *
   * Indicates that the #HudSource may have changed.  After this signal,
   * calls to hud_source_search() may return different results than they
   * did before.
   **/
  hud_source_changed_signal = g_signal_new ("changed", HUD_TYPE_SOURCE, G_SIGNAL_RUN_LAST, 0, NULL,
                                            NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

/**
 * hud_source_search:
 * @source: a #HudSource
 * @results_array: (element-type HudResult): array to append results to
 * @search_string: the search string
 *
 * Searches for #HudItems in @source that potentially match
 * @search_string and creates #HudResults for them, appending them to
 * @results_array.
 *
 * @source will emit a ::changed signal if the results of calling this
 * function may have changed, at which point you should call it again.
 **/
void
hud_source_search (HudSource   *source,
                   GPtrArray   *results_array,
                   const gchar *search_string)
{
  g_debug ("search on %s %p", G_OBJECT_TYPE_NAME (source), source);

  HUD_SOURCE_GET_IFACE (source)
    ->search (source, results_array, search_string);
}

/**
 * hud_source_changed:
 * @source: a #HudSource
 *
 * Signals that @source may have changed (ie: emits the "changed"
 * signal).
 *
 * This function should only ever be called by implementations of
 * #HudSource.
 **/
void
hud_source_changed (HudSource *source)
{
  g_debug ("%s %p changed", G_OBJECT_TYPE_NAME (source), source);

  g_signal_emit (source, hud_source_changed_signal, 0);
}
