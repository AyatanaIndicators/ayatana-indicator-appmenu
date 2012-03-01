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

#define G_LOG_DOMAIN "hudwindowsource"

#include "hudwindowsource.h"

#include <libbamf/libbamf.h>
#include <string.h>

#include "hudmenumodelcollector.h"
#include "huddbusmenucollector.h"
#include "hudsource.h"

/**
 * SECTION:hudwindowsource
 * @title: HudWindowSource
 * @short_description: a #HudSource for the menubars of windows
 *
 * #HudWindowSource is a #HudSource that allows searching for items in
 * the menubars of application windows.
 *
 * The source tracks which is the active window of the application,
 * using BAMF.  hud_source_search() calls will be redirected to an
 * appropriate source corresponding to the active window.  When the
 * active window changes, the HudSource::changed signal will be emitted.
 *
 * #GMenuModel and Dbusmenu-style menus are both understood.  They are
 * implemented via #HudMenuModelCollector and #HudDbusmenuCollector,
 * respectively.
 *
 * #HudWindowSource takes care to avoid various bits of desktop chrome
 * from becoming considered as the active window.  This is done via a
 * built-in blacklist.  It is also possible, for testing purposes, to
 * use the <envar>INDICATOR_APPMENU_DEBUG_APPS</envar> environment
 * variable to specify a list of desktop file names corresponding to
 * applications to ignore windows from (for example, the terminal).
 **/

/**
 * HudWindowSource:
 *
 * This is an opaque structure type.
 **/

struct _HudWindowSource
{
  GObject parent_instance;

  BamfMatcher *matcher;

  BamfWindow *active_window;
};

typedef GObjectClass HudWindowSourceClass;

static void hud_window_source_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudWindowSource, hud_window_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_window_source_iface_init))

static gboolean
hud_window_source_desktop_file_in_debug_list (const gchar *desktop_file)
{
  static GStrv debug_list = NULL;
  gint i;

  /* Looks at the envvar to see if there is a list of items that we shouldn't
     view as focus changes so that we can use those tools for debugging */
  if (debug_list == NULL)
    {
      const gchar * dbgenv = g_getenv ("INDICATOR_APPMENU_DEBUG_APPS");
      if (dbgenv != NULL)
        debug_list = g_strsplit (dbgenv, ":", 0);
      else
        debug_list = g_new0 (gchar *, 1);
    }

  g_debug ("checking desktop file '%s'", desktop_file);

  for (i = 0; debug_list[i] != NULL; i++)
    if (debug_list[i][0] != '\0' && strstr (desktop_file, debug_list[i]))
      {
        g_debug ("desktop file name '%s' blocked (hit debug list item '%s')", desktop_file, debug_list[i]);
        return TRUE;
      }

  return FALSE;
}

static gboolean
hud_window_source_name_in_ignore_list (BamfWindow *window)
{
  static const gchar * const ignored_names[] = {
    "Hud Prototype Test",
    "Hud",
    "DNDCollectionWindow",
    "launcher",
    "dash",
    "Dash",
    "panel",
    "hud",
    "unity-2d-shell"
  };
  gboolean ignored = FALSE;
  gchar *window_name;
  gint i;

  window_name = bamf_view_get_name (BAMF_VIEW (window));
  g_debug ("checking window name '%s'", window_name);

  for (i = 0; i < G_N_ELEMENTS (ignored_names); i++)
    if (g_str_equal (ignored_names[i], window_name))
      {
        g_debug ("window name '%s' blocked", window_name);
        ignored = TRUE;
        break;
      }

  g_free (window_name);

  return ignored;
}

static gboolean
hud_window_source_should_ignore_window (HudWindowSource *source,
                                        BamfWindow      *window)
{
  BamfApplication *application;
  const gchar *desktop_file;

  application = bamf_matcher_get_application_for_window (source->matcher, window);

  if (application == NULL)
    {
      g_debug ("ignoring window with no application");
      return TRUE;
    }

  desktop_file = bamf_application_get_desktop_file (application);

  if (desktop_file == NULL)
    {
      g_debug ("ignoring application with no desktop file");
      return TRUE;
    }

  if (hud_window_source_desktop_file_in_debug_list (desktop_file))
    return TRUE;

  if (hud_window_source_name_in_ignore_list (window))
    return TRUE;

  return FALSE;
}

