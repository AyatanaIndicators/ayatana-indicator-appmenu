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

#define G_LOG_DOMAIN "hudappmenuregistrar"

#include "hudappmenuregistrar.h"

#include <gio/gio.h>

/**
 * SECTION:hudappmenuregistrar
 * @title: HudAppMenuRegistrar
 * @short_description: client for the com.canonical.AppMenu.Registrar
 *   D-Bus service
 *
 * The #HudAppMenuRegistrar is a singleton object that monitors the
 * com.canonical.AppMenu.Registrar D-Bus service.
 *
 * On instantiation, a D-Bus name watch is setup for the registrar.
 * When the registrar is found to exist, a local copy is made of the
 * windows and menus that the registrar knows about.  Change
 * notifications are also monitored to keep the local cache in sync.
 *
 * After that point, all queries for information from the registrar are
 * satisfied from the local cache, without blocking.
 *
 * Information is acquired from #HudAppMenuRegistrar by using
 * hud_app_menu_registrar_add_observer().  This immediately calls a
 * callback with the initial information and makes future calls to the
 * same callback if the information is found to have changed.
 *
 * If the registrar is offline or the information is not yet available
 * at the time of the original query, the window will initially be
 * reported as having no menu but a change notification will arrive when
 * the proper information becomes available.
 **/

#define APPMENU_REGISTRAR_BUS_NAME    "com.canonical.AppMenu.Registrar"
#define APPMENU_REGISTRAR_OBJECT_PATH "/com/canonical/AppMenu/Registrar"
#define APPMENU_REGISTRAR_IFACE       "com.canonical.AppMenu.Registrar"

typedef struct
{
  HudAppMenuRegistrarObserverFunc callback;
  gpointer                        user_data;
} HudAppMenuRegistrarObserver;

typedef struct
{
  guint xid;
  gchar *bus_name;
  gchar *object_path;
  GSList *observers;
} HudAppMenuRegistrarWindow;

struct _HudAppMenuRegistrar
{
  GObject parent_instance;

  guint subscription;
  GCancellable *cancellable;
  GHashTable *windows;
  gboolean notifying;
  gboolean ready;
};

typedef GObjectClass HudAppMenuRegistrarClass;

G_DEFINE_TYPE (HudAppMenuRegistrar, hud_app_menu_registrar, G_TYPE_OBJECT)

static void
hud_app_menu_registrar_window_free (gpointer user_data)
{
  HudAppMenuRegistrarWindow *window = user_data;

  g_assert (window->bus_name == NULL);
  g_assert (window->object_path == NULL);
  g_assert (window->observers == NULL);

  g_debug ("free window instance for %u", window->xid);

  g_slice_free (HudAppMenuRegistrarWindow, window);
}

static HudAppMenuRegistrarWindow *
hud_app_menu_registrar_get_window (HudAppMenuRegistrar *registrar,
                                   guint                xid)
{
  HudAppMenuRegistrarWindow *window;

  window = g_hash_table_lookup (registrar->windows, GINT_TO_POINTER (xid));

  if (!window)
    {
      window = g_slice_new0 (HudAppMenuRegistrarWindow);
      window->xid = xid;

      g_debug ("create window instance for %u", xid);
      g_hash_table_insert (registrar->windows, GINT_TO_POINTER (xid), window);
    }

  return window;
}

static void
hud_app_menu_registrar_possibly_free_window (HudAppMenuRegistrar       *registrar,
                                             HudAppMenuRegistrarWindow *window)
{
  if (window->bus_name == NULL && window->observers == NULL)
    g_hash_table_remove (registrar->windows, GINT_TO_POINTER (window->xid));
}

static void
hud_app_menu_registrar_notify_window_observers (HudAppMenuRegistrar       *registrar,
                                                HudAppMenuRegistrarWindow *window)
{
  GSList *node;

  registrar->notifying = TRUE;

  for (node = window->observers; node; node = node->next)
    {
      HudAppMenuRegistrarObserver *observer = node->data;

      g_debug ("notifying %p about %u", observer->user_data, window->xid);
      (* observer->callback) (registrar, window->xid, window->bus_name, window->object_path, observer->user_data);
    }

  registrar->notifying = FALSE;
}

