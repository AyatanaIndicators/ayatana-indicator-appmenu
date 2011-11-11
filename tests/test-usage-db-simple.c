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
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Two") == 2);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Three") == 3);
	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Four") == 4);

	g_object_unref(tracker);
	return;
}

static void
test_usage_db_insert (void)
{
	UsageTracker * tracker = usage_tracker_new();
	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	int i = 0;

	for (i = 0; i < 5; i++) {
		usage_tracker_mark_usage(tracker, "testapp.desktop", "Five");
	}

	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Five") == 5);

	for (i = 0; i < 100; i++) {
		usage_tracker_mark_usage(tracker, "testapp.desktop", "Hundred");
	}

	g_assert(usage_tracker_get_usage(tracker, "testapp.desktop", "Hundred") == 100);

	g_object_unref(tracker);
	return;
}

/* Build the test suite */
static void
test_usage_db_suite (void)
{
	g_test_add_func ("/hud/usage/simple/base",          test_usage_db_base);
	g_test_add_func ("/hud/usage/simple/counts",        test_usage_db_counts);
	g_test_add_func ("/hud/usage/simple/insert",        test_usage_db_insert);
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
