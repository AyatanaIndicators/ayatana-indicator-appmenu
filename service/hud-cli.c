
#include <glib.h>
#include <gio/gunixinputstream.h>
#include <unistd.h>
#include <stdio.h>

#include "shared-values.h"
#include "hud.interface.h"

static void input_cb (GObject * object, GAsyncResult * res, gpointer user_data);
static void print_suggestions(const gchar * query);
static void execute_query(const gchar * query);
static void append_query (gchar ** query, const gchar * append);

static GMainLoop * mainloop = NULL;
static gchar * query = NULL;
static GDBusProxy * proxy = NULL;
static GVariant * last_key = NULL;

int
main (int argv, char * argc[])
{
	g_type_init();

	query = g_strdup("");

	GInputStream * stdinput = g_unix_input_stream_new(STDIN_FILENO, FALSE);
	g_return_val_if_fail(stdinput != NULL, 1);

	gchar buffer[256];
	g_input_stream_read_async(stdinput,
	                          /* buffer */    buffer,
	                          /* size */      256,
	                          /* priority */  G_PRIORITY_DEFAULT,
	                          /* cancel */    NULL,
	                          /* callback */  input_cb,
	                          /* userdata */  buffer);

	GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(session != NULL, 1);

	GDBusNodeInfo * nodeinfo = g_dbus_node_info_new_for_xml(hud_interface, NULL);
	g_return_val_if_fail(nodeinfo != NULL, 1);

	GDBusInterfaceInfo * ifaceinfo = g_dbus_node_info_lookup_interface(nodeinfo, DBUS_IFACE);
	g_return_val_if_fail(ifaceinfo != NULL, 1);

	proxy = g_dbus_proxy_new_sync(session,
	                              G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                              ifaceinfo,
	                              DBUS_NAME,
	                              DBUS_PATH,
	                              DBUS_IFACE,
	                              NULL, NULL);
	g_return_val_if_fail(proxy != NULL, 1);

	print_suggestions(query);
	g_print("Query: ");

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);
	g_object_unref(proxy);
	g_dbus_node_info_unref(nodeinfo);
	g_object_unref(session);
	g_object_unref(stdinput);

	return 0;
}

/* Input coming from STDIN */
static void
input_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GInputStream * stdinput = G_INPUT_STREAM(object);
	GError * error = NULL;
	gssize size = 0;

	size = g_input_stream_read_finish(stdinput, res, &error);

	if (error != NULL) {
		g_error("Error from STDIN: %s", error->message);
		g_error_free(error);
		g_main_loop_quit(mainloop);
		return;
	}

	if (size == 0 || size == 1) {
		g_print("Final command, executing\n");
		execute_query(query);
		g_main_loop_quit(mainloop);
		return;
	}

	gchar * buffer = (gchar *)user_data;
	g_input_stream_read_async(stdinput,
	                          /* buffer */    buffer,
	                          /* size */      256,
	                          /* priority */  G_PRIORITY_DEFAULT,
	                          /* cancel */    NULL,
	                          /* callback */  input_cb,
	                          /* userdata */  buffer);

	buffer[size - 1] = '\0';

	append_query(&query, buffer);

	print_suggestions(query);
	g_print("Query: %s", query);

	return;
}

static void
print_suggestions (const gchar * query)
{
	GError * error = NULL;
	GVariant * suggests = g_dbus_proxy_call_sync(proxy,
	                                             "GetSuggestions",
	                                             g_variant_new("(s)", query),
	                                             G_DBUS_CALL_FLAGS_NONE,
	                                             -1,
	                                             NULL,
	                                             &error);

	if (error != NULL) {
		g_warning("Unable to get suggestions: %s", error->message);
		g_error_free(error);
		return;
	}

	GVariant * target = g_variant_get_child_value(suggests, 0);
	// g_print("Target: %s\n", g_variant_get_string(target, NULL));
	g_variant_unref(target);

	GVariant * suggestions = g_variant_get_child_value(suggests, 1);
	GVariantIter iter;
	g_variant_iter_init(&iter, suggestions);
	gchar * suggestion = NULL;
	gchar * icon = NULL;
	GVariant * key = NULL;

	if (last_key != NULL) {
		g_variant_unref(last_key);
		last_key = NULL;
	}

	while (g_variant_iter_loop(&iter, "(ssv)", &suggestion, &icon, &key)) {
		g_print(" %s\n", suggestion);
		if (last_key == NULL) {
			last_key = key;
			g_variant_ref(last_key);
		}
	}

	g_variant_unref(suggestions);

	return;
}

static void
execute_query (const gchar * query)
{
	GError * error = NULL;
	g_dbus_proxy_call_sync(proxy,
	                       "ExecuteQuery",
	                       g_variant_new("(s)", query),
	                       G_DBUS_CALL_FLAGS_NONE,
	                       -1,
	                       NULL,
	                       &error);

	if (error != NULL) {
		g_warning("Unable to get suggestions: %s", error->message);
		g_error_free(error);
		return;
	}

	return;
}

static void
append_query (gchar ** query, const gchar * append)
{
	gchar * temp = g_strdup_printf("%s%s", *query, append);
	g_free(*query);
	*query = temp;
	return;
}
