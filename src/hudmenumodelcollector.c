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

typedef struct _HudMenuModelContext HudMenuModelContext;

struct _HudMenuModelContext
{
  HudStringList *tokens;
  gchar *action_namespace;
  gint ref_count;
};

struct _HudMenuModelCollector
{
  GObject parent_instance;

  /* Cancelled on finalize */
  GCancellable *cancellable;

  /* GDBus shared session bus and D-Bus name of the app/indicator */
  GDBusConnection *session;
  gchar *unique_bus_name;

  /* If this is an application, is_application will be set and
   * 'application' and 'window' will contain the two action groups for
   * the window that we are collecting.
   *
   * If this is an indicator, is_application will be false and the
   * (singular) action group for the indicator will be in 'application'.
   */
  GDBusActionGroup *application;
  GDBusActionGroup *window;
  gboolean is_application;

  /* The GMenuModel for the app menu.
   *
   * If this is an indicator, the indicator menu is stored here.
   *
   * app_menu_is_hud_aware is TRUE if we should send HudActiveChanged
   * calls to app_menu_object_path when our use_count goes above 0.
   */
  GDBusMenuModel *app_menu;
  gchar *app_menu_object_path;
  gboolean app_menu_is_hud_aware;

  /* Ditto for the menubar.
   *
   * If this is an indicator then these will all be unset.
   */
  GDBusMenuModel *menubar;
  gchar *menubar_object_path;
  gboolean menubar_is_hud_aware;

  /* Boring details about the app/indicator we are showing. */
  gchar *prefix;
  gchar *desktop_file;
  gchar *icon;
  guint penalty;

  /* Each time we see a new menumodel added we add it to 'models', start
   * watching it for changes and add its contents to 'items', possibly
   * finding more menumodels to do the same to.
   *
   * Each time an item is removed, we schedule an idle (in 'refresh_id')
   * to wipe out all the 'items', disconnect signals from each model in
   * 'models' and add them all back again.
   *
   * Searching just iterates over 'items'.
   */
  GPtrArray *items;
  GSList *models;
  guint refresh_id;

  /* Keep track of our use_count in order to send signals to HUD-aware
   * apps and indicators.
   */
  gint use_count;
};

typedef struct
{
  HudItem parent_instance;

  GRemoteActionGroup *group;
  gchar *action_name;
  GVariant *target;
} HudModelItem;

typedef HudItemClass HudModelItemClass;

static gchar *
hud_menu_model_context_get_action_name (HudMenuModelContext *context,
                                        const gchar         *action_name)
{
  if (context && context->action_namespace)
    /* Note: this will (intentionally) work if action_name is NULL */
    return g_strjoin (".", context->action_namespace, action_name, NULL);
  else
    return g_strdup (action_name);
}

static HudStringList *
hud_menu_model_context_get_label (HudMenuModelContext *context,
                                  const gchar         *label)
{
  HudStringList *parent_tokens = context ? context->tokens : NULL;

  if (label)
    return hud_string_list_cons_label (label, parent_tokens);
  else
    return hud_string_list_ref (parent_tokens);
}

static HudMenuModelContext *
hud_menu_model_context_ref (HudMenuModelContext *context)
{
  if (context)
    g_atomic_int_inc (&context->ref_count);

  return context;
}

static void
hud_menu_model_context_unref (HudMenuModelContext *context)
{
  if (context && g_atomic_int_dec_and_test (&context->ref_count))
    {
      hud_string_list_unref (context->tokens);
      g_free (context->action_namespace);
      g_slice_free (HudMenuModelContext, context);
    }
}

