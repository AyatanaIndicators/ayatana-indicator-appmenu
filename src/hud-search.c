/*
The HUD searching logic that brings together the DBus menus along
with the user data to make a good search.

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

#include <libbamf/bamf-matcher.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>

#include "hud-search.h"
#include "dbusmenu-collector.h"
#include "indicator-tracker.h"
#include "usage-tracker.h"
#include "utils.h"

struct _HudSearchPrivate {
	BamfMatcher * matcher;
	gulong window_changed_sig;

	guint32 active_xid;
	BamfApplication * active_app;

	DbusmenuCollector * collector;
	UsageTracker * usage;

	GDBusProxy * appmenu;

	GSettings * search_settings;

	IndicatorTracker * tracker;
};

#define HUD_SEARCH_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_SEARCH_TYPE, HudSearchPrivate))

static void hud_search_dispose    (GObject *object);
static void hud_search_finalize   (GObject *object);

static void active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data);
HudSearchSuggest * hud_search_suggest_new (const gchar * app, const gchar * app_icon, const gchar * display, const gchar * db, const gchar * icon, const gchar * dbus_address, const gchar * dbus_path, gint dbus_id);

G_DEFINE_TYPE (HudSearch, hud_search, G_TYPE_OBJECT);

static void
hud_search_class_init (HudSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudSearchPrivate));

	object_class->dispose = hud_search_dispose;
	object_class->finalize = hud_search_finalize;

	return;
}

static void
hud_search_init (HudSearch *self)
{
	/* Get Private */
	self->priv = HUD_SEARCH_GET_PRIVATE(self);

	/* Initialize Private */
	self->priv->matcher = NULL;
	self->priv->window_changed_sig = 0;
	self->priv->active_xid = 0;
	self->priv->active_app = NULL;
	self->priv->collector = NULL;
	self->priv->usage = NULL;
	self->priv->appmenu = NULL;
	self->priv->search_settings = NULL;
	self->priv->tracker = NULL;

	/* Settings */
	if (settings_schema_exists("com.canonical.indicator.appmenu.hud.search")) {
		self->priv->search_settings = g_settings_new("com.canonical.indicator.appmenu.hud.search");
	}

	/* Indicator Tracker */
	self->priv->tracker = indicator_tracker_new();

	/* BAMF */
	self->priv->matcher = bamf_matcher_get_default();
	self->priv->window_changed_sig = g_signal_connect(G_OBJECT(self->priv->matcher), "active-window-changed", G_CALLBACK(active_window_changed), self);

	BamfWindow * active_window = bamf_matcher_get_active_window(self->priv->matcher);
	if (active_window != NULL) {
		self->priv->active_xid = bamf_window_get_xid(active_window);
		self->priv->active_app = bamf_matcher_get_application_for_window(self->priv->matcher, active_window);
	}

	/* DBusMenu */
	self->priv->collector = dbusmenu_collector_new();

	/* Usage Tracker */
	self->priv->usage = usage_tracker_new();

	/* Appmenu */
	self->priv->appmenu = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
	                                                    G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                                                    /* info */ NULL,
	                                                    "com.canonical.AppMenu.Registrar",
	                                                    "/com/canonical/AppMenu/Registrar",
	                                                    "com.canonical.AppMenu.Registrar",
	                                                    NULL, NULL);

	return;
}

static void
hud_search_dispose (GObject *object)
{
	HudSearch * self = HUD_SEARCH(object);

	if (self->priv->search_settings != NULL) {
		g_object_unref(self->priv->search_settings);
		self->priv->search_settings = NULL;
	}

	if (self->priv->tracker != NULL) {
		g_object_unref(self->priv->tracker);
		self->priv->tracker = NULL;
	}

	if (self->priv->window_changed_sig != 0) {
		g_signal_handler_disconnect(self->priv->matcher, self->priv->window_changed_sig);
		self->priv->window_changed_sig = 0;
	}

	if (self->priv->matcher != NULL) {
		g_object_unref(self->priv->matcher);
		self->priv->matcher = NULL;
	}

	if (self->priv->collector != NULL) {
		g_object_unref(self->priv->collector);
		self->priv->collector = NULL;
	}

	if (self->priv->usage != NULL) {
		g_object_unref(self->priv->usage);
		self->priv->usage = NULL;
	}

	if (self->priv->appmenu != NULL) {
		g_object_unref(self->priv->appmenu);
		self->priv->appmenu = NULL;
	}

	G_OBJECT_CLASS (hud_search_parent_class)->dispose (object);
	return;
}

