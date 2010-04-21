
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

GType indicator_appmenu_get_type (void);

INDICATOR_SET_VERSION
INDICATOR_SET_TYPE(INDICATOR_APPMENU_TYPE)

typedef struct _IndicatorAppmenu      IndicatorAppmenu;
typedef struct _IndicatorAppmenuClass IndicatorAppmenuClass;

struct _IndicatorAppmenuClass {
	IndicatorObjectClass parent_class;

};

struct _IndicatorAppmenu {
	IndicatorObject parent;

	WindowMenus * default_app;
	GHashTable * apps;
};

static void indicator_appmenu_class_init (IndicatorAppmenuClass *klass);
static void indicator_appmenu_init       (IndicatorAppmenu *self);
static void indicator_appmenu_dispose    (GObject *object);
static void indicator_appmenu_finalize   (GObject *object);
static GList * get_entries (IndicatorObject * io);
static void switch_default_app (IndicatorAppmenu * iapp, WindowMenus * newdef);

G_DEFINE_TYPE (IndicatorAppmenu, indicator_appmenu, INDICATOR_OBJECT_TYPE);

/* One time init */
static void
indicator_appmenu_class_init (IndicatorAppmenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = indicator_appmenu_dispose;
	object_class->finalize = indicator_appmenu_finalize;

	IndicatorObjectClass * ioclass = INDICATOR_OBJECT_CLASS(klass);

	ioclass->get_entries = get_entries;

	return;
}

/* Per instance Init */
static void
indicator_appmenu_init (IndicatorAppmenu *self)
{
	self->default_app = NULL;
	self->apps = g_hash_table_new_full(NULL, NULL, NULL, g_object_unref);

	return;
}

/* Object refs decrement */
static void
indicator_appmenu_dispose (GObject *object)
{
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(object);

	/* No specific ref */
	switch_default_app (iapp, NULL);

	if (iapp->apps != NULL) {
		g_hash_table_destroy(iapp->apps);
		iapp->apps = NULL;
	}

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

/* Get the current set of entries */
static GList *
get_entries (IndicatorObject * io)
{
	g_return_val_if_fail(IS_INDICATOR_APPMENU(io), NULL);
	IndicatorAppmenu * iapp = INDICATOR_APPMENU(io);

	if (iapp->default_app == NULL) {
		return NULL;
	}

	return window_menus_get_entries(iapp->default_app);
}

/* Switch applications, remove all the entires for the previous
   one and add them for the new application */
static void
switch_default_app (IndicatorAppmenu * iapp, WindowMenus * newdef)
{
	if (iapp->default_app == newdef) {
		return;
	}

	GList * entries;

	/* Remove old */
	if (iapp->default_app != NULL) {
		for (entries = window_menus_get_entries(iapp->default_app); entries != NULL; entries = g_list_next(entries)) {
			g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_REMOVED_ID, 0, entries->data, TRUE);
		}
	}

	/* Switch */
	iapp->default_app = newdef;

	/* Add new */
	if (iapp->default_app != NULL) {
		for (entries = window_menus_get_entries(iapp->default_app); entries != NULL; entries = g_list_next(entries)) {
			g_signal_emit(G_OBJECT(iapp), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED_ID, 0, entries->data, TRUE);
		}
	}

	return;
}
