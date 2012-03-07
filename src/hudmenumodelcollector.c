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

#include "hudmenumodelcollector.h"

#include "hudsource.h"
#include "hudresult.h"
#include "huditem.h"

#include <libbamf/libbamf.h>
#include <gio/gio.h>

/**
 * SECTION:hudmenumodelcollector
 * @title: HudMenuModelCollector
 * @short_description: a #HudSource that collects #HudItems from
 *   #GMenuModel
 *
 * The #HudMenuModelCollector collects menu items from the menus
 * associated with a window exported from an application using
 * #GMenuModel.  Activations are performed using #GActionGroup in the
 * usual way.
 *
 * The #GMenuModel is acquired using #GDBusMenuModel according to the
 * properties set on the #BamfWindow which must be passed to
 * hud_menu_model_collector_get().
 **/

/**
 * HudMenuModelCollector:
 *
 * This is an opaque structure type.
 **/

struct _HudMenuModelCollector
{
  GObject parent_instance;

  GSList *models;
  guint refresh_id;
  /* stuff... */
  GDBusMenuModel *app_menu;
  GDBusMenuModel *menubar;
  GDBusActionGroup *application;
  GDBusActionGroup *window;

  gchar *desktop_file;
  GPtrArray *items;
};


typedef struct
{
  HudItem parent_instance;

  GRemoteActionGroup *group;
  gchar *action_name;
  GVariant *target;
} HudModelItem;

typedef HudItemClass HudModelItemClass;

G_DEFINE_TYPE (HudModelItem, hud_model_item, HUD_TYPE_ITEM)

static void
hud_model_item_activate (HudItem  *hud_item,
                         GVariant *platform_data)
{
  HudModelItem *item = (HudModelItem *) hud_item;

  g_remote_action_group_activate_action_full (item->group, item->action_name, item->target, platform_data);
}

static void
hud_model_item_finalize (GObject *object)
{
  HudModelItem *item = (HudModelItem *) object;

  g_object_unref (item->group);
  g_free (item->action_name);

  if (item->target)
    g_variant_unref (item->target);

  G_OBJECT_CLASS (hud_model_item_parent_class)
    ->finalize (object);
}

static void
hud_model_item_init (HudModelItem *item)
{
}

static void
hud_model_item_class_init (HudModelItemClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);

  gobject_class->finalize = hud_model_item_finalize;

  class->activate = hud_model_item_activate;
}

static HudItem *
hud_model_item_new (HudStringList      *tokens,
                    const gchar        *desktop_file,
                    GRemoteActionGroup *action_group,
                    const gchar        *action_name,
                    GVariant           *target)
{
  HudModelItem *item;

  item = hud_item_construct (hud_model_item_get_type (), tokens, desktop_file, TRUE);
  item->group = g_object_ref (action_group);
  item->action_name = g_strdup (action_name);
  item->target = target ? g_variant_ref_sink (target) : NULL;

  return HUD_ITEM (item);
}

typedef GObjectClass HudMenuModelCollectorClass;

static void hud_menu_model_collector_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudMenuModelCollector, hud_menu_model_collector, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_menu_model_collector_iface_init))

/* XXX: There is a potential for unbounded recursion here if a hostile
 * client were to feed us corrupted data.  We should figure out a way to
 * address that.
 *
 * It's not really a security problem for as long as we generally trust
 * the programs that we run (ie: under the same UID).  If we ever start
 * receiving menus from untrusted sources, we need to take another look,
 * though.
 */
static void hud_menu_model_collector_add_model  (HudMenuModelCollector *collector,
                                                 GMenuModel            *model);
static void hud_menu_model_collector_disconnect (gpointer               data,
                                                 gpointer               user_data);

static gboolean
hud_menu_model_collector_refresh (gpointer user_data)
{
  HudMenuModelCollector *collector = user_data;
  GSList *free_list;

  g_ptr_array_set_size (collector->items, 0);
  free_list = collector->models;
  collector->models = NULL;

  if (collector->app_menu)
    hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->app_menu));
  if (collector->menubar)
    hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->menubar));

  g_slist_foreach (free_list, hud_menu_model_collector_disconnect, collector);
  g_slist_free_full (free_list, g_object_unref);

  return G_SOURCE_REMOVE;
}