static void
hud_search_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_search_parent_class)->finalize (object);
	return;
}

HudSearch *
hud_search_new (void)
{
	HudSearch * ret = HUD_SEARCH(g_object_new(HUD_SEARCH_TYPE, NULL));
	return ret;
}

static void
get_current_window_address(HudSearch * search, gchar ** address, gchar ** path)
{
	if (search->priv->active_xid == 0) {
		g_warning("Active application is unknown");
		return;
	}

	if (search->priv->appmenu == NULL) {
		g_warning("Unable to proxy appmenu");
		return;
	}

	GError * error = NULL;
	GVariant * dbusinfo = g_dbus_proxy_call_sync(search->priv->appmenu,
	                                             "GetMenuForWindow",
	                                             g_variant_new("(u)", search->priv->active_xid),
	                                             G_DBUS_CALL_FLAGS_NONE,
	                                             -1,
	                                             NULL,
	                                             &error);

	if (error != NULL) {
		g_warning("Unable to get menus from appmenu: %s", error->message);
		g_error_free(error);
		return;
	}

	g_variant_get(dbusinfo, "(so)", address, path);
	return;
}

typedef struct _usage_wrapper_t usage_wrapper_t;
struct _usage_wrapper_t {
	DbusmenuCollectorFound * found;
	guint count;
	gfloat percent_usage;
	gfloat percent_distance;
};

/* Sort the usage data based on the percentages */
static gint
usage_sort (gconstpointer a, gconstpointer b)
{
	usage_wrapper_t * wa = (usage_wrapper_t *)a;
	usage_wrapper_t * wb = (usage_wrapper_t *)b;

	gfloat totala = (1.0 - wa->percent_usage) + wa->percent_distance;
	gfloat totalb = (1.0 - wb->percent_usage) + wb->percent_distance;

	gfloat difference = (totala - totalb) * 100.0;
	return (gint)difference;
}

/* Sort the array based on the distance in the found object */
static gint
distance_sort (gconstpointer a, gconstpointer b)
{
	usage_wrapper_t * wa = (usage_wrapper_t *)a;
	usage_wrapper_t * wb = (usage_wrapper_t *)b;

	return dbusmenu_collector_found_get_distance(wa->found) - dbusmenu_collector_found_get_distance(wb->found);
}

/* Take the found list and put it into the usage array so that
   we can start to work with it. */
static void
found_list_to_usage_array (HudSearch * search, GList * found_list, GArray * usagedata)
{
	GList * found = NULL;
	found = found_list;

	while (found != NULL) {
		usage_wrapper_t usage;
		usage.found = (DbusmenuCollectorFound *)found->data;
		usage.count = 0;

		if (dbusmenu_collector_found_get_distance(usage.found) > get_settings_uint(search->priv->search_settings, "max-distance", 30)) {
			break;
		}

		g_array_append_val(usagedata, usage);
		found = g_list_next(found);
	}

	return;
}

/* Take a path to a desktop file and find its icon */
static gchar *
desktop2icon (const gchar * desktop)
{
	g_return_val_if_fail(desktop != NULL, g_strdup(""));

	if (!g_file_test(desktop, G_FILE_TEST_EXISTS)) {
		return g_strdup("");
	}

	/* Try to build an app info from the desktop file
	   path */
	GDesktopAppInfo * appinfo = g_desktop_app_info_new_from_filename(desktop);

	if (!G_IS_DESKTOP_APP_INFO(appinfo)) {
		return g_strdup("");
	}

	/* Get the name out of the icon, note the icon is not
	   ref'd so it doesn't need to be free'd */
	gchar * retval = NULL;
	GIcon * icon = g_app_info_get_icon(G_APP_INFO(appinfo));
	if (icon != NULL) {
		retval = g_icon_to_string(icon);
	} else {
		retval = g_strdup("");
	}

	/* Drop the app info */
	g_object_unref(appinfo);
	appinfo = NULL;

	return retval;
}

