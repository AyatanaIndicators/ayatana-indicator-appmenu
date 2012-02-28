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

#ifndef __HUD_STRING_LIST_H__
#define __HUD_STRING_LIST_H__

#include <glib.h>

typedef struct _HudStringList                               HudStringList;

void                    hud_string_list_unref                           (HudStringList *list);
HudStringList *         hud_string_list_ref                             (HudStringList *list);

HudStringList *         hud_string_list_cons                            (const gchar   *head,
                                                                         HudStringList *tail);

const gchar *           hud_string_list_get_head                        (HudStringList *list);
HudStringList *         hud_string_list_get_tail                        (HudStringList *list);

gchar *                 hud_string_list_pretty_print                    (HudStringList *list);

#endif /* __HUD_STRING_LIST_H__ */
