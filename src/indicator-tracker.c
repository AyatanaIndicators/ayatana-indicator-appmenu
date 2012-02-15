/*
Tracks the various indicators to know when they come on and off
the bus for searching their menus.

Copyright 2011-2012 Canonical Ltd.

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
	gchar * icon;
};

/* TODO: Delete extra sound after everyone has the new version */

SystemIndicator system_indicators[] = {
	{dbus_name: "com.canonical.indicator.datetime", dbus_menu_path: "/com/canonical/indicator/datetime/menu", indicator_name: "indicator-datetime",       user_visible_name: N_("Date"),      icon: "office-calendar"},
	{dbus_name: "com.canonical.indicator.session",  dbus_menu_path: "/com/canonical/indicator/session/menu",  indicator_name: "indicator-session-device", user_visible_name: N_("Device"),    icon: "system-devices-panel"},
	{dbus_name: "com.canonical.indicator.session",  dbus_menu_path: "/com/canonical/indicator/users/menu",    indicator_name: "indicator-session-user",   user_visible_name: N_("Users"),     icon: "avatar-default"},
	{dbus_name: "com.canonical.indicators.sound",   dbus_menu_path: "/com/canonical/indicators/sound/menu",   indicator_name: "indicator-sound",          user_visible_name: N_("Sound"),     icon: "audio-volume-high-panel"},
	{dbus_name: "com.canonical.indicator.sound",    dbus_menu_path: "/com/canonical/indicator/sound/menu",    indicator_name: "indicator-sound",          user_visible_name: N_("Sound"),     icon: "audio-volume-high-panel"},
	{dbus_name: "com.canonical.indicator.messages", dbus_menu_path: "/com/canonical/indicator/messages/menu", indicator_name: "indicator-messages",       user_visible_name: N_("Messages"),  icon: "indicator-messages"}
};

typedef struct _AppIndicator AppIndicator;
struct _AppIndicator {
	IndicatorTrackerIndicator system;

	gboolean alert;

	gchar * alert_name;
	gchar * normal_name;
};


struct _IndicatorTrackerPrivate {
	GArray * indicators;
	guint watches[G_N_ELEMENTS(system_indicators)];

	GDBusProxy * app_proxy;
	GCancellable * app_proxy_cancel;
	GArray * app_indicators;
};

#define INDICATOR_TRACKER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_TRACKER_TYPE, IndicatorTrackerPrivate))

static void indicator_tracker_dispose    (GObject *object);
static void indicator_tracker_finalize   (GObject *object);
static void system_watch_appeared        (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data);
static void system_watch_vanished        (GDBusConnection * connection, const gchar * name, gpointer user_data);
static void system_indicator_cleanup     (gpointer pindicator);
static void app_indicator_cleanup        (gpointer pindicator);
static void app_proxy_built              (GObject * object, GAsyncResult * result, gpointer user_data);
static void app_proxy_name_change        (GObject * gobject, GParamSpec * pspec, gpointer user_data);
static void app_proxy_signal             (GDBusProxy *proxy, gchar * sender_name, gchar * signal_name, GVariant * parameters, gpointer user_data);
static void app_proxy_apps_replace       (GObject * obj, GAsyncResult * res, gpointer user_data);
static void app_proxy_new_indicator      (IndicatorTracker * self, gint position, const gchar * id, const gchar * accessibledesc, const gchar * dbusaddress, const gchar * dbusobject, const gchar * iconname);
static gboolean app_proxy_remove_indicator (IndicatorTracker * self, gint position);
static void app_proxy_icon_changed       (IndicatorTracker * self, gint position, const gchar * iconname);
static void app_proxy_title_changed      (IndicatorTracker * self, gint position, const gchar * title);

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
	self->priv->app_indicators = NULL;

	/* NOTE: It would seem like we could combine these, eh?
	   Well, not really.  And the reason is because the app
	   indicator service sends everything based on array index
	   so it's a lot easier to have an array to track that */
	self->priv->indicators = g_array_new(FALSE, TRUE, sizeof(IndicatorTrackerIndicator));
	self->priv->app_indicators = g_array_new(FALSE, FALSE, sizeof(AppIndicator));

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

	g_clear_object(&self->priv->app_proxy);

	G_OBJECT_CLASS (indicator_tracker_parent_class)->dispose (object);
	return;
}