/* Looks through the menus of the currently focused application
   for the search string */
static void
search_current_app (HudSearch * search, const gchar * searchstr, GArray * usagedata, GList ** foundlist)
{
	if (search->priv->active_app == NULL) {
		return;
	}

	gchar * address = NULL;
	gchar * path = NULL;
	GList * found_list = NULL;

	get_current_window_address(search, &address, &path);
	found_list = dbusmenu_collector_search(search->priv->collector, address, path, NULL, searchstr);

	g_free(address);
	g_free(path);

	/* Set the name for the application */
	const gchar * desktop_file = bamf_application_get_desktop_file(search->priv->active_app);

	if (desktop_file != NULL) {
		GList * founditem = found_list;
		gchar * icon = desktop2icon(desktop_file);

		while (founditem != NULL) {
			DbusmenuCollectorFound * found = (DbusmenuCollectorFound *)founditem->data;

			/* Indicator */
			dbusmenu_collector_found_set_indicator(found, desktop_file);

			/* Icon */
			dbusmenu_collector_found_set_app_icon(found, icon);

			founditem = g_list_next(founditem);
		}

		g_free(icon);
	}

	/* Copy the found list into the array */
	found_list_to_usage_array(search, found_list, usagedata);
	*foundlist = g_list_concat(*foundlist, found_list);

	return;
}

/* Gets each indicator from the tracker and then looks at it's menus
   with the search string.  It also penalizes the entries by a configurable
   value compared to what they'd be normally. */
static void
search_indicators (HudSearch * search, const gchar * searchstr, GArray * usagedata, GList ** foundlist)
{
	if (search->priv->tracker == NULL) {
		return;
	}

	GList * indicators = indicator_tracker_get_indicators(search->priv->tracker);
	GList * lindicator = NULL;

	for (lindicator = indicators; lindicator != NULL; lindicator = g_list_next(lindicator)) {
		IndicatorTrackerIndicator * indicator = (IndicatorTrackerIndicator *)lindicator->data;

		/* Search the menu items of the indicator */
		GList * found_list = dbusmenu_collector_search(search->priv->collector, indicator->dbus_name, indicator->dbus_object, indicator->prefix, searchstr);

		/* Increase distance */
		GList * founditem = found_list;
		while (founditem != NULL) {
			DbusmenuCollectorFound * found = (DbusmenuCollectorFound *)founditem->data;

			/* Distance */
			guint distance = dbusmenu_collector_found_get_distance(found);
			distance = distance + ((distance * get_settings_uint(search->priv->search_settings, "indicator-penalty", 50)) / 100);
			dbusmenu_collector_found_set_distance(found, distance);

			/* Names */
			dbusmenu_collector_found_set_indicator(found, indicator->name);

			/* Icon */
			dbusmenu_collector_found_set_app_icon(found, indicator->icon);

			founditem = g_list_next(founditem);
		}

		found_list_to_usage_array(search, found_list, usagedata);
		
		*foundlist = g_list_concat(*foundlist, found_list);
	}

	g_list_free(indicators);

	return;
}

/* Grabs all the menus items, sorts them, then looks up usage data on
   the top entries and sorts them again.  Lastly, we have our top
   entries. */
