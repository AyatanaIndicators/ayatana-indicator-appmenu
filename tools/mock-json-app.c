
#include <gtk/gtk.h>

GtkWidget * window = NULL;

static gboolean
idle_func (gpointer user_data)
{

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

	g_idle_add(idle_func, NULL);

	gtk_main();

	return 0;
}
