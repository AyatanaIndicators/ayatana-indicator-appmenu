#include <gtk/gtk.h>
#include <libdbusmenu-glib/menuitem.h>
#include <libdbusmenu-glib/server.h>
#include "../src/dbus-shared.h"

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




	gtk_main();


	return 0;
}
