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

#include "hudquery.h"

#include "hudresult.h"

struct _HudQuery
{
  GObject parent_instance;

  HudSource *source;
  gchar *search_string;
  gint num_results;

  guint64 generation;
  GPtrArray *results;
};

typedef GObjectClass HudQueryClass;

G_DEFINE_TYPE (HudQuery, hud_query, G_TYPE_OBJECT)
static guint hud_query_changed_signal;

static HudQuery *last_created_query;

static void
hud_query_find_max_usage (gpointer data,
                          gpointer user_data)
{
  guint *max_usage = user_data;
  HudResult *result = data;
  HudItem *item;
  guint usage;

  item = hud_result_get_item (result);
  usage = hud_item_get_usage (item);

  *max_usage = MAX (*max_usage, usage);
}

static gint
hud_query_compare_results (gconstpointer a,
                           gconstpointer b,
                           gpointer      user_data)
{
  HudResult *result_a = *(HudResult * const *) a;
  HudResult *result_b = *(HudResult * const *) b;
  gint max_usage = GPOINTER_TO_INT (user_data);
  guint distance_a;
  guint distance_b;

  distance_a = hud_result_get_distance (result_a, max_usage);
  distance_b = hud_result_get_distance (result_b, max_usage);

  return distance_a - distance_b;
}

static void
hud_query_refresh (HudQuery *query)
{
  guint max_usage = 0;

  g_ptr_array_set_size (query->results, 0);
  query->generation++;

  if (query->search_string[0] != '\0')
    hud_source_search (query->source, query->results, query->search_string);

  /* XXX: The old code queried, sorted, truncated to 15, got usage data,
   * then sorted again.
   *
   * We try to do it only once.
   *
   * This may change the results...
   */

  g_ptr_array_foreach (query->results, hud_query_find_max_usage, &max_usage);
  g_ptr_array_sort_with_data (query->results, hud_query_compare_results, GINT_TO_POINTER (max_usage));
}

static void
hud_query_source_changed (HudSource *source,
                          gpointer   user_data)
{
  HudQuery *query = user_data;

  hud_query_refresh (query);

  g_signal_emit (query, hud_query_changed_signal, 0);
}

static void
hud_query_finalize (GObject *object)
{
  HudQuery *query = HUD_QUERY (object);

  g_object_unref (query->source);
  g_free (query->search_string);
  g_ptr_array_unref (query->results);

  G_OBJECT_CLASS (hud_query_parent_class)
    ->finalize (object);
}

static void
hud_query_init (HudQuery *query)
{
}

static void
hud_query_class_init (HudQueryClass *class)
{
  hud_query_changed_signal = g_signal_new ("changed", HUD_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
                                           NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  class->finalize = hud_query_finalize;
}

HudQuery *
hud_query_new (HudSource   *source,
               const gchar *search_string,
               gint         num_results)
{
  HudQuery *query;

  query = g_object_new (HUD_TYPE_QUERY, NULL);
  query->source = g_object_ref (source);
  query->results = g_ptr_array_new_with_free_func (g_object_unref);
  query->search_string = g_strdup (search_string);
  query->num_results = num_results;

  hud_query_refresh (query);

  g_signal_connect_object (source, "changed", G_CALLBACK (hud_query_source_changed), query, 0);

  g_clear_object (&last_created_query);
  last_created_query = g_object_ref (query);

  return query;
}

const gchar *
hud_query_get_target (HudQuery *query)
{
  return "";
}

GVariant *
hud_query_get_query_key (HudQuery *query)
{
  static GVariant *query_key;

  if (query_key == NULL)
    query_key = g_variant_ref_sink (g_variant_new_string ("query key"));

  return query_key;
}

HudQuery *
hud_query_lookup (GVariant *query_key)
{
  return last_created_query;
}

void
hud_query_close (HudQuery *query)
{
  if (query == last_created_query)
    g_clear_object (&last_created_query);
}

guint64
hud_query_get_generation (HudQuery *query)
{
  return query->generation;
}

guint
hud_query_get_n_results (HudQuery *query)
{
  return query->results->len;
}

HudResult *
hud_query_get_result_by_index (HudQuery *query,
                               guint     i)
{
  return query->results->pdata[i];
}
