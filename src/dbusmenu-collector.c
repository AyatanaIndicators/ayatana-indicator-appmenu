/*
An object to collect the various DBusmenu objects that exist
on dbus.

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

#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>

#include <libdbusmenu-glib/client.h>

#include "dbusmenu-collector.h"
#include "distance.h"
#include "indicator-tracker.h"
#include "shared-values.h"
#include "utils.h"

#define GENERIC_ICON   "dbusmenu-lens-panel"

typedef struct _DbusmenuCollectorPrivate DbusmenuCollectorPrivate;

struct _DbusmenuCollectorPrivate {
	GDBusConnection * bus;
	guint signal;
	GHashTable * hash;
	IndicatorTracker * tracker;
	GSettings * search_settings;
};

struct _DbusmenuCollectorFound {
	gchar * dbus_addr;
	gchar * dbus_path;
	gint dbus_id;

	gchar * display_string;
	gchar * db_string;

	guint distance;
	DbusmenuMenuitem * item;
	gchar * indicator;
};

typedef struct _menu_key_t menu_key_t;
struct _menu_key_t {
	gchar * sender;
	gchar * path;
};

typedef struct _search_item_t search_item_t;
struct _search_item_t {
	gchar * string;
	guint distance;
};

static void dbusmenu_collector_class_init (DbusmenuCollectorClass *klass);
static void dbusmenu_collector_init       (DbusmenuCollector *self);
static void dbusmenu_collector_dispose    (GObject *object);
static void dbusmenu_collector_finalize   (GObject *object);
static void update_layout_cb (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data);
static guint menu_hash_func (gconstpointer key);
static gboolean menu_equal_func (gconstpointer a, gconstpointer b);
static void menu_key_destroy (gpointer key);
static DbusmenuCollectorFound * dbusmenu_collector_found_new (DbusmenuClient * client, DbusmenuMenuitem * item, GStrv strings, guint distance, GStrv usedstrings, const gchar * indicator_name);

G_DEFINE_TYPE (DbusmenuCollector, dbusmenu_collector, G_TYPE_OBJECT);

static void
dbusmenu_collector_class_init (DbusmenuCollectorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (DbusmenuCollectorPrivate));

	object_class->dispose = dbusmenu_collector_dispose;
	object_class->finalize = dbusmenu_collector_finalize;

	return;
}

static void
dbusmenu_collector_init (DbusmenuCollector *self)
{
	self->priv = G_TYPE_INSTANCE_GET_PRIVATE ((self), DBUSMENU_COLLECTOR_TYPE, DbusmenuCollectorPrivate);

	self->priv->bus = NULL;
	self->priv->signal = 0;
	self->priv->hash = NULL;
	self->priv->tracker = NULL;
	self->priv->search_settings = NULL;

	if (settings_schema_exists("com.canonical.indicator.appmenu.hud.search")) {
		self->priv->search_settings = g_settings_new("com.canonical.indicator.appmenu.hud.search");
	}

	self->priv->hash = g_hash_table_new_full(menu_hash_func, menu_equal_func,
	                                         menu_key_destroy, g_object_unref /* DbusmenuClient */);

	self->priv->bus = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	self->priv->signal = g_dbus_connection_signal_subscribe(self->priv->bus,
	                                                        NULL, /* sender */
	                                                        "com.canonical.dbusmenu", /* interface */
	                                                        "LayoutUpdated", /* member */
	                                                        NULL, /* object path */
	                                                        NULL, /* arg0 */
	                                                        G_DBUS_SIGNAL_FLAGS_NONE, /* flags */
	                                                        update_layout_cb, /* cb */
	                                                        self, /* data */
	                                                        NULL); /* free func */

	GError * error = NULL;
	g_dbus_connection_emit_signal(self->priv->bus,
	                              NULL, /* destination */
	                              "/", /* object */
	                              "com.canonical.dbusmenu",
	                              "FindServers",
	                              NULL, /* params */
	                              &error);
	if (error != NULL) {
		g_warning("Unable to emit 'FindServers': %s", error->message);
		g_error_free(error);
	}

	self->priv->tracker = indicator_tracker_new();

	return;
}

