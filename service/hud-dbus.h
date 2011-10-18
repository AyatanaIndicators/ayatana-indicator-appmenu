#ifndef __HUD_DBUS_H__
#define __HUD_DBUS_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define HUD_DBUS_TYPE            (hud_dbus_get_type ())
#define HUD_DBUS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), HUD_DBUS_TYPE, HudDbus))
#define HUD_DBUS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), HUD_DBUS_TYPE, HudDbusClass))
#define IS_HUD_DBUS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), HUD_DBUS_TYPE))
#define IS_HUD_DBUS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), HUD_DBUS_TYPE))
#define HUD_DBUS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), HUD_DBUS_TYPE, HudDbusClass))

typedef struct _HudDbus      HudDbus;
typedef struct _HudDbusClass HudDbusClass;

struct _HudDbusClass {
	GObjectClass parent_class;
};

struct _HudDbus {
	GObject parent;
};

GType hud_dbus_get_type (void);
HudDbus * hud_dbus_new (void);

G_END_DECLS

#endif
