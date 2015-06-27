/*
 * Copyright © 2012 Canonical Limited
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the licence, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Ted Gould <ted.gould@canonical.com>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <libbamf/libbamf.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gdesktopappinfo.h>

#include "window-menu-model.h"

struct _WindowMenuModelPrivate {
	guint xid;

	GtkAccelGroup * accel_group;
	GActionGroup * app_actions;
	GActionGroup * win_actions;
	GActionGroup * unity_actions;

	/* Application Menu */
	GDBusMenuModel * app_menu_model;
	IndicatorObjectEntry application_menu;
	gboolean has_application_menu;

	/* Window Menus */
	GDBusMenuModel * win_menu_model;
	GtkMenuBar * win_menu;
};

#define WINDOW_MENU_MODEL_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), WINDOW_MENU_MODEL_TYPE, WindowMenuModelPrivate))

/* Base class stuff */
static void                window_menu_model_class_init (WindowMenuModelClass *klass);
static void                window_menu_model_init       (WindowMenuModel *self);
static void                window_menu_model_dispose    (GObject *object);

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
#define ACTION_MUX_PREFIX_APP   "app"
#define ACTION_MUX_PREFIX_WIN   "win"
#define ACTION_MUX_PREFIX_UNITY "unity"

/* Entry data on the menuitem */
#define ENTRY_DATA  "window-menu-model-menuitem-entry"

static void
window_menu_model_class_init (WindowMenuModelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (WindowMenuModelPrivate));

	object_class->dispose = window_menu_model_dispose;

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

	self->priv->accel_group = gtk_accel_group_new();

	return;
}

static void
window_menu_model_dispose (GObject *object)
{
	WindowMenuModel * menu = WINDOW_MENU_MODEL(object);

	if (menu->priv->has_application_menu) {
		g_signal_emit_by_name(menu, WINDOW_MENU_SIGNAL_ENTRY_REMOVED, &menu->priv->application_menu);
		menu->priv->has_application_menu = FALSE;
	}

	g_clear_object(&menu->priv->accel_group);

	/* Application Menu */
	g_clear_object(&menu->priv->app_menu_model);
	g_clear_object(&menu->priv->application_menu.label);
	g_clear_object(&menu->priv->application_menu.menu);

	/* Window Menus */
	g_clear_object(&menu->priv->win_menu_model);

	if (menu->priv->win_menu) {
		g_signal_handlers_disconnect_by_data(menu->priv->win_menu, menu);
		gtk_widget_destroy (GTK_WIDGET (menu->priv->win_menu));
		g_object_unref (menu->priv->win_menu);
		menu->priv->win_menu = NULL;
	}

	g_clear_object(&menu->priv->unity_actions);
	g_clear_object(&menu->priv->win_actions);
	g_clear_object(&menu->priv->app_actions);

	G_OBJECT_CLASS (window_menu_model_parent_class)->dispose (object);
	return;
}

/* Adds the application menu and turns the whole thing into an object
   entry that can be used elsewhere */
static void
add_application_menu (WindowMenuModel * menu, const gchar * appname, GMenuModel * model)
{
	g_return_if_fail(G_IS_MENU_MODEL(model));

	menu->priv->app_menu_model = g_object_ref(model);
	menu->priv->application_menu.parent_window = menu->priv->xid;

	if (appname != NULL) {
		menu->priv->application_menu.label = GTK_LABEL(gtk_label_new(appname));
	} else {
		menu->priv->application_menu.label = GTK_LABEL(gtk_label_new(_("Unknown Application Name")));
	}
	g_object_ref_sink(menu->priv->application_menu.label);
	gtk_widget_show(GTK_WIDGET(menu->priv->application_menu.label));

	menu->priv->application_menu.menu = GTK_MENU(gtk_menu_new_from_model(model));
	if (menu->priv->app_actions) {
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->application_menu.menu), ACTION_MUX_PREFIX_APP, menu->priv->app_actions);
	}
	if (menu->priv->win_actions) {
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->application_menu.menu), ACTION_MUX_PREFIX_WIN, menu->priv->win_actions);
	}
	if (menu->priv->unity_actions) {
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->application_menu.menu), ACTION_MUX_PREFIX_UNITY, menu->priv->unity_actions);
	}

	gtk_widget_show(GTK_WIDGET(menu->priv->application_menu.menu));
	g_object_ref_sink(menu->priv->application_menu.menu);

	menu->priv->has_application_menu = TRUE;
	g_signal_emit_by_name(menu, WINDOW_MENU_SIGNAL_ENTRY_ADDED, &menu->priv->application_menu);
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