static void
dbusmenu_collector_dispose (GObject *object)
{
	DbusmenuCollector * collector = DBUSMENU_COLLECTOR(object);

	if (collector->priv->signal != 0) {
		g_dbus_connection_signal_unsubscribe(collector->priv->bus, collector->priv->signal);
		collector->priv->signal = 0;
	}

	if (collector->priv->hash != NULL) {
		g_hash_table_destroy(collector->priv->hash);
		collector->priv->hash = NULL;
	}

	if (collector->priv->tracker != NULL) {
		g_object_unref(collector->priv->tracker);
		collector->priv->tracker = NULL;
	}

	if (collector->priv->search_settings != NULL) {
		g_object_unref(collector->priv->search_settings);
		collector->priv->search_settings = NULL;
	}

	G_OBJECT_CLASS (dbusmenu_collector_parent_class)->dispose (object);
	return;
}

static void
dbusmenu_collector_finalize (GObject *object)
{

	G_OBJECT_CLASS (dbusmenu_collector_parent_class)->finalize (object);
	return;
}

DbusmenuCollector *
dbusmenu_collector_new (void)
{
	return DBUSMENU_COLLECTOR(g_object_new(DBUSMENU_COLLECTOR_TYPE, NULL));
}

static void
update_layout_cb (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data)
{
	g_debug("Client updating %s:%s", sender, path);
	DbusmenuCollector * collector = DBUSMENU_COLLECTOR(user_data);
	g_return_if_fail(collector != NULL);

	menu_key_t search_key = {
		sender: (gchar *)sender,
		path:   (gchar *)path
	};

	gpointer found = g_hash_table_lookup(collector->priv->hash, &search_key);
	if (found == NULL) {
		/* Build one becasue we don't have it */
		menu_key_t * built_key = g_new0(menu_key_t, 1);
		built_key->sender = g_strdup(sender);
		built_key->path = g_strdup(path);

		DbusmenuClient * client = dbusmenu_client_new(sender, path);

		g_hash_table_insert(collector->priv->hash, built_key, client);
	} else {
		g_debug("\tAlready exists");
	}

	/* Assume that dbusmenu client is doing this for us */
	return;
}

static guint
menu_hash_func (gconstpointer key)
{
	const menu_key_t * mk = (const menu_key_t*)key;

	return g_str_hash(mk->sender) + g_str_hash(mk->path) - 5381;
}

static gboolean
menu_equal_func (gconstpointer a, gconstpointer b)
{
	const menu_key_t * ak = (const menu_key_t *)a;
	const menu_key_t * bk = (const menu_key_t *)b;

	if (g_strcmp0(ak->sender, bk->sender) == 0 &&
			g_strcmp0(ak->path, bk->path) == 0) {
		return TRUE;
	}

	return FALSE;
}

static void
menu_key_destroy (gpointer key)
{
	menu_key_t * ikey = (menu_key_t *)key;

	g_free(ikey->sender);
	g_free(ikey->path);

	g_free(ikey);
	return;
}

gchar *
remove_underline (const gchar * input)
{
	const gchar * in = input;
	gchar * output = g_strdup(input);
	gchar * out = output;

	while (in[0] != '\0') {
		if (g_utf8_get_char(in) == '_') {
			in = g_utf8_next_char(in);
		} else {
			out[0] = g_utf8_get_char(in);
			in = g_utf8_next_char(in);
			out = g_utf8_next_char(out);
		}
	}

	return output;
}

static GStrv
menuitem_to_tokens (DbusmenuMenuitem * item, GStrv label_prefix)
{
	const gchar * label_property = NULL;

	if (label_property == NULL && !dbusmenu_menuitem_property_exist(item, DBUSMENU_MENUITEM_PROP_TYPE)) {
		label_property = DBUSMENU_MENUITEM_PROP_LABEL;
	}

	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), DBUSMENU_CLIENT_TYPES_SEPARATOR) == 0) {
		return NULL;
	}

	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), DBUSMENU_CLIENT_TYPES_DEFAULT) == 0) {
		label_property = DBUSMENU_MENUITEM_PROP_LABEL;
	}

	/* Indicator Messages */
	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "application-item") == 0) {
		label_property = "label";
	}

	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "indicator-item") == 0) {
		label_property = "indicator-label";
	}

	/* Indicator Date Time */
	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "appointment-item") == 0) {
		label_property = "appointment-label";
	}

	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "timezone-item") == 0) {
		label_property = "timezone-name";
	}

	/* Indicator Sound */
	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "x-canonical-sound-menu-player-metadata-type") == 0) {
		label_property = "x-canonical-sound-menu-player-metadata-player-name";
	}

	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "x-canonical-sound-menu-mute-type") == 0) {
		label_property = "label";
	}

	/* NOTE: Need to handle the transport item at some point */

	/* Indicator User */
	if (label_property == NULL && g_strcmp0(dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_TYPE), "x-canonical-user-item") == 0) {
		label_property = "user-item-name";
	}

	/* Tokenize */
	if (label_property != NULL && dbusmenu_menuitem_property_exist(item, label_property)) {
		GStrv newstr = NULL;
		const gchar * label = dbusmenu_menuitem_property_get(item, label_property);

		if (label_prefix != NULL && label_prefix[0] != NULL) {
			gint i;
			guint prefix_len = g_strv_length(label_prefix);
			newstr = g_new(gchar *, prefix_len + 2);

			for (i = 0; i < prefix_len; i++) {
				newstr[i] = g_strdup(label_prefix[i]);
			}

			newstr[prefix_len] = g_strdup(label);
			newstr[prefix_len + 1] = NULL;
		} else {
			newstr = g_new0(gchar *, 2);
			newstr[0] = g_strdup(label);
			newstr[1] = NULL;
		}

		return newstr;
	}

	return NULL;
}

