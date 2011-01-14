/*
A little utility to make a mock application using the JSON
menu description.

Copyright 2010 Canonical Ltd.

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

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-jsonloader/json-loader.h>

#include "../src/dbus-shared.h"

#define MENU_PATH "/mock/json/app/menu"

GtkWidget * window = NULL;
DbusmenuServer * server = NULL;
GDBusProxy * registrar = NULL;

static void
dbus_owner_change (GObject *gobject, GParamSpec *pspec, gpointer user_data)
{
	GDBusProxy * proxy = G_DBUS_PROXY(gobject);

	gchar * owner = g_dbus_proxy_get_name_owner(proxy);

	if (owner == NULL || owner[0] == '\0') {
		/* We only care about folks coming on the bus.  Exit quickly otherwise. */
		g_free(owner);
		return;
	}

	if (g_strcmp0(owner, DBUS_NAME)) {
		/* We only care about this address, reject all others. */
		g_free(owner);
		return;
	}

	GError * error = NULL;
	g_dbus_proxy_call_sync(registrar, "RegisterWindow",
	                       g_variant_new("(uo)",
	                                     GDK_WINDOW_XID (gtk_widget_get_window (window)),
	                                     MENU_PATH),
	                       G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL);
	if (error != NULL) {
		g_warning("Unable to register: %s", error->message);
		g_error_free(error);
	}

	g_free(owner);
	return;
}

static gboolean
idle_func (gpointer user_data)
{
	DbusmenuMenuitem * root = dbusmenu_json_build_from_file((const gchar *)user_data);
	g_return_val_if_fail(root != NULL, FALSE);

	DbusmenuServer * server = dbusmenu_server_new(MENU_PATH);
	dbusmenu_server_set_root(server, root);

	registrar = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
	                                          G_DBUS_PROXY_FLAGS_NONE,
	                                          NULL, DBUS_NAME,
	                                          REG_OBJECT, REG_IFACE,
	                                          NULL, NULL);
	g_return_val_if_fail(registrar != NULL, FALSE);

	GError * error = NULL;
	g_dbus_proxy_call_sync(registrar, "RegisterWindow",
	                       g_variant_new("(uo)",
	                                     GDK_WINDOW_XID (gtk_widget_get_window (window)),
	                                     MENU_PATH),
	                       G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
	if (error != NULL) {
		g_warning("Unable to register: %s", error->message);
		g_error_free(error);
	}

	g_signal_connect(registrar, "notify::g-name-owner",
	                 G_CALLBACK(dbus_owner_change), NULL);

	return FALSE;
}

static void
destroy_cb (void)
{
	gtk_main_quit();
	return;
}

int
main (int argv, char ** argc)
{
	if (argv != 2) {
		g_print("'%s <JSON file>' is how you should use this program.\n", argc[0]);
		return 1;
	}

	gtk_init(&argv, &argc);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy_cb), NULL);
	gtk_widget_show(window);

	g_idle_add(idle_func, argc[1]);

	gtk_main();

	return 0;
}
