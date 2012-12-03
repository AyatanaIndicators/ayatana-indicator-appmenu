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
  gunichar    *str;
  guint        length;

  const gchar *original;
  guint        original_length;
};

/* This is actually one greater than the max-length.
 * Should be power-of-two for best performance.
 */
#define TOKEN_LENGTH_LIMIT 32

#if 0
static void
hud_token_distance_debug (const HudToken *haystack,
                          const HudToken *needle,
                          guint           d[TOKEN_LENGTH_LIMIT][TOKEN_LENGTH_LIMIT])
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
  static guint d[TOKEN_LENGTH_LIMIT][TOKEN_LENGTH_LIMIT];
  gunichar h, n;
  gint result;
  gint i, j;

  g_assert (haystack->length < TOKEN_LENGTH_LIMIT && needle->length < TOKEN_LENGTH_LIMIT);
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
    result = MIN (d[i][j], result + hud_settings.end_drop_penalty);

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

  token->original = str;
  token->original_length = length;
  normal = g_utf8_normalize (str, length, G_NORMALIZE_ALL);
  folded = g_utf8_casefold (normal, -1);
  token->str = g_utf8_to_ucs4_fast (folded, -1, &items);
  token->length = items;
  g_free (folded);
  g_free (normal);

  if (!(token->length < TOKEN_LENGTH_LIMIT))
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

const gchar *
hud_token_get_original (const HudToken *token,
                        guint          *length)
{
  if (length)
    *length = token->original_length;

  return token->original;
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
hud_token_list_add_string_to_array (GPtrArray   *array,
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

static void
hud_token_list_add_string_list_to_array (GPtrArray     *array,
                                         HudStringList *list)
{
  if (list == NULL)
    return;

  hud_token_list_add_string_list_to_array (array, hud_string_list_get_tail (list));
  hud_token_list_add_string_to_array (array, hud_string_list_get_head (list));
}

HudTokenList *
hud_token_list_new_from_string (const gchar *string)
{
  GPtrArray *array;

  array = g_ptr_array_new ();
  hud_token_list_add_string_to_array (array, string);
  return hud_token_list_new_consume_array (array);
}

HudTokenList *
hud_token_list_new_from_string_list (HudStringList *string_list)
{
  GPtrArray *array;

  array = g_ptr_array_new ();
  hud_token_list_add_string_list_to_array (array, string_list);
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
hud_token_list_distance (HudTokenList     *haystack,
                         HudTokenList     *needle,
                         const HudToken ***matches)
{
  static guint d[32][32];
  gint i, j;

  if (needle->length > haystack->length)
    return G_MAXUINT;

  /* Simply refuse to deal with ridiculous situations.
   *
   * This only happens when the user has more than 32 words in their
   * search or the same happens in a menu item.
   */
  if (haystack->length > 32 || needle->length > 32)
    return G_MAXUINT;

  /* unroll the handling of the first needle term */
  {
    guint cost;

    /* unroll the handling of the first haystack term */
    cost = hud_token_distance (haystack->tokens[0], needle->tokens[0]);
    d[0][0] = cost;

    for (j = 0; j < haystack->length; j++)
      {
        guint take_cost;

        take_cost = hud_token_distance (haystack->tokens[j], needle->tokens[0]);
        cost = MIN (take_cost, cost + 1);
        d[0][j] = cost;
      }
  }

  /* if we have only one needle, this loop won't run at all */
  for (i = 1; i < needle->length; i++)
    {
      guint cost;

      /* unroll the handling of the first haystack term */
      cost = d[i - 1][i - 1] + hud_token_distance (haystack->tokens[i], needle->tokens[i]);
      d[i][i] = cost;

      for (j = i + 1; j < haystack->length; j++)
        {
          guint prev_cost;

          prev_cost = d[i - 1][j - 1];
          /* Only do checking of additional terms of it's possible that
           * we'll come in under the max distance AND beat the cost of
           * just dropping the term.
           *
           * hud_token_distance() could return zero so we need to try it
           * if:
           *
           *   - prev_cost is less than or equal to the max distance
           *     (because then our result could equal the max distance)
           *
           *   - prev_cost is less than or equal to the last cost
           *     Even if it's equal, skipping has a cost and a perfect
           *     match has no cost, so we need to try the match.
           */
          if (prev_cost <= hud_settings.max_distance && prev_cost <= cost)
            {
              guint take_cost;

              take_cost = prev_cost + hud_token_distance (haystack->tokens[j], needle->tokens[i]);
              cost = MIN (take_cost, cost + 1);
            }
          else
            cost = cost + 1;

          d[i][j] = cost;
        }
    }

  /* Discover which terms were matched */
  if (matches)
    {
      *matches = g_new (const HudToken *, needle->length + 1);

      j = haystack->length - 1;

      for (i = needle->length - 1; i >= 0; i--)
        {
          while (j > i && d[i][j-1] == d[i][j] - 1)
            j--;

          (*matches)[i] = haystack->tokens[j];
          j--;
        }

      (*matches)[needle->length] = NULL;
    }

  return d[needle->length - 1][haystack->length - 1];
}

guint
hud_token_list_get_length (HudTokenList *list)
{
  return list->length;
}
