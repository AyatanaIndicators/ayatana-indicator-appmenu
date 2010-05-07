#include <gtk/gtk.h>
#include <dbus/dbus-glib.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/server.h>
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
	dbusmenu_menuitem_child_append(firstlevel, secondlevel);

	secondlevel = dbusmenu_menuitem_new();
	dbusmenu_menuitem_property_set(secondlevel, DBUSMENU_MENUITEM_PROP_LABEL, "Save");
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

	org_ayatana_indicator_appmenu_registrar_register_window(proxy, 0, "/this/is/a/long/object/path", NULL);

	gtk_main();


	return 0;
}
