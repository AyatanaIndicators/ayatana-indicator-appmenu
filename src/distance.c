/*
Functions to calculate the distance between two strings.

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
#include <glib/gprintf.h>
#include <glib-object.h>
#include <glib/gi18n.h>

#include "distance.h"

#define ADD_PENALTY 10
#define PRE_ADD_PENALTY 1
#define DROP_PENALTY 10
#define END_DROP_PENALTY 10
#define TRANSPOSE_PENALTY 10
#define DELETE_PENALTY 10
#define SWAP_PENALTY 10

static gboolean
ignore_character (gchar inchar)
{
	static gchar * ignore = NULL;
	if (ignore == NULL) {
		/* TRANSLATORS: These are chacaters that should not be considered
		   mistakes in the comparison functions.  Typically they are gramatical
		   characters that can be found in menus. */
		ignore = _(" _->");
	}

	int i;
	for (i = 0; i < 4; i++) {
		if (ignore[i] == inchar) {
			return TRUE;
		}
	}
	return FALSE;
}

static guint
swap_cost (gchar a, gchar b)
{
	if (a == b)
		return 0;
	if (ignore_character(a) || ignore_character(b))
		return 0;
	if (g_unichar_toupper(a) == g_unichar_toupper(b))
		return SWAP_PENALTY / 10; /* Some penalty, but close */
	return SWAP_PENALTY;
}

#define MATRIX_VAL(needle_loc, haystack_loc) (penalty_matrix[(needle_loc + 1) + (haystack_loc + 1) * (len_needle + 1)])

static void
dumpmatrix (const gchar * needle, guint len_needle, const gchar * haystack, guint len_haystack, guint * penalty_matrix)
{
#ifndef DUMP_MATRIX
	return;
#endif
	gint i, j;

	g_printf("\n");
	/* Character Column */
	g_printf("  ");
	/* Base val column */
	g_printf("  ");
	for (i = 0; i < len_needle; i++) {
		g_printf("%c ", needle[i]);
	}
	g_printf("\n");

	/* Character Column */
	g_printf("  ");
	for (i = -1; i < (gint)len_needle; i++) {
		g_printf("%u ", MATRIX_VAL(i, -1));
	}
	g_printf("\n");

	for (j = 0; j < len_haystack; j++) {
		g_printf("%c ", haystack[j]);
		for (i = -1; i < (gint)len_needle; i++) {
			g_printf("%u ", MATRIX_VAL(i, j));
		}
		g_printf("\n");
	}
	g_printf("\n");

	return;
}

