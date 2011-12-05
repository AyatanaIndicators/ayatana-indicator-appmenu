/*
Tracks the various indicators to know when they come on and off
the bus for searching their menus.

Copyright 2011 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "indicator-tracker.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

typedef struct _SystemIndicator SystemIndicator;
struct _SystemIndicator {
	gchar * dbus_name;
	gchar * dbus_menu_path;
	gchar * indicator_name;
	gchar * user_visible_name;
};

SystemIndicator system_indicators[] = {
	{dbus_name: "com.canonical.indicator.datetime", dbus_menu_path: "/com/canonical/indicator/datetime/menu", indicator_name: "indicator-datetime",       user_visible_name: N_("Date") },
	{dbus_name: "com.canonical.indicator.session",  dbus_menu_path: "/com/canonical/indicator/session/menu",  indicator_name: "indicator-session-device", user_visible_name: N_("Device") },
	{dbus_name: "com.canonical.indicator.session",  dbus_menu_path: "/com/canonical/indicator/user/menu",     indicator_name: "indicator-session-user",   user_visible_name: N_("User") },
	{dbus_name: "com.canonical.indicators.sound",   dbus_menu_path: "/com/canonical/indicators/sound/menu",   indicator_name: "indicator-sound",          user_visible_name: N_("Sound") },
	{dbus_name: "com.canonical.indicator.messages", dbus_menu_path: "/com/canonical/indicator/messages/menu", indicator_name: "indicator-messages",       user_visible_name: N_("Messages") }
};

struct _IndicatorTrackerPrivate {
	GArray * indicators;
	guint watches[G_N_ELEMENTS(system_indicators)];

	GDBusProxy * app_proxy;
	GCancellable * app_proxy_cancel;
};

#define INDICATOR_TRACKER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_TRACKER_TYPE, IndicatorTrackerPrivate))

static void indicator_tracker_class_init (IndicatorTrackerClass *klass);
static void indicator_tracker_init       (IndicatorTracker *self);
static void indicator_tracker_dispose    (GObject *object);
static void indicator_tracker_finalize   (GObject *object);
static void system_watch_appeared        (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data);
static void system_watch_vanished        (GDBusConnection * connection, const gchar * name, gpointer user_data);
static void app_proxy_built              (GObject * object, GAsyncResult * result, gpointer user_data);

G_DEFINE_TYPE (IndicatorTracker, indicator_tracker, G_TYPE_OBJECT);

static void
indicator_tracker_class_init (IndicatorTrackerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (IndicatorTrackerPrivate));

	object_class->dispose = indicator_tracker_dispose;
	object_class->finalize = indicator_tracker_finalize;

	return;
}

static void
indicator_tracker_init (IndicatorTracker *self)
{
	self->priv = INDICATOR_TRACKER_GET_PRIVATE(self);
	self->priv->indicators = NULL;
	self->priv->app_proxy = NULL;
	self->priv->app_proxy_cancel = NULL;

	self->priv->indicators = g_array_new(FALSE, TRUE, sizeof(IndicatorTrackerIndicator));

	int indicator_cnt;
	for (indicator_cnt = 0; indicator_cnt < G_N_ELEMENTS(system_indicators); indicator_cnt++) {
		self->priv->watches[indicator_cnt] = g_bus_watch_name(G_BUS_TYPE_SESSION,
		                                                      system_indicators[indicator_cnt].dbus_name,
		                                                      G_BUS_NAME_WATCHER_FLAGS_NONE,
		                                                      system_watch_appeared, /* acquired */
		                                                      system_watch_vanished, /* vanished */
		                                                      self,
		                                                      NULL); /* free func */
	}

	self->priv->app_proxy_cancel = g_cancellable_new();

	g_dbus_proxy_new_for_bus(G_BUS_TYPE_SESSION,
	                         G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START | G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                         NULL, /* interface info */
	                         "com.canonical.indicator.application",
	                         "/com/canonical/indicator/application/service",
	                         "com.canonical.indicator.application.service",
	                         self->priv->app_proxy_cancel,
	                         app_proxy_built,
	                         self);

	return;
}