static HudMenuModelContext *
hud_menu_model_context_new (HudMenuModelContext *parent,
                            const gchar         *namespace,
                            const gchar         *label)
{
  HudMenuModelContext *context;

  /* If we would be an unmodified copy of the parent, just take a ref */
  if (!namespace && !label)
    return hud_menu_model_context_ref (parent);

  context = g_slice_new (HudMenuModelContext);
  context->action_namespace = hud_menu_model_context_get_action_name (parent, namespace);
  context->tokens = hud_menu_model_context_get_label (parent, label);
  context->ref_count = 1;

  return context;
}

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
hud_model_item_new (HudMenuModelCollector *collector,
                    HudMenuModelContext   *context,
                    const gchar           *label,
                    const gchar           *action_name,
                    GVariant              *target)
{
  HudModelItem *item;
  const gchar *stripped_action_name;
  gchar *full_action_name;
  GDBusActionGroup *group = NULL;
  HudStringList *full_label;

  full_action_name = hud_menu_model_context_get_action_name (context, action_name);

  if (collector->is_application)
    {
      /* For applications we support "app." and "win." actions and
       * deliver them to the application or the window, with the prefix
       * removed.
       */
      if (g_str_has_prefix (full_action_name, "app."))
        group = collector->application;
      else if (g_str_has_prefix (full_action_name, "win."))
        group = collector->window;

      stripped_action_name = full_action_name + 4;
    }
  else
    {
      /* For indicators, we deliver directly to the (one) action group
       * that we were given the object path for at construction.
       */
      stripped_action_name = full_action_name;
      group = collector->application;
    }

  if (!group)
    {
      g_free (full_action_name);
      return NULL;
    }

  full_label = hud_menu_model_context_get_label (context, label);

  item = hud_item_construct (hud_model_item_get_type (), full_label, collector->desktop_file, collector->icon, TRUE);
  item->group = g_object_ref (group);
  item->action_name = g_strdup (stripped_action_name);
  item->target = target ? g_variant_ref_sink (target) : NULL;

  hud_string_list_unref (full_label);
  g_free (full_action_name);

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
                                                 GMenuModel            *model,
                                                 HudMenuModelContext   *parent_context,
                                                 const gchar           *action_namespace,
                                                 const gchar           *label);
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
    hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->app_menu), NULL, NULL, collector->prefix);
  if (collector->menubar)
    hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->menubar), NULL, NULL, collector->prefix);

  g_slist_foreach (free_list, hud_menu_model_collector_disconnect, collector);
  g_slist_free_full (free_list, g_object_unref);

  return G_SOURCE_REMOVE;
}

static GQuark
hud_menu_model_collector_context_quark ()
{
  static GQuark context_quark;

  if (!context_quark)
    context_quark = g_quark_from_string ("menu item context");

  return context_quark;
}

static void
hud_menu_model_collector_model_changed (GMenuModel *model,
                                        gint        position,
                                        gint        removed,
                                        gint        added,
                                        gpointer    user_data)
{
  HudMenuModelCollector *collector = user_data;
  HudMenuModelContext *context;
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

  context = g_object_get_qdata (G_OBJECT (model), hud_menu_model_collector_context_quark ());

  changed = FALSE;
  for (i = position; i < position + added; i++)
    {
      GMenuModel *link;
      gchar *label = NULL;
      gchar *action_namespace = NULL;
      gchar *action = NULL;

      g_menu_model_get_item_attribute (model, i, "action-namespace", "s", &action_namespace);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_ACTION, "s", &action);
      g_menu_model_get_item_attribute (model, i, G_MENU_ATTRIBUTE_LABEL, "s", &label);

      /* Check if this is an action.  Here's where we may end up
       * creating a HudItem.
       */
      if (action && label)
        {
          GVariant *target;
          HudItem *item;

          target = g_menu_model_get_item_attribute_value (model, i, G_MENU_ATTRIBUTE_TARGET, NULL);

          item = hud_model_item_new (collector, context, label, action, target);

          if (item)
            g_ptr_array_add (collector->items, item);

          if (target)
            g_variant_unref (target);

          changed = TRUE;
        }

      /* For 'section' and 'submenu' links, we should recurse.  This is
       * where the danger comes in (due to the possibility of unbounded
       * recursion).
       */
      if ((link = g_menu_model_get_item_link (model, i, G_MENU_LINK_SECTION)))
        {
          hud_menu_model_collector_add_model (collector, link, context, action_namespace, label);
          g_object_unref (link);
        }

      if ((link = g_menu_model_get_item_link (model, i, G_MENU_LINK_SUBMENU)))
        {
          hud_menu_model_collector_add_model (collector, link, context, action_namespace, label);
          g_object_unref (link);
        }

      g_free (action_namespace);
      g_free (action);
      g_free (label);
    }

  if (changed)
    hud_source_changed (HUD_SOURCE (collector));
}

static void
hud_menu_model_collector_add_model (HudMenuModelCollector *collector,
                                    GMenuModel            *model,
                                    HudMenuModelContext   *parent_context,
                                    const gchar           *action_namespace,
                                    const gchar           *label)
{
  gint n_items;

  g_signal_connect (model, "items-changed", G_CALLBACK (hud_menu_model_collector_model_changed), collector);
  collector->models = g_slist_prepend (collector->models, g_object_ref (model));

  /* The tokens in 'context' are the list of strings that got us up to
   * where we are now, like "View > Toolbars".
   *
   * Strictly speaking, GMenuModel structures are DAGs, but we more or
   * less assume that they are trees here and replace the data
   * unconditionally when we visit it the second time (which will be
   * more or less never, because really, a menu is a tree).
   */
  g_object_set_qdata_full (G_OBJECT (model),
                           hud_menu_model_collector_context_quark (),
                           hud_menu_model_context_new (parent_context, action_namespace, label),
                           (GDestroyNotify) hud_menu_model_context_unref);

  n_items = g_menu_model_get_n_items (model);
  if (n_items > 0)
    hud_menu_model_collector_model_changed (model, 0, 0, n_items, collector);
}