static guint
calculate_token_distance (const gchar * needle, const gchar * haystack)
{
	// g_debug("Comparing token '%s' to token '%s'", needle, haystack);

	guint len_needle = 0;
	guint len_haystack = 0;

	if (needle != NULL) {
		len_needle = g_utf8_strlen(needle, -1);
	}

	if (haystack != NULL) {
		len_haystack = g_utf8_strlen(haystack, -1);
	}

	/* Handle the cases of very short or NULL strings quickly */
	if (len_needle == 0) {
		return DROP_PENALTY * len_haystack;
	}

	if (len_haystack == 0) {
		return ADD_PENALTY * len_needle;
	}

	/* Allocate the matrix of penalties */
	guint * penalty_matrix = g_malloc0(sizeof(guint) * (len_needle + 1) * (len_haystack + 1));
	int i;

	/* Take the first row and first column and make them additional letter penalties */
	for (i = 0; i < len_needle + 1; i++) {
		MATRIX_VAL(i - 1, -1) = i * ADD_PENALTY;
	}

	for (i = 0; i < len_haystack + 1; i++) {
		if (i < len_haystack - len_needle) {
			MATRIX_VAL(-1, i - 1) = i * PRE_ADD_PENALTY;
		} else {
			MATRIX_VAL(-1, i - 1) = (len_haystack - len_needle) * PRE_ADD_PENALTY + (i - (len_haystack - len_needle)) * DROP_PENALTY;
		}
	}

	/* Now go through the matrix building up the penalties */
	int ineedle, ihaystack;
	for (ineedle = 0; ineedle < len_needle; ineedle++) {
		for (ihaystack = 0; ihaystack < len_haystack; ihaystack++) {
			char needle_let = needle[ineedle];
			char haystack_let = haystack[ihaystack];

			guint subst_pen = MATRIX_VAL(ineedle - 1, ihaystack - 1) + swap_cost(needle_let, haystack_let);
			guint drop_pen = MATRIX_VAL(ineedle - 1, ihaystack);
			if (ineedle < ihaystack) {
				drop_pen += DROP_PENALTY;
			} else {
				drop_pen += END_DROP_PENALTY;
			}

			guint add_pen = MATRIX_VAL(ineedle, ihaystack - 1);
			if (len_haystack - len_needle - ineedle > 0) {
				add_pen += PRE_ADD_PENALTY;
			} else {
				add_pen += ADD_PENALTY;
			}
			guint transpose_pen = drop_pen + 1; /* ensures won't be chosen */

			if (ineedle > 0 && ihaystack > 0 && needle_let == haystack[ihaystack - 1] && haystack_let == needle[ineedle - 1]) {
				transpose_pen = MATRIX_VAL(ineedle - 2, ihaystack - 2) + TRANSPOSE_PENALTY;
			}

			MATRIX_VAL(ineedle, ihaystack) = MIN(MIN(subst_pen, drop_pen), MIN(add_pen, transpose_pen));
		}
	}

	dumpmatrix(needle, len_needle, haystack, len_haystack, penalty_matrix);

	guint retval = penalty_matrix[(len_needle + 1) * (len_haystack + 1) - 1];
	g_free(penalty_matrix);

	return retval;
}

/* Looks through the array of paths and tries to find a minimum path
   looking up from the needle specified.  This way we can look through
   all the possible paths */
guint
minimize_distance_recurse (guint needle, guint num_needles, guint haystack, guint num_haystacks, guint * distances, guint * matches)
{
	gint i;

	/* Put where we are in the array so that we don't forget */
	matches[needle] = haystack;

	/* First check to see if we've already used this entry */
	for (i = needle - 1; i >= 0; i--) {
		if (matches[i] == haystack) {
			return G_MAXUINT;
		}
	}

	/* If we're the last needle, we can return our distance */
	if (needle + 1 >= num_needles) {
		return distances[(needle * num_haystacks) + haystack];
	}

	guint * local_match = g_new0(guint, num_needles);
	for (i = 0; i < num_needles && i < needle + 1; i++) {
		local_match[i] = matches[i];
	}

	/* Now look where we can get the minimum with the other needles */
	guint min = G_MAXUINT;
	for (i = 0; i < num_haystacks; i++) {
		guint local = minimize_distance_recurse(needle + 1, num_needles, i, num_haystacks, distances, local_match);

		if (local < min) {
			min = local;

			int j;
			for (j = needle + 1; j < num_needles; j++) {
				matches[j] = local_match[j];
			}
		}
	}

	g_free(local_match);

	/* Return the min of everyone else plus our distance */
	if (min < G_MAXUINT) {
		min += distances[(needle * num_haystacks) + haystack];
	}

	return min;
}

/* Figuring out the lowest path through the distance array
   where we don't use the same haystack tokens */
guint
minimize_distance (guint num_needles, guint num_haystacks, guint * distances, guint * matches)
{
	guint final_distance = G_MAXUINT;
	guint * local_matches = g_new0(guint, num_needles);

	guint haystack_token;
	for (haystack_token = 0; haystack_token < num_haystacks; haystack_token++) {
		guint distance = minimize_distance_recurse(0, num_needles, haystack_token, num_haystacks, distances, local_matches);

		if (distance < final_distance) {
			final_distance = distance;

			guint match_cnt;
			for (match_cnt = 0; match_cnt < num_needles; match_cnt++) {
				matches[match_cnt] = local_matches[match_cnt];
			}
		}
	}

	g_free(local_matches);

	return final_distance;
}

