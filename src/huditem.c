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

#include "huditem.h"

#include "usage-tracker.h"

/**
 * SECTION:huditem
 * @title: HudItem
 * @short_description: a user-interesting item that can be activated
 *
 * A #HudItem represents a user-interesting action that can be activated
 * from the Hud user interface.
 **/

/**
 * HudItem:
 *
 * This is an opaque structure type.
 **/

/**
 * HudItemClass:
 * @parent_class: the #GObjectClass
 * @activate: virtual function pointer for hud_item_activate()
 *
 * This is the class vtable for #HudItem.
 **/

struct _HudItemPrivate
{
  GObject parent_instance;

  gchar *desktop_file;

  HudStringList *tokens;
  gboolean enabled;
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

/**
 * hud_item_construct:
 * @g_type: a #GType
 * @tokens: the search tokens for the item
 * @desktop_file: the desktop file of the provider of the item
 * @enabled: if the item is enabled
 *
 * This is the Vala-style chain-up constructor corresponding to
 * hud_item_new().  @g_type must be a subtype of #HudItem.
 *
 * Only subclasses of #HudItem should call this.
 *
 * Returns: a new #HudItem or #HudItem subclass
 **/
gpointer
hud_item_construct (GType          g_type,
                    HudStringList *tokens,
                    const gchar   *desktop_file,
                    gboolean       enabled)
{
  HudItem *item;

  item = g_object_new (g_type, NULL);
  item->priv->tokens = hud_string_list_ref (tokens);
  item->priv->desktop_file = g_strdup (desktop_file);
  item->priv->enabled = enabled;

  //item->usage = usage_tracker_get_usage (usage_tracker_get_instance (), desktop_file, identifier);

  return item;
}

/**
 * hud_item_new:
 * @tokens: the search tokens for the item
 * @desktop_file: the desktop file of the provider of the item
 * @enabled: if the item is enabled
 *
 * Creates a new #HudItem.
 *
 * If @enabled is %FALSE then the item will never be in the result of a
 * search.
 *
 * Returns: a new #HudItem
 **/
HudItem *
hud_item_new (HudStringList *tokens,
              const gchar   *desktop_file,
              gboolean       enabled)
{
  return hud_item_construct (HUD_TYPE_ITEM, tokens, desktop_file, enabled);
}

/**
 * hud_item_activate:
 * @item: a #HudItem
 * @platform_data: platform data
 *
 * Activates @item.
 *
 * @platform_data is platform data in the #GApplication or
 * #GRemoteActionGroup sense.  It should be a #GVariant with the type
 * <literal>a{sv}</literal>.
 **/
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

/**
 * hud_item_get_tokens:
 * @item: a #HudItem
 *
 * Gets the tokens that represent the description of @item.
 *
 * This is a #HudStringList in reverse order of how the item should
 * appear in the Hud.  For example, "File > Open" would be represneted
 * by the list <code>['Open', 'File']</code>.
 *
 * Returns: (transfer none): the tokens
 **/
HudStringList *
hud_item_get_tokens (HudItem *item)
{
  g_return_val_if_fail (HUD_IS_ITEM (item), NULL);

  return item->priv->tokens;
}

/**
 * hud_item_get_item_icon:
 * @item: a #HudItem
 *
 * Gets the icon for the action represented by @item, if one exists.
 *
 * Returns: the icon name, or %NULL if there is no icon
 **/
const gchar *
hud_item_get_item_icon (HudItem *item)
{
  return "";
}

/**
 * hud_item_get_app_icon:
 * @item: a #HudItem
 *
 * Gets the icon of the application that @item lies within.
 *
 * Returns: the icon name, or %NULL if there is no icon
 **/
const gchar *
hud_item_get_app_icon (HudItem *item)
{
  return "";
}

/**
 * hud_item_get_usage:
 * @item: a #HudItem
 *
 * Gets the use-count of @item.
 *
 * This is the number of times the item has been activated in recent
 * history.
 *
 * Returns: the usage count
 **/
guint
hud_item_get_usage (HudItem *item)
{
  return item->priv->usage;
}

/**
 * hud_item_get_enabled:
 * @item: a #HudItem
 *
 * Checks if the item is disabled or enabled.
 *
 * Disabled items should never appear in search results.
 *
 * Returns: if the item is enabled
 **/
gboolean
hud_item_get_enabled (HudItem *item)
{
  return item->priv->enabled;
}
