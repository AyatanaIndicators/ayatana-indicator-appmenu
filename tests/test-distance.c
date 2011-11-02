#include <glib.h>
#include <glib-object.h>

#define DUMP_MATRIX 1
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

/* Test a set of strings */
static void
test_set (const gchar ** teststrings, int num_tests, const gchar * search, int right)
{
	int i;

	for (i = 0; i < num_tests; i++) {
		if (i == right)
			continue;

		if (calculate_distance(search, teststrings[i]) < calculate_distance(search, teststrings[right])) {
			g_error("Found '%s' with search string '%s' instead of '%s'", teststrings[i], search, teststrings[right]);
		}
	}

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

	test_set(teststrings, 4, "Print Pre", 3);
	return;
}

/* Ensure that we can handle some misspelling */
static void
test_distance_missspelll (void)
{
	const gchar * teststrings[] = {
		"File > Open",
		"File > New",
		"File > Print",
		"File > Print Preview",
	};

	test_set(teststrings, 4, "Prnt Pr", 3);
	test_set(teststrings, 4, "Print Preiw", 3);
	test_set(teststrings, 4, "Prnt Pr", 3);

	return;
}

/* Build the test suite */
static void
test_distance_suite (void)
{
	g_test_add_func ("/hud/distance/base",          test_distance_base);
	g_test_add_func ("/hud/distance/subfunction",   test_distance_subfunction);
	g_test_add_func ("/hud/distance/missspelll",    test_distance_missspelll);
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
