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
 * Authors: Robert Carr
 */

#include "hudwebappsource.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

#include <libbamf/libbamf.h>
#include <libbamf/bamf-application.h>

#include "hudsettings.h"
#include "huddbusmenucollector.h"
#include "hudsource.h"

/**
 * SECTION:hudwebappsource
 * @title:HudWebappSource
 * @short_description: a #HudSource to search through the menus of
 *   webapps registered with the Unity Webapps System.
 *
 * #HudWebappsSource searches through the menu items of webapps
 **/

/**
 * HudWebappSource:
 *
 * This is an opaque structure type.
 **/

struct _HudWebappSource
{
  GObject parent_instance;
  
  GList *applications;
};

typedef GObjectClass HudWebappSourceClass;

typedef struct _HudWebappApplicationSource {
  BamfApplication *application;

  HudSource *source;
  HudSource *collector;
} HudWebappApplicationSource;

static void hud_webapp_source_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (HudWebappSource, hud_webapp_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, hud_webapp_source_iface_init))



static void
hud_webapp_source_collector_changed (HudSource *source,
				     gpointer   user_data)
{
  hud_source_changed ((HudSource *)user_data);
}


static void
hud_webapp_source_search (HudSource   *hud_source,
			  GPtrArray   *results_array,
			  const gchar *search_string)
{
  HudWebappSource *source;
  GList *walk;
  
  source = HUD_WEBAPP_SOURCE (hud_source);
  
  for (walk = source->applications; walk != NULL; walk = walk->next)
    {
      HudWebappApplicationSource *application_source;
      
      application_source = (HudWebappApplicationSource *)walk->data;

      hud_source_search (application_source->collector, results_array, search_string);
    }
}

HudWebappApplicationSource *
hud_webapp_application_source_new (HudSource *source,
				   BamfApplication *application)
{
  HudWebappApplicationSource *application_source;
  gchar *name, *path;

  name = path = NULL;
  
  bamf_application_get_application_menu (application,
					 &name, &path);
  
  if (name == NULL || *name == '\0')
    return NULL;

  application_source = g_malloc0 (sizeof (HudWebappApplicationSource));
  
  application_source->application = g_object_ref (G_OBJECT (application));
  application_source->source = source;
  
  application_source->collector = (HudSource *)hud_dbusmenu_collector_new_for_endpoint (bamf_view_get_name (BAMF_VIEW (application)),
											50,
											name, path);
  
  g_free (name);
  g_free (path);
  
  return application_source;
}

static void
hud_webapp_application_source_free (HudWebappApplicationSource *application_source)
{
  g_object_unref (G_OBJECT (application_source->application));
  g_object_unref (G_OBJECT (application_source->collector));
  
  g_free (application_source);
}


static void
on_active_changed (BamfApplication *application,
		   gboolean active,
		   HudSource *source)
{
  hud_source_changed (source);
}

static void
hud_webapp_source_remove_application (HudWebappSource *source,
				      BamfApplication *application)
{
  HudWebappApplicationSource *application_source;
  GList *walk;
  
  application_source = NULL;
  
  for (walk = source->applications; walk != NULL; walk = walk->next)
    {
      HudWebappApplicationSource *t;
      
      t = (HudWebappApplicationSource *)walk->data;
      
      if (t->application == application)
	application_source = t;
    }
  
  if (application_source == NULL)
    return;
  
  g_signal_handlers_disconnect_by_func (application_source->collector,
					G_CALLBACK (hud_webapp_source_collector_changed),
					source);
  g_signal_handlers_disconnect_by_func (application, G_CALLBACK (on_active_changed),
					source);
  
  source->applications = g_list_remove (source->applications, application_source);
  
  hud_webapp_application_source_free (application_source);
  
  hud_webapp_source_collector_changed ((HudSource *)source, source);
  
}

static void
hud_webapp_source_bamf_view_closed (BamfView *view,
				    gpointer user_data)
{
  HudWebappSource *source;
  
  source = (HudWebappSource *)user_data;
  hud_webapp_source_remove_application (source, (BamfApplication *)user_data);
}

static void
hud_webapp_source_register_application (HudWebappSource *source,
					BamfApplication *application)
{
  HudWebappApplicationSource *application_source;
  
  application_source = hud_webapp_application_source_new (HUD_SOURCE (source), application);
  
  if (application_source == NULL)
    return;
  
  g_signal_connect (application_source->collector, "changed", G_CALLBACK (hud_webapp_source_collector_changed), source);
  
  source->applications = g_list_append (source->applications, application_source);
  
  hud_webapp_source_collector_changed ((HudSource *)source, source);
  
  g_signal_connect (application, "active-changed", G_CALLBACK (on_active_changed), source);
  g_signal_connect (application, "closed", G_CALLBACK (hud_webapp_source_bamf_view_closed), source);
}

static void
hud_webapp_source_finalize (GObject *object)
{
  g_assert_not_reached ();
}

static void
hud_webapp_source_bamf_view_opened (BamfMatcher *matcher,
				    BamfView *view,
				    gpointer user_data)
{
  HudWebappSource *source;
  BamfApplication *application;
  
  source = (HudWebappSource *)user_data;
  
  if (BAMF_IS_APPLICATION (view) == FALSE)
    return;
  
  application = BAMF_APPLICATION (view);
  
  if (g_strcmp0 (bamf_application_get_application_type (application), "webapp") != 0)
    return;
  
  hud_webapp_source_register_application (source, application);
}

static void
hud_webapp_source_init (HudWebappSource *source)
{
  BamfMatcher *matcher;
  GList *applications, *walk;
  
  source->applications = NULL;
  
  matcher = bamf_matcher_get_default ();
  
  applications = bamf_matcher_get_applications (matcher);
  
  for (walk = applications; walk != NULL; walk = walk -> next)
    {
      BamfApplication *application;
      
      application = (BamfApplication *)walk->data;
      
      if (g_strcmp0 (bamf_application_get_application_type (application), "webapp") != 0)
	continue;
      
      hud_webapp_source_register_application (source, application);      
    }
  
  g_signal_connect (matcher, "view-opened", G_CALLBACK (hud_webapp_source_bamf_view_opened), source);
  
  g_list_free (applications);
}

static void
hud_webapp_source_iface_init (HudSourceInterface *iface)
{
  iface->search = hud_webapp_source_search;
}

static void
hud_webapp_source_class_init (HudWebappSourceClass *class)
{
  class->finalize = hud_webapp_source_finalize;
}

/**
 * hud_webapp_source_new:
 *
 * Creates a #HudWebappSource.
 *
 * Returns: a new #HudWebappSource
 **/
HudWebappSource *
hud_webapp_source_new (void)
{
  return g_object_new (HUD_TYPE_WEBAPP_SOURCE, NULL);
}
