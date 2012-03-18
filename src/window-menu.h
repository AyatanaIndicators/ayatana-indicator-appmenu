/*
An implementation of indicator object showing menus from applications.

Copyright 2012 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>

This program is free software: you can redistribute it and/or modify it 
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but 
WITHOUT ANY WARRANTY; without even the implied warranties of 
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR 
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along 
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __WINDOW_MENU_H__
#define __WINDOW_MENU_H__

#include <glib-object.h>
#include <libindicator/indicator-object.h>

G_BEGIN_DECLS

#define WINDOW_MENU_TYPE             (window_menu_get_type ())
#define WINDOW_MENU(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENU_TYPE, WindowMenu))
#define WINDOW_MENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENU_TYPE, WindowMenuClass))
#define IS_WINDOW_MENU(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENU_TYPE))
#define IS_WINDOW_MENU_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENU_TYPE))
#define WINDOW_MENU_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENU_TYPE, WindowMenuClass))

#define WINDOW_MENU_SIGNAL_ENTRY_ADDED    "entry-added"
#define WINDOW_MENU_SIGNAL_ENTRY_REMOVED  "entry-removed"
#define WINDOW_MENU_SIGNAL_ERROR_STATE    "error-state"
#define WINDOW_MENU_SIGNAL_STATUS_CHANGED "status-changed"
#define WINDOW_MENU_SIGNAL_SHOW_MENU      "show-menu"
#define WINDOW_MENU_SIGNAL_A11Y_UPDATE    "a11y-update"

typedef enum _WindowMenuStatus WindowMenuStatus;
enum _WindowMenuStatus {
	WINDOW_MENU_STATUS_NORMAL,
	WINDOW_MENU_STATUS_ACTIVE
};

typedef struct _WindowMenu      WindowMenu;
typedef struct _WindowMenuClass WindowMenuClass;

struct _WindowMenuClass {
	GObjectClass parent_class;

	/* Virtual Funcs */
	GList *          (*get_entries)      (WindowMenu * wm);
	guint            (*get_location)     (WindowMenu * wm, IndicatorObjectEntry * entry);

	guint            (*get_xid)          (WindowMenu * wm);

	gboolean         (*get_error_state)  (WindowMenu * wm);
	WindowMenuStatus (*get_status)       (WindowMenu * wm);
	void             (*entry_restore)    (WindowMenu * wm, IndicatorObjectEntry * entry);

	void             (*entry_activate)   (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp);

	/* Signals */
	void (*entry_added)    (WindowMenu * wm, IndicatorObjectEntry * entry, gpointer user_data);
	void (*entry_removed)  (WindowMenu * wm, IndicatorObjectEntry * entry, gpointer user_data);

	void (*error_state)    (WindowMenu * wm, gboolean state, gpointer user_data);
	void (*status_changed) (WindowMenu * wm, WindowMenuStatus status, gpointer user_data);

	void (*show_menu)      (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp, gpointer user_data);
	void (*a11y_update)    (WindowMenu * wm, IndicatorObjectEntry * entry, gpointer user_data);
};

struct _WindowMenu {
	GObject parent;
};

GType window_menu_get_type (void);

GList * window_menu_get_entries (WindowMenu * wm);
guint window_menu_get_location (WindowMenu * wm, IndicatorObjectEntry * entry);

guint window_menu_get_xid (WindowMenu * wm);

gboolean window_menu_get_error_state (WindowMenu * wm);
WindowMenuStatus window_menu_get_status (WindowMenu * wm);
void window_menu_entry_restore (WindowMenu * wm, IndicatorObjectEntry * entry);

void window_menu_entry_activate (WindowMenu * wm, IndicatorObjectEntry * entry, guint timestamp);

G_END_DECLS

#endif
