#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "indicator-tracker.h"

#include <glib/gi18n.h>
#include <gio/gio.h>

typedef struct _SystemIndicator SystemIndicator;
struct _SystemIndicator {
	gchar * dbus_name;
	gchar * dbus_menu_path;
	gchar * indicator_name;
	gchar * user_visible_name;
};

SystemIndicator system_indicators[] = {
	{dbus_name: "com.canonical.indicator.messages", dbus_menu_path: "/com/canonical/indicator/messages/menu", indicator_name: "indicator-messages", user_visible_name: N_("Messages") }
};

struct _IndicatorTrackerPrivate {
	GArray * indicators;
	guint watches[G_N_ELEMENTS(system_indicators)];
};

#define INDICATOR_TRACKER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), INDICATOR_TRACKER_TYPE, IndicatorTrackerPrivate))

static void indicator_tracker_class_init (IndicatorTrackerClass *klass);
static void indicator_tracker_init       (IndicatorTracker *self);
static void indicator_tracker_dispose    (GObject *object);
static void indicator_tracker_finalize   (GObject *object);
static void system_watch_appeared        (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data);
static void system_watch_vanished        (GDBusConnection * connection, const gchar * name, gpointer user_data);

G_DEFINE_TYPE (IndicatorTracker, indicator_tracker, G_TYPE_OBJECT);

static void
indicator_tracker_class_init (IndicatorTrackerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (IndicatorTrackerPrivate));

	object_class->dispose = indicator_tracker_dispose;
	object_class->finalize = indicator_tracker_finalize;

	return;
}

static void
indicator_tracker_init (IndicatorTracker *self)
{
	self->priv = INDICATOR_TRACKER_GET_PRIVATE(self);
	self->priv->indicators = NULL;

	self->priv->indicators = g_array_new(FALSE, TRUE, sizeof(IndicatorTrackerIndicator));

	int indicator_cnt;
	for (indicator_cnt = 0; indicator_cnt < G_N_ELEMENTS(system_indicators); indicator_cnt++) {
		self->priv->watches[indicator_cnt] = g_bus_watch_name(G_BUS_TYPE_SESSION,
		                                                      system_indicators[indicator_cnt].dbus_name,
		                                                      G_BUS_NAME_WATCHER_FLAGS_NONE,
		                                                      system_watch_appeared, /* acquired */
		                                                      system_watch_vanished, /* vanished */
		                                                      self,
		                                                      NULL); /* free func */
	}

	return;
}

static void
indicator_tracker_dispose (GObject *object)
{
	IndicatorTracker * self = INDICATOR_TRACKER(object);

	int indicator_cnt;
	for (indicator_cnt = 0; indicator_cnt < G_N_ELEMENTS(system_indicators); indicator_cnt++) {
		if (self->priv->watches[indicator_cnt] != 0) {
			g_bus_unwatch_name(self->priv->watches[indicator_cnt]);
			self->priv->watches[indicator_cnt] = 0;
		}
	}
	

	G_OBJECT_CLASS (indicator_tracker_parent_class)->dispose (object);
	return;
}

static void
indicator_tracker_finalize (GObject *object)
{

	G_OBJECT_CLASS (indicator_tracker_parent_class)->finalize (object);
	return;
}

IndicatorTracker *
indicator_tracker_new (void)
{
	return INDICATOR_TRACKER(g_object_new(INDICATOR_TRACKER_TYPE, NULL));
}

GArray *
indicator_tracker_get_indicators (IndicatorTracker * tracker)
{
	g_return_val_if_fail(IS_INDICATOR_TRACKER(tracker), NULL);
	return tracker->priv->indicators;
}

static void
system_watch_appeared (GDBusConnection * connection, const gchar * name, const gchar * name_owner, gpointer user_data)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));

	return;
}

static void
system_watch_vanished (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_return_if_fail(IS_INDICATOR_TRACKER(user_data));


	return;
}