static void
hud_menu_model_collector_disconnect (gpointer data,
                                     gpointer user_data)
{
  g_signal_handlers_disconnect_by_func (data, hud_menu_model_collector_model_changed, user_data);
}

static void
hud_menu_model_collector_active_changed (HudMenuModelCollector *collector,
                                         gboolean               active)
{
  if (collector->app_menu_is_hud_aware)
    g_dbus_connection_call (collector->session, collector->unique_bus_name, collector->app_menu_object_path,
                            "com.canonical.hud.Awareness", "HudActiveChanged", g_variant_new ("(b)", active),
                            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

  if (collector->menubar_is_hud_aware)
    g_dbus_connection_call (collector->session, collector->unique_bus_name, collector->app_menu_object_path,
                            "com.canonical.hud.Awareness", "HudActiveChanged", g_variant_new ("(b)", active),
                            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}

static void
hud_menu_model_collector_use (HudSource *source)
{
  HudMenuModelCollector *collector = HUD_MENU_MODEL_COLLECTOR (source);

  if (collector->use_count == 0)
    hud_menu_model_collector_active_changed (collector, TRUE);

  collector->use_count++;
}

static void
hud_menu_model_collector_unuse (HudSource *source)
{
  HudMenuModelCollector *collector = HUD_MENU_MODEL_COLLECTOR (source);

  collector->use_count--;

  if (collector->use_count == 0)
    hud_menu_model_collector_active_changed (collector, FALSE);
}

static void
hud_menu_model_collector_search (HudSource    *source,
                                 GPtrArray    *results_array,
                                 HudTokenList *search_string)
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
      result = hud_result_get_if_matched (item, search_string, collector->penalty);
      if (result)
        g_ptr_array_add (results_array, result);
    }
}
static void
hud_menu_model_collector_finalize (GObject *object)
{
  HudMenuModelCollector *collector = HUD_MENU_MODEL_COLLECTOR (object);

  g_cancellable_cancel (collector->cancellable);
  g_object_unref (collector->cancellable);

  if (collector->refresh_id)
    g_source_remove (collector->refresh_id);

  g_slist_free_full (collector->models, g_object_unref);
  g_clear_object (&collector->app_menu);
  g_clear_object (&collector->menubar);
  g_clear_object (&collector->application);
  g_clear_object (&collector->window);

  g_object_unref (collector->session);
  g_free (collector->unique_bus_name);
  g_free (collector->app_menu_object_path);
  g_free (collector->menubar_object_path);
  g_free (collector->prefix);
  g_free (collector->desktop_file);
  g_free (collector->icon);

  g_ptr_array_unref (collector->items);

  G_OBJECT_CLASS (hud_menu_model_collector_parent_class)
    ->finalize (object);
}

static void
hud_menu_model_collector_init (HudMenuModelCollector *collector)
{
  collector->items = g_ptr_array_new_with_free_func (g_object_unref);
  collector->cancellable = g_cancellable_new ();
}

static void
hud_menu_model_collector_iface_init (HudSourceInterface *iface)
{
  iface->use = hud_menu_model_collector_use;
  iface->unuse = hud_menu_model_collector_unuse;
  iface->search = hud_menu_model_collector_search;
}

static void
hud_menu_model_collector_class_init (HudMenuModelCollectorClass *class)
{
  class->finalize = hud_menu_model_collector_finalize;
}

static void
hud_menu_model_collector_hud_awareness_cb (GObject      *source,
                                           GAsyncResult *result,
                                           gpointer      user_data)
{
  GVariant *reply;

  /* The goal of this function is to set either the
   * app_menu_is_hud_aware or menubar_is_hud_aware flag (which we have a
   * pointer to in user_data) to TRUE in the case that the remote
   * appears to support the com.canonical.hud.Awareness protocol.
   *
   * If it supports it, the async call will be successful.  In that
   * case, we want to set *(gboolean *) user_data = TRUE;
   *
   * There are two cases that we don't want to do that write.  The first
   * is the event that the remote doesn't support the protocol.  In that
   * case, we will see an error when we inspect the result.  The other
   * is the case in which the flag to which user_data points no longer
   * exists (ie: collector has been finalized).  In this case, the
   * cancellable will have been cancelled and we will also see an error.
   *
   * Long story short: If we get any error, just do nothing.
   */

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, NULL);

  if (reply)
    {
      *(gboolean *) user_data = TRUE;
      g_variant_unref (reply);
    }
}

