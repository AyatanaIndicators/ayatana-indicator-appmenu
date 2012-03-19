#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libbamf/libbamf.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include "window-menu-model.h"
#include "gactionmuxer.h"

struct _WindowMenuModelPrivate {
	guint xid;

	/* All the actions */
	GActionMuxer * action_mux;

	/* Application Menu */
	GDBusMenuModel * app_menu_model;
	IndicatorObjectEntry application_menu;
	gboolean has_application_menu;

	/* Window Menus */
	GDBusMenuModel * win_menu_model;
};

#define WINDOW_MENU_MODEL_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_MODEL_TYPE, WindowMenuModelPrivate))

/* Base class stuff */
static void                window_menu_model_class_init (WindowMenuModelClass *klass);
static void                window_menu_model_init       (WindowMenuModel *self);
static void                window_menu_model_dispose    (GObject *object);
static void                window_menu_model_finalize   (GObject *object);

/* Window Menu subclassin' */
static GList *             get_entries                  (WindowMenu * wm);
static guint               get_location                 (WindowMenu * wm,
                                                         IndicatorObjectEntry * entry);
static WindowMenuStatus    get_status                   (WindowMenu * wm);
static gboolean            get_error_state              (WindowMenu * wm);
static guint               get_xid                      (WindowMenu * wm);

/* GLib boilerplate */
G_DEFINE_TYPE (WindowMenuModel, window_menu_model, WINDOW_MENU_TYPE);

/* Prefixes to the action muxer */
#define ACTION_MUX_PREFIX_WIN  "window"
#define ACTION_MUX_PREFIX_APP  "application"

static void
window_menu_model_class_init (WindowMenuModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenuModelPrivate));

	object_class->dispose = window_menu_model_dispose;
	object_class->finalize = window_menu_model_finalize;

	WindowMenuClass * wm_class = WINDOW_MENU_CLASS(klass);

	wm_class->get_entries = get_entries;
	wm_class->get_location = get_location;
	wm_class->get_status = get_status;
	wm_class->get_error_state = get_error_state;
	wm_class->get_xid = get_xid;

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

	/* Application Menu */
	g_clear_object(&menu->priv->app_menu_model);
	g_clear_object(&menu->priv->application_menu.label);
	g_clear_object(&menu->priv->application_menu.menu);

	/* Window Menus */
	g_clear_object(&menu->priv->win_menu_model);

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
add_application_menu (WindowMenuModel * menu, const gchar * appname, GMenuModel * model)
{
	g_return_if_fail(G_IS_MENU_MODEL(model));

	menu->priv->app_menu_model = g_object_ref(model);

	if (appname != NULL) {
		menu->priv->application_menu.label = GTK_LABEL(gtk_label_new(appname));
	} else {
		menu->priv->application_menu.label = GTK_LABEL(gtk_label_new(_("Unknown Application Name")));
	}
	g_object_ref_sink(menu->priv->application_menu.label);
	gtk_widget_show(GTK_WIDGET(menu->priv->application_menu.label));

	menu->priv->application_menu.menu = GTK_MENU(gtk_menu_new());

	GtkWidget * item = gtk_menu_item_new_with_label("Test");
	gtk_widget_show(item);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu->priv->application_menu.menu), item);

	gtk_widget_show(GTK_WIDGET(menu->priv->application_menu.menu));
	g_object_ref_sink(menu->priv->application_menu.menu);

	menu->priv->has_application_menu = TRUE;

	return;
}

/* Adds the window menu and turns it into a set of IndicatorObjectEntries
   that can be used elsewhere */
static void
add_window_menu (WindowMenuModel * menu, GMenuModel * model)
{
	menu->priv->win_menu_model = g_object_ref(model);


	return;
}

/* Builds the menu model from the window for the application */
WindowMenuModel *
window_menu_model_new (BamfApplication * app, BamfWindow * window)
{
	g_return_val_if_fail(BAMF_IS_APPLICATION(app), NULL);
	g_return_val_if_fail(BAMF_IS_WINDOW(window), NULL);

	WindowMenuModel * menu = g_object_new(WINDOW_MENU_MODEL_TYPE, NULL);

	menu->priv->xid = bamf_window_get_xid(window);

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
		const gchar * desktop_path = bamf_application_get_desktop_file(app);
		gchar * app_name = NULL;

		if (desktop_path != NULL) {
			GDesktopAppInfo * desktop = g_desktop_app_info_new_from_filename(desktop_path);

			app_name = g_strdup(g_app_info_get_name(G_APP_INFO(desktop)));

			g_object_unref(desktop);
		}

		GMenuModel * model = G_MENU_MODEL(g_dbus_menu_model_get (session, unique_bus_name, app_menu_object_path));

		add_application_menu(menu, app_name, model);

		g_object_unref(model);
		g_free(app_name);
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

/* Get the list of entries */
static GList *
get_entries (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENU_MODEL(wm), NULL);
	WindowMenuModel * menu = WINDOW_MENU_MODEL(wm);

	GList * ret = NULL;

	if (menu->priv->has_application_menu) {
		ret = g_list_append(ret, &menu->priv->application_menu);
	}

	return ret;
}

/* Find the location of an entry */
static guint
get_location (WindowMenu * wm, IndicatorObjectEntry * entry)
{
	g_return_val_if_fail(IS_WINDOW_MENU_MODEL(wm), 0);
	WindowMenuModel * menu = WINDOW_MENU_MODEL(wm);

	gboolean found = FALSE;
	guint pos = 0;

	if (menu->priv->has_application_menu) {
		if (entry == &menu->priv->application_menu) {
			pos = 0;
			found = TRUE;
		} else {
			/* We need to put a shift in if there is an application
			   menu and we're not looking for that one */
			pos = 1;
		}
	}

	if (!found) {
		g_warning("Unable to find entry: %p", entry);
	}

	return pos;
}

/* Get's the status of the application to whether underlines should be
   shown to the application.  GMenuModel doesn't give us this info. */
static WindowMenuStatus
get_status (WindowMenu * wm)
{
	return WINDOW_MENU_STATUS_NORMAL;
}

/* Says whether the application is in error, GMenuModel doesn't give us this
   information on the app */
static gboolean
get_error_state (WindowMenu * wm)
{
	return FALSE;
}

/* Get the XID of this guy */
static guint
get_xid (WindowMenu * wm)
{
	g_return_val_if_fail(IS_WINDOW_MENU_MODEL(wm), 0);
	return WINDOW_MENU_MODEL(wm)->priv->xid;
}
