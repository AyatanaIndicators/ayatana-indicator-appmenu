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

	int i;
	gboolean found_messaging = FALSE;
	gboolean found_sound = FALSE;
	gboolean failed = FALSE;

	GArray * indicators = indicator_tracker_get_indicators(tracker);

	for (i = 0; i < indicators->len; i++) {
		IndicatorTrackerIndicator * indicator = &g_array_index(indicators, IndicatorTrackerIndicator, i);

		if (g_strcmp0(indicator->dbus_name_wellknown, "com.canonical.indicator.messages") == 0) {
			found_messaging = TRUE;
			continue;
		}

		if (g_strcmp0(indicator->dbus_name_wellknown, "com.canonical.indicators.sound") == 0) {
			found_sound = TRUE;
			continue;
		}

		g_warning("Found indicator we didn't expect: %s", indicator->dbus_name_wellknown);
		failed = TRUE;
		break;
	}

	g_object_unref(tracker);

	if (!found_messaging || !found_sound) {
		g_warning("Missing Indicators");
		failed = TRUE;
	}

	if (!failed) {
		g_debug("Found everything");
		return 0;
	} else {
		g_warning("Failed");
		return 1;
	}
}
