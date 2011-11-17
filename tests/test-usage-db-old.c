/*
Test function to ensure we auto-clear the old databse

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

#include "../service/usage-tracker.h"
#include "../service/usage-tracker.c"

/* Ensure the base object works */
static void
test_usage_db_base (void)
{
	UsageTracker * tracker = usage_tracker_new();
	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	g_object_unref(tracker);
	return;
}

static void
test_usage_db_counts (void)
{
	UsageTracker * tracker = usage_tracker_new();
	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Zero") == 0);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "One") == 1);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Two") == 1);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Three") == 1);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Four") == 1);

	g_object_unref(tracker);
	return;
}

/* Build the test suite */
static void
test_usage_db_suite (void)
{
	g_test_add_func ("/hud/usage/old/base",          test_usage_db_base);
	g_test_add_func ("/hud/usage/old/counts",        test_usage_db_counts);
	return;
}

gint
main (gint argc, gchar * argv[])
{
	//gtk_init(&argc, &argv);
	g_type_init();

	g_test_init(&argc, &argv, NULL);

	/* Test suites */
	test_usage_db_suite();

	return g_test_run ();
}
