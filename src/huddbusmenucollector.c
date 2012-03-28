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
 * Authors: Ryan Lortie <desrt@desrt.ca>
 *          Ted Gould <ted@canonical.com>
 */

#define G_LOG_DOMAIN "huddbusmenucollector"

#include "huddbusmenucollector.h"

#include <libdbusmenu-glib/client.h>
#include <string.h>

#include "hudappmenuregistrar.h"
#include "hudresult.h"
#include "hudsource.h"

/**
 * SECTION:huddbusmenucollector
 * @title: HudDbusmenuCollector
 * @short_description: a #HudSource that collects #HudItems from
 *   Dbusmenu
 *
 * The #HudDbusmenuCollector collects menu items from a #DbusmenuClient.
 *
 * There are two modes of operation.
 *
 * In the simple mode, the collector is created with a specified
 * endpoint using hud_dbusmenu_collector_new_for_endpoint().  A
 * #DbusmenuClient is constructed using this endpoint and the collector
 * constructs #HudItems for the contents of the menu found there.  This
 * mode is intended for use with indicators.
 *
 * For menus associated with application windows (ie: menubars), we must
 * consult the AppMenu registrar in order to discover the endpoint to
 * use.  This second mode of the collector is used by calling
 * hud_dbusmenu_collector_new_for_window().
 **/

/**
 * HudDbusmenuCollector:
 *
 * This is an opaque structure type.
 **/

typedef struct
{
  HudItem parent_instance;

  DbusmenuMenuitem *menuitem;
  gboolean is_opened;
} HudDbusmenuItem;

typedef HudItemClass HudDbusmenuItemClass;

G_DEFINE_TYPE (HudDbusmenuItem, hud_dbusmenu_item, HUD_TYPE_ITEM)

static void
hud_dbusmenu_item_activate (HudItem  *hud_item,
                            GVariant *platform_data)
{
  HudDbusmenuItem *item = (HudDbusmenuItem *) hud_item;
  const gchar *startup_id;
  guint32 timestamp = 0;

  if (g_variant_lookup (platform_data, "desktop-startup-id", "&s", &startup_id))
    {
      const gchar *time_tag;

      if ((time_tag = strstr (startup_id, "_TIME")))
        {
          gint64 result;

          result = g_ascii_strtoll (time_tag + 5, NULL, 10);

          if (0 <= result && result <= G_MAXINT32)
           timestamp = result;
        }
    }

  dbusmenu_menuitem_handle_event(item->menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, NULL, timestamp);
}

static void
hud_dbusmenu_item_finalize (GObject *object)
{
  HudDbusmenuItem *item = (HudDbusmenuItem *) object;

  g_object_unref (item->menuitem);

  G_OBJECT_CLASS (hud_dbusmenu_item_parent_class)
    ->finalize (object);
}

static void
hud_dbusmenu_item_init (HudDbusmenuItem *item)
{
}

static void
hud_dbusmenu_item_class_init (HudDbusmenuItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = hud_dbusmenu_item_finalize;
  class->activate = hud_dbusmenu_item_activate;
}

static const gchar *
hud_dbusmenu_item_get_label_property (const gchar *type)
{
  static const gchar * const property_table[][2] =
  {
    { DBUSMENU_CLIENT_TYPES_DEFAULT,                 DBUSMENU_MENUITEM_PROP_LABEL                         },
    /* Indicator Messages */
    { "application-item",                            DBUSMENU_MENUITEM_PROP_LABEL                         },
    { "indicator-item",                              "indicator-label"                                    },
    /* Indicator Datetime */
    { "appointment-item",                            "appointment-label"                                  },
    { "timezone-item",                               "timezone-name"                                      },
    /* Indicator Sound */
    { "x-canonical-sound-menu-player-metadata-type", "x-canonical-sound-menu-player-metadata-player-name" },
    { "x-canonical-sound-menu-mute-type",            DBUSMENU_MENUITEM_PROP_LABEL                         },
    /* Indicator User */
    { "x-canonical-user-item",                       "user-item-name"                                     }
  };
  static GHashTable *property_hash;

  if G_UNLIKELY (property_hash == NULL)
    {
      gint i;

      property_hash = g_hash_table_new (g_str_hash, g_str_equal);

      for (i = 0; i < G_N_ELEMENTS (property_table); i++)
        g_hash_table_insert (property_hash, (gpointer) property_table[i][0], (gpointer) property_table[i][1]);
    }

  if (type == NULL)
    return DBUSMENU_MENUITEM_PROP_LABEL;

  return g_hash_table_lookup (property_hash, type);
}