static void
indicator_tracker_finalize (GObject *object)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(object));
	IndicatorTracker * self = INDICATOR_TRACKER(object);


	if (self->priv->indicators != NULL) {
		/* Clear all the entries in the system indicator array */
		while (self->priv->indicators->len != 0) {
			IndicatorTrackerIndicator * indicator = &g_array_index(self->priv->indicators, IndicatorTrackerIndicator, 0);

			system_indicator_cleanup(indicator);

			g_array_remove_index_fast(self->priv->indicators, 0);
		}

		g_array_free(self->priv->indicators, TRUE /* delete segment */);
		self->priv->indicators = NULL;
	}

	if (self->priv->app_indicators != NULL) {
		/* Clear all the entries in the system indicator array */
		while (self->priv->app_indicators->len != 0) {
			AppIndicator * indicator = &g_array_index(self->priv->app_indicators, AppIndicator, 0);

			app_indicator_cleanup(indicator);

			g_array_remove_index_fast(self->priv->app_indicators, 0);
		}

		g_array_free(self->priv->app_indicators, TRUE /* delete segment */);
		self->priv->app_indicators = NULL;
	}

	G_OBJECT_CLASS (indicator_tracker_parent_class)->finalize (object);
	return;
}

IndicatorTracker *
indicator_tracker_new (void)
{
	return INDICATOR_TRACKER(g_object_new(INDICATOR_TRACKER_TYPE, NULL));
}

GList *
indicator_tracker_get_indicators (IndicatorTracker * tracker)
{
	g_return_val_if_fail(IS_INDICATOR_TRACKER(tracker), NULL);

	GList * retval = NULL;
	gint i;

	for (i = 0; i < tracker->priv->indicators->len; i++) {
		IndicatorTrackerIndicator * item = &g_array_index(tracker->priv->indicators, IndicatorTrackerIndicator, i);
		retval = g_list_prepend(retval, item);
	}

	for (i = 0; i < tracker->priv->app_indicators->len; i++) {
		AppIndicator * item = &g_array_index(tracker->priv->app_indicators, AppIndicator, i);
		retval = g_list_prepend(retval, &(item->system));
	}

	return retval;
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
			prefix: g_strdup(_(sys_indicator->user_visible_name)),
			icon: g_strdup(sys_indicator->icon)
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

		system_indicator_cleanup(indicator);

		g_array_remove_index_fast(self->priv->indicators, i);
		/* Oh, this is confusing.  Basically becasue we shorten the array
		   we need to check the one that replaced the entry we deleted
		   so we have to look in this same slot again. */
		i--;
	}

	return;
}

/* Removes all the memory that is allocated inside the system indicator
   structure.  The actual indicator is not free'd */
static void
system_indicator_cleanup (gpointer pindicator)
{
	g_return_if_fail(pindicator != NULL);

	IndicatorTrackerIndicator * indicator = (IndicatorTrackerIndicator *)pindicator;

	g_free(indicator->name);
	g_free(indicator->dbus_name);
	g_free(indicator->dbus_name_wellknown);
	g_free(indicator->dbus_object);
	g_free(indicator->prefix);

	return;
}

/* Deletes the allocated memory in the app indicator structure so
   that we don't leak any! */
static void
app_indicator_cleanup (gpointer pindicator)
{
	g_return_if_fail(pindicator != NULL);

	AppIndicator * indicator = (AppIndicator *)pindicator;

	system_indicator_cleanup(&(indicator->system));

	return;
}

/* Gets called when we have an app proxy.  Now we can start talking
   to it, and learning from it */
static void
app_proxy_built (GObject * object, GAsyncResult * result, gpointer user_data)
{
	GError * error = NULL;
	GDBusProxy * proxy = g_dbus_proxy_new_for_bus_finish(result, &error);

	if (error != NULL) {
		g_warning("Unable to build App Indicator Proxy: %s", error->message);
		g_error_free(error);
		return;
	}

	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);

	self->priv->app_proxy = proxy;

	/* Watch to see if we get dropped off the bus or not */
	g_signal_connect(G_OBJECT(self->priv->app_proxy), "notify::g-name-owner", G_CALLBACK(app_proxy_name_change), self);
	g_signal_connect(G_OBJECT(self->priv->app_proxy), "g-signal",             G_CALLBACK(app_proxy_signal),      self);

	/* Act like the name changed, as this function checks to see
	   if it is there or not anyway */
	app_proxy_name_change(G_OBJECT(self->priv->app_proxy), NULL, self);

	return;
}

