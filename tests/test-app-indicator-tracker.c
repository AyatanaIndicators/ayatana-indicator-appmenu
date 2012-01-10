/*
Test code for indicator tracker.

Copyright 2011 Canonical Ltd.

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

#include <glib.h>
#include <glib-object.h>

#include "indicator-tracker.h"
#include "indicator-tracker.c"

gboolean
end_of_line (gpointer user_data)
{
	g_main_loop_quit((GMainLoop *)user_data);
	return FALSE;
}

gint
main (gint argc, gchar * argv[])
{
	g_type_init();

	IndicatorTracker * tracker = indicator_tracker_new();

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);

	g_timeout_add_seconds(1, end_of_line, mainloop);

	g_main_loop_run(mainloop);
	g_main_loop_unref(mainloop);

	g_print("Checking for indicators\n");

	gboolean found_appindicator = FALSE;
	gboolean failed = FALSE;

	GList * indicators = indicator_tracker_get_indicators(tracker);
	GList * indicator_pntr;

	for (indicator_pntr = indicators; indicator_pntr != NULL; indicator_pntr = g_list_next(indicator_pntr)) {
		IndicatorTrackerIndicator * indicator = (IndicatorTrackerIndicator *)indicator_pntr->data;

		if (g_strcmp0(indicator->dbus_object, "/org/ayatana/NotificationItem/example_simple_client/Menu") == 0) {
			found_appindicator = TRUE;
			continue;
		}

		g_print("Found indicator we didn't expect: %s\n", indicator->dbus_object);
		failed = TRUE;
		break;
	}

	g_object_unref(tracker);

	if (!found_appindicator) {
		g_print("Missing Indicators\n");
		failed = TRUE;
	}

	if (!failed) {
		g_print("Found everything\n");
		return 0;
	} else {
		g_print("Failed\n");
		return 1;
	}
}
