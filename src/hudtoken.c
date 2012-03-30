/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ryan Lortie <desrt@desrt.ca>
 */

#include "hudtoken.h"

#include "hudsettings.h"

#include <string.h>

struct _HudToken
{
  guint     length;
  gunichar *str;
};

#if 0
static void
hud_token_distance_debug (const HudToken *haystack,
                          const HudToken *needle,
                          guint           d[32][32])
{
  gint i, j;

  g_print (" ");
  for (j = 0; j <= haystack->length; j++)
    g_print ("%6lc", (wchar_t) (j ? haystack->str[j - 1] : ' '));
  g_print ("\n");

  for (i = 0; i <= needle->length; i++)
    {
      g_print ("%lc", (wchar_t) (i ? needle->str[i - 1] : ' '));
      for (j = 0; j <= haystack->length; j++)
        g_print ("%6u", d[i][j]);
      g_print ("\n");
    }
}
#endif

#define MIN3(a,b,c) (MIN(MIN((a),(b)),(c)))

/* Keeping in mind the fact that we're matching single words against
 * single words, we can expect the extremely vast majority of cases to
 * see needle and haystack both be less than 32 characters in length.
 *
 * For the common case we can avoid memory allocation by using a static
 * array.  By making the array a constant factor-of-two size we can
 * replace multiplication by a variable with bitshift by a constant.
 *
 * Tokens longer than this are expected to be so unlikely that we
 * simply deal with them by truncation during the normalisation phase.
 */
guint
hud_token_distance (const HudToken *haystack,
                    const HudToken *needle)
{
  static guint d[32][32];
  gunichar h, n;
  gint result;
  gint i, j;

  g_assert (haystack->length < 32 && needle->length < 32);
  g_assert (haystack->str[haystack->length] == 0);
  g_assert (needle->str[needle->length] == 0);

  /* This function only performs memory writes to 'd' and calls no other
   * functions.  No pointer to 'd' is ever leaked.  Hopefully the
   * compiler is clever enough to therefore realise that no other memory
   * will be modified during the running of this function and optimise
   * aggressively.
   */

  /* By convention we refer to "add" and "drop" in terms of "mistakes
   * the user made".  The user's token is the one in "needle".  To give
   * examples against the menu item "preferences", if the user typed:
   *
   *  - "prefereces": this has a dropped character
   *
   *  - "prefferences": this has an added character
   *
   *  - "prefefences": this has a swap
   *
   * We organise the matrix with each column (j) corresponding to a
   * character in the menu item text ('haystack') and each row (i)
   * corresponding to a character in the search token ('needle').
   *
   * We modify the Levenshtein algorithm in the following ways:
   *
   *  - configurable penalties for various mistakes (add, drop,
   *    swap).  This is done by replacing additions of '1' with
   *    additions of these configurable values.
   *
   *  - a lower penalties for drops that occur at the end of the token
   *    that the user typed.  For example, "prefer" would be given a
   *    lower penalty for those 5 missing letters than would occur if
   *    they were not missing from the end.
   *
   *    This is done by special-casing the last row (i == nlen) in the
   *    matrix, corresponding to the last character in the search
   *    token.  In this case, we calculate drops at a lower penalty.
   *
   * Implementing the first of these two changes is a simple tweak:
   * instead of adding '1', add the configurable value.
   *
   * Implementing the second one is somewhat more difficult: we could
   * modify the core algorithm, but that would violate the 'pureness' of
   * the dynamic programming approach.
   *
   * Instead, we wait until the 'pure' algorithm is done then instead of
   * only considering the result for the full menu item text (ie: the
   * number in the bottom right corner) we consider all prefixes of the
   * menu item text by scanning the entire bottom row (and adding a
   * penalty according to the number of characters removed).
   */

  /* http://en.wikipedia.org/wiki/Levenshtein_distance#Computing_Levenshtein_distance */
  for (i = 0; i <= needle->length; i++)
    d[i][0] = i * hud_settings.add_penalty;

  for (j = 0; j <= haystack->length; j++)
    d[0][j] = j * hud_settings.drop_penalty;

  for (i = 1; (n = needle->str[i - 1]); i++)
    for (j = 1; (h = haystack->str[j - 1]); j++)
      if (n == h)
        d[i][j] = d[i - 1][j - 1];

      else
        d[i][j] = MIN3(d[i - 1][j - 1] + hud_settings.swap_penalty,
                       d[i    ][j - 1] + hud_settings.drop_penalty,
                       d[i - 1][j    ] + hud_settings.add_penalty);

  /* Noe we consider all prefixes of the menu item text to discover
   * which one gives us the best score.
   *
   * If we end up picking a result from a column other than the
   * rightmost, it will have had the correct multiple of the end-drop
   * penalty added to it by the time we're done.
   */
  result = d[--i][0];
  for (j = 1; j <= haystack->length; j++)
    result = MIN (d[i][j], result + hud_settings.drop_penalty_end);

  return result;
}