static GList *
tokens_to_children (DbusmenuMenuitem * rootitem, const gchar * search, GList * results, GStrv label_prefix, DbusmenuClient * client, const gchar * indicator_name)
{
	if (search == NULL) {
		return results;
	}

	if (rootitem == NULL) {
		return results;
	}

	/* We can only evaluate these properties if we know the type */
	if (!dbusmenu_menuitem_property_exist(rootitem, DBUSMENU_MENUITEM_PROP_TYPE) ||
			g_strcmp0(dbusmenu_menuitem_property_get(rootitem, DBUSMENU_MENUITEM_PROP_TYPE), DBUSMENU_CLIENT_TYPES_DEFAULT) == 0) {
		if (!dbusmenu_menuitem_property_get_bool(rootitem, DBUSMENU_MENUITEM_PROP_ENABLED)) {
			return results;
		}

		if (!dbusmenu_menuitem_property_get_bool(rootitem, DBUSMENU_MENUITEM_PROP_VISIBLE)) {
			return results;
		}
	}

	GStrv newstr = menuitem_to_tokens(rootitem, label_prefix);

	if (!dbusmenu_menuitem_get_root(rootitem) && newstr != NULL) {
		GStrv used_strings = NULL;
		guint distance = calculate_distance(search, newstr, &used_strings);
		if (distance < G_MAXUINT) {
			// g_debug("Distance %d for '%s' in \"'%s'\" using \"'%s'\"", distance, search, g_strjoinv("' '", newstr), g_strjoinv("' '", used_strings));
			results = g_list_prepend(results, dbusmenu_collector_found_new(client, rootitem, newstr, distance, used_strings, indicator_name));
		}
		g_strfreev(used_strings);
	}

	if (newstr == NULL) {
		newstr = g_strdupv(label_prefix);
	}

	GList * children = dbusmenu_menuitem_get_children(rootitem);
	GList * child;

	for (child = children; child != NULL; child = g_list_next(child)) {
		DbusmenuMenuitem * item = DBUSMENU_MENUITEM(child->data);

		results = tokens_to_children(item, search, results, newstr, client, indicator_name);
	}

	g_strfreev(newstr);
	return results;
}

static GList *
process_client (DbusmenuCollector * collector, DbusmenuClient * client, const gchar * search, GList * results, const gchar * indicator_name, GStrv prefix)
{
	/* Handle the case where there are no search terms */
	if (search == NULL || search[0] == '\0') {
		GList * children = dbusmenu_menuitem_get_children(dbusmenu_client_get_root(client));
		GList * child;

		for (child = children; child != NULL; child = g_list_next(child)) {
			DbusmenuMenuitem * item = DBUSMENU_MENUITEM(child->data);

			if (!dbusmenu_menuitem_property_exist(item, DBUSMENU_MENUITEM_PROP_LABEL)) {
				continue;
			}

			const gchar * label = dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_LABEL);
			const gchar * array[2];
			array[0] = label;
			array[1] = NULL;

			results = g_list_prepend(results, dbusmenu_collector_found_new(client, item, (GStrv)array, calculate_distance(NULL, (GStrv)array, NULL), NULL, indicator_name));
		}

		return results;
	}

	results = tokens_to_children(dbusmenu_client_get_root(client), search, results, prefix, client, indicator_name);
	return results;
}

