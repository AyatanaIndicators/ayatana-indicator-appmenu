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
#include <dbus/dbus-glib.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-jsonloader/json-loader.h>

#include "../src/dbus-shared.h"
#include "../src/application-menu-registrar-client.h"

#define MENU_PATH "/mock/json/app/menu"

GtkWidget * window = NULL;
DbusmenuServer * server = NULL;
DBusGProxy * registrar = NULL;

static void
register_cb (DBusGProxy *proxy, GError *error, gpointer userdata)
{
	if (error != NULL) {
		g_warning("Unable to register: %s", error->message);
	}
	return;
}

static void
dbus_owner_change (DBusGProxy * proxy, const gchar * name, const gchar * prev, const gchar * new, gpointer data)
{
	if (new == NULL || new[0] == '\0') {
		/* We only care about folks coming on the bus.  Exit quickly otherwise. */
		return;
	}

	if (g_strcmp0(name, DBUS_NAME)) {
		/* We only care about this address, reject all others. */
		return;
	}

	org_ayatana_AppMenu_Registrar_register_window_async(registrar, GDK_WINDOW_XID (gtk_widget_get_window (window)), MENU_PATH, register_cb, NULL);

	return;
}

static gboolean
idle_func (gpointer user_data)
{
	DbusmenuMenuitem * root = dbusmenu_json_build_from_file((const gchar *)user_data);
	g_return_val_if_fail(root != NULL, FALSE);

	DbusmenuServer * server = dbusmenu_server_new(MENU_PATH);
	dbusmenu_server_set_root(server, root);

	DBusGConnection * session_bus = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	registrar = dbus_g_proxy_new_for_name(session_bus, DBUS_NAME, REG_OBJECT, REG_IFACE);
	g_return_val_if_fail(registrar != NULL, FALSE);

	org_ayatana_AppMenu_Registrar_register_window_async(registrar, GDK_WINDOW_XID (gtk_widget_get_window (window)), MENU_PATH, register_cb, NULL);

	DBusGProxy * dbus_proxy = dbus_g_proxy_new_for_name(session_bus,
		                                                DBUS_SERVICE_DBUS,
		                                                DBUS_PATH_DBUS,
		                                                DBUS_INTERFACE_DBUS);
	dbus_g_proxy_add_signal(dbus_proxy, "NameOwnerChanged",
	                        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
	                        G_TYPE_INVALID);
	dbus_g_proxy_connect_signal(dbus_proxy, "NameOwnerChanged",
	                            G_CALLBACK(dbus_owner_change), NULL, NULL);

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