static HudDbusmenuItem *
hud_dbusmenu_item_new (HudStringList    *context,
                       const gchar      *desktop_file,
                       const gchar      *icon,
                       DbusmenuMenuitem *menuitem)
{
  HudStringList *tokens;
  HudDbusmenuItem *item;
  const gchar *type;
  const gchar *prop;
  gboolean enabled;

  type = dbusmenu_menuitem_property_get (menuitem, DBUSMENU_MENUITEM_PROP_TYPE);
  prop = hud_dbusmenu_item_get_label_property (type);

  if (prop && dbusmenu_menuitem_property_exist (menuitem, prop))
    {
      const gchar *label;

      label = dbusmenu_menuitem_property_get (menuitem, prop);
      tokens = hud_string_list_cons_label (label, context);
      enabled = TRUE;
    }
  else
    {
      tokens = hud_string_list_ref (context);
      enabled = FALSE;
    }

  if (enabled)
    enabled &= !dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE) ||
               dbusmenu_menuitem_property_get_bool (menuitem, DBUSMENU_MENUITEM_PROP_VISIBLE);

  if (enabled)
    enabled &= !dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED) ||
               dbusmenu_menuitem_property_get_bool (menuitem, DBUSMENU_MENUITEM_PROP_ENABLED);

  if (enabled)
    enabled &= !dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY);

  item = hud_item_construct (hud_dbusmenu_item_get_type (), tokens, desktop_file, icon, enabled);
  item->menuitem = g_object_ref (menuitem);

  hud_string_list_unref (tokens);

  return item;
}

struct _HudDbusmenuCollector
{
  GObject parent_instance;

  DbusmenuClient *client;
  DbusmenuMenuitem *root;
  gchar *application_id;
  HudStringList *prefix;
  gchar *icon;
  GHashTable *items;
  guint penalty;
  guint xid;
  gboolean alive;
  gint use_count;
  gboolean reentrance_check;
};

typedef GObjectClass HudDbusmenuCollectorClass;

static void hud_dbusmenu_collector_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudDbusmenuCollector, hud_dbusmenu_collector, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_dbusmenu_collector_iface_init))

static void
hud_dbusmenu_collector_open_submenu (gpointer key,
                                     gpointer value,
                                     gpointer user_data)
{
  DbusmenuMenuitem *menuitem = key;
  HudDbusmenuItem *item = value;

  if (dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY))
    {
      dbusmenu_menuitem_handle_event (menuitem, DBUSMENU_MENUITEM_EVENT_OPENED, NULL, 0);
      item->is_opened = TRUE;
    }
}

static void
hud_dbusmenu_collector_close_submenu (gpointer key,
                                      gpointer value,
                                      gpointer user_data)
{
  DbusmenuMenuitem *menuitem = key;
  HudDbusmenuItem *item = value;

  if (item->is_opened)
    {
      dbusmenu_menuitem_handle_event (menuitem, DBUSMENU_MENUITEM_EVENT_CLOSED, NULL, 0);
      item->is_opened = FALSE;
    }
}

static void
hud_dbusmenu_collector_use (HudSource *source)
{
  HudDbusmenuCollector *collector = HUD_DBUSMENU_COLLECTOR (source);

  collector->reentrance_check = TRUE;

  if (collector->use_count == 0)
    g_hash_table_foreach (collector->items, hud_dbusmenu_collector_open_submenu, NULL);

  collector->use_count++;

  collector->reentrance_check = FALSE;
}

static void
hud_dbusmenu_collector_unuse (HudSource *source)
{
  HudDbusmenuCollector *collector = HUD_DBUSMENU_COLLECTOR (source);

  g_return_if_fail (collector->use_count > 0);

  collector->reentrance_check = TRUE;

  collector->use_count--;

  if (collector->use_count == 0)
    g_hash_table_foreach (collector->items, hud_dbusmenu_collector_close_submenu, NULL);

  collector->reentrance_check = FALSE;
}

