
#include <gtk/gtk.h>
#include <libdbusmenu-glib/server.h>
#include <libdbusmenu-jsonloader/json-loader.h>

#define MENU_PATH "/mock/json/app/menu"

GtkWidget * window = NULL;
DbusmenuServer * server = NULL;

static gboolean
idle_func (gpointer user_data)
{
	DbusmenuMenuitem * root = dbusmenu_json_build_from_file((const gchar *)user_data);
	g_return_val_if_fail(root != NULL, FALSE);

	DbusmenuServer * server = dbusmenu_server_new(MENU_PATH);
	dbusmenu_server_set_root(server, root);

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
	gtk_init(&argv, &argc);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy_cb), NULL);
	gtk_widget_show(window);

	g_idle_add(idle_func, argc[1]);

	gtk_main();

	return 0;
}
