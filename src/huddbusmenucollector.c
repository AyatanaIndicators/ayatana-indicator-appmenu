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

#define G_LOG_DOMAIN "huddbusmenucollector"

#include "huddbusmenucollector.h"

#include <libdbusmenu-glib/client.h>

#include "hudappmenuregistrar.h"
#include "indicator-tracker.h"
#include "hudsource.h"

struct _HudDbusmenuCollector
{
  GObject parent_instance;

  DbusmenuClient *client;
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
}

static void
hud_dbusmenu_collector_layout_updated (DbusmenuClient *client,
                                       gpointer        user_data)
{
  HudDbusmenuCollector *collector = user_data;

  hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_dbusmenu_collector_setup_endpoint (HudDbusmenuCollector *collector,
                                       const gchar          *bus_name,
                                       const gchar          *object_path)
{
  g_debug ("endpoint is %s %s\n", bus_name, object_path);

  if (collector->client)
    {
      g_signal_handlers_disconnect_by_func (collector->client, hud_dbusmenu_collector_layout_updated, collector);
      g_clear_object (&collector->client);
    }

  if (bus_name && object_path)
    {
      collector->client = dbusmenu_client_new (bus_name, object_path);
      g_signal_connect (collector->client, "layout-updated",
                        G_CALLBACK (hud_dbusmenu_collector_layout_updated), collector);
    }

  hud_source_changed (HUD_SOURCE (collector));
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

  G_OBJECT_CLASS (hud_dbusmenu_collector_parent_class)
    ->finalize (object);
}

static void
hud_dbusmenu_collector_init (HudDbusmenuCollector *collector)
{
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

HudDbusmenuCollector *
hud_dbusmenu_collector_new_for_endpoint (const gchar *bus_name,
                                         const gchar *object_path)
{
  HudDbusmenuCollector *collector;

  collector = g_object_new (HUD_TYPE_DBUSMENU_COLLECTOR, NULL);
  hud_dbusmenu_collector_setup_endpoint (collector, bus_name, object_path);

  return collector;
}

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
