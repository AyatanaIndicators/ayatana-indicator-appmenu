/*
Test code for usage DB based on testapp data

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

#define DUMP_MATRIX 1
#include "load-app-info.h"
#include "load-app-info.c"
#include "usage-tracker.h"
#include "usage-tracker.c"

static void
test_usage_testapp (void)
{
	UsageTracker * tracker = usage_tracker_new();

	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	g_assert(usage_tracker_get_usage(tracker, "/usr/share/applications/testapp.desktop", "Menu > Item") == 7);

	g_object_unref(tracker);
	return;
}

static void
test_usage_testapp_clip (void)
{
	UsageTracker * tracker = usage_tracker_new();

	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	g_assert(usage_tracker_get_usage(tracker, "/usr/share/applications/testapp100.desktop", "Menu > Item") == 30);

	g_object_unref(tracker);
	return;
}

/* Build the test suite */
static void
test_usage_testapp_suite (void)
{
	g_test_add_func ("/hud/usage/testapp/basic",     test_usage_testapp);
	g_test_add_func ("/hud/usage/testapp/clip",      test_usage_testapp_clip);
	return;
}

gint
main (gint argc, gchar * argv[])
{
	//gtk_init(&argc, &argv);
	g_type_init();

	g_test_init(&argc, &argv, NULL);

	/* Test suites */
	test_usage_testapp_suite();

	return g_test_run ();
}