static void
hud_menu_model_collector_model_changed (GMenuModel *model,
                                        gint        position,
                                        gint        removed,
                                        gint        added,
                                        gpointer    user_data)
{
  HudMenuModelCollector *collector = user_data;
  static GQuark context_quark;
  HudStringList *context;
  gboolean changed;
  gint i;

  if (collector->refresh_id)
    /* We have a refresh scheduled already.  Ignore. */
    return;

  if (removed)
    {
      /* Items being removed is an unusual case.  Instead of having some
       * fancy algorithms for figuring out how to deal with that we just
       * start over again.
       *
       * ps: refresh_id is never set at this point (look up)
       */
      collector->refresh_id = g_idle_add (hud_menu_model_collector_refresh, collector);
      return;
    }

  if (!context_quark)
    context_quark = g_quark_from_string ("menu item context");

  /* The 'context' is the list of strings that got us up to where we are
   * now, like "View > Toolbars".  We hang this on the GMenuModel with
   * qdata.
   *
   * Strictly speaking, GMenuModel structures are DAGs, but we more or
   * less assume that they are trees here and replace the data
   * unconditionally when we visit it the second time (which will be
   * more or less never, because really, a menu is a tree).
   */
  context = g_object_get_qdata (G_OBJECT (model), context_quark);

  changed = FALSE;
  for (i = position; i < position + added; i++)
    {
      HudStringList *tokens;
      GMenuModel *link;
      gchar *value;

      /* If this item has a label then we add it onto the context to get
       * our 'tokens'.  For example, if context is 'File' then we will
       * have 'File > New'.
       *
       * If there is no label (which only really makes sense for
       * sections) then we just reuse the existing context by taking a
       * ref to it.
       *
       * Either way, we need to free it at the end of the loop.
       */
      if (g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_LABEL, "s", &value))
        {
          tokens = hud_string_list_cons_label (value, context);
          g_free (value);
        }
      else
        tokens = hud_string_list_ref (context);

      /* Check if this is an action.  Here's where we may end up
       * creating a HudItem.
       */
      if (g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ACTION, "s", &value))
        {
          GDBusActionGroup *action_group = NULL;

          /* It's an action, so add it. */
          if (g_str_has_prefix (value, "app."))
            action_group = collector->application;
          else if (g_str_has_prefix (value, "win."))
            action_group = collector->window;

          if (action_group)
            {
              GVariant *target;
              HudItem *item;

              target = g_menu_model_get_item_attribute_value (model, i, G_MENU_ATTRIBUTE_TARGET, NULL);

              /* XXX: todo: target */
              item = hud_model_item_new (tokens, collector->desktop_file,
                                         G_REMOTE_ACTION_GROUP (action_group),
                                         value + 4, target);
              g_ptr_array_add (collector->items, item);

              if (target)
                g_variant_unref (target);

              changed = TRUE;
            }

          g_free (value);
        }

      /* For 'section' and 'submenu' links, we should recurse.  This is
       * where the danger comes in (due to the possibility of unbounded
       * recursion).
       */
      if ((link = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION)))
        {
          g_object_set_qdata_full (G_OBJECT (link), context_quark,
                                   hud_string_list_ref (tokens),
                                   (GDestroyNotify) hud_string_list_unref);
          hud_menu_model_collector_add_model (collector, link);
          g_object_unref (link);
        }

      if ((link = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU)))
        {
          /* for submenus, we add the submenu label to the context */
          g_object_set_qdata_full (G_OBJECT (link), context_quark,
                                   hud_string_list_ref (tokens),
                                   (GDestroyNotify) hud_string_list_unref);
          hud_menu_model_collector_add_model (collector, link);
          g_object_unref (link);
        }

      hud_string_list_unref (tokens);
    }

  if (changed)
    hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_menu_model_collector_add_model (HudMenuModelCollector *search_data,
                                    GMenuModel            *model)
{
  gint n_items;

  g_signal_connect (model, "items-changed", G_CALLBACK (hud_menu_model_collector_model_changed), search_data);
  search_data->models = g_slist_prepend (search_data->models, g_object_ref (model));

  n_items = g_menu_model_get_n_items (model);
  if (n_items > 0)
    hud_menu_model_collector_model_changed (model, 0, 0, n_items, search_data);
}

