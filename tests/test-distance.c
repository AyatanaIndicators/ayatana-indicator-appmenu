#include <glib.h>
#include <glib-object.h>

#include "../service/distance.h"
#include "../service/distance.c"

/* Ensure the parser can find children */
static void
test_distance_base (void) {

	return;
}

/* Build the test suite */
static void
test_distance_suite (void)
{
	g_test_add_func ("/hud/distance/base",          test_distance_base);
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
