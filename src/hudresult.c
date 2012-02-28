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

#include "hudresult.h"

#include "distance.h"

/**
 * SECTION:hudresult
 * @title: HudResult
 * @short_description: a search result: a #HudItem plus metadata about
 *   why it matched
 *
 * A #HudResult is a wrapper around a #HudItem plus information about
 * why (and how closely) it matched a particular search.
 **/

/**
 * HudResult:
 *
 * This is an opaque structure type.
 **/

typedef GObjectClass HudResultClass;

struct _HudResult
{
  GObject parent_instance;

  HudItem *item;

  guint   distance;
  gchar **matched;
  gchar  *description;
};

G_DEFINE_TYPE (HudResult, hud_result, G_TYPE_OBJECT)

static guint
calculate_distance_from_list (const gchar   *search_string,
                              HudStringList *list,
                              GStrv         *matched)
{
  HudStringList *iter;
  const gchar **strv;
  guint distance;
  gint i = 0;

  for (iter = list; iter; iter = hud_string_list_get_tail (iter))
    i++;

  strv = g_new (const char *, i + 1);
  strv[i] = NULL;

  for (iter = list; iter; iter = hud_string_list_get_tail (iter))
    strv[--i] = hud_string_list_get_head (iter);

  distance = calculate_distance (search_string, (char **) strv, matched);

  g_free (strv);


  return distance;
}

static void
hud_result_finalize (GObject *object)
{
  HudResult *result = HUD_RESULT (object);

  g_object_unref (result->item);
  g_strfreev (result->matched);
  g_free (result->description);

  G_OBJECT_CLASS (hud_result_parent_class)
    ->finalize (object);
}

static void
hud_result_init (HudResult *result)
{
}

static void
hud_result_class_init (HudResultClass *class)
{
  class->finalize = hud_result_finalize;
}

/**
 * hud_result_get_if_matched:
 * @item: a #HudItem
 * @search_string: the search string used
 * @max_distance: the maximum distance allowed
 *
 * Creates a #HudResult for @item, only if the resulting unadjusted
 * distance would be less than or equal to @max_distance.
 *
 * This is the same as hud_result_new() except that it will return %NULL
 * if the distance is too great.
 *
 * Returns: a new #HudResult, or %NULL in event of a poor match
 **/
HudResult *
hud_result_get_if_matched (HudItem     *item,
                           const gchar *search_string,
                           guint        max_distance)
{
  if (calculate_distance_from_list (search_string, hud_item_get_tokens (item), NULL) <= max_distance)
    return hud_result_new (item, search_string);
  else
    return NULL;
}

/**
 * hud_result_new:
 * @item: a #HudItem
 * @search_string: the search string used
 *
 * Creates a #HudResult for @item as search for using @search_string.
 *
 * Returns: the new #HudResult
 **/
HudResult *
hud_result_new (HudItem     *item,
                const gchar *search_string)
{
  HudResult *result;

  g_return_val_if_fail (HUD_IS_ITEM (item), NULL);
  g_return_val_if_fail (search_string != NULL, NULL);

  result = g_object_new (HUD_TYPE_RESULT, NULL);
  result->item = g_object_ref (item);
  result->distance = calculate_distance_from_list (search_string, hud_item_get_tokens (item), &result->matched);
  result->description = hud_string_list_pretty_print (hud_item_get_tokens (result->item));

  return result;
}

/**
 * hud_result_get_distance:
 * @result: a #HudResult
 * @max_usage: the maximum usage count we consider
 *
 * Returns the "adjusted" distance of @result.
 *
 * If @max_usage is zero then the returned value is equal to the
 * distance between the #HudItem used to create the result and the
 * search string.
 *
 * If @max_usage is non-zero then it is taken to be the usage count of
 * the most-used item in the same query as this result.  The distance is
 * adjusted for this fact to penalise less-frequently-used item.
 *
 * Returns: the adjusted distance
 **/
guint
hud_result_get_distance (HudResult *result,
                         guint      max_usage)
{
  guint distance;

  g_return_val_if_fail (HUD_IS_RESULT (result), G_MAXINT);

  distance = result->distance;

  if (max_usage != 0)
    {
      guint usage, inverse_usage;

      usage = hud_item_get_usage (result->item);
      inverse_usage = max_usage - usage;
      distance += (distance * inverse_usage) / max_usage;
    }

  return distance;
}

/**
 * hud_result_get_item:
 * @result: a #HudResult
 *
 * Gets the #HudItem for @result.
 *
 * Returns: (transfer none): a #HudItem
 **/
HudItem *
hud_result_get_item (HudResult *result)
{
  return result->item;
}

/**
 * hud_result_get_html_description:
 * @result: a #HudResult
 *
 * Returns a textual description of @result with the parts of the text
 * that matched the search string strenghtened (ie: in bold).
 *
 * Returns: the description
 **/
const gchar *
hud_result_get_html_description (HudResult *result)
{
  return result->description;
}