static void
hud_dbusmenu_collector_search (HudSource   *source,
                               GPtrArray   *results_array,
                               const gchar *search_string)
{
  HudDbusmenuCollector *collector = HUD_DBUSMENU_COLLECTOR (source);
  GHashTableIter iter;
  gpointer item;

  g_hash_table_iter_init (&iter, collector->items);
  while (g_hash_table_iter_next (&iter, NULL, &item))
    {
      HudResult *result;

      result = hud_result_get_if_matched (item, search_string, collector->penalty);
      if (result)
        g_ptr_array_add (results_array, result);
    }
}

static void
hud_dbusmenu_collector_add_item (HudDbusmenuCollector *collector,
                                 HudStringList        *context,
                                 DbusmenuMenuitem     *menuitem);
static void
hud_dbusmenu_collector_remove_item (HudDbusmenuCollector *collector,
                                    DbusmenuMenuitem     *menuitem);

static void
hud_dbusmenu_collector_child_added (DbusmenuMenuitem *menuitem,
                                    DbusmenuMenuitem *child,
                                    guint             position,
                                    gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;
  HudStringList *context;
  HudItem *item;

  g_assert (!collector->reentrance_check);

  item = g_hash_table_lookup (collector->items, menuitem);
  g_assert (item != NULL);

  context = hud_item_get_tokens (item);

  hud_dbusmenu_collector_add_item (collector, context, child);
}

static void
hud_dbusmenu_collector_child_removed (DbusmenuMenuitem *menuitem,
                                      DbusmenuMenuitem *child,
                                      gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;

  g_assert (!collector->reentrance_check);

  hud_dbusmenu_collector_remove_item (collector, child);
}

