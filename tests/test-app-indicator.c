/*
Test code for appindicator

Copyright 2012 Canonical Ltd.

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

#include <libappindicator/app-indicator.h>

GMainLoop * mainloop = NULL;

gboolean
timeout (gpointer user_data)
{
	g_print("Timeout\n");
	g_main_loop_quit(mainloop);
	return FALSE;
}

int
main (int argc, char ** argv)
{
	GtkWidget *menu = NULL;
	AppIndicator *ci = NULL;

	gtk_init (&argc, &argv);

	ci = app_indicator_new ("example-simple-client",
	                        "indicator-messages",
	                        APP_INDICATOR_CATEGORY_APPLICATION_STATUS);

	g_assert (IS_APP_INDICATOR (ci));
	g_assert (G_IS_OBJECT (ci));

	menu = gtk_menu_new (); 
	GtkWidget *item = gtk_check_menu_item_new_with_label("Test");
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	gtk_widget_show (item);

	app_indicator_set_menu (ci, GTK_MENU (menu));

	g_timeout_add_seconds(2, timeout, NULL);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	return 0;
}

