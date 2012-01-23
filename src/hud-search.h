/*
The HUD searching logic that brings together the DBus menus along
with the user data to make a good search.

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

#ifndef __HUD_SEARCH_H__
#define __HUD_SEARCH_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HUD_SEARCH_TYPE            (hud_search_get_type ())
#define HUD_SEARCH(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HUD_SEARCH_TYPE, HudSearch))
#define HUD_SEARCH_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HUD_SEARCH_TYPE, HudSearchClass))
#define IS_HUD_SEARCH(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HUD_SEARCH_TYPE))
#define IS_HUD_SEARCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HUD_SEARCH_TYPE))
#define HUD_SEARCH_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HUD_SEARCH_TYPE, HudSearchClass))

typedef struct _HudSearch         HudSearch;
typedef struct _HudSearchClass    HudSearchClass;
typedef struct _HudSearchPrivate  HudSearchPrivate;
typedef struct _HudSearchSuggest  HudSearchSuggest;

struct _HudSearchClass {
	GObjectClass parent_class;
};

struct _HudSearch {
	GObject parent;
	HudSearchPrivate * priv;
};

GType hud_search_get_type (void);
HudSearch * hud_search_new (void);
GList * hud_search_suggestions (HudSearch * search, const gchar * searchstr, gchar ** desktop, gchar ** target);
void hud_search_execute (HudSearch * search, GVariant * key, guint timestamp);

const gchar * hud_search_suggest_get_app_icon (HudSearchSuggest * suggest);
const gchar * hud_search_suggest_get_item_icon (HudSearchSuggest * suggest);
const gchar * hud_search_suggest_get_display (HudSearchSuggest * suggest);
GVariant * hud_search_suggest_get_key (HudSearchSuggest * suggest);
void hud_search_suggest_free (HudSearchSuggest * suggest);

G_END_DECLS

#endif
