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

struct _HudSearchClass {
	GObjectClass parent_class;
};

struct _HudSearch {
	GObject parent;
	HudSearchPrivate * priv;
};

GType hud_search_get_type (void);
HudSearch * hud_search_new (void);

G_END_DECLS

#endif
