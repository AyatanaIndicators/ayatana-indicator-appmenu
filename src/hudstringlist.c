/**
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
 **/

#include "hudstringlist.h"

#include <string.h>

struct _HudStringList
{
  HudStringList *tail;
  gint ref_count;

  /* variable length.  must come last! */
  gchar head[1];
};

void
hud_string_list_unref (HudStringList *list)
{
  if (list && g_atomic_int_dec_and_test (&list->ref_count))
    {
      hud_string_list_unref (list->tail);
      g_free (list);
    }
}

HudStringList *
hud_string_list_ref (HudStringList *list)
{
  if (list)
    g_atomic_int_inc (&list->ref_count);

  return list;
}

HudStringList *
hud_string_list_cons (const gchar   *head,
                      HudStringList *tail)
{
  HudStringList *list;
  gsize headlen;

  headlen = strlen (head);

  list = g_malloc (G_STRUCT_OFFSET (HudStringList, head) + headlen + 1);
  list->tail = hud_string_list_ref (tail);
  strcpy (list->head, head);
  list->ref_count = 1;

  return list;
}

const gchar *
hud_string_list_get_head (HudStringList *list)
{
  return list->head;
}

HudStringList *
hud_string_list_get_tail (HudStringList *list)
{
  return list->tail;
}

gchar *
hud_string_list_pretty_print (HudStringList *list)
{
  GString *string;

  string = g_string_new (NULL);
  while (list)
    {
      g_string_prepend (string, list->head);

      list = list->tail;

      if (list)
        g_string_prepend (string, " > ");
    }

  return g_string_free (string, FALSE);
}
