#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gio/gio.h>
#include <glib.h>

#include <libdbusmenu-glib/client.h>

#include "dbusmenu-collector.h"
#include "distance.h"

#define GENERIC_ICON   "dbusmenu-lens-panel"

typedef struct _DbusmenuCollectorPrivate DbusmenuCollectorPrivate;

struct _DbusmenuCollectorPrivate {
	GDBusConnection * bus;
	guint signal;
	GHashTable * hash;
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

typedef gboolean (*action_function_t) (DbusmenuMenuitem * menuitem, const gchar * full_label, guint distance, GArray * results);

static void dbusmenu_collector_class_init (DbusmenuCollectorClass *klass);
static void dbusmenu_collector_init       (DbusmenuCollector *self);
static void dbusmenu_collector_dispose    (GObject *object);
static void dbusmenu_collector_finalize   (GObject *object);
static void update_layout_cb (GDBusConnection * connection, const gchar * sender, const gchar * path, const gchar * interface, const gchar * signal, GVariant * params, gpointer user_data);
static guint menu_hash_func (gconstpointer key);
static gboolean menu_equal_func (gconstpointer a, gconstpointer b);
static void menu_key_destroy (gpointer key);
static gboolean place_in_results (DbusmenuMenuitem * menuitem, const gchar * full_label, guint distance, GArray * results);
//static gboolean exec_in_results (DbusmenuMenuitem * menuitem, GArray * results);
static gint search_item_sort (gconstpointer a, gconstpointer b);

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

	if (g_strcmp0(ak->sender, bk->sender) == 0) {
		return TRUE;
	}

	if (g_strcmp0(ak->path, bk->path) == 0) {
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
	gchar * output = g_new0(gchar, g_utf8_strlen(input, -1) + 1);
	gchar * out = output;

	while (in[0] != '\0') {
		if (in[0] == '_') {
			in++;
		} else {
			out[0] = in[0];
			in++;
			out++;
		}
	}

	return output;
}

static void
tokens_to_children (DbusmenuMenuitem * rootitem, const gchar * search, GArray * results, const gchar * label_prefix, DbusmenuClient * client, action_function_t action_func)
{
	if (search == NULL) {
		return;
	}

	if (rootitem == NULL) {
		return;
	}

	gchar * newstr = NULL;
	if (dbusmenu_menuitem_property_exist(rootitem, DBUSMENU_MENUITEM_PROP_LABEL)) {
		gchar * nounderline = remove_underline(dbusmenu_menuitem_property_get(rootitem, DBUSMENU_MENUITEM_PROP_LABEL));
		if (label_prefix != NULL && label_prefix[0] != '\0') {
			newstr = g_strdup_printf("%s > %s", label_prefix, nounderline);
			g_free(nounderline);
		} else {
			newstr = nounderline;
		}
	} else {
		newstr = g_strdup(label_prefix);
	}

	if (!dbusmenu_menuitem_get_root(rootitem) && dbusmenu_menuitem_property_exist(rootitem, DBUSMENU_MENUITEM_PROP_LABEL)) {
		guint distance = calculate_distance(search, newstr);
		action_func(rootitem, newstr, distance, results);
		g_debug("String '%s' distance %d", newstr, distance);
	}

	GList * children = dbusmenu_menuitem_get_children(rootitem);
	GList * child;

	for (child = children; child != NULL; child = g_list_next(child)) {
		DbusmenuMenuitem * item = DBUSMENU_MENUITEM(child->data);

		tokens_to_children(item, search, results, newstr, client, action_func);
	}

	g_free(newstr);
	return;
}

static void
process_client (DbusmenuCollector * collector, DbusmenuClient * client, const gchar * search, GArray * results, action_function_t action_func)
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

			if (action_func(item, label, calculate_distance(label, NULL), results)) {
				break;
			}
		}

		return;
	}

	tokens_to_children(dbusmenu_client_get_root(client), search, results, "", client, action_func);
	return;
}

static void
just_do_it (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * search, GArray * results, action_function_t action_func)
{
	g_return_if_fail(IS_DBUSMENU_COLLECTOR(collector));
	g_return_if_fail(search != NULL);

	menu_key_t search_key = {
		sender: (gchar *)dbus_addr,
		path:   (gchar *)dbus_path
	};

	gpointer found = g_hash_table_lookup(collector->priv->hash, &search_key);
	if (found != NULL) {
		process_client(collector, DBUSMENU_CLIENT(found), search, results, action_func);
		// g_array_sort(results, mycomparefunc);
	}

	return;
}

GStrv
dbusmenu_collector_search (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, const gchar * search)
{
	GStrv retval = NULL;

	GArray * searchitems = g_array_new(TRUE, TRUE, sizeof(search_item_t));

	just_do_it(collector, dbus_addr, dbus_path, search, searchitems, place_in_results);

	GArray * results = g_array_new(TRUE, TRUE, sizeof(gchar *));
	guint count = 0;

	g_array_sort(searchitems, search_item_sort);

	g_debug("Migrating over strings");
	for (count = 0; count < 5 && count < searchitems->len; count++) {
		gchar * value = ((search_item_t *)&g_array_index(searchitems, search_item_t, count))->string;
		g_array_append_val(results, value);
	}

	g_debug("Freeing the other entries");
	for (count = 0; count < searchitems->len; count++) {
		search_item_t * search_item = &g_array_index(searchitems, search_item_t, count);

		if (count >= 5) {
			g_free(search_item->string);
		}
	}

	g_array_free(searchitems, TRUE);

	retval = (GStrv)results->data;
	g_array_free(results, FALSE);

	return retval;
}

gboolean
dbusmenu_collector_exec (DbusmenuCollector * collector, const gchar * dbus_addr, const gchar * dbus_path, GStrv tokens)
{
	return TRUE;
}

/*
static gboolean
exec_in_results (DbusmenuMenuitem * menuitem, GArray * results)
{
	dbusmenu_menuitem_handle_event(menuitem, DBUSMENU_MENUITEM_EVENT_ACTIVATED, NULL, 0);
	return FALSE;
}
*/

static gint
search_item_sort (gconstpointer a, gconstpointer b)
{
	search_item_t * sa;
	search_item_t * sb;

	sa = (search_item_t *)a;
	sb = (search_item_t *)b;

	return sa->distance - sb->distance;
}

static gboolean
place_in_results (DbusmenuMenuitem * item, const gchar * full_label, guint distance, GArray * results)
{
	search_item_t search_item = {
		string: g_strdup(full_label),
		distance: distance
	};
	g_array_append_val(results, search_item);

	return FALSE;
}