static void
hud_menu_model_collector_disconnect (gpointer data,
                                     gpointer user_data)
{
  g_signal_handlers_disconnect_by_func (data, hud_menu_model_collector_model_changed, user_data);
}

static void
hud_menu_model_collector_search (HudSource   *source,
                                 GPtrArray   *results_array,
                                 const gchar *search_string)
{
  HudMenuModelCollector *collector = HUD_MENU_MODEL_COLLECTOR (source);
  GPtrArray *items;
  gint i;

  items = collector->items;

  for (i = 0; i < items->len; i++)
    {
      HudResult *result;
      HudItem *item;

      item = g_ptr_array_index (items, i);
      result = hud_result_get_if_matched (item, search_string, 0);
      if (result)
        g_ptr_array_add (results_array, result);
    }
}
static void
hud_menu_model_collector_finalize (GObject *object)
{
  HudMenuModelCollector *collector = HUD_MENU_MODEL_COLLECTOR (object);

  if (collector->refresh_id)
    g_source_remove (collector->refresh_id);

  g_slist_free_full (collector->models, g_object_unref);
  g_clear_object (&collector->app_menu);
  g_clear_object (&collector->menubar);
  g_clear_object (&collector->application);
  g_clear_object (&collector->window);

  g_free (collector->desktop_file);

  g_ptr_array_unref (collector->items);

  G_OBJECT_CLASS (hud_menu_model_collector_parent_class)
    ->finalize (object);
}

static void
hud_menu_model_collector_init (HudMenuModelCollector *collector)
{
  collector->items = g_ptr_array_new_with_free_func (g_object_unref);
}

static void
hud_menu_model_collector_iface_init (HudSourceInterface *iface)
{
  iface->search = hud_menu_model_collector_search;
}

static void
hud_menu_model_collector_class_init (HudMenuModelCollectorClass *class)
{
  class->finalize = hud_menu_model_collector_finalize;
}

/**
 * hud_menu_model_collector_get:
 * @window: a #BamfWindow
 *
 * If the given @window has #GMenuModel-style menus then returns a
 * collector for them, otherwise returns %NULL.
 *
 * Returns: a #HudMenuModelCollector, or %NULL
 **/
HudMenuModelCollector *
hud_menu_model_collector_get (BamfWindow *window)
{
  HudMenuModelCollector *collector;
  gchar *unique_bus_name;
  gchar *app_menu_object_path;
  gchar *menubar_object_path;
  gchar *application_object_path;
  gchar *window_object_path;
  GDBusConnection *session;

  unique_bus_name = bamf_window_get_utf8_prop (window, "_GTK_UNIQUE_BUS_NAME");

  if (!unique_bus_name)
    /* If this isn't set, we won't get very far... */
    return NULL;

  collector = g_object_new (HUD_TYPE_MENU_MODEL_COLLECTOR, NULL);

  app_menu_object_path = bamf_window_get_utf8_prop (window, "_GTK_APP_MENU_OBJECT_PATH");
  menubar_object_path = bamf_window_get_utf8_prop (window, "_GTK_MENUBAR_OBJECT_PATH");
  application_object_path = bamf_window_get_utf8_prop (window, "_GTK_APPLICATION_OBJECT_PATH");
  window_object_path = bamf_window_get_utf8_prop (window, "_GTK_WINDOW_OBJECT_PATH");

  session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  if (app_menu_object_path)
    {
      collector->app_menu = g_dbus_menu_model_get (session, unique_bus_name, app_menu_object_path);
      hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->app_menu));
    }

  if (menubar_object_path)
    {
      collector->menubar = g_dbus_menu_model_get (session, unique_bus_name, menubar_object_path);
      hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->menubar));
    }

  if (application_object_path)
    collector->application = g_dbus_action_group_get (session, unique_bus_name, application_object_path);

  if (window_object_path)
    collector->window = g_dbus_action_group_get (session, unique_bus_name, window_object_path);

  /* when the action groups change, we could end up having items
   * enabled/disabled.  how to deal with that?
   */

  g_free (unique_bus_name);
  g_free (app_menu_object_path);
  g_free (menubar_object_path);
  g_free (application_object_path);
  g_free (window_object_path);

  g_object_unref (session);

  return collector;
}