static void
search_and_sort (HudSearch * search, const gchar * searchstr, GArray * usagedata, GList ** foundlist)
{
	/* Get all the entries from the current app and the indicators
	   and put them into the array.  We keep the found list as it
	   tracks the memory as well. */
	search_current_app(search, searchstr, usagedata, foundlist);
	search_indicators(search, searchstr, usagedata, foundlist);

	/* Sort the list */
	g_array_sort(usagedata, distance_sort);

	/* Drop all but the first few */
	if (usagedata->len > 15) {
		usagedata = g_array_remove_range(usagedata, 15, usagedata->len - 15);
	}

	/* Get usage data */
	int count;
	for (count = 0; count < usagedata->len; count++) {
		usage_wrapper_t * usage = &g_array_index(usagedata, usage_wrapper_t, count);

		const gchar * desktopfile = NULL;

		desktopfile = dbusmenu_collector_found_get_indicator(usage->found);

		if (desktopfile != NULL) {
			usage->count = usage_tracker_get_usage(search->priv->usage, desktopfile, dbusmenu_collector_found_get_db(usage->found));
		} else {
			usage->count = 0;
		}
	}

	/* Calculate overall */
	guint overall_usage = 0;
	guint overall_distance = 0;
	for (count = 0; count < usagedata->len; count++) {
		usage_wrapper_t * usage = &g_array_index(usagedata, usage_wrapper_t, count);
		overall_usage += usage->count;
		overall_distance += dbusmenu_collector_found_get_distance(usage->found);
	}


	/* Build percentages */
	for (count = 0; count < usagedata->len; count++) {
		usage_wrapper_t * usage = &g_array_index(usagedata, usage_wrapper_t, count);

		if (overall_usage != 0) {
			usage->percent_usage = (gfloat)usage->count/(gfloat)overall_usage;
		} else {
			usage->percent_usage = 1.0;
		}

		usage->percent_distance = (gfloat)dbusmenu_collector_found_get_distance(usage->found)/(gfloat)overall_distance;
	}

	/* Sort based on aggregate */
	g_array_sort(usagedata, usage_sort);

	return;
}

/* Find a list of suggestions that should be sent to the user.  Looks
   through both applications and indicators. */
GList *
hud_search_suggestions (HudSearch * search, const gchar * searchstr, gchar ** desktop, gchar ** target)
{
	g_return_val_if_fail(IS_HUD_SEARCH(search), NULL);

	/* We're currently not getting this data, let's just give a
	   NULL string back for now */
	if (target != NULL) {
		*target = g_strdup("");
	}

	/* Build an array of the entries that we're going
	   to use as an interim structure */
	GArray * usagedata = g_array_sized_new(FALSE, TRUE, sizeof(usage_wrapper_t), 15);
	GList * found_list = NULL;


	/* The application desktop file is the default if the
	   entry in question is not for an indicator */
	const gchar * appdesktopfile = NULL;

	if (search->priv->active_app != NULL) {
		appdesktopfile = bamf_application_get_desktop_file(search->priv->active_app);
	}

	if (appdesktopfile == NULL) {
		appdesktopfile = "";
	}

	/* If we're given a valid pointer, return the value back to
	   it */
	if (desktop != NULL) {
		*desktop = g_strdup(appdesktopfile);
	}

	/* This is the core of searching a given application for their
	   entries.  It will also delve into the indicators */
	search_and_sort(search, searchstr, usagedata, &found_list);

	/* Convert the entries that we were given into a list of structures
	   that we want to export an an API */
	GList * retval = NULL;
	int count;
	for (count = 0; count < 5 && count < usagedata->len; count++) {
		usage_wrapper_t * usage = &g_array_index(usagedata, usage_wrapper_t, count);

		const gchar * desktopfile = dbusmenu_collector_found_get_indicator(usage->found);
		if (desktopfile == NULL) {
			desktopfile = appdesktopfile;
		}

		HudSearchSuggest * suggest = hud_search_suggest_new(desktopfile,
		                                                    dbusmenu_collector_found_get_app_icon(usage->found),
		                                                    dbusmenu_collector_found_get_display(usage->found),
		                                                    dbusmenu_collector_found_get_db(usage->found),
		                                                    "none",
		                                                    dbusmenu_collector_found_get_dbus_addr(usage->found),
		                                                    dbusmenu_collector_found_get_dbus_path(usage->found),
		                                                    dbusmenu_collector_found_get_dbus_id(usage->found)
		                                                    );

		retval = g_list_append(retval, suggest);
	}

	/* Free the interim arrays of data */
	g_array_free(usagedata, TRUE);
	dbusmenu_collector_found_list_free(found_list);

	return retval;
}