HudToken *
hud_token_new (const gchar *str,
               gssize       length)
{
  HudToken *token;
  gchar *normal;
  gchar *folded;
  glong items;

  token = g_slice_new (HudToken);

  normal = g_utf8_normalize (str, length, G_NORMALIZE_ALL);
  folded = g_utf8_casefold (normal, -1);
  token->str = g_utf8_to_ucs4_fast (folded, -1, &items);
  token->length = items;
  g_free (folded);
  g_free (normal);

  if (!(token->length < 32))
    {
      token->length = 31;
      token->str[31] = 0;
    }

  return token;
}

void
hud_token_free (HudToken *token)
{
  g_free (token->str);
  g_slice_free (HudToken, token);
}

struct _HudTokenList
{
  HudToken **tokens;
  gint       length;
};

static HudTokenList *
hud_token_list_new_consume_array (GPtrArray *array)
{
  HudTokenList *list;

  list = g_slice_new (HudTokenList);
  list->tokens = (HudToken **) array->pdata;
  list->length = array->len;

  g_ptr_array_free (array, FALSE);

  return list;
}

#define SEPARATORS " .->"
static void
hud_token_list_add_to_array (GPtrArray   *array,
                             const gchar *string)
{
  while (*string)
    {
      /* strip separators */
      string += strspn (string, SEPARATORS);

      if (*string)
        {
          gint length;

          /* consume a token */
          length = strcspn (string, SEPARATORS);
          g_ptr_array_add (array, hud_token_new (string, length));
          string += length;
        }
    }
}

HudTokenList *
hud_token_list_new_from_string (const gchar *string)
{
  GPtrArray *array;

  array = g_ptr_array_new ();
  hud_token_list_add_to_array (array, string);
  return hud_token_list_new_consume_array (array);
}

HudTokenList *
hud_token_list_new_from_string_list (HudStringList *string_list)
{
  GPtrArray *array;

  array = g_ptr_array_new ();
  while (string_list)
    {
      hud_token_list_add_to_array (array, hud_string_list_get_head (string_list));
      string_list = hud_string_list_get_tail (string_list);
    }
  return hud_token_list_new_consume_array (array);
}

void
hud_token_list_free (HudTokenList *list)
{
  gint i;

  for (i = 0; i < list->length; i++)
    hud_token_free (list->tokens[i]);

  g_free (list->tokens);

  g_slice_free (HudTokenList, list);
}

guint
hud_token_list_distance_slow (HudTokenList *haystack,
                              HudTokenList *needle)
{
  g_assert_not_reached ();
}

static gboolean
item_is_in_array (guint item,
                  guint *array,
                  guint array_len)
{
  gint i;

  for (i = 0; i < array_len; i++)
    if (array[i] == item)
      return TRUE;

  return FALSE;
}

static guint
hud_token_list_select_matches (guint d[32][32],
                               guint matches[32],
                               gint  iteration,
                               gint  num_needles,
                               gint  num_haystacks)
{
  guint best = G_MAXINT;
  gint i;

  /* Base case: no more needles -> no more distance */
  if (iteration == num_needles)
    return 0;

  /* Try all of the possible haystacks as a potential best-match */
  for (i = 0; i < num_haystacks; i++)
    {
      guint distance;

      /* In case of multiple search terms, check if we already picked
       * this haystack (to avoid using the same haystack for both search
       * terms).
       */
      if (item_is_in_array (i, matches, iteration))
        continue;

      /* Store our guess in the array so that the recursive call can see
       * that we've already picked this haystack for this iteration
       */
      matches[iteration] = i;
      distance = d[iteration][i];
      distance += hud_token_list_select_matches (d, matches, iteration + 1, num_needles, num_haystacks);
      best = MIN (best, distance);
    }

  return best;
}

guint
hud_token_list_distance (HudTokenList *haystack,
                         HudTokenList *needle)
{
  static guint d[32][32];
  guint matches[32];
  gint i, j;

  if (needle->length > haystack->length)
    return G_MAXUINT;

  if G_UNLIKELY (haystack->length > 32 || needle->length > 32)
    return hud_token_list_distance_slow (haystack, needle);

  for (i = 0; i < needle->length; i++)
    for (j = 0; j < haystack->length; j++)
      d[i][j] = hud_token_distance (haystack->tokens[j], needle->tokens[i]);

  return hud_token_list_select_matches (d, matches, 0, needle->length, haystack->length);
}
