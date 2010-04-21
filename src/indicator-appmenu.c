
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include "window-menus.h"

#define INDICATOR_APPMENU_TYPE            (indicator_appmenu_get_type ())
#define INDICATOR_APPMENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_APPMENU_TYPE, IndicatorAppmenu))
#define INDICATOR_APPMENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_APPMENU_TYPE, IndicatorAppmenuClass))
#define IS_INDICATOR_APPMENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_APPMENU_TYPE))
#define IS_INDICATOR_APPMENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_APPMENU_TYPE))
#define INDICATOR_APPMENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_APPMENU_TYPE, IndicatorAppmenuClass))

typedef struct _IndicatorAppmenu      IndicatorAppmenu;
typedef struct _IndicatorAppmenuClass IndicatorAppmenuClass;

struct _IndicatorAppmenuClass {
	IndicatorObjectClass parent_class;

};

struct _IndicatorAppmenu {
	IndicatorObject parent;

};

GType indicator_appmenu_get_type (void);
static void indicator_appmenu_class_init (IndicatorAppmenuClass *klass);
static void indicator_appmenu_init       (IndicatorAppmenu *self);
static void indicator_appmenu_dispose    (GObject *object);
static void indicator_appmenu_finalize   (GObject *object);

G_DEFINE_TYPE (IndicatorAppmenu, indicator_appmenu, INDICATOR_OBJECT_TYPE);

/* One time init */
static void
indicator_appmenu_class_init (IndicatorAppmenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_appmenu_dispose;
	object_class->finalize = indicator_appmenu_finalize;

	return;
}

/* Per instance Init */
static void
indicator_appmenu_init (IndicatorAppmenu *self)
{

	return;
}

/* Object refs decrement */
static void
indicator_appmenu_dispose (GObject *object)
{

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->dispose (object);
	return;
}

/* Free memory */
static void
indicator_appmenu_finalize (GObject *object)
{

	G_OBJECT_CLASS (indicator_appmenu_parent_class)->finalize (object);
	return;
}
