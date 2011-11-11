#include <glib.h>
#include <glib-object.h>

#include "../service/usage-tracker.h"
#include "../service/usage-tracker.c"

/* Ensure the base calculation works */
static void
test_usage_db_base (void)
{
	UsageTracker * tracker = usage_tracker_new();
	g_assert(tracker != NULL);
	g_assert(IS_USAGE_TRACKER(tracker));

	g_object_unref(tracker);
	return;
}

/* Build the test suite */
static void
test_usage_db_suite (void)
{
	g_test_add_func ("/hud/usage/simple/base",          test_usage_db_base);
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