/* React to the name changing, this usually means the service is either
   leaving or joining the bus. */
static void
app_proxy_name_change (GObject * gobject, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);

	/* Check to see if this wire is hot! */
	gchar * owner = g_dbus_proxy_get_name_owner(self->priv->app_proxy);
	if (owner == NULL) {
		/* Delete entries if we have them */

		return;
	}
	g_free(owner);

	/* Query to see if there's any indicator already out there. */
	g_dbus_proxy_call (self->priv->app_proxy,
	                   "GetApplications",
	                   NULL, /* paramaters */
	                   G_DBUS_CALL_FLAGS_NO_AUTO_START,
	                   -1, /* timeout */
	                   NULL, /* cancellable */
	                   app_proxy_apps_replace,
	                   self);

	return;
}

/* Replace our current list of applications with the new ones that we've
   gotten from a 'GetApplications' to the application service */
static void
app_proxy_apps_replace (GObject * obj, GAsyncResult * res, gpointer user_data)
{
	GError * error = NULL;

	GVariant * params = g_dbus_proxy_call_finish(G_DBUS_PROXY(obj), res, &error);

	if (error != NULL) {
		g_warning("Unable to get applications for indicator service: %s", error->message);
		g_error_free(error);
		return;
	}

	/* Remove the first application indicator until it starts
	   returning an error.  This will clear the array. */
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);
	while (app_proxy_remove_indicator(self, 0));

	GVariant * array = g_variant_get_child_value(params, 0);

	GVariantIter iter;
	g_variant_iter_init(&iter, array);

	gchar * iconname = NULL;
	guint position = G_MAXUINT;
	gchar * dbusaddress = NULL;
	gchar * dbusobject = NULL;
	gchar * iconpath = NULL;
	gchar * label = NULL;
	gchar * labelguide = NULL;
	gchar * accessibledesc = NULL;
	gchar * hint = NULL;
	gchar * title = NULL;

	while (g_variant_iter_loop(&iter, 
	                           "(sisossssss)",
	                           &iconname,
	                           &position,
	                           &dbusaddress,
	                           &dbusobject,
	                           &iconpath,
	                           &label,
	                           &labelguide,
	                           &accessibledesc,
	                           &hint,
		                       &title)) {
		app_proxy_new_indicator(self, position, hint, title, dbusaddress, dbusobject, iconname);
	}

	g_variant_unref(array);
	g_variant_unref(params);

	return;
}

/* Signals coming over dbus from the app indicator service.  Gives
   us updates to the indicator cache that we have. */
static void
app_proxy_signal (GDBusProxy *proxy, gchar * sender_name, gchar * signal_name, GVariant * parameters, gpointer user_data) 
{
	IndicatorTracker * self = INDICATOR_TRACKER(user_data);

	if (g_strcmp0(signal_name, "ApplicationAdded") == 0) {
		gchar * iconname = NULL;
		guint position = G_MAXUINT;
		gchar * dbusaddress = NULL;
		gchar * dbusobject = NULL;
		gchar * iconpath = NULL;
		gchar * label = NULL;
		gchar * labelguide = NULL;
		gchar * accessibledesc = NULL;
		gchar * hint = NULL;
		gchar * title = NULL;

		g_variant_get(parameters, "(sisossssss)",
		              &iconname,
		              &position,
		              &dbusaddress,
		              &dbusobject,
		              &iconpath,
		              &label,
		              &labelguide,
		              &accessibledesc,
		              &hint,
		              &title);

		app_proxy_new_indicator(self, position, hint, title, dbusaddress, dbusobject, iconname);

		g_free(iconname);
		g_free(dbusaddress);
		g_free(dbusobject);
		g_free(iconpath);
		g_free(label);
		g_free(labelguide);
		g_free(accessibledesc);
		g_free(hint);
		g_free(title);
	} else if (g_strcmp0(signal_name, "ApplicationRemoved") == 0) {
		gint position;

		g_variant_get_child(parameters, 0, "i", &position);

		gboolean remove = app_proxy_remove_indicator(self, position);

		/* If we can't remove it, something is really goofy, we're probably
		   out of sync.  So let's go nuclear. */
		if (!remove) {
			g_warning("Unable to remove indicator '%d' getting full list", position);
			app_proxy_name_change(G_OBJECT(self->priv->app_proxy), NULL, self);
		}
	} else if (g_strcmp0(signal_name, "ApplicationIconChanged") == 0) {
		gchar * iconname = NULL;
		guint position = 0;
		gchar * accessibledesc = NULL;

		g_variant_get(parameters, "(iss)",
		              &position,
		              &iconname,
		              &accessibledesc);

		app_proxy_icon_changed(self, position, iconname);

		g_free(iconname);
		g_free(accessibledesc);
	} else if (g_strcmp0(signal_name, "ApplicationTitleChanged") == 0) {
		guint position = 0;
		gchar * title = NULL;

		g_variant_get(parameters, "(is)",
		              &position,
		              &title);

		app_proxy_title_changed(self, position, title);

		g_free(title);
	} else if (g_strcmp0(signal_name, "ApplicationIconThemePathChanged") == 0) {
		/* Don't care */
	} else if (g_strcmp0(signal_name, "ApplicationLabelChanged") == 0) {
		/* Don't care */
	} else {
		g_debug("Application Service signal '%s' not handled", signal_name);
	}

	return;
}

