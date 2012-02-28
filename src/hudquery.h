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

#ifndef __HUD_QUERY_H__
#define __HUD_QUERY_H__

#include "hudsource.h"
#include "hudresult.h"

#define HUD_TYPE_QUERY                                      (hud_query_get_type ())
#define HUD_QUERY(inst)                                     (G_TYPE_CHECK_INSTANCE_CAST ((inst),                     \
                                                             HUD_TYPE_QUERY, HudQuery))
#define HUD_IS_QUERY(inst)                                  (G_TYPE_CHECK_INSTANCE_TYPE ((inst),                     \
                                                             HUD_TYPE_QUERY))

typedef struct _HudQuery                                    HudQuery;

GType                   hud_query_get_type                              (void);

HudQuery *              hud_query_new                                   (HudSource   *source,
                                                                         const gchar *search_string,
                                                                         gint         num_results);

guint64                 hud_query_get_generation                        (HudQuery    *query);

const gchar *           hud_query_get_target                            (HudQuery    *query);
GVariant *              hud_query_get_query_key                         (HudQuery    *query);

HudResult *             hud_query_get_result_by_key                     (HudQuery    *query,
                                                                         GVariant    *key);

HudResult *             hud_query_get_result_by_index                   (HudQuery    *query,
                                                                         guint        i);

guint                   hud_query_get_n_results                         (HudQuery    *query);

void                    hud_query_close                                 (HudQuery    *query);

HudQuery *              hud_query_lookup                                (GVariant    *query_key);

#endif /* __HUD_QUERY_H__ */
