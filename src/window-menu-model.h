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

#ifndef __WINDOW_MENU_MODEL_H__
#define __WINDOW_MENU_MODEL_H__

#include <glib.h>
#include <glib-object.h>
#include <libbamf/bamf-window.h>
#include "window-menu.h"

G_BEGIN_DECLS

#define WINDOW_MENU_MODEL_TYPE            (window_menu_model_get_type ())
#define WINDOW_MENU_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), WINDOW_MENU_MODEL_TYPE, WindowMenuModel))
#define WINDOW_MENU_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), WINDOW_MENU_MODEL_TYPE, WindowMenuModelClass))
#define IS_WINDOW_MENU_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WINDOW_MENU_MODEL_TYPE))
#define IS_WINDOW_MENU_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), WINDOW_MENU_MODEL_TYPE))
#define WINDOW_MENU_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), WINDOW_MENU_MODEL_TYPE, WindowMenuModelClass))

typedef struct _WindowMenuModel        WindowMenuModel;
typedef struct _WindowMenuModelClass   WindowMenuModelClass;
typedef struct _WindowMenuModelPrivate WindowMenuModelPrivate;

struct _WindowMenuModelClass {
	WindowMenuClass parent_class;
};

struct _WindowMenuModel {
	WindowMenu parent;

	WindowMenuModelPrivate * priv;
};

GType window_menu_model_get_type (void);
WindowMenuModel * window_menu_model_new (BamfApplication * app, BamfWindow * window);

G_END_DECLS

#endif