static void
hud_dbusmenu_collector_property_changed (DbusmenuMenuitem *menuitem,
                                         const gchar      *property_name,
                                         GVariant         *new_value,
                                         gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;
  DbusmenuMenuitem *parent;
  HudStringList *context;
  HudDbusmenuItem *item;
  gboolean was_open;

  g_assert (!collector->reentrance_check);

  parent = dbusmenu_menuitem_get_parent (menuitem);

  if (parent)
    {
      HudItem *parentitem;

      parentitem = g_hash_table_lookup (collector->items, parent);
      context = hud_item_get_tokens (parentitem);
    }
  else
    context = collector->prefix;

  item = g_hash_table_lookup (collector->items, menuitem);
  was_open = item->is_opened;
  g_hash_table_remove (collector->items, menuitem);

  item = hud_dbusmenu_item_new (context, collector->application_id, collector->icon, menuitem);

  if (collector->use_count && !was_open && dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY))
    {
      dbusmenu_menuitem_handle_event (menuitem, DBUSMENU_MENUITEM_EVENT_OPENED, NULL, 0);
      item->is_opened = TRUE;
    }

  g_hash_table_insert (collector->items, menuitem, item);

  hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_add_item (HudDbusmenuCollector *collector,
                                 HudStringList        *context,
                                 DbusmenuMenuitem     *menuitem)
{
  HudDbusmenuItem *item;
  GList *child;

  item = hud_dbusmenu_item_new (context, collector->application_id, collector->icon, menuitem);
  context = hud_item_get_tokens (HUD_ITEM (item));

  g_signal_connect (menuitem, "property-changed", G_CALLBACK (hud_dbusmenu_collector_property_changed), collector);
  g_signal_connect (menuitem, "child-added", G_CALLBACK (hud_dbusmenu_collector_child_added), collector);
  g_signal_connect (menuitem, "child-removed", G_CALLBACK (hud_dbusmenu_collector_child_removed), collector);

  /* If we're actively being queried and we add a new submenu item, open it. */
  if (collector->use_count && dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_CHILD_DISPLAY))
    {
      dbusmenu_menuitem_handle_event (menuitem, DBUSMENU_MENUITEM_EVENT_OPENED, NULL, 0);
      item->is_opened = TRUE;
    }

  g_hash_table_insert (collector->items, menuitem, item);

  for (child = dbusmenu_menuitem_get_children (menuitem); child; child = child->next)
    hud_dbusmenu_collector_add_item (collector, context, child->data);

  if (collector->alive)
    hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_remove_item (HudDbusmenuCollector *collector,
                                    DbusmenuMenuitem     *menuitem)
{
  GList *child;

  g_signal_handlers_disconnect_by_func (menuitem, hud_dbusmenu_collector_property_changed, collector);
  g_signal_handlers_disconnect_by_func (menuitem, hud_dbusmenu_collector_child_added, collector);
  g_signal_handlers_disconnect_by_func (menuitem, hud_dbusmenu_collector_child_removed, collector);
  g_hash_table_remove (collector->items, menuitem);

  for (child = dbusmenu_menuitem_get_children (menuitem); child; child = child->next)
    hud_dbusmenu_collector_remove_item (collector, child->data);

  if (collector->alive)
    hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_setup_root (HudDbusmenuCollector *collector,
                                   DbusmenuMenuitem     *root)
{
  if (collector->root)
    {
      /* If the collector has the submenus opened, close them before we
       * remove them all.  The use_count being non-zero will cause them
       * to be reopened as they are added back below (if they will be).
       */
      if (collector->use_count > 0)
        g_hash_table_foreach (collector->items, hud_dbusmenu_collector_close_submenu, NULL);

      hud_dbusmenu_collector_remove_item (collector, collector->root);
      g_clear_object (&collector->root);
    }

  if (root)
    {
      hud_dbusmenu_collector_add_item (collector, collector->prefix, root);
      collector->root = g_object_ref (root);
    }
}

static void
hud_dbusmenu_collector_root_changed (DbusmenuClient   *client,
                                     DbusmenuMenuitem *root,
                                     gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;

  g_assert (!collector->reentrance_check);

  hud_dbusmenu_collector_setup_root (collector, root);
}

static void
hud_dbusmenu_collector_setup_endpoint (HudDbusmenuCollector *collector,
                                       const gchar          *bus_name,
                                       const gchar          *object_path)
{
  g_debug ("endpoint is %s %s", bus_name, object_path);

  if (collector->client)
    {
      g_signal_handlers_disconnect_by_func (collector->client, hud_dbusmenu_collector_root_changed, collector);
      hud_dbusmenu_collector_setup_root (collector, NULL);
      g_clear_object (&collector->client);
    }

  if (bus_name && object_path)
    {
      collector->client = dbusmenu_client_new (bus_name, object_path);
      g_signal_connect_object (collector->client, "root-changed",
                               G_CALLBACK (hud_dbusmenu_collector_root_changed), collector, 0);
      hud_dbusmenu_collector_setup_root (collector, dbusmenu_client_get_root (collector->client));
    }
}

static void
hud_dbusmenu_collector_registrar_observer_func (HudAppMenuRegistrar *registrar,
                                                guint                xid,
                                                const gchar         *bus_name,
                                                const gchar         *object_path,
                                                gpointer             user_data)
{
  HudDbusmenuCollector *collector = user_data;

  hud_dbusmenu_collector_setup_endpoint (collector, bus_name, object_path);
}


static void
hud_dbusmenu_collector_finalize (GObject *object)
{
  HudDbusmenuCollector *collector = HUD_DBUSMENU_COLLECTOR (object);

  if (collector->xid)
    hud_app_menu_registrar_remove_observer (hud_app_menu_registrar_get (), collector->xid,
                                            hud_dbusmenu_collector_registrar_observer_func, collector);

  /* remove all the items without firing change signals */
  collector->alive = FALSE;
  hud_dbusmenu_collector_setup_endpoint (collector, NULL, NULL);

  /* make sure the table is empty before we free it */
  g_assert (g_hash_table_size (collector->items) == 0);
  g_hash_table_unref (collector->items);

  g_free (collector->application_id);
  g_free (collector->icon);

  hud_string_list_unref (collector->prefix);
  g_clear_object (&collector->client);

  G_OBJECT_CLASS (hud_dbusmenu_collector_parent_class)
    ->finalize (object);
}

static void
hud_dbusmenu_collector_init (HudDbusmenuCollector *collector)
{
  collector->items = g_hash_table_new_full (NULL, NULL, NULL, g_object_unref);
}

static void
hud_dbusmenu_collector_iface_init (HudSourceInterface *iface)
{
  iface->use = hud_dbusmenu_collector_use;
  iface->unuse = hud_dbusmenu_collector_unuse;
  iface->search = hud_dbusmenu_collector_search;
}

static void
hud_dbusmenu_collector_class_init (HudDbusmenuCollectorClass *class)
{
  class->finalize = hud_dbusmenu_collector_finalize;
}

/**
 * hud_dbusmenu_collector_new_for_endpoint:
 * @application_id: a unique identifier for the application
 * @prefix: the title to prefix to all items
 * @icon: the icon for the appliction
 * @penalty: the penalty to apply to all results
 * @bus_name: a D-Bus bus name
 * @object_path: an object path at the destination given by @bus_name
 *
 * Creates a new #HudDbusmenuCollector for the specified endpoint.
 *
 * Internally, a #DbusmenuClient is created for this endpoint.  Searches
 * are performed against the contents of those menus.
 *
 * This call is intended to be used for indicators.
 *
 * If @prefix is non-%NULL (which, for indicators, it ought to be), then
 * it is prefixed to every item created by the collector.
 *
 * If @penalty is non-zero then all results returned from the collector
 * have their distance increased by a percentage equal to the penalty.
 * This allows items from indicators to score lower than they would
 * otherwise.
 *
 * Returns: a new #HudDbusmenuCollector
 **/
HudDbusmenuCollector *
hud_dbusmenu_collector_new_for_endpoint (const gchar *application_id,
                                         const gchar *prefix,
                                         const gchar *icon,
                                         guint        penalty,
                                         const gchar *bus_name,
                                         const gchar *object_path)
{
  HudDbusmenuCollector *collector;

  collector = g_object_new (HUD_TYPE_DBUSMENU_COLLECTOR, NULL);
  collector->application_id = g_strdup (application_id);
  collector->icon = g_strdup (icon);
  if (prefix)
    collector->prefix = hud_string_list_cons (prefix, NULL);
  collector->penalty = penalty;
  hud_dbusmenu_collector_setup_endpoint (collector, bus_name, object_path);

  collector->alive = TRUE;

  return collector;
}

/**
 * hud_dbusmenu_collector_new_for_window:
 * @window: a #BamfWindow
 *
 * Creates a new #HudDbusmenuCollector for the endpoint indicated by the
 * #HudAppMenuRegistrar for @window.
 *
 * This call is intended to be used for application menus.
 *
 * Returns: a new #HudDbusmenuCollector
 **/
HudDbusmenuCollector *
hud_dbusmenu_collector_new_for_window (BamfWindow  *window,
                                       const gchar *desktop_file,
                                       const gchar *icon)
{
  HudDbusmenuCollector *collector;

  collector = g_object_new (HUD_TYPE_DBUSMENU_COLLECTOR, NULL);
  collector->application_id = g_strdup (desktop_file);
  collector->icon = g_strdup (icon);
  collector->xid = bamf_window_get_xid (window);
  g_debug ("dbusmenu on %d", collector->xid);
  hud_app_menu_registrar_add_observer (hud_app_menu_registrar_get (), collector->xid,
                                       hud_dbusmenu_collector_registrar_observer_func, collector);

  collector->alive = TRUE;

  return collector;
}

/**
 * hud_dbusmenu_collector_set_prefix:
 * @collector: a #HudDbusmenuCollector
 * @prefix: the new prefix to use
 *
 * Changes the prefix applied to all items of the collector.
 *
 * This will involve destroying all of the items and recreating them
 * (since each item's prefix has to be changed).
 **/
void
hud_dbusmenu_collector_set_prefix (HudDbusmenuCollector *collector,
                                   const gchar          *prefix)
{
  hud_string_list_unref (collector->prefix);
  collector->prefix = hud_string_list_cons (prefix, NULL);
  hud_dbusmenu_collector_setup_root (collector, collector->root);
}

/**
 * hud_dbusmenu_collector_set_icon:
 * @collector: a #HudDbusmenuCollector
 * @icon: the application icon
 *
 * Changes the application icon used for all items of the collector.
 *
 * This will involve destroying all of the items and recreating them
 * (since each item's icon has to be changed).
 **/
void
hud_dbusmenu_collector_set_icon (HudDbusmenuCollector *collector,
                                 const gchar          *icon)
{
  g_free (collector->icon);
  collector->icon = g_strdup (icon);
  hud_dbusmenu_collector_setup_root (collector, collector->root);
}