/**
 * hud_menu_model_collector_get:
 * @window: a #BamfWindow
 * @desktop_file: the desktop file of the application of @window
 * @icon: the application icon's name
 *
 * If the given @window has #GMenuModel-style menus then returns a
 * collector for them, otherwise returns %NULL.
 *
 * @desktop_file is used for usage tracking.
 *
 * Returns: a #HudMenuModelCollector, or %NULL
 **/
HudMenuModelCollector *
hud_menu_model_collector_get (BamfWindow  *window,
                              const gchar *desktop_file,
                              const gchar *icon)
{
  HudMenuModelCollector *collector;
  gchar *unique_bus_name;
  gchar *application_object_path;
  gchar *window_object_path;
  GDBusConnection *session;

  session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  if (!session)
    return NULL;

  unique_bus_name = bamf_window_get_utf8_prop (window, "_GTK_UNIQUE_BUS_NAME");

  if (!unique_bus_name)
    /* If this isn't set, we won't get very far... */
    return NULL;

  collector = g_object_new (HUD_TYPE_MENU_MODEL_COLLECTOR, NULL);
  collector->session = session;
  collector->unique_bus_name = unique_bus_name;

  collector->app_menu_object_path = bamf_window_get_utf8_prop (window, "_GTK_APP_MENU_OBJECT_PATH");
  collector->menubar_object_path = bamf_window_get_utf8_prop (window, "_GTK_MENUBAR_OBJECT_PATH");
  application_object_path = bamf_window_get_utf8_prop (window, "_GTK_APPLICATION_OBJECT_PATH");
  window_object_path = bamf_window_get_utf8_prop (window, "_GTK_WINDOW_OBJECT_PATH");

  if (collector->app_menu_object_path)
    {
      collector->app_menu = g_dbus_menu_model_get (session, unique_bus_name, collector->app_menu_object_path);
      hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->app_menu), NULL, NULL, NULL);
      g_dbus_connection_call (session, unique_bus_name, collector->app_menu_object_path,
                              "com.canonical.hud.Awareness", "CheckAwareness",
                              NULL, G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NONE, -1, collector->cancellable,
                              hud_menu_model_collector_hud_awareness_cb, &collector->app_menu_is_hud_aware);
    }

  if (collector->menubar_object_path)
    {
      collector->menubar = g_dbus_menu_model_get (session, unique_bus_name, collector->menubar_object_path);
      hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->menubar), NULL, NULL, NULL);
      g_dbus_connection_call (session, unique_bus_name, collector->app_menu_object_path,
                              "com.canonical.hud.Awareness", "CheckAwareness",
                              NULL, G_VARIANT_TYPE_UNIT, G_DBUS_CALL_FLAGS_NONE, -1, collector->cancellable,
                              hud_menu_model_collector_hud_awareness_cb, &collector->menubar_is_hud_aware);
    }

  if (application_object_path)
    collector->application = g_dbus_action_group_get (session, unique_bus_name, application_object_path);

  if (window_object_path)
    collector->window = g_dbus_action_group_get (session, unique_bus_name, window_object_path);

  collector->is_application = TRUE;
  collector->desktop_file = g_strdup (desktop_file);
  collector->icon = g_strdup (icon);

  /* when the action groups change, we could end up having items
   * enabled/disabled.  how to deal with that?
   */

  g_free (application_object_path);
  g_free (window_object_path);

  return collector;
}

/**
 * hud_menu_model_collector_new_for_endpoint:
 * @application_id: a unique identifier for the application
 * @prefix: the title to prefix to all items
 * @icon: the icon for the appliction
 * @penalty: the penalty to apply to all results
 * @bus_name: a D-Bus bus name
 * @object_path: an object path at the destination given by @bus_name
 *
 * Creates a new #HudMenuModelCollector for the specified endpoint.
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
 * Returns: a new #HudMenuModelCollector
 */
HudMenuModelCollector *
hud_menu_model_collector_new_for_endpoint (const gchar *application_id,
                                           const gchar *prefix,
                                           const gchar *icon,
                                           guint        penalty,
                                           const gchar *bus_name,
                                           const gchar *object_path)
{
  HudMenuModelCollector *collector;
  GDBusConnection *session;

  collector = g_object_new (HUD_TYPE_MENU_MODEL_COLLECTOR, NULL);

  session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

  collector->app_menu = g_dbus_menu_model_get (session, bus_name, object_path);
  collector->application = g_dbus_action_group_get (session, bus_name, object_path);

  collector->is_application = FALSE;
  collector->prefix = g_strdup (prefix);
  collector->desktop_file = g_strdup (application_id);
  collector->icon = g_strdup (icon);
  collector->penalty = penalty;

  hud_menu_model_collector_add_model (collector, G_MENU_MODEL (collector->app_menu), NULL, NULL, prefix);

  g_object_unref (session);

  return collector;
}