static void
hash_print (gpointer key, gpointer value, gpointer user_data)
{
	menu_key_t * menukey = (menu_key_t *)key;

	g_debug("Addr: '%s'  Path: '%s'  Hash: '%u'", menukey->sender, menukey->path, menu_hash_func(key));

	return;
}

static GList *
just_do_it (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * search, GList * results, const gchar * indicator_name, GStrv prefix)
{
	g_return_val_if_fail(IS_DBUSMENU_COLLECTOR(collector), results);

	menu_key_t search_key = {
		sender: (gchar *)dbus_addr,
		path:   (gchar *)dbus_path
	};

	gpointer found = g_hash_table_lookup(collector->priv->hash, &search_key);
	if (found != NULL) {
		results = process_client(collector, DBUSMENU_CLIENT(found), search, results, indicator_name, prefix);
	} else {
		g_warning("Unable to find menu '%s' on '%s' with hash '%u'", dbus_path, dbus_addr, menu_hash_func(&search_key));

		g_debug("Dumping Hash");
		g_hash_table_foreach(collector->priv->hash, hash_print, NULL);
	}

	return results;
}

static gint
dbusmenu_collector_found_sort (gconstpointer a, gconstpointer b)
{
	DbusmenuCollectorFound * founda;
	DbusmenuCollectorFound * foundb;

	founda = (DbusmenuCollectorFound *)a;
	foundb = (DbusmenuCollectorFound *)b;

	return dbusmenu_collector_found_get_distance(founda) - dbusmenu_collector_found_get_distance(foundb);
}

GList *
dbusmenu_collector_search (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * search)
{
	GList * items = NULL;

	if (dbus_addr != NULL && dbus_path != NULL) {
		items = just_do_it(collector, dbus_addr, dbus_path, search, NULL, NULL, NULL);
	}

	/* This is where we'll do the indicators if we're not
	   looking at the null search.  In that case we'll let
	   the client take over. */
	if (search != NULL && search[0] != '\0') {
		GList * indicators = indicator_tracker_get_indicators(collector->priv->tracker);
		GList * lindicator = NULL;
		for (lindicator = indicators; lindicator != NULL; lindicator = g_list_next(lindicator)) {
			IndicatorTrackerIndicator * indicator = (IndicatorTrackerIndicator *)lindicator->data;

			gchar * array[2];
			array[0] = indicator->prefix;
			array[1] = NULL;

			GList * iitems = just_do_it(collector, indicator->dbus_name, indicator->dbus_object, search, NULL, indicator->name, array);

			/* Increase indicator's distance by 50% */
			GList * iitem = iitems;
			while (iitem != NULL) {
				DbusmenuCollectorFound * found = (DbusmenuCollectorFound *)iitem->data;
				found->distance = found->distance + ((found->distance * get_settings_uint(collector->priv->search_settings, "indicator-penalty",50)) / 100);
				iitem = g_list_next(iitem);
			}

			items = g_list_concat(items, iitems);
		}

		g_list_free(indicators);
	}

	items = g_list_sort(items, dbusmenu_collector_found_sort);

	return items;
}

void
dbusmenu_collector_execute (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, gint id, guint timestamp)
{
	g_return_if_fail(IS_DBUSMENU_COLLECTOR(collector));

	menu_key_t search_key = {
		sender: (gchar *)dbus_addr,
		path:   (gchar *)dbus_path
	};

	gpointer found = g_hash_table_lookup(collector->priv->hash, &search_key);
	if (found == NULL) {
		g_warning("Unable to find dbusmenu client: %s:%s", dbus_addr, dbus_path);
		return;
	}

	DbusmenuMenuitem * root = dbusmenu_client_get_root(DBUSMENU_CLIENT(found));
	if (root == NULL) {
		g_warning("Dbusmenu Client %s:%s has no menuitems", dbus_addr, dbus_path);
		return;
	}

	DbusmenuMenuitem * item = dbusmenu_menuitem_find_id(root, id);
	if (item == NULL) {
		g_warning("Unable to find menuitem %d on client %s:%s", id, dbus_addr, dbus_path);
		return;
	}

	g_debug("Executing menuitem %d: %s", id, dbusmenu_menuitem_property_get(item, DBUSMENU_MENUITEM_PROP_LABEL));
	dbusmenu_menuitem_handle_event(item, DBUSMENU_MENUITEM_EVENT_ACTIVATED, NULL, timestamp);

	return;
}

guint
dbusmenu_collector_found_get_distance (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, G_MAXUINT);
	return found->distance;
}

