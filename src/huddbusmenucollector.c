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

#define G_LOG_DOMAIN "huddbusmenucollector"

#include "huddbusmenucollector.h"

#include <libdbusmenu-glib/client.h>
#include <string.h>

#include "hudappmenuregistrar.h"
#include "indicator-tracker.h"
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

static HudItem *
hud_dbusmenu_item_new (HudStringList    *context,
                       const gchar      *desktop_file,
                       DbusmenuMenuitem *menuitem)
{
  HudStringList *tokens;
  HudDbusmenuItem *item;

  if (dbusmenu_menuitem_property_exist (menuitem, DBUSMENU_MENUITEM_PROP_LABEL))
    {
      const gchar *label;

      label = dbusmenu_menuitem_property_get (menuitem, DBUSMENU_MENUITEM_PROP_LABEL);
      tokens = hud_string_list_cons_label (label, context);
    }
  else
    tokens = hud_string_list_ref (context);

  item = hud_item_construct (hud_dbusmenu_item_get_type (), tokens, desktop_file);
  item->menuitem = g_object_ref (menuitem);

  hud_string_list_unref (tokens);

  return HUD_ITEM (item);
}

struct _HudDbusmenuCollector
{
  GObject parent_instance;

  DbusmenuClient *client;
  DbusmenuMenuitem *root;
  GHashTable *items;
  guint xid;
};

typedef GObjectClass HudDbusmenuCollectorClass;

static void hud_dbusmenu_collector_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudDbusmenuCollector, hud_dbusmenu_collector, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_dbusmenu_collector_iface_init))

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

      result = hud_result_get_if_matched (item, search_string, 30);
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
                                    gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;
  HudStringList *context;
  HudItem *item;

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

  hud_dbusmenu_collector_remove_item (collector, child);
}

static void
hud_dbusmenu_collector_add_item (HudDbusmenuCollector *collector,
                                 HudStringList        *context,
                                 DbusmenuMenuitem     *menuitem)
{
  HudItem *item;
  GList *child;

  item = hud_dbusmenu_item_new (context, NULL, menuitem);
  context = hud_item_get_tokens (item);

  g_signal_connect (menuitem, "child-added", G_CALLBACK (hud_dbusmenu_collector_child_added), collector);
  g_signal_connect (menuitem, "child-removed", G_CALLBACK (hud_dbusmenu_collector_child_removed), collector);
  g_hash_table_insert (collector->items, menuitem, item);

  for (child = dbusmenu_menuitem_get_children (menuitem); child; child = child->next)
    hud_dbusmenu_collector_add_item (collector, context, child->data);

  hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_remove_item (HudDbusmenuCollector *collector,
                                    DbusmenuMenuitem     *menuitem)
{
  GList *child;

  g_signal_handlers_disconnect_by_func (menuitem, hud_dbusmenu_collector_child_added, collector);
  g_signal_handlers_disconnect_by_func (menuitem, hud_dbusmenu_collector_child_removed, collector);
  g_hash_table_remove (collector->items, menuitem);

  for (child = dbusmenu_menuitem_get_children (menuitem); child; child = child->next)
    hud_dbusmenu_collector_remove_item (collector, child->data);

  hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_setup_root (HudDbusmenuCollector *collector,
                                   DbusmenuMenuitem     *root)
{
  if (collector->root)
    {
      hud_dbusmenu_collector_remove_item (collector, collector->root);
      collector->root = NULL;
    }

  if (root)
    {
      hud_dbusmenu_collector_add_item (collector, NULL, root);
      collector->root = root;
    }
}

static void
hud_dbusmenu_collector_root_changed (DbusmenuClient   *client,
                                     DbusmenuMenuitem *root,
                                     gpointer          user_data)
{
  HudDbusmenuCollector *collector = user_data;

  hud_dbusmenu_collector_setup_root (collector, root);
}

static void
hud_dbusmenu_collector_setup_endpoint (HudDbusmenuCollector *collector,
                                       const gchar          *bus_name,
                                       const gchar          *object_path)
{
  g_debug ("endpoint is %s %s\n", bus_name, object_path);

  if (collector->client)
    {
      g_signal_handlers_disconnect_by_func (collector->client, hud_dbusmenu_collector_root_changed, collector);
      hud_dbusmenu_collector_setup_root (collector, NULL);
      g_clear_object (&collector->client);
    }

  if (bus_name && object_path)
    {
      collector->client = dbusmenu_client_new (bus_name, object_path);
      g_signal_connect (collector->client, "root-changed", G_CALLBACK (hud_dbusmenu_collector_root_changed), collector);
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

  g_hash_table_unref (collector->items);
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
  iface->search = hud_dbusmenu_collector_search;
}

static void
hud_dbusmenu_collector_class_init (HudDbusmenuCollectorClass *class)
{
  class->finalize = hud_dbusmenu_collector_finalize;
}

/**
 * hud_dbusmenu_collector_new_for_endpoint:
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
 * Returns: a new #HudDbusmenuCollector
 **/
HudDbusmenuCollector *
hud_dbusmenu_collector_new_for_endpoint (const gchar *bus_name,
                                         const gchar *object_path)
{
  HudDbusmenuCollector *collector;

  collector = g_object_new (HUD_TYPE_DBUSMENU_COLLECTOR, NULL);
  hud_dbusmenu_collector_setup_endpoint (collector, bus_name, object_path);

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
hud_dbusmenu_collector_new_for_window (BamfWindow *window)
{
  HudDbusmenuCollector *collector;

  collector = g_object_new (HUD_TYPE_DBUSMENU_COLLECTOR, NULL);
  collector->xid = bamf_window_get_xid (window);
  g_debug ("dbusmenu on %d\n", collector->xid);
  hud_app_menu_registrar_add_observer (hud_app_menu_registrar_get (), collector->xid,
                                       hud_dbusmenu_collector_registrar_observer_func, collector);

  return collector;
}
