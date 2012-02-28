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

#include "hudresult.h"

#include "distance.h"

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

HudItem *
hud_result_get_item (HudResult *result)
{
  return result->item;
}

const gchar *
hud_result_get_html_description (HudResult *result)
{
  return result->description;
}