typedef struct _WindowMenuEntry WindowMenuEntry;
struct _WindowMenuEntry {
	IndicatorObjectEntry entry;

	GtkMenuItem * gmi;
};

/* Sync the menu label changing to the label object */
static void
entry_label_notify (GObject * obj, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(GTK_IS_MENU_ITEM(obj));

	GtkMenuItem * gmi = GTK_MENU_ITEM(obj);
	WindowMenuEntry * entry = (WindowMenuEntry *)user_data;

	if (entry->entry.label != NULL) {
		const gchar * label = gtk_menu_item_get_label(gmi);
		gtk_label_set_label(entry->entry.label, label);
	}

	return;
}

/* Watch for visible changes and ensure they can be picked up by the
   indicator object host */
static void
entry_visible_notify (GObject * obj, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(obj));
	GtkWidget * widget = GTK_WIDGET(obj);
	WindowMenuEntry * entry = (WindowMenuEntry *)user_data;
	gboolean visible = gtk_widget_get_visible(widget);

	if (entry->entry.label != NULL) {
		gtk_widget_set_visible(GTK_WIDGET(entry->entry.label), visible);
	}

	if (entry->entry.image != NULL) {
		gtk_widget_set_visible(GTK_WIDGET(entry->entry.image), visible);
	}

	return;
}

/* Watch for sensitive changes and ensure they can be picked up by the
   indicator object host */
static void
entry_sensitive_notify (GObject * obj, GParamSpec * pspec, gpointer user_data)
{
	g_return_if_fail(GTK_IS_WIDGET(obj));
	GtkWidget * widget = GTK_WIDGET(obj);
	WindowMenuEntry * entry = (WindowMenuEntry *)user_data;
	gboolean sensitive = gtk_widget_get_sensitive(widget);

	if (entry->entry.label != NULL) {
		gtk_widget_set_sensitive(GTK_WIDGET(entry->entry.label), sensitive);
	}

	if (entry->entry.image != NULL) {
		gtk_widget_set_sensitive(GTK_WIDGET(entry->entry.image), sensitive);
	}

	return;
}

/* Destroy and unref the items of the object entry */
static void
entry_object_free (gpointer inentry)
{
	WindowMenuEntry * entry = (WindowMenuEntry *)inentry;

	g_signal_handlers_disconnect_by_data(entry->gmi, entry);

	g_clear_object(&entry->entry.label);
	g_clear_object(&entry->entry.image);
	g_clear_object(&entry->entry.menu);

	g_free(entry);
	return;
}

/* Put an entry on a menu item */
static void
entry_on_menuitem (WindowMenuModel * menu, GtkMenuItem * gmi)
{
	WindowMenuEntry * entry = g_new0(WindowMenuEntry, 1);

	entry->gmi = gmi;

	entry->entry.parent_window = menu->priv->xid;
	entry->entry.label = mi_find_label(GTK_WIDGET(gmi));
	entry->entry.image = mi_find_icon(GTK_WIDGET(gmi));
	entry->entry.menu = mi_find_menu(gmi);

	if (entry->entry.label == NULL && entry->entry.image == NULL) {
		const gchar * label = gtk_menu_item_get_label(gmi);
		if (label == NULL) {
			g_warning("Item doesn't have a label or an image, aborting");
			return;
		}

		entry->entry.label = GTK_LABEL(gtk_label_new(label));
		gtk_widget_show(GTK_WIDGET(entry->entry.label));
		g_signal_connect(G_OBJECT(gmi), "notify::label", G_CALLBACK(entry_label_notify), entry);
	}
	if (entry->entry.label != NULL) {
		g_object_ref_sink(entry->entry.label);
	}

	if (entry->entry.image != NULL) {
		g_object_ref_sink(entry->entry.image);
	}

	if (entry->entry.menu != NULL) {
		g_object_ref_sink(entry->entry.menu);
	}

	g_signal_connect(G_OBJECT(gmi), "notify::sensitive", G_CALLBACK(entry_sensitive_notify), entry);
	g_signal_connect(G_OBJECT(gmi), "notify::visible", G_CALLBACK(entry_visible_notify), entry);

	g_object_set_data_full(G_OBJECT(gmi), ENTRY_DATA, entry, entry_object_free);

	return;
}

