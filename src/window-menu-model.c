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
#include "gtkmodelmenu.h"

struct _WindowMenuModelPrivate {
	guint xid;

	/* All the actions */
	GActionMuxer * action_mux;
	GtkAccelGroup * accel_group;

	/* Application Menu */
	GDBusMenuModel * app_menu_model;
	IndicatorObjectEntry application_menu;
	gboolean has_application_menu;

	/* Window Menus */
	GDBusMenuModel * win_menu_model;
	GtkMenu * win_menu;
	gulong win_menu_insert;
	gulong win_menu_remove;
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
#define ACTION_MUX_PREFIX_WIN  "win"
#define ACTION_MUX_PREFIX_APP  "app"

/* Entry data on the menuitem */
#define ENTRY_DATA  "window-menu-model-menuitem-entry"

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
	self->priv->accel_group = gtk_accel_group_new();

	return;
}

static void
window_menu_model_dispose (GObject *object)
{
	WindowMenuModel * menu = WINDOW_MENU_MODEL(object);

	g_clear_object(&menu->priv->action_mux);
	g_clear_object(&menu->priv->accel_group);

	/* Application Menu */
	g_clear_object(&menu->priv->app_menu_model);
	g_clear_object(&menu->priv->application_menu.label);
	g_clear_object(&menu->priv->application_menu.menu);

	/* Window Menus */
	if (menu->priv->win_menu_insert != 0) {
		g_signal_handler_disconnect(menu->priv->win_menu, menu->priv->win_menu_insert);
		menu->priv->win_menu_insert = 0;
	}

	if (menu->priv->win_menu_remove != 0) {
		g_signal_handler_disconnect(menu->priv->win_menu, menu->priv->win_menu_remove);
		menu->priv->win_menu_remove = 0;
	}

	g_clear_object(&menu->priv->win_menu_model);
	g_clear_object(&menu->priv->win_menu);

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

	menu->priv->application_menu.menu = GTK_MENU(gtk_model_menu_create_menu(model, G_ACTION_OBSERVABLE(menu->priv->action_mux), menu->priv->accel_group));

	gtk_widget_show(GTK_WIDGET(menu->priv->application_menu.menu));
	g_object_ref_sink(menu->priv->application_menu.menu);

	menu->priv->has_application_menu = TRUE;

	return;
}

/* Find the label in a GTK MenuItem */
GtkLabel *
mi_find_label (GtkWidget * mi)
{
	if (GTK_IS_LABEL(mi)) {
		return GTK_LABEL(mi);
	}

	GtkLabel * retval = NULL;

	if (GTK_IS_CONTAINER(mi)) {
		GList * children = gtk_container_get_children(GTK_CONTAINER(mi));
		GList * child = children;

		while (child != NULL && retval == NULL) {
			if (GTK_IS_WIDGET(child->data)) {
				retval = mi_find_label(GTK_WIDGET(child->data));
			}
			child = g_list_next(child);
		}

		g_list_free(children);
	}

	return retval;
}

/* Find the icon in a GTK MenuItem */
GtkImage *
mi_find_icon (GtkWidget * mi)
{
	if (GTK_IS_IMAGE(mi)) {
		return GTK_IMAGE(mi);
	}

	GtkImage * retval = NULL;

	if (GTK_IS_CONTAINER(mi)) {
		GList * children = gtk_container_get_children(GTK_CONTAINER(mi));
		GList * child = children;

		while (child != NULL && retval == NULL) {
			if (GTK_IS_WIDGET(child->data)) {
				retval = mi_find_icon(GTK_WIDGET(child->data));
			}
			child = g_list_next(child);
		}

		g_list_free(children);
	}

	return retval;
}

/* Check the menu and make sure we return it if it's a menu
   all proper like that */
GtkMenu *
mi_find_menu (GtkMenuItem * mi)
{
	GtkWidget * retval = gtk_menu_item_get_submenu(mi);
	if (GTK_IS_MENU(retval)) {
		return GTK_MENU(retval);
	} else {
		return NULL;
	}
}

