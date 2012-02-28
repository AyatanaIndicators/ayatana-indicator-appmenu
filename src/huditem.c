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

#include "huditem.h"

#include "usage-tracker.h"

struct _HudItemPrivate
{
  GObject parent_instance;

  gchar *desktop_file;

  HudStringList *tokens;
  guint usage;
};

G_DEFINE_TYPE (HudItem, hud_item, G_TYPE_OBJECT)

static void
hud_item_finalize (GObject *object)
{
  HudItem *item = HUD_ITEM (object);

  hud_string_list_unref (item->priv->tokens);
  g_free (item->priv->desktop_file);

  G_OBJECT_CLASS (hud_item_parent_class)
    ->finalize (object);
}

static void
hud_item_init (HudItem *item)
{
  item->priv = G_TYPE_INSTANCE_GET_PRIVATE (item, HUD_TYPE_ITEM, HudItemPrivate);
}

static void
hud_item_class_init (HudItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = hud_item_finalize;

  g_type_class_add_private (class, sizeof (HudItemPrivate));
}

gpointer
hud_item_construct (GType          g_type,
                    HudStringList *tokens,
                    const gchar   *desktop_file)
{
  HudItem *item;

  g_return_val_if_fail (tokens != NULL, NULL);

  {
    gchar *pretty = hud_string_list_pretty_print (tokens);
    g_print ("new item > %s\n", pretty);
    g_free (pretty);
  }

  item = g_object_new (g_type, NULL);
  item->priv->tokens = hud_string_list_ref (tokens);
  item->priv->desktop_file = g_strdup (desktop_file);

  //item->usage = usage_tracker_get_usage (usage_tracker_get_instance (), desktop_file, identifier);

  return item;
}

HudItem *
hud_item_new (HudStringList *tokens,
              const gchar   *desktop_file)
{
  return hud_item_construct (HUD_TYPE_ITEM, tokens, desktop_file);
}

void
hud_item_activate (HudItem  *item,
                   GVariant *platform_data)
{
  g_return_if_fail (HUD_IS_ITEM (item));

  HUD_ITEM_GET_CLASS (item)
    ->activate (item, platform_data);

  //usage_tracker_mark_usage (usage_tracker_get_instance (), item->desktop_file, item->identifier);
  //item->usage = usage_tracker_get_usage (usage_tracker_get_instance (), item->desktop_file, item->identifier);
}

HudStringList *
hud_item_get_tokens (HudItem *item)
{
  g_return_val_if_fail (HUD_IS_ITEM (item), NULL);

  return item->priv->tokens;
}

const gchar *
hud_item_get_item_icon (HudItem *item)
{
  return "";
}

const gchar *
hud_item_get_app_icon (HudItem *item)
{
  return "";
}

guint
hud_item_get_usage (HudItem *item)
{
  return item->priv->usage;
}