/* A child item was added to a menu we're watching.  Let's try to integrate it. */
static void
item_inserted_cb (GtkContainer *menu,
                  GtkWidget    *widget,
                  gint          position,
                  gpointer      data)
{
	if (g_object_get_data(G_OBJECT(widget), ENTRY_DATA) == NULL) {
		entry_on_menuitem(WINDOW_MENU_MODEL(data), GTK_MENU_ITEM(widget));
	}

	if (g_object_get_data(G_OBJECT(widget), ENTRY_DATA) != NULL) {
		g_signal_emit_by_name(data, WINDOW_MENU_SIGNAL_ENTRY_ADDED, g_object_get_data(G_OBJECT(widget), ENTRY_DATA));
	}

	return;
}

/* A child item was removed from a menu we're watching. */
static void
item_removed_cb (GtkContainer *menu, GtkWidget *widget, gpointer data)
{
	g_signal_emit_by_name(data, WINDOW_MENU_SIGNAL_ENTRY_REMOVED, g_object_get_data(G_OBJECT(widget), ENTRY_DATA));
}

/* Adds the window menu and turns it into a set of IndicatorObjectEntries
   that can be used elsewhere */
static void
add_window_menu (WindowMenuModel * menu, GMenuModel * model)
{
	menu->priv->win_menu_model = g_object_ref(model);

	menu->priv->win_menu = GTK_MENU_BAR(gtk_menu_bar_new_from_model(model));
	g_assert(menu->priv->win_menu != NULL);
	g_object_ref_sink(menu->priv->win_menu);

	if (menu->priv->app_actions)
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->win_menu), ACTION_MUX_PREFIX_APP, menu->priv->app_actions);
	if (menu->priv->win_actions)
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->win_menu), ACTION_MUX_PREFIX_WIN, menu->priv->win_actions);
	if (menu->priv->unity_actions)
		gtk_widget_insert_action_group(GTK_WIDGET(menu->priv->win_menu), ACTION_MUX_PREFIX_UNITY, menu->priv->unity_actions);

	g_signal_connect(G_OBJECT(menu->priv->win_menu), "insert", G_CALLBACK (item_inserted_cb), menu);
	g_signal_connect(G_OBJECT(menu->priv->win_menu), "remove", G_CALLBACK (item_removed_cb), menu);

	GList * children = gtk_container_get_children(GTK_CONTAINER(menu->priv->win_menu));
	GList * child;
	for (child = children; child != NULL; child = g_list_next(child)) {
		GtkMenuItem * gmi = GTK_MENU_ITEM(child->data);

		if (gmi == NULL) {
			continue;
		}

		entry_on_menuitem(menu, gmi);
	}
	g_list_free(children);

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
	gchar *unity_object_path;
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
	unity_object_path = bamf_window_get_utf8_prop (window, "_UNITY_OBJECT_PATH");

	session = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

	/* Setup actions */
	if (application_object_path != NULL) {
		menu->priv->app_actions = G_ACTION_GROUP(g_dbus_action_group_get (session, unique_bus_name, application_object_path));
	}

	if (window_object_path != NULL) {
		menu->priv->win_actions = G_ACTION_GROUP(g_dbus_action_group_get (session, unique_bus_name, window_object_path));
	}

	if (unity_object_path != NULL) {
		menu->priv->unity_actions = G_ACTION_GROUP(g_dbus_action_group_get (session, unique_bus_name, unity_object_path));
	}

	/* Build us some menus */
	if (app_menu_object_path != NULL) {
		const gchar * desktop_path = bamf_application_get_desktop_file(app);
		gchar * app_name = NULL;

		if (desktop_path != NULL) {
			GDesktopAppInfo * desktop = g_desktop_app_info_new_from_filename(desktop_path);

			if (desktop != NULL) {
				app_name = g_strdup(g_app_info_get_name(G_APP_INFO(desktop)));

				g_object_unref(desktop);
			}
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
	g_free (unity_object_path);

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

			if (entry == NULL) {
				/* Try to build the entry, it is possible (but unlikely) that
				   we could beat the signal that this isn't created.  So we'll
				   just handle that race here */
				entry_on_menuitem(menu, GTK_MENU_ITEM(child->data));
				entry = g_object_get_data(child->data, ENTRY_DATA);
			}

			if (entry != NULL) {
				ret = g_list_append(ret, entry);
			}
		}

		g_list_free(children);
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

		g_list_free(children);
	}

	if (!found) {
		/* NOTE: Not printing any of the values here because there's
		   a pretty good chance that they're not valid.  Let's not crash
		   things here. */
		pos = G_MAXUINT;
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
