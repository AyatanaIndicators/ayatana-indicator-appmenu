#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libbamf/libbamf.h>
#include <gio/gio.h>

#include "window-menu-model.h"

struct _WindowMenuModelPrivate {
	GDBusMenuModel *app_menu;
	GDBusMenuModel *menubar;
	GDBusActionGroup *application;
	GDBusActionGroup *window;
};

#define WINDOW_MENU_MODEL_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_MODEL_TYPE, WindowMenuModelPrivate))

static void window_menu_model_class_init (WindowMenuModelClass *klass);
static void window_menu_model_init       (WindowMenuModel *self);
static void window_menu_model_dispose    (GObject *object);
static void window_menu_model_finalize   (GObject *object);

G_DEFINE_TYPE (WindowMenuModel, window_menu_model, WINDOW_MENU_TYPE);

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

	return;
}

static void
window_menu_model_dispose (GObject *object)
{
	WindowMenuModel * menu = WINDOW_MENU_MODEL(object);

	g_clear_object(&menu->priv->app_menu);
	g_clear_object(&menu->priv->menubar);
	g_clear_object(&menu->priv->application);
	g_clear_object(&menu->priv->window);

	G_OBJECT_CLASS (window_menu_model_parent_class)->dispose (object);
	return;
}

static void
window_menu_model_finalize (GObject *object)
{

	G_OBJECT_CLASS (window_menu_model_parent_class)->finalize (object);
	return;
}

WindowMenuModel *
window_menu_model_new (BamfWindow * window)
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

	if (app_menu_object_path != NULL) {
		menu->priv->app_menu = g_dbus_menu_model_get (session, unique_bus_name, app_menu_object_path);
	}

	if (menubar_object_path != NULL) {
		menu->priv->menubar = g_dbus_menu_model_get (session, unique_bus_name, menubar_object_path);
	}

	if (application_object_path != NULL) {
		menu->priv->application = g_dbus_action_group_get (session, unique_bus_name, application_object_path);
	}

	if (window_object_path != NULL) {
		menu->priv->window = g_dbus_action_group_get (session, unique_bus_name, window_object_path);
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
