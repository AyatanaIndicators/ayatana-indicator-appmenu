#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libbamf/libbamf.h>
#include <gio/gio.h>

#include "window-menu-model.h"
#include "gactionmuxer.h"

struct _WindowMenuModelPrivate {
	GActionMuxer * action_mux;
	GDBusMenuModel *app_menu;
	GDBusMenuModel *menubar;

	IndicatorObjectEntry application_menu;
	gboolean has_application_menu;
};

#define WINDOW_MENU_MODEL_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_MODEL_TYPE, WindowMenuModelPrivate))

static void window_menu_model_class_init (WindowMenuModelClass *klass);
static void window_menu_model_init       (WindowMenuModel *self);
static void window_menu_model_dispose    (GObject *object);
static void window_menu_model_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenuModel, window_menu_model, WINDOW_MENU_TYPE);

#define ACTION_MUX_PREFIX_WIN  "window"
#define ACTION_MUX_PREFIX_APP  "application"

static void
window_menu_model_class_init (WindowMenuModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenuModelPrivate));

	object_class->dispose = window_menu_model_dispose;
	object_class->finalize = window_menu_model_finalize;

	return;
}

static void
window_menu_model_init (WindowMenuModel *self)
{
	self->priv = WINDOW_MENU_MODEL_GET_PRIVATE(self);

	self->priv->action_mux = g_action_muxer_new();

	return;
}

static void
window_menu_model_dispose (GObject *object)
{
	WindowMenuModel * menu = WINDOW_MENU_MODEL(object);

	g_clear_object(&menu->priv->action_mux);
	g_clear_object(&menu->priv->app_menu);
	g_clear_object(&menu->priv->menubar);

	G_OBJECT_CLASS (window_menu_model_parent_class)->dispose (object);
	return;
}

static void
window_menu_model_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menu_model_parent_class)->finalize (object);
	return;
}

/* Adds the application menu and turns the whole thing into an object
   entry that can be used elsewhere */
static void
add_application_menu (WindowMenuModel * menu, gchar * appname, GMenuModel * model)
{



	return;
}

/* Adds the window menu and turns it into a set of IndicatorObjectEntries
   that can be used elsewhere */
static void
add_window_menu (WindowMenuModel * menu, GMenuModel * model)
{


	return;
}

/* Builds the menu model from the window for the application */
WindowMenuModel *
window_menu_model_new (BamfApplication * app, BamfWindow * window)
{
	WindowMenuModel * menu = g_object_new(WINDOW_MENU_MODEL_TYPE, NULL);

	gchar *unique_bus_name;
	gchar *app_menu_object_path;
	gchar *menubar_object_path;
	gchar *application_object_path;
	gchar *window_object_path;
	GDBusConnection *session;

	unique_bus_name = bamf_window_get_utf8_prop (window, "_GTK_UNIQUE_BUS_NAME");

	if (unique_bus_name == NULL) {
		/* If this isn't set, we won't get very far... */
		return NULL;
	}

	app_menu_object_path = bamf_window_get_utf8_prop (window, "_GTK_APP_MENU_OBJECT_PATH");
	menubar_object_path = bamf_window_get_utf8_prop (window, "_GTK_MENUBAR_OBJECT_PATH");
	application_object_path = bamf_window_get_utf8_prop (window, "_GTK_APPLICATION_OBJECT_PATH");
	window_object_path = bamf_window_get_utf8_prop (window, "_GTK_WINDOW_OBJECT_PATH");

	session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

	/* Setup actions */
	if (application_object_path != NULL) {
		g_action_muxer_insert(menu->priv->action_mux, ACTION_MUX_PREFIX_APP, G_ACTION_GROUP(g_dbus_action_group_get (session, unique_bus_name, application_object_path)));
	}

	if (window_object_path != NULL) {
		g_action_muxer_insert(menu->priv->action_mux, ACTION_MUX_PREFIX_WIN, G_ACTION_GROUP(g_dbus_action_group_get (session, unique_bus_name, window_object_path)));
	}

	/* Build us some menus */
	if (app_menu_object_path != NULL) {
		GMenuModel * model = G_MENU_MODEL(g_dbus_menu_model_get (session, unique_bus_name, app_menu_object_path));

		add_application_menu(menu, NULL, model);

		g_object_unref(model);
	}

	if (menubar_object_path != NULL) {
		GMenuModel * model = G_MENU_MODEL(g_dbus_menu_model_get (session, unique_bus_name, menubar_object_path));

		add_window_menu(menu, model);

		g_object_unref(model);
	}

	/* when the action groups change, we could end up having items
	 * enabled/disabled.  how to deal with that?
	 */

	g_free (unique_bus_name);
	g_free (app_menu_object_path);
	g_free (menubar_object_path);
	g_free (application_object_path);
	g_free (window_object_path);

	g_object_unref (session);

	return menu;
}
