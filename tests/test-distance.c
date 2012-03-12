/*
Test code for distance functions

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
#include "distance.h"
#include "distance.c"

/* hardcode some parameters so the test doesn't fail if the user
 * has bogus things in GSettings.
 */
HudSettings hud_settings = {
	.indicator_penalty = 50,
	.add_penalty = 10,
	.add_penalty_pre = 1,
	.drop_penalty = 10,
	.drop_penalty_end = 10,
	.transpose_penalty = 10,
	.swap_penalty = 10,
	.swap_penalty_case = 1,
	.max_distance = 30
};

/* Ensure the base calculation works */
static void
test_distance_base (void)
{
	gchar * testdata1[] = {"foo", NULL};
	g_assert(calculate_distance("foo", testdata1, NULL) == 0);

	gchar * testdata2[] = {"bar", NULL};
	g_assert(calculate_distance("foo", testdata2, NULL) != 0);

	g_assert(calculate_distance("foo", NULL, NULL) != 0);

	g_assert(calculate_distance(NULL, testdata1, NULL) != 0);

	return;
}

/* Test a set of strings */
static void
test_set (GStrv * teststrings, int num_tests, const gchar * search, int right)
{
	int i;

	for (i = 0; i < num_tests; i++) {
		if (i == right)
			continue;

		if (calculate_distance(search, teststrings[i], NULL) < calculate_distance(search, teststrings[right], NULL)) {
			gchar * teststr = g_strjoinv(" > ", teststrings[i]);
			gchar * rightstr = g_strjoinv(" > ", teststrings[right]);

			g_error("Found '%s' with search string '%s' instead of '%s'", teststr, search, rightstr);

			g_free(teststr);
			g_free(rightstr);
		}
	}

	return;
}

/* Ensure the base calculation works */
static void
test_distance_subfunction (void)
{
	GStrv teststrings[4];
	gchar * teststrings0[] = {"File", "Open", NULL}; teststrings[0] = teststrings0;
	gchar * teststrings1[] = {"File", "New", NULL}; teststrings[1] = teststrings1;
	gchar * teststrings2[] = {"File", "Print", NULL}; teststrings[2] = teststrings2;
	gchar * teststrings3[] = {"File", "Print Preview", NULL}; teststrings[3] = teststrings3;

	test_set(teststrings, 4, "Print Pre", 3);
	return;
}

/* Ensure that we can handle some misspelling */
static void
test_distance_missspelll (void)
{
	GStrv teststrings[4];
	gchar * teststrings0[] = {"File", "Open", NULL}; teststrings[0] = teststrings0;
	gchar * teststrings1[] = {"File", "New", NULL}; teststrings[1] = teststrings1;
	gchar * teststrings2[] = {"File", "Print", NULL}; teststrings[2] = teststrings2;
	gchar * teststrings3[] = {"File", "Print Preview", NULL}; teststrings[3] = teststrings3;

	test_set(teststrings, 4, "Prnt Pr", 3);
	test_set(teststrings, 4, "Print Preiw", 3);
	test_set(teststrings, 4, "Prnt Pr", 3);

	return;
}

/* Ensure that we can find print with short strings */
static void
test_distance_print_issues (void)
{
	GStrv teststrings[6];
	gchar * teststrings0[] = {"File", "New", NULL}; teststrings[0] = teststrings0;
	gchar * teststrings1[] = {"File", "Open", NULL}; teststrings[1] = teststrings1;
	gchar * teststrings2[] = {"Edit", "Undo", NULL}; teststrings[2] = teststrings2;
	gchar * teststrings3[] = {"Help", "About", NULL}; teststrings[3] = teststrings3;
	gchar * teststrings4[] = {"Help", "Empty", NULL}; teststrings[4] = teststrings4;
	gchar * teststrings5[] = {"File", "Print...", NULL}; teststrings[5] = teststrings5;

	test_set(teststrings, 6, "Pr", 5);
	test_set(teststrings, 6, "Print", 5);
	test_set(teststrings, 6, "Print...", 5);

	return;
}

/* A variety of strings that should have predictable results */
static void
test_distance_variety (void)
{
	GStrv teststrings[4];
	gchar * teststrings0[] = {"Date", "House Cleaning", NULL}; teststrings[0] = teststrings0;
	gchar * teststrings1[] = {"File", "Close Window", NULL}; teststrings[1] = teststrings1;
	gchar * teststrings2[] = {"Edit", "Keyboard Shortcuts...", NULL}; teststrings[2] = teststrings2;
	gchar * teststrings3[] = {"Network", "VPN Configuration", "Configure VPN...", NULL}; teststrings[3] = teststrings3;

	test_set(teststrings, 4, "House", 0);
	test_set(teststrings, 4, "House C", 0);
	test_set(teststrings, 4, "House Cle", 0);
	test_set(teststrings, 4, "House Clean", 0);
	test_set(teststrings, 4, "Clean House", 0);

	return;
}

/* A variety of strings that should have predictable results */
static void
test_distance_french_pref (void)
{
	GStrv teststrings[3];
	gchar * teststrings0[] = {"Fichier", "aperçu avant impression", NULL}; teststrings[0] = teststrings0;
	gchar * teststrings1[] = {"Connexion au réseau...", NULL}; teststrings[1] = teststrings1;
	gchar * teststrings2[] = {"Edition", "préférences", NULL}; teststrings[2] = teststrings2;

	test_set(teststrings, 3, "préférences", 2);
	test_set(teststrings, 3, "pré", 2);
	test_set(teststrings, 3, "préf", 2);
	test_set(teststrings, 3, "préfé", 2);
	test_set(teststrings, 3, "pref", 2);

	return;
}

/* Check to make sure the returned hits are not dups and the
   proper number */
static void
test_distance_dups (void)
{
	GStrv hits = NULL;
	gchar * teststrings[] = {"Inflated", "Confluated", "Sublimated", "Sadated", "Situated", "Infatuated", NULL};

	g_assert(calculate_distance("ted inf", teststrings, &hits) != 0);
	g_assert(g_strv_length(hits) == 2);
	g_assert(g_strcmp0(hits[0], hits[1]) != 0);

	g_strfreev(hits);

	return;
}

/* Build the test suite */
static void
test_distance_suite (void)
{
	g_test_add_func ("/hud/distance/base",          test_distance_base);
	g_test_add_func ("/hud/distance/subfunction",   test_distance_subfunction);
	g_test_add_func ("/hud/distance/missspelll",    test_distance_missspelll);
	g_test_add_func ("/hud/distance/print_issues",  test_distance_print_issues);
	g_test_add_func ("/hud/distance/duplicates",    test_distance_dups);
	g_test_add_func ("/hud/distance/variety",       test_distance_variety);
	g_test_add_func ("/hud/distance/french_pref",   test_distance_french_pref);
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
