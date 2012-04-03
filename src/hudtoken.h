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

#ifndef __HUD_TOKEN_H__
#define __HUD_TOKEN_H__

#include "hudstringlist.h"

typedef struct _HudTokenList HudTokenList;
typedef struct _HudToken HudToken;

HudToken *              hud_token_new                                   (const gchar      *token,
                                                                         gssize            length);
void                    hud_token_free                                  (HudToken         *token);
guint                   hud_token_distance                              (const HudToken   *haystack,
                                                                         const HudToken   *needle);

HudTokenList *          hud_token_list_new_from_string                  (const gchar      *string);
HudTokenList *          hud_token_list_new_from_string_list             (HudStringList    *string_list);
void                    hud_token_list_free                             (HudTokenList     *list);
guint                   hud_token_list_distance                         (HudTokenList     *haystack,
                                                                         HudTokenList     *needle,
                                                                         const gchar    ***matches);

#endif /* __HUD_TOKEN_H__ */