const gchar *
dbusmenu_collector_found_get_display (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, NULL);
	return found->display_string;
}

const gchar *
dbusmenu_collector_found_get_db (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, NULL);
	return found->db_string;
}

void
dbusmenu_collector_found_list_free (GList * found_list)
{
	g_list_free_full(found_list, (GDestroyNotify)dbusmenu_collector_found_free);
	return;
}

static DbusmenuCollectorFound *
dbusmenu_collector_found_new (DbusmenuClient * client, DbusmenuMenuitem * item, GStrv strings, guint distance, GStrv usedstrings, const gchar * indicator_name)
{
	// g_debug("New Found: '%s', %d, '%s'", string, distance, indicator_name);
	DbusmenuCollectorFound * found = g_new0(DbusmenuCollectorFound, 1);

	g_object_get(client,
	             DBUSMENU_CLIENT_PROP_DBUS_NAME, &found->dbus_addr,
	             DBUSMENU_CLIENT_PROP_DBUS_OBJECT, &found->dbus_path,
	             NULL);
	found->dbus_id = dbusmenu_menuitem_get_id(item);

	found->db_string = g_strjoinv(DB_SEPARATOR, strings);
	found->distance = distance;
	found->item = item;
	found->indicator = NULL;

	found->display_string = NULL;
	if (strings != NULL) {
		static gchar * connector = NULL;

		if (connector == NULL) {
			/* TRANSLATORS: This string is a printf format string to build
			   a string representing menu hierarchy in an application.  The
			   strings are <top> <separator> <bottom>.  So if the separator
			   is ">" and the item is "Open" in the "File" menu the final
			   string would be "File > Open" */
			connector = g_markup_escape_text(_("%s > %s"), -1);
		}

		gchar * firstunderline = remove_underline(strings[0]);
		found->display_string = g_markup_escape_text(firstunderline, -1);
		g_free(firstunderline);
		int i;
		for (i = 1; strings[i] != NULL; i++) {
			gchar * nounder = remove_underline(strings[i]);
			gchar * escaped = g_markup_escape_text(nounder, -1);
			gchar * tmp = g_strdup_printf(connector, found->display_string, escaped);
			g_free(found->display_string);
			g_free(escaped);
			g_free(nounder);
			found->display_string = tmp;
		}

		/* NOTE: Should probably find some way to use remalloc here, not sure
		   how to do that with the escaping and the translated connector
		   though.  Will take some thinking. */
	}

	if (found->display_string != NULL && usedstrings != NULL) {
		int str;
		for (str = 0; usedstrings[str] != NULL; str++) {
			if (usedstrings[str][0] == '\0') continue; // No NULL strings
			if (usedstrings[str][0] == '&')  continue; // Not a useful match and it violates markup rules
			gchar * nounder = remove_underline(usedstrings[str]);
			GStrv split = g_strsplit(found->display_string, nounder, -1);
			gchar * bold = g_strconcat("<b>", nounder, "</b>", NULL);
			gchar * tmp = g_strjoinv(bold, split);

			g_free(found->display_string);
			found->display_string = tmp;

			g_strfreev(split);
			g_free(bold);
			g_free(nounder);
		}
	}

	if (indicator_name != NULL) {
		found->indicator = g_strdup(indicator_name);
	}

	g_object_ref(G_OBJECT(item));

	return found;
}

void
dbusmenu_collector_found_free (DbusmenuCollectorFound * found)
{
	g_return_if_fail(found != NULL);
	g_free(found->dbus_addr);
	g_free(found->dbus_path);
	g_free(found->display_string);
	g_free(found->db_string);
	g_free(found->indicator);
	g_object_unref(found->item);
	g_free(found);
	return;
}

const gchar *
dbusmenu_collector_found_get_indicator (DbusmenuCollectorFound * found)
{
	// g_debug("Getting indicator for found '%s', indicator: '%s'", found->display_string, found->indicator);
	g_return_val_if_fail(found != NULL, NULL);
	return found->indicator;
}

const gchar *
dbusmenu_collector_found_get_dbus_addr (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, NULL);
	return found->dbus_addr;
}

const gchar *
dbusmenu_collector_found_get_dbus_path (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, NULL);
	return found->dbus_path;
}

gint
dbusmenu_collector_found_get_dbus_id (DbusmenuCollectorFound * found)
{
	g_return_val_if_fail(found != NULL, -1);
	return found->dbus_id;
}
