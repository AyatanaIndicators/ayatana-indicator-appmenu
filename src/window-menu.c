#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "window-menu.h"
#include "indicator-appmenu-marshal.h"

#define WINDOW_MENU_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_TYPE, WindowMenuPrivate))

/* Signals */

enum {
	ENTRY_ADDED,
	ENTRY_REMOVED,
	ERROR_STATE,
	STATUS_CHANGED,
	SHOW_MENU,
	A11Y_UPDATE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Prototypes */

static void window_menu_class_init (WindowMenuClass *klass);
static void window_menu_init       (WindowMenu *self);
static void window_menu_dispose    (GObject *object);
static void window_menu_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenu, window_menu, G_TYPE_OBJECT);

static void
window_menu_class_init (WindowMenuClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = window_menu_dispose;
	object_class->finalize = window_menu_finalize;

	/* Signals */
	signals[ENTRY_ADDED] =  g_signal_new(WINDOW_MENU_SIGNAL_ENTRY_ADDED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, entry_added),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__POINTER,
	                                      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ENTRY_REMOVED] =  g_signal_new(WINDOW_MENU_SIGNAL_ENTRY_REMOVED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, entry_removed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__POINTER,
	                                      G_TYPE_NONE, 1, G_TYPE_POINTER);
	signals[ERROR_STATE] =   g_signal_new(WINDOW_MENU_SIGNAL_ERROR_STATE,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, error_state),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__BOOLEAN,
	                                      G_TYPE_NONE, 1, G_TYPE_BOOLEAN, G_TYPE_NONE);
	signals[STATUS_CHANGED] = g_signal_new(WINDOW_MENU_SIGNAL_STATUS_CHANGED,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, status_changed),
	                                      NULL, NULL,
	                                      g_cclosure_marshal_VOID__INT,
	                                      G_TYPE_NONE, 1, G_TYPE_INT, G_TYPE_NONE);
	signals[SHOW_MENU] =     g_signal_new(WINDOW_MENU_SIGNAL_SHOW_MENU,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, show_menu),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__POINTER_UINT,
	                                      G_TYPE_NONE, 2, G_TYPE_POINTER, G_TYPE_UINT, G_TYPE_NONE);
	signals[A11Y_UPDATE] =   g_signal_new(WINDOW_MENU_SIGNAL_A11Y_UPDATE,
	                                      G_TYPE_FROM_CLASS(klass),
	                                      G_SIGNAL_RUN_LAST,
	                                      G_STRUCT_OFFSET (WindowMenuClass, a11y_update),
	                                      NULL, NULL,
	                                      _indicator_appmenu_marshal_VOID__POINTER,
	                                      G_TYPE_NONE, 1, G_TYPE_POINTER, G_TYPE_NONE);

	return;
}

static void
window_menu_init (WindowMenu *self)
{


	return;
}

static void
window_menu_dispose (GObject *object)
{

	G_OBJECT_CLASS (window_menu_parent_class)->dispose (object);
	return;
}

static void
window_menu_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menu_parent_class)->finalize (object);
	return;
}


/**************************
  API
 **************************/
GList *
window_menu_get_entries (WindowMenu * wm)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->get_entries != NULL) {
		return class->get_entries(wm);
	} else {
		return NULL;
	}
}

guint
window_menu_get_location (WindowMenu * wm, IndicatorObjectEntry * entry)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->get_location != NULL) {
		return class->get_location(wm, entry);
	} else {
		return 0;
	}
}

guint
window_menu_get_xid (WindowMenu * wm)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->get_xid != NULL) {
		return class->get_xid(wm);
	} else {
		return 0;
	}
}

gboolean
window_menu_get_error_state (WindowMenu * wm)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->get_error_state != NULL) {
		return class->get_error_state(wm);
	} else {
		return TRUE;
	}
}

WindowMenuStatus
window_menu_get_status (WindowMenu * wm)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->get_status != NULL) {
		return class->get_status(wm);
	} else {
		return WINDOW_MENU_STATUS_NORMAL;
	}
}

void
window_menu_entry_restore (WindowMenu * wm, IndicatorObjectEntry * entry)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->entry_restore != NULL) {
		return class->entry_restore(wm, entry);
	} else {
		return;
	}
}

void
window_menu_entry_activate (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp)
{
	WindowMenuClass * class = WINDOW_MENU_GET_CLASS(wm);

	if (class->entry_activate != NULL) {
		return class->entry_activate(wm, entry, timestamp);
	} else {
		return;
	}
}