static HudSource *
hud_window_source_get_collector (HudWindowSource *source)
{
  static GQuark menu_collector_quark;
  HudSource *collector;

  if (source->active_window == NULL)
    return NULL;

  if (!menu_collector_quark)
    menu_collector_quark = g_quark_from_string ("menu collector");

  collector = g_object_get_qdata (G_OBJECT (source->active_window), menu_collector_quark);
  if (collector == NULL)
    {
      HudMenuModelCollector *menumodel_collector;

      /* GMenuModel menus either exist at the start or will never exist.
       * dbusmenu menus can appear later.
       *
       * For that reason, we check first for GMenuModel and assume if it
       * doesn't exist then it must be dbusmenu.
       */
      menumodel_collector = hud_menu_model_collector_get (source->active_window);
      if (menumodel_collector)
        collector = HUD_SOURCE (menumodel_collector);
      else
        collector = HUD_SOURCE (hud_dbusmenu_collector_new_for_window (source->active_window));

      g_object_set_qdata_full (G_OBJECT (source->active_window), menu_collector_quark, collector, g_object_unref);
    }

  return collector;
}

static void
hud_window_source_collector_changed (HudSource *collector,
                                     gpointer   user_data)
{
  HudWindowSource *source = user_data;

  hud_source_changed (HUD_SOURCE (source));
}

static void
hud_window_source_active_window_changed (BamfMatcher *matcher,
                                         BamfView    *oldview,
                                         BamfView    *newview,
                                         gpointer     user_data)
{
  HudWindowSource *source = user_data;
  HudSource *collector;
  BamfWindow *window;

  g_debug ("Switching windows");

  if (!BAMF_IS_WINDOW (newview))
    {
      g_debug ("ignoring switch to non-window");
      return;
    }

  window = BAMF_WINDOW (newview);

  if (window == source->active_window)
    {
      g_debug ("this is already the active window");
      return;
    }

  if (hud_window_source_should_ignore_window (source, window))
    return;

  g_debug ("new active window (xid %u)", bamf_window_get_xid (window));

  collector = hud_window_source_get_collector (source);
  if (collector)
    g_signal_handlers_disconnect_by_func (collector, hud_window_source_collector_changed, source);

  g_clear_object (&source->active_window);
  source->active_window = g_object_ref (window);

  collector = hud_window_source_get_collector (source);
  g_signal_connect_object (collector, "changed", G_CALLBACK (hud_window_source_collector_changed), source, 0);

  hud_source_changed (HUD_SOURCE (source));
}

static void
hud_window_source_search (HudSource   *hud_source,
                          GPtrArray   *results_array,
                          const gchar *search_string)
{
  HudWindowSource *source = HUD_WINDOW_SOURCE (hud_source);
  HudSource *collector;

  collector = hud_window_source_get_collector (source);

  if (collector)
    hud_source_search (collector, results_array, search_string);
}

static void
hud_window_source_finalize (GObject *object)
{
  HudWindowSource *source = HUD_WINDOW_SOURCE (object);

  /* bamf matcher signals already disconnected in dispose */
  g_clear_object (&source->active_window);

  G_OBJECT_CLASS (hud_window_source_parent_class)
    ->finalize (object);
}

static void
hud_window_source_init (HudWindowSource *source)
{
  source->matcher = bamf_matcher_get_default ();

  g_signal_connect_object (source->matcher, "active-window-changed",
                           G_CALLBACK (hud_window_source_active_window_changed), source, 0);
}

static void
hud_window_source_iface_init (HudSourceInterface *iface)
{
  iface->search = hud_window_source_search;
}

static void
hud_window_source_class_init (HudWindowSourceClass *class)
{
  class->finalize = hud_window_source_finalize;
}

/**
 * hud_window_source_new:
 *
 * Creates a #HudWindowSource.
 *
 * Returns: a new #HudWindowSource
 **/
HudWindowSource *
hud_window_source_new (void)
{
  return g_object_new (HUD_TYPE_WINDOW_SOURCE, NULL);
}
