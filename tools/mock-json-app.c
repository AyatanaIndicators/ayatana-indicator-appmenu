
#include <gtk/gtk.h>

int
main (int argv, char ** argc)
{
	gtk_init(&argv, &argc);

	GtkWidget * window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_show(window);

	gtk_main();

	return 0;
}