static void
hud_app_menu_registrar_dbus_signal (GDBusConnection *connection,
                                    const gchar     *sender_name,
                                    const gchar     *object_path,
                                    const gchar     *interface_name,
                                    const gchar     *signal_name,
                                    GVariant        *parameters,
                                    gpointer         user_data)
{
  HudAppMenuRegistrar *registrar = user_data;

  g_debug ("got signal");

  if (!registrar->ready)
    {
      g_debug ("not ready, so ignoring signal");
      return;
    }

  if (g_str_equal (signal_name, "WindowRegistered"))
    {
      HudAppMenuRegistrarWindow *window;
      guint xid;

      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(uso)")))
          return;

      g_variant_get_child (parameters, 0, "u", &xid);
      window = hud_app_menu_registrar_get_window (registrar, xid);

      g_free (window->bus_name);
      g_variant_get_child (parameters, 1, "s", &window->bus_name);
      g_free (window->object_path);
      g_variant_get_child (parameters, 2, "o", &window->object_path);

      g_debug ("xid %u is now at (%s, %s)", xid, window->bus_name, window->object_path);

      hud_app_menu_registrar_notify_window_observers (registrar, window);
    }

  else if (g_str_equal (signal_name, "WindowUnregistered"))
    {
      HudAppMenuRegistrarWindow *window;
      guint xid;

      if (!g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(u)")))
        return;

      g_variant_get (parameters, 0, "u", &xid);

      g_debug ("xid %u disappeared", xid);

      window = hud_app_menu_registrar_get_window (registrar, xid);

      g_free (window->bus_name);
      window->bus_name = NULL;
      g_free (window->object_path);
      window->object_path = NULL;

      hud_app_menu_registrar_notify_window_observers (registrar, window);

      hud_app_menu_registrar_possibly_free_window (registrar, window);
    }
}

static void
hud_app_menu_registrar_ready (GObject      *source,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  HudAppMenuRegistrar *registrar = user_data;
  GError *error = NULL;
  GVariant *reply;

  g_debug ("GetMenus returned");

  g_clear_object (&registrar->cancellable);

  reply = g_dbus_connection_call_finish (G_DBUS_CONNECTION (source), result, &error);

  if (reply)
    {
      GVariantIter *iter;
      const gchar *bus_name;
      const gchar *object_path;
      guint xid;

      g_assert (!registrar->ready);
      registrar->ready = TRUE;

      g_debug ("going ready");

      g_variant_get (reply, "(a(uso))", &iter);

      while (g_variant_iter_next (iter, "(u&s&o)", &xid, &bus_name, &object_path))
        {
          HudAppMenuRegistrarWindow *window;

          window = hud_app_menu_registrar_get_window (registrar, xid);

          /* we were not ready until now, so we expect this to be unset */
          g_assert (window->bus_name == NULL);

          window->bus_name = g_strdup (bus_name);
          window->object_path = g_strdup (object_path);

          hud_app_menu_registrar_notify_window_observers (registrar, window);
        }

      g_variant_iter_free (iter);
      g_variant_unref (reply);
    }
  else
    {
      g_warning ("GetMenus returned an error: %s", error->message);
      g_error_free (error);
    }

  g_object_unref (registrar);

  g_debug ("done handling GetMenus reply");
}

static void
hud_app_menu_registrar_name_appeared (GDBusConnection *connection,
                                      const gchar     *name,
                                      const gchar     *name_owner,
                                      gpointer         user_data)
{
  HudAppMenuRegistrar *registrar = user_data;

  g_debug ("name appeared (owner is %s)", name_owner);

  g_assert (registrar->subscription == 0);
  registrar->subscription = g_dbus_connection_signal_subscribe (connection, name_owner,
                                                                APPMENU_REGISTRAR_IFACE, NULL,
                                                                APPMENU_REGISTRAR_OBJECT_PATH, NULL,
                                                                G_DBUS_SIGNAL_FLAGS_NONE,
                                                                hud_app_menu_registrar_dbus_signal,
                                                                registrar, NULL);

  g_assert (registrar->cancellable == NULL);
  registrar->cancellable = g_cancellable_new ();
  g_dbus_connection_call (connection, name_owner, APPMENU_REGISTRAR_OBJECT_PATH,
                          APPMENU_REGISTRAR_IFACE, "GetMenus", NULL, G_VARIANT_TYPE ("(a(uso))"),
                          G_DBUS_CALL_FLAGS_NONE, -1, registrar->cancellable,
                          hud_app_menu_registrar_ready, g_object_ref (registrar));
}

static void
hud_app_menu_registrar_name_vanished (GDBusConnection *connection,
                                      const gchar     *name,
                                      gpointer         user_data)
{
  HudAppMenuRegistrar *registrar = user_data;

  g_debug ("name vanished");

  if (registrar->subscription > 0)
    {
      g_dbus_connection_signal_unsubscribe (connection, registrar->subscription);
      registrar->subscription = 0;
    }

  if (registrar->cancellable)
    {
      g_cancellable_cancel (registrar->cancellable);
      g_clear_object (&registrar->cancellable);
    }

  if (registrar->ready)
    {
      GHashTableIter iter;
      gpointer value;

      registrar->ready = FALSE;

      g_hash_table_iter_init (&iter, registrar->windows);
      while (g_hash_table_iter_next (&iter, NULL, &value))
        {
          HudAppMenuRegistrarWindow *window = value;

          g_free (window->bus_name);
          window->bus_name = NULL;
          g_free (window->object_path);
          window->object_path = NULL;

          hud_app_menu_registrar_notify_window_observers (registrar, window);

          /* Cannot go the normal route here because we are iterating... */
          if (window->observers == NULL)
            g_hash_table_iter_remove (&iter);
        }
    }
}

static void
hud_app_menu_registrar_finalize (GObject *object)
{
  /* This is an immortal singleton.  If we're here, we have trouble. */
  g_assert_not_reached ();
}

static void
hud_app_menu_registrar_init (HudAppMenuRegistrar *registrar)
{
  g_debug ("online");

  registrar->windows = g_hash_table_new_full (NULL, NULL, NULL, hud_app_menu_registrar_window_free);
  g_bus_watch_name (G_BUS_TYPE_SESSION, APPMENU_REGISTRAR_BUS_NAME, G_BUS_NAME_WATCHER_FLAGS_NONE,
                    hud_app_menu_registrar_name_appeared, hud_app_menu_registrar_name_vanished,
                    g_object_ref (registrar), g_object_unref);
}

static void
hud_app_menu_registrar_class_init (HudAppMenuRegistrarClass *class)
{
  class->finalize = hud_app_menu_registrar_finalize;
}

/**
 * HudAppMenuRegistrarObserverFunc:
 * @registrar: the #HudAppMenuRegistrar
 * @xid: the xid that we are notifying about
 * @bus_name: the bus name for the dbusmenu, or %NULL
 * @object_path: the object path for the dbusmenu, or %NULL
 * @user_data: the data pointer
 *
 * Notifies about the initial values for or changes to the bus name and
 * object path at which to find the dbusmenu for @xid.
 *
 * You should pass the values of @bus_name and @object_path to
 * dbusmenu_client_new() to get started.
 *
 * If no menu is available then @bus_name and @object_path will both be
 * given as %NULL.
 **/

/**
 * hud_app_menu_registrar_add_observer:
 * @registrar: the #HudAppMenuRegistrar
 * @xid: the xid to begin observing
 * @callback: a #HudAppMenuRegistrarObserverFunc
 * @user_data: user data for @callback
 *
 * Begins observing @xid.
 *
 * @callback will be called exactly once before the function returns
 * with a set of initial values (the bus name and object path at which
 * to find the menu for the window).
 *
 * If the location of the menu for @xid changes (including being created
 * or destroyed) then @callback will be called each time an update is
 * required.
 *
 * It is possible that the values are not initially known because they
 * have not yet been retreived from the registrar or because the
 * registrar is not running.  In this case, %NULL values will be
 * provided initially and @callback will be invoked again when the
 * real values are known.
 *
 * Call hud_app_menu_registrar_remove_observer() to when you are no
 * longer interested in @xid.
 **/
void
hud_app_menu_registrar_add_observer (HudAppMenuRegistrar             *registrar,
                                     guint                            xid,
                                     HudAppMenuRegistrarObserverFunc  callback,
                                     gpointer                         user_data)
{
  HudAppMenuRegistrarObserver *observer;
  HudAppMenuRegistrarWindow *window;

  g_return_if_fail (xid != 0);
  g_return_if_fail (callback != NULL);
  g_return_if_fail (!registrar->notifying);

  g_debug ("observer added for xid %u (%p)", xid, user_data);

  observer = g_slice_new (HudAppMenuRegistrarObserver);
  observer->callback = callback;
  observer->user_data = user_data;

  window = hud_app_menu_registrar_get_window (registrar, xid);
  window->observers = g_slist_prepend (window->observers, observer);

  /* send the first update */
  (* callback) (registrar, xid, window->bus_name, window->object_path, user_data);
}

/**
 * hud_app_menu_registrar_remove_observer:
 * @registrar: the #HudAppMenuRegistrar
 * @xid: the xid to begin observing
 * @callback: a #HudAppMenuRegistrarObserverFunc
 * @user_data: user data for @callback
 *
 * Reverses the effect of a previous call to
 * hud_app_menu_registrar_add_observer().
 *
 * @callback and @user_data must be exactly equal to the values passed
 * to that function.
 *
 * One call does not remove all instances of @callback and @user_data.
 * You need to call this function the same number of times that you
 * called hud_app_menu_registrar_add_observer().
 **/
void
hud_app_menu_registrar_remove_observer (HudAppMenuRegistrar             *registrar,
                                        guint                            xid,
                                        HudAppMenuRegistrarObserverFunc  callback,
                                        gpointer                         user_data)
{
  HudAppMenuRegistrarWindow *window;
  GSList **node;

  g_return_if_fail (xid != 0);
  g_return_if_fail (callback != NULL);
  g_return_if_fail (!registrar->notifying);

  g_debug ("observer removed for xid %u (%p)", xid, user_data);

  window = hud_app_menu_registrar_get_window (registrar, xid);
  for (node = &window->observers; *node; node = &(*node)->next)
    {
      HudAppMenuRegistrarObserver *observer = (*node)->data;

      if (observer->callback == callback && observer->user_data == user_data)
        {
          g_slice_free (HudAppMenuRegistrarObserver, observer);
          *node = g_slist_delete_link (*node, *node);
          break;
        }
    }

  hud_app_menu_registrar_possibly_free_window (registrar, window);
}

/**
 * hud_app_menu_registrar_get:
 *
 * Gets the singleton instance of #HudAppMenuRegistrar.
 *
 * Returns: (transfer none): the instance
 **/
HudAppMenuRegistrar *
hud_app_menu_registrar_get (void)
{
  static HudAppMenuRegistrar *singleton;

  if (!singleton)
    singleton = g_object_new (HUD_TYPE_APP_MENU_REGISTRAR, NULL);

  return singleton;
}
