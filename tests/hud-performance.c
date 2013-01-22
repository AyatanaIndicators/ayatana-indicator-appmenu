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

#include "hudsettings.h"
#include "hudquery.h"
#include "hudtoken.h"
#include "hudsource.h"

#include <glib-object.h>

#include "word-list.h"

/* Max nested depth of menu items */
#define MAX_DEPTH 6

/* Max number of items per submenu */
#define MAX_ITEMS 20

/* Max number of words per label.
 * NB: keep MAX_WORDS * MAX_DEPTH under 32
 */
#define MAX_WORDS 4

/* Longest word in the word-list (upper bound) */
#define MAX_LETTERS 20

/* hardcode some parameters for reasons of determinism.
 */
HudSettings hud_settings = {
  .indicator_penalty = 50,
  .add_penalty = 10,
  .drop_penalty = 10,
  .end_drop_penalty = 1,
  .swap_penalty = 15,
  .max_distance = 30
};

typedef struct
{
  GObject object;

  GHashTable *items;
} RandomSource;

typedef GObjectClass RandomSourceClass;

static void random_source_iface_init (HudSourceInterface *iface);
G_DEFINE_TYPE_WITH_CODE (RandomSource, random_source, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (HUD_TYPE_SOURCE, random_source_iface_init))

static void
random_source_search (HudSource    *hud_source,
                      GPtrArray    *results_array,
                      HudTokenList *search_tokens)
{
  RandomSource *source = (RandomSource *) hud_source;
  GHashTableIter iter;
  gpointer item;

  g_hash_table_iter_init (&iter, source->items);
  while (g_hash_table_iter_next (&iter, &item, NULL))
    {
      HudResult *result;

      result = hud_result_get_if_matched (item, search_tokens, 0);
      if (result)
        g_ptr_array_add (results_array, result);
    }
}

static void
random_source_ignore_use (HudSource *source)
{
}

static gchar *
make_word (GRand *rand,
           gchar *buffer)
{
  const gchar *word;
  gint choice;
  gint len;

  choice = g_rand_int_range (rand, 0, G_N_ELEMENTS (word_list));
  word = word_list[choice];

  while (*word)
    *buffer++ = *word++;

  return buffer;
}

static gchar *
make_words (GRand *rand,
            gint   n_words)
{
  gchar *buffer;
  gchar *ptr;
  gint i;

  buffer = g_malloc ((MAX_LETTERS + 1) * n_words);

  ptr = buffer;
  for (i = 0; i < n_words; i++)
    {
      if (i)
        *ptr++ = ' ';

      ptr = make_word (rand, ptr);
    }

  *ptr = '\0';

  return buffer;
}

static HudStringList *
random_source_make_name (GRand         *rand,
                         HudStringList *context)
{
  HudStringList *name;
  gchar *label;

  label = make_words (rand, g_rand_int_range (rand, 1, MAX_WORDS + 1));
  name = hud_string_list_cons (label, context);
  g_free (label);

  return name;
}

static void
random_source_populate_table (GRand         *rand,
                              GHashTable    *items,
                              HudStringList *context,
                              gint           depth)
{
  gint n_items;
  gint i;

  n_items = g_rand_int_range (rand, 1, MAX_ITEMS + 1);

  for (i = 0; i < n_items; i++)
    {
      HudStringList *name;
      gboolean is_submenu;
      HudItem *item;

      name = random_source_make_name (rand, context);

      if (depth != MAX_DEPTH)
        /* Decrease the chances of a particular item being a submenu as we
         * go deeper into the menu structure.
         */
        is_submenu = g_rand_int_range (rand, 0, depth + 1) == 0;
      else
        /* At the maximum depth, prevent any items from being submenus. */
        is_submenu = FALSE;

      item = hud_item_new (name, NULL, NULL, !is_submenu);
      g_hash_table_add (items, item);

      if (is_submenu)
        random_source_populate_table (rand, items, name, depth + 1);

      hud_string_list_unref (name);
    }
}

static void
random_source_finalize (GObject *object)
{
  RandomSource *source = (RandomSource *) object;

  g_hash_table_unref (source->items);

  G_OBJECT_CLASS (random_source_parent_class)
    ->finalize (object);
}

static void
random_source_init (RandomSource *source)
{
  source->items = g_hash_table_new_full (NULL, NULL, g_object_unref, NULL);
}

static void
random_source_iface_init (HudSourceInterface *iface)
{
  iface->use = random_source_ignore_use;
  iface->unuse = random_source_ignore_use;
  iface->search = random_source_search;
}

static void
random_source_class_init (RandomSourceClass *class)
{
  class->finalize = random_source_finalize;
}

static HudSource *
random_source_new (GRand *rand)
{
  RandomSource *source;

  source = g_object_new (random_source_get_type (), NULL);
  random_source_populate_table (rand, source->items, NULL, 0);

  return HUD_SOURCE (source);
}

void
test_query_performance (void)
{
  HudSource *source;
  HudQuery *query;
  GRand *rand;
  gint i;

  rand = g_rand_new_with_seed (1234);
  source = random_source_new (rand);

  for (i = 1; i <= 6; i++)
    {
      guint64 start_time;
      gchar *search;
      gint j;

      g_print ("\n");

      search = make_words (rand, i);

      /* simulate the user typing it in, one character at a time */
      for (j = 1; search[j - 1]; j++)
        {
          gchar *part_search = g_strndup (search, j);

          start_time = g_get_monotonic_time ();
          query = hud_query_new (source, part_search, 1u<<30);
          g_print ("%-60s: %dus (%d hits)\n", part_search,
                   (int) (g_get_monotonic_time () - start_time),
                   hud_query_get_n_results (query));
          hud_query_close (query);
          g_object_unref (query);
          g_free (part_search);
        }

      g_free (search);
    }

  g_object_unref (source);
  g_rand_free (rand);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  if (g_test_perf ())
    g_test_add_func ("/hud/query-performance", test_query_performance);

  return g_test_run ();
}