/* Add a new application indicator to our list */
static void
app_proxy_new_indicator (IndicatorTracker * self, gint position, const gchar * id, const gchar * title, const gchar * dbusaddress, const gchar * dbusobject, const gchar * iconname)
{
	g_return_if_fail(position != G_MAXUINT);
	g_return_if_fail(id != NULL);
	g_return_if_fail(dbusaddress != NULL);
	g_return_if_fail(dbusobject != NULL);

	g_debug("New application indicator: %s", dbusobject);

	AppIndicator indicator = {
		system: {
			name: g_strdup_printf("appindicator:%s", id),
			dbus_name: g_strdup(dbusaddress),
			dbus_name_wellknown: NULL,
			dbus_object: g_strdup(dbusobject),
			prefix: g_strdup(title),
			icon: g_strdup(iconname)
		},
		alert: FALSE,
		alert_name: NULL,
		normal_name: NULL
	};

	if (indicator.system.prefix == NULL || indicator.system.prefix[0] == '\0') {
		g_free(indicator.system.prefix);
		/* TRANSLATORS:  This is used for Application indicators that
		   are not providing a title string.  The '%s' represents the
		   unique ID that the app indicator provides, but it is usually
		   the package name and not generally human readable.  An example
		   for Network Manager would be 'nm-applet'. */
		indicator.system.prefix = g_strdup_printf(_("Untitled Indicator (%s)"), id);
	}

	g_array_insert_val(self->priv->app_indicators, position, indicator);

	return;
}

/* Remove an application indicator */
static gboolean
app_proxy_remove_indicator(IndicatorTracker * self, gint position)
{
	if (position >= self->priv->app_indicators->len) {
		g_warning("Application removed for position outside of array");
		return FALSE;
	}

	AppIndicator * indicator = &g_array_index(self->priv->app_indicators, AppIndicator, position);

	app_indicator_cleanup(indicator);

	g_array_remove_index(self->priv->app_indicators, position);

	return TRUE;
}

/* Change the name of the icon */
static void
app_proxy_icon_changed (IndicatorTracker * self, gint position, const gchar * iconname)
{
	if (position >= self->priv->app_indicators->len) {
		g_warning("Application icon changed for position outside of array");
		return;
	}

	AppIndicator * indicator = &g_array_index(self->priv->app_indicators, AppIndicator, position);

	g_free(indicator->system.icon);
	indicator->system.icon = g_strdup(iconname);

	return;
}

/* Change the title of the entry */
static void
app_proxy_title_changed (IndicatorTracker * self, gint position, const gchar * title)
{
	if (position >= self->priv->app_indicators->len) {
		g_warning("Application title changed for position outside of array");
		return;
	}

	AppIndicator * indicator = &g_array_index(self->priv->app_indicators, AppIndicator, position);

	g_debug("AppIndicator '%s' changed title to: '%s'", indicator->system.name, title);

	if (title != NULL && title[0] != '\0') {
		g_free(indicator->system.prefix);
		indicator->system.prefix = g_strdup(title);
	} else {
		g_debug("\tIgnoring, it's NULL");
	}

	return;
}
