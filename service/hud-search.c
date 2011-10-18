#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libbamf/bamf-matcher.h>
#include <gio/gio.h>

#include "hud-search.h"
#include "dbusmenu-collector.h"

struct _HudSearchPrivate {
	BamfMatcher * matcher;
	gulong window_changed_sig;

	guint32 active_xid;

	DbusmenuCollector * collector;

	GDBusProxy * appmenu;
};

#define HUD_SEARCH_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), HUD_SEARCH_TYPE, HudSearchPrivate))

static void hud_search_class_init (HudSearchClass *klass);
static void hud_search_init       (HudSearch *self);
static void hud_search_dispose    (GObject *object);
static void hud_search_finalize   (GObject *object);

static void active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data);


G_DEFINE_TYPE (HudSearch, hud_search, G_TYPE_OBJECT);

static void
hud_search_class_init (HudSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (HudSearchPrivate));

	object_class->dispose = hud_search_dispose;
	object_class->finalize = hud_search_finalize;

	return;
}

static void
hud_search_init (HudSearch *self)
{
	/* Get Private */
	self->priv = HUD_SEARCH_GET_PRIVATE(self);

	/* Initialize Private */
	self->priv->matcher = NULL;
	self->priv->window_changed_sig = 0;
	self->priv->active_xid = 0;
	self->priv->collector = NULL;
	self->priv->appmenu = NULL;

	/* BAMF */
	self->priv->matcher = bamf_matcher_get_default();
	self->priv->window_changed_sig = g_signal_connect(G_OBJECT(self->priv->matcher), "active-window-changed", G_CALLBACK(active_window_changed), self);

	BamfWindow * active_window = bamf_matcher_get_active_window(self->priv->matcher);
	if (active_window != NULL) {
		self->priv->active_xid = bamf_window_get_xid(active_window);
	}

	/* DBusMenu */
	self->priv->collector = dbusmenu_collector_new();

	/* Appmenu */
	self->priv->appmenu = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
	                                                    G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                                                    /* info */ NULL,
	                                                    "org.canonical.AppMenu.Registrar",
	                                                    "/org/canonical/AppMenu/Registrar",
	                                                    "org.canonical.AppMenu.Registrar",
	                                                    NULL, NULL);

	return;
}

static void
hud_search_dispose (GObject *object)
{
	HudSearch * self = HUD_SEARCH(object);

	if (self->priv->window_changed_sig != 0) {
		g_signal_handler_disconnect(self->priv->matcher, self->priv->window_changed_sig);
		self->priv->window_changed_sig = 0;
	}

	if (self->priv->matcher != NULL) {
		g_object_unref(self->priv->matcher);
		self->priv->matcher = NULL;
	}

	if (self->priv->appmenu != NULL) {
		g_object_unref(self->priv->appmenu);
		self->priv->appmenu = NULL;
	}

	G_OBJECT_CLASS (hud_search_parent_class)->dispose (object);
	return;
}

static void
hud_search_finalize (GObject *object)
{

	G_OBJECT_CLASS (hud_search_parent_class)->finalize (object);
	return;
}

HudSearch *
hud_search_new (void)
{
	HudSearch * ret = HUD_SEARCH(g_object_new(HUD_SEARCH_TYPE, NULL));
	return ret;
}

GStrv
hud_search_suggestions (HudSearch * search, const GStrv tokens)
{
	g_return_val_if_fail(IS_HUD_SEARCH(search), NULL);

	if (search->priv->active_xid == 0) {
		g_warning("Active application is unknown");
		return NULL;
	}

	if (search->priv->appmenu == NULL) {
		g_warning("Unable to proxy appmenu");
		return NULL;
	}

	GError * error = NULL;
	GVariant * dbusinfo = g_dbus_proxy_call_sync(search->priv->appmenu,
	                                             "GetMenuForWindow",
	                                             g_variant_new("(u)", search->priv->active_xid),
	                                             G_DBUS_CALL_FLAGS_NONE,
	                                             -1,
	                                             NULL,
	                                             &error);

	if (error != NULL) {
		g_warning("Unable to get menus from appmenu: %s", error->message);
		g_error_free(error);
		return NULL;
	}

	gchar * address = NULL;
	gchar * path = NULL;
	g_variant_get(dbusinfo, "(so)", &address, &path);

	GStrv retval = dbusmenu_collector_search(search->priv->collector, address, path, tokens);

	g_free(address);
	g_free(path);

	return retval;
}

void
hud_search_execute (HudSearch * search, const GStrv tokens)
{

	return;
}

static void
active_window_changed (BamfMatcher * matcher, BamfView * oldview, BamfView * newview, gpointer user_data)
{
	HudSearch * self = HUD_SEARCH(user_data);
	BamfWindow * window = BAMF_WINDOW(newview);
	if (window == NULL) return;

	BamfApplication * app = bamf_matcher_get_application_for_window(self->priv->matcher, window);
	const gchar * desktop = bamf_application_get_desktop_file(app);

	/* NOTE: Both of these are debugging tools for now, so we want
	   to be able to use them effectively to work on the HUD, so we're
	   going to pretend we didn't switch windows if we switch to one 
	   of them */
	if (g_strstr_len(desktop, -1, "termina") == NULL &&
	        g_strstr_len(desktop, -1, "dfeet") == NULL) {
		self->priv->active_xid = bamf_window_get_xid(window);
	}

	return;
}
