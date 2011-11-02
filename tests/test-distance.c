#include <glib.h>
#include <glib-object.h>

#include "../service/distance.h"
#include "../service/distance.c"

/* Ensure the base calculation works */
static void
test_distance_base (void)
{
	g_assert(calculate_distance("foo", "foo") == 0);
	g_assert(calculate_distance("foo", "bar") != 0);
	g_assert(calculate_distance("foo", NULL) != 0);
	g_assert(calculate_distance(NULL, "foo") != 0);

	return;
}

/* Ensure the base calculation works */
static void
test_distance_subfunction (void)
{
	const gchar * teststrings[] = {
		"File > Open",
		"File > New",
		"File > Print",
		"File > Print Preview",
	};
	const gchar * search = "Print Pre";
	int i;
	int right = 3;
	int num_tests = 4;

	for (i = 0; i < num_tests; i++) {
		if (i == right)
			continue;

		if (calculate_distance(search, teststrings[i]) < calculate_distance(search, teststrings[right])) {
			g_error("Found '%s' with search string '%s' instead of '%s'", teststrings[i], search, teststrings[right]);
		}
	}

	return;
}

/* Build the test suite */
static void
test_distance_suite (void)
{
	g_test_add_func ("/hud/distance/base",          test_distance_base);
	g_test_add_func ("/hud/distance/subfunction",   test_distance_subfunction);
	return;
}

gint
main (gint argc, gchar * argv[])
{
	//gtk_init(&argc, &argv);
	g_type_init();

	g_test_init(&argc, &argv, NULL);

	/* Test suites */
	test_distance_suite();

	return g_test_run ();
}