void
hud_search_execute (HudSearch * search, GVariant * key, guint timestamp)
{
	g_return_if_fail(IS_HUD_SEARCH(search));
	gchar * app = NULL;
	gchar * display = NULL;
	gchar * address = NULL;
	gchar * path = NULL;
	gint id = 0;
	GVariant * unwrapped_key = g_variant_get_variant(key);

	g_variant_get(unwrapped_key, "(sssoi)", &app, &display, &address, &path, &id);

	dbusmenu_collector_execute(search->priv->collector, address, path, id, timestamp);
	usage_tracker_mark_usage(search->priv->usage, app, display);

	g_free(address);
	g_free(path);
	g_free(app);
	g_free(display);

	g_variant_unref(unwrapped_key);

	return;
}

static void
active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data)
{
	HudSearch * self = HUD_SEARCH(user_data);

	if (!BAMF_IS_WINDOW(newview)) { return; }
	BamfWindow * window = BAMF_WINDOW(newview);

	BamfApplication * app = bamf_matcher_get_application_for_window(self->priv->matcher, window);
	const gchar * desktop = NULL;

	if (app != NULL) {
		desktop = bamf_application_get_desktop_file(app);
	}

	/* If BAMF can't match it to an application we probably don't
	   want to be involved with it anyway. */
	if (desktop == NULL) {
		return;
	}

	/* NOTE: Both of these are debugging tools for now, so we want
	   to be able to use them effectively to work on the HUD, so we're
	   going to pretend we didn't switch windows if we switch to one 
	   of them */
	if (g_strstr_len(desktop, -1, "termina") != NULL ||
	        g_strstr_len(desktop, -1, "dfeet") != NULL) {
		return;
	}

	/* Ignore the hud prototype window directly */
	const gchar * window_name = bamf_view_get_name(newview);
	if (g_strcmp0(window_name, "Hud Prototype Test") == 0
	  || g_strcmp0(window_name, "Hud") == 0) {
		return;
	}

	/* This should ignore most of the windows involved in Unity */
	const gchar * name = bamf_view_get_name(newview);
	if (g_strcmp0(name, "DNDCollectionWindow") == 0
	    || g_strcmp0(name, "launcher") == 0
	    || g_strcmp0(name, "dash") == 0
	    || g_strcmp0(name, "panel") == 0
	    || g_strcmp0(name, "hud") == 0) {
		return;
	}

	self->priv->active_xid = bamf_window_get_xid(window);
	self->priv->active_app = app;

	return;
}

struct _HudSearchSuggest {
	gchar * display;
	gchar * app_icon;
	gchar * item_icon;
	GVariant * key;
};

HudSearchSuggest *
hud_search_suggest_new (const gchar * app, const gchar * app_icon, const gchar * display, const gchar * db, const gchar * item_icon, const gchar * dbus_address, const gchar * dbus_path, gint dbus_id)
{
	HudSearchSuggest * suggest = g_new0(HudSearchSuggest, 1);

	suggest->display = g_strdup(display);
	suggest->app_icon = g_strdup(app_icon);
	suggest->item_icon = g_strdup(item_icon);

	GVariantBuilder builder;
	g_variant_builder_init(&builder, G_VARIANT_TYPE_TUPLE);
	g_variant_builder_add_value(&builder, g_variant_new_string(app));
	g_variant_builder_add_value(&builder, g_variant_new_string(db));
	g_variant_builder_add_value(&builder, g_variant_new_string(dbus_address));
	g_variant_builder_add_value(&builder, g_variant_new_object_path(dbus_path));
	g_variant_builder_add_value(&builder, g_variant_new_int32(dbus_id));

	suggest->key = g_variant_new_variant(g_variant_builder_end(&builder));
	g_variant_ref_sink(suggest->key);

	return suggest;
}

const gchar *
hud_search_suggest_get_app_icon (HudSearchSuggest * suggest)
{
	return suggest->app_icon;
}

const gchar *
hud_search_suggest_get_item_icon (HudSearchSuggest * suggest)
{
	return suggest->item_icon;
}

const gchar *
hud_search_suggest_get_display (HudSearchSuggest * suggest)
{
	return suggest->display;
}

GVariant *
hud_search_suggest_get_key (HudSearchSuggest * suggest)
{
	return suggest->key;
}

void
hud_search_suggest_free (HudSearchSuggest * suggest)
{
	g_free(suggest->display);
	g_free(suggest->app_icon);
	g_free(suggest->item_icon);
	g_variant_unref(suggest->key);
	g_free(suggest);
	return;
}
