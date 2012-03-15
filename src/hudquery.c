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

#define G_LOG_DOMAIN "hudquery"

#include "hudquery.h"

#include "hudresult.h"

/**
 * SECTION:hudquery
 * @title: HudQuery
 * @short_description: a stateful query against a #HudSource
 *
 * #HudQuery is a stateful query for a particular search string against
 * a given #HudSource.
 *
 * The query monitors its source for the "change" signal and re-submits
 * the query when changes are reported.  The query has its own change
 * signal that is fired when this happens.
 *
 * The query maintains a list of results from the search which are
 * sorted by relevance and accessible by index.  Contrast this with the
 * stateless nature of #HudSource.
 **/

/**
 * HudQuery:
 *
 * This is an opaque structure type.
 **/

struct _HudQuery
{
  GObject parent_instance;

  HudSource *source;
  gchar *search_string;
  gint num_results;
  guint refresh_id;

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
  if (query->results->len > query->num_results)
    g_ptr_array_set_size (query->results, query->num_results);
}

static gboolean
hud_query_dispatch_refresh (gpointer user_data)
{
  HudQuery *query = user_data;

  hud_query_refresh (query);

  g_signal_emit (query, hud_query_changed_signal, 0);

  query->refresh_id = 0;

  return G_SOURCE_REMOVE;
}
static void
hud_query_source_changed (HudSource *source,
                          gpointer   user_data)
{
  HudQuery *query = user_data;

  if (!query->refresh_id)
    query->refresh_id = g_idle_add (hud_query_dispatch_refresh, g_object_ref (query));
}

static void
hud_query_finalize (GObject *object)
{
  HudQuery *query = HUD_QUERY (object);

  g_debug ("Destroyed query '%s'", query->search_string);

  hud_source_unuse (query->source);

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
  /**
   * HudQuery::changed:
   * @query: a #HudQuery
   *
   * Indicates that the results of @query have changed.
   **/
  hud_query_changed_signal = g_signal_new ("changed", HUD_TYPE_QUERY, G_SIGNAL_RUN_LAST, 0, NULL,
                                           NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

  class->finalize = hud_query_finalize;
}

/**
 * hud_query_new:
 * @source: the #HudSource against which to search
 * @search_string: the string to search for
 * @num_results: the maximum number of results to report
 *
 * Creates a #HudQuery.
 *
 * A #HudQuery is a stateful search for @search_string against a @source.
 *
 * Each #HudQuery is assigned a "query key" when it is created.  This
 * can be used to lookup the hud query later using hud_query_lookup().
 * Because of this, an internal reference is held on the query and the
 * query won't be completely freed until you call hud_query_close() on
 * it in addition to releasing your ref.
 *
 * Returns: the new #HudQuery
 **/
HudQuery *
hud_query_new (HudSource   *source,
               const gchar *search_string,
               gint         num_results)
{
  HudQuery *query;

  g_debug ("Created query '%s'", search_string);

  query = g_object_new (HUD_TYPE_QUERY, NULL);
  query->source = g_object_ref (source);
  query->results = g_ptr_array_new_with_free_func (g_object_unref);
  query->search_string = g_strdup (search_string);
  query->num_results = num_results;

  hud_source_use (query->source);

  hud_query_refresh (query);

  g_signal_connect_object (source, "changed", G_CALLBACK (hud_query_source_changed), query, 0);

  g_clear_object (&last_created_query);
  last_created_query = g_object_ref (query);

  return query;
}

/**
 * hud_query_get_query_key:
 * @query: a #HudQuery
 *
 * Returns the query key for @HudQuery.
 *
 * Each #HudQuery has a unique identifying key that is assigned when the
 * query is created.
 *
 * FIXME: This is a lie.
 *
 * Returns: (transfer none): the query key for @query
 **/
GVariant *
hud_query_get_query_key (HudQuery *query)
{
  static GVariant *query_key;

  if (query_key == NULL)
    query_key = g_variant_ref_sink (g_variant_new_string ("query key"));

  return query_key;
}

/**
 * hud_query_lookup:
 * @query_key: a query key
 *
 * Finds the query that has the given @query_key.
 *
 * Returns: (transfer none): the query, or %NULL if no such query exists
 **/
HudQuery *
hud_query_lookup (GVariant *query_key)
{
  return last_created_query;
}

/**
 * hud_query_close:
 * @query: a #HudQuery
 *
 * Closes a #HudQuery.
 *
 * This drops the query from the internal list of queries.  Future use
 * of hud_query_lookup() to find this query will fail.
 *
 * You must still release your own reference on @query, if you have one.
 * This only drops the internal reference.
 **/
void
hud_query_close (HudQuery *query)
{
  if (query == last_created_query)
    g_clear_object (&last_created_query);
}

/**
 * hud_query_get_n_results:
 * @query: a #HudQuery
 *
 * Gets the number of results in @query.
 *
 * Returns: the number of results
 **/
guint
hud_query_get_n_results (HudQuery *query)
{
  return query->results->len;
}

/**
 * hud_query_get_result_by_index:
 * @query: a #HudQuery
 * @i: the index of the result
 *
 * Gets the @i<!-- -->th result from @query.
 *
 * @i must be less than the number of results in the query.  See
 * hud_query_get_n_results().
 *
 * Returns: (transfer none): the #HudResult at position @i
 **/
HudResult *
hud_query_get_result_by_index (HudQuery *query,
                               guint     i)
{
  return query->results->pdata[i];
}