/* Dups a specific token in the array of strv arrays */
gchar *
find_token (guint token_number, GStrv * haystacks, guint num_haystacks)
{
	guint haystack;

	for (haystack = 0; haystack < num_haystacks; haystack++) {
		guint strvlen = g_strv_length(haystacks[haystack]);

		if (token_number < strvlen) {
			return g_strdup(haystacks[haystack][token_number]);
		}

		token_number -= strvlen;
	}

	return NULL;
}

#define SEPARATORS " .->"

guint
calculate_distance (const gchar * needle, GStrv haystacks, GStrv * matches)
{
	g_return_val_if_fail(needle != NULL || haystacks != NULL, G_MAXUINT);
	guint final_distance = G_MAXUINT;

	if (needle == NULL) {
		return DROP_PENALTY * g_utf8_strlen(haystacks[0], 1024);
	}
	if (haystacks == NULL) {
		return ADD_PENALTY * g_utf8_strlen(needle, 1024);
	}

	/* Tokenize all the haystack strings */
	gint i;
	guint num_haystacks = g_strv_length(haystacks);
	guint num_haystack_tokens = 0;
	GStrv * haystacks_array = g_new0(GStrv, num_haystacks);
	for (i = 0; i < num_haystacks; i++) {
		haystacks_array[i] = g_strsplit_set(haystacks[i], SEPARATORS, 0);
		num_haystack_tokens += g_strv_length(haystacks_array[i]);
	}

	/* Tokenize our needles the same way */
	GStrv needle_tokens = g_strsplit_set(needle, SEPARATORS, 0);
	guint num_needle_tokens = g_strv_length(needle_tokens);

	/* If we can't even find a set that works, let's just cut
	   our losses here */
	if (num_needle_tokens > num_haystack_tokens) {
		goto cleanup_tokens;
	}

	/* We need a place to store all the distances */
	guint * distances = g_new0(guint, num_haystack_tokens * num_needle_tokens);

	/* Calculate all the distance combinations */
	gint needle_token;
	for (needle_token = 0; needle_tokens[needle_token] != NULL; needle_token++) {
		gchar * ineedle = needle_tokens[needle_token];

		guint haystacks_cnt;
		guint haystack_token_cnt = 0;
		for (haystacks_cnt = 0; haystacks_cnt < num_haystacks; haystacks_cnt++) {
			guint haystack_cnt;
			for (haystack_cnt = 0; haystacks_array[haystacks_cnt][haystack_cnt] != NULL; haystack_cnt++) {
				gchar * ihaystack = haystacks_array[haystacks_cnt][haystack_cnt];
				guint distance = calculate_token_distance(ineedle, ihaystack);

				distances[(needle_token * num_haystack_tokens) + haystack_token_cnt] = distance;
				haystack_token_cnt++;
			}
		}
	}

	/* Now, try to find a path through the array that results in the
	   lowest total value */
	guint * final_matches = g_new0(guint, num_needle_tokens);

	final_distance = minimize_distance(num_needle_tokens, num_haystack_tokens, distances, final_matches);

	/* Set up an array for matches so that we can enter
	   the items as we go */
	if (matches != NULL) {
		GStrv match_array = NULL;
		match_array = g_new0(gchar *, num_needle_tokens + 1);
		match_array[num_needle_tokens] = NULL;

		/* Copy the strings that we care about */
		int i;
		for (i = 0; i < num_needle_tokens; i++) {
			match_array[i] = find_token(final_matches[i], haystacks_array, num_haystacks);
		}

		*matches = match_array;
	}

	g_free(final_matches);
	g_free(distances);

cleanup_tokens:
	g_strfreev(needle_tokens);
	for (i = 0; i < num_haystacks; i++) {
		g_strfreev(haystacks_array[i]);
	}
	g_free(haystacks_array);

	if (final_distance != G_MAXUINT) {
		return final_distance / num_needle_tokens;
	} else {
		return G_MAXUINT;
	}
}