/* Put an entry on a menu item */
static void
entry_on_menuitem (WindowMenuModel * menu, GtkMenuItem * gmi)
{
	IndicatorObjectEntry * entry = g_new0(IndicatorObjectEntry, 1);

	entry->label = mi_find_label(GTK_WIDGET(gmi));
	entry->image = mi_find_icon(GTK_WIDGET(gmi));
	entry->menu = mi_find_menu(gmi);

	/* TODO: set up some weak pointers here */
	/* TODO: Oh, and some label update signals and stuff */

	g_object_set_data_full(G_OBJECT(gmi), ENTRY_DATA, entry, g_free);

	return;
}

/* A child item was added to a menu we're watching.  Let's try to integrate it. */
static void
item_inserted_cb (GtkContainer *menu,
                  GtkWidget    *widget,
#ifdef HAVE_GTK3
                  gint          position,
#endif
                  gpointer      data)
{
	if (g_object_get_data(G_OBJECT(widget), ENTRY_DATA) != NULL) {
		entry_on_menuitem(WINDOW_MENU_MODEL(data), GTK_MENU_ITEM(widget));
	}

	g_signal_emit_by_name(data, WINDOW_MENU_SIGNAL_ENTRY_ADDED, g_object_get_data(G_OBJECT(widget), ENTRY_DATA));

	return;
}

/* A child item was removed from a menu we're watching. */
static void
item_removed_cb (GtkContainer *menu, GtkWidget *widget, gpointer data)
{
	g_signal_emit_by_name(data, WINDOW_MENU_SIGNAL_ENTRY_REMOVED, g_object_get_data(G_OBJECT(widget), ENTRY_DATA));
	return;
}

/* Adds the window menu and turns it into a set of IndicatorObjectEntries
   that can be used elsewhere */
static void
add_window_menu (WindowMenuModel * menu, GMenuModel * model)
{
	menu->priv->win_menu_model = g_object_ref(model);

	menu->priv->win_menu = GTK_MENU(gtk_model_menu_create_menu(model, G_ACTION_OBSERVABLE(menu->priv->action_mux), menu->priv->accel_group));
	g_assert(menu->priv->win_menu != NULL);
	g_object_ref_sink(menu->priv->win_menu);

	menu->priv->win_menu_insert = g_signal_connect(G_OBJECT (menu->priv->win_menu),
#ifdef HAVE_GTK3
		"insert",
#else
		"child-added",
#endif
		G_CALLBACK (item_inserted_cb),
		menu);
	menu->priv->win_menu_remove = g_signal_connect (G_OBJECT (menu->priv->win_menu),
		"remove",
		G_CALLBACK (item_removed_cb),
		menu);

	GList * children = gtk_container_get_children(GTK_CONTAINER(menu->priv->win_menu));
	GList * child;
	for (child = children; child != NULL; child = g_list_next(child)) {
		GtkMenuItem * gmi = GTK_MENU_ITEM(child->data);

		if (gmi == NULL) {
			continue;
		}

		entry_on_menuitem(menu, gmi);
	}

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

	if (menu->priv->win_menu != NULL) {
		GList * children = gtk_container_get_children(GTK_CONTAINER(menu->priv->win_menu));
		GList * child;
		for (child = children; child != NULL; child = g_list_next(child)) {
			gpointer entry = g_object_get_data(child->data, ENTRY_DATA);
			/* TODO: Handle case of no entry */
			if (entry != NULL) {
				ret = g_list_append(ret, entry);
			}
		}
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

	if (menu->priv->win_menu != NULL) {
		GList * children = gtk_container_get_children(GTK_CONTAINER(menu->priv->win_menu));
		GList * child;
		for (child = children; child != NULL; child = g_list_next(child), pos++) {
			gpointer lentry = g_object_get_data(child->data, ENTRY_DATA);

			if (entry == lentry) {
				found = TRUE;
				break;
			}
		}
	}

	if (!found) {
		/* NOTE: Not printing any of the values here because there's
		   a pretty good chance that they're not valid.  Let's not crash
		   things here. */
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
