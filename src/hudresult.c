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

#include <string.h>

#include "hudsettings.h"
#include "hudtoken.h"

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
  const gchar **matched;
  gchar  *description;
};

G_DEFINE_TYPE (HudResult, hud_result, G_TYPE_OBJECT)

static void
hud_result_finalize (GObject *object)
{
  HudResult *result = HUD_RESULT (object);

  g_object_unref (result->item);
  g_free (result->matched);
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
 * @penalty: a penalty value
 *
 * Creates a #HudResult for @item, only if the resulting unadjusted
 * distance would be less than or equal to the maximum distance
 * specified in the HUD settings.
 *
 * This is the same as hud_result_new() except that it will return %NULL
 * if the distance is too great.
 *
 * The penalty value is ignored when checking the maximum distance but
 * will impact the distance of the created result.  As a result, the
 * returned #HudResult may have an effective distance greater than the
 * maximum distance.
 *
 * Returns: a new #HudResult, or %NULL in event of a poor match
 **/
HudResult *
hud_result_get_if_matched (HudItem      *item,
                           HudTokenList *search_tokens,
                           guint         penalty)
{
  if (!hud_item_get_enabled (item))
    return NULL;

  /* ignore the penalty in the max-distance calculation */
  if (hud_token_list_distance (hud_item_get_token_list (item), search_tokens, NULL) <= hud_settings.max_distance)
    return hud_result_new (item, search_tokens, penalty);
  else
    return NULL;
}

/* recurse so that we can avoid having to prepend the string */
static void
hud_result_format_tokens (GString       *string,
                          HudStringList *tokens)
{
  HudStringList *tail;
  gchar *escaped;

  tail = hud_string_list_get_tail (tokens);

  if (tail)
    {
      hud_result_format_tokens (string, tail);
      g_string_append (string, " &gt; ");
    }

  escaped = g_markup_escape_text (hud_string_list_get_head (tokens), -1);
  g_string_append (string, escaped);
  g_free (escaped);
}

static void
hud_result_format_description (HudResult *result)
{
  GString *description;
  gint i;

  description = g_string_new (NULL);
  hud_result_format_tokens (description, hud_item_get_tokens (result->item));

  for (i = 0; result->matched[i]; i++)
    {
      gchar *escaped;
      gchar *match;

      escaped = g_markup_escape_text (result->matched[i], -1);
      match = strstr (description->str, escaped);

      if (match != NULL)
        {
          gsize start, end;

          start = match - description->str;
          end = start + strlen (escaped);

          /* modify the end first so that the modification to the start
           * doesn't change the offset of the end */
          g_string_insert (description, end, "</b>");
          g_string_insert (description, start, "<b>");
        }

      g_free (escaped);
    }

  result->description = g_string_free (description, FALSE);
}

/**
 * hud_result_new:
 * @item: a #HudItem
 * @search_string: the search string used
 * @penalty: a penalty value
 *
 * Creates a #HudResult for @item as search for using @search_string.
 *
 * If @penalty is non-zero then it is used to increase the distance of
 * the result.  This is used to decrease the ranking of matches from the
 * indicators.
 *
 * Returns: the new #HudResult
 **/
HudResult *
hud_result_new (HudItem      *item,
                HudTokenList *search_tokens,
                guint         penalty)
{
  HudResult *result;

  g_return_val_if_fail (HUD_IS_ITEM (item), NULL);
  g_return_val_if_fail (search_tokens != NULL, NULL);

  result = g_object_new (HUD_TYPE_RESULT, NULL);
  result->item = g_object_ref (item);
  result->distance = hud_token_list_distance (hud_item_get_token_list (item), search_tokens, &result->matched);
  hud_result_format_description (result);

  result->distance += (result->distance * penalty) / 100;

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
