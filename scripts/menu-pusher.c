/*
A test menu pusher to the appmenu.

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
#include <dbus/dbus-glib.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-gtk/menuitem.h>
#include "../src/dbus-shared.h"
#include "../src/application-menu-registrar-client.h"

int
main (int argv, char ** argc)
{
	gtk_init(&argv, &argc);

	DbusmenuMenuitem * root = dbusmenu_menuitem_new();

	DbusmenuMenuitem * firstlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(firstlevel, DBUSMENU_MENUITEM_PROP_LABEL, "File");
	dbusmenu_menuitem_child_append(root, firstlevel);

	DbusmenuMenuitem * secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Open");
	dbusmenu_menuitem_property_set_shortcut_string(secondlevel, "<Control>O");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Save");
	dbusmenu_menuitem_property_set_shortcut_string(secondlevel, "<Control>S");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Exit");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	firstlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(firstlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Edit");
	dbusmenu_menuitem_child_append(root, firstlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Copy");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Paste");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Cut");
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);


	DbusmenuServer * server = dbusmenu_server_new("/this/is/a/long/object/path");
	dbusmenu_server_set_root(server, root);


	DBusGConnection * session = dbus_g_bus_get(DBUS_BUS_SESSION, NULL);
	g_return_val_if_fail(session != NULL, 1);

	DBusGProxy * proxy = dbus_g_proxy_new_for_name_owner(session, DBUS_NAME, REG_OBJECT, REG_IFACE, NULL);
	g_return_val_if_fail(proxy != NULL, 1);

	org_ayatana_WindowMenu_Registrar_register_window(proxy, 0, "/this/is/a/long/object/path", NULL);

	gtk_main();


	return 0;
}
