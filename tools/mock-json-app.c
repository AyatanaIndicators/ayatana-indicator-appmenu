
#include <gtk/gtk.h>

GtkWidget * window = NULL;

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

	gtk_main();

	return 0;
}
