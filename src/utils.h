/*
Small functions that don't really have their own place

Copyright 2011 Canonical Ltd.

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

#ifndef __UTILS_H__
#define __UTILS_H__

#include <glib.h>
#include <gio/gio.h>

G_BEGIN_DECLS

gboolean settings_schema_exists (const gchar * schema);
guint get_settings_uint (GSettings * settings, const gchar * setting_name, guint fallback);

G_END_DECLS

#endif /* __UTILS_H__ */