static void
indicator_tracker_dispose (GObject *object)
{
	IndicatorTracker * self = INDICATOR_TRACKER(object);

	int indicator_cnt;
	for (indicator_cnt = 0; indicator_cnt < G_N_ELEMENTS(system_indicators); indicator_cnt++) {
		if (self->priv->watches[indicator_cnt] != 0) {
			g_bus_unwatch_name(self->priv->watches[indicator_cnt]);
			self->priv->watches[indicator_cnt] = 0;
		}
	}

	if (self->priv->app_proxy_cancel != NULL) {
		g_cancellable_cancel(self->priv->app_proxy_cancel);
		g_object_unref(self->priv->app_proxy_cancel);
		self->priv->app_proxy_cancel = NULL;
	}

	if (self->priv->app_proxy != NULL) {
		g_object_unref(self->priv->app_proxy);
		self->priv->app_proxy = NULL;
	}

	G_OBJECT_CLASS (indicator_tracker_parent_class)->dispose (object);
	return;
}

static void
indicator_tracker_finalize (GObject *object)
{

	G_OBJECT_CLASS (indicator_tracker_parent_class)->finalize (object);
	return;
}

IndicatorTracker *
indicator_tracker_new (void)
{
	return INDICATOR_TRACKER(g_object_new(INDICATOR_TRACKER_TYPE, NULL));
}

GArray *
indicator_tracker_get_indicators (IndicatorTracker * tracker)
{
	g_return_val_if_fail(IS_INDICATOR_TRACKER(tracker), NULL);
	return tracker->priv->indicators;
}

/* Function watches names on Dbus to find out when the system indicators
   go on and off the bus.  This makes it so that we can handle them properly
   at the higher levels in the system.  If a system indicator gets added to
   the list it gets put the indicator structure. */
static void
system_watch_appeared (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));
	g_return_if_fail(name_owner != NULL);
	g_return_if_fail(name_owner[0] != '\0');
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);

	/* Check all the system indicators because there might be more than
	   one menu/indicator on a well known name */
	int indicator_cnt;
	for (indicator_cnt = 0; indicator_cnt < G_N_ELEMENTS(system_indicators); indicator_cnt++) {
		SystemIndicator * sys_indicator = &system_indicators[indicator_cnt];

		if (g_strcmp0(sys_indicator->dbus_name, name) != 0) {
			continue;
		}

		/* Check to see if we already have this system indicator in the
		   list of indicators */
		int i;
		for (i = 0; i < self->priv->indicators->len; i++) {
			IndicatorTrackerIndicator * indicator = &g_array_index(self->priv->indicators, IndicatorTrackerIndicator, i);

			if (g_strcmp0(sys_indicator->dbus_name, indicator->dbus_name_wellknown) != 0) {
				continue;
			}

			if (g_strcmp0(sys_indicator->dbus_menu_path, indicator->dbus_object) != 0) {
				continue;
			}

			/* If both of them match, we need to break */
			break;
		}

		/* We found it in the list so we broke out eary */
		if (i != self->priv->indicators->len) {
			continue;
		}

		g_debug("Adding an indicator for '%s' at owner '%s'", name, name_owner);
		/* Okay, we need to build one for this system indicator */
		IndicatorTrackerIndicator final_indicator = {
			name: g_strdup(sys_indicator->indicator_name),
			dbus_name: g_strdup(name_owner),
			dbus_name_wellknown: g_strdup(sys_indicator->dbus_name),
			dbus_object: g_strdup(sys_indicator->dbus_menu_path),
			prefix: g_strdup(_(sys_indicator->user_visible_name))
		};

		g_array_append_val(self->priv->indicators, final_indicator);
	}

	return;
}

/* When the names drop off of DBus we need to check to see if that
   means any indicators getting lost as well.  If so, remove them from
   the indicator list. */
static void
system_watch_vanished (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);

	/* See if any of our indicators know this name */
	int i;
	for (i = 0; i < self->priv->indicators->len; i++) {
		IndicatorTrackerIndicator * indicator = &g_array_index(self->priv->indicators, IndicatorTrackerIndicator, i);

		/* Doesn't match */
		if (g_strcmp0(indicator->dbus_name_wellknown, name) != 0) {
			continue;
		}

		g_free(indicator->name);
		g_free(indicator->dbus_name);
		g_free(indicator->dbus_name_wellknown);
		g_free(indicator->dbus_object);
		g_free(indicator->prefix);

		g_array_remove_index_fast(self->priv->indicators, i);
		/* Oh, this is confusing.  Basically becasue we shorten the array
		   we need to check the one that replaced the entry we deleted
		   so we have to look in this same slot again. */
		i--;
	}

	return;
}

/* Gets called when we have an app proxy.  Now we can start talking
   to it, and learning from it */
static void
app_proxy_built (GObject * object, GAsyncResult * result, gpointer user_data)
{



	return;
}
