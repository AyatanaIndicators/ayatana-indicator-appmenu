/*
Loads application info for initial app usage and verification

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

#ifndef __LOAD_APP_INFO_H__
#define __LOAD_APP_INFO_H__

#include <glib.h>
#include <sqlite3.h>

gboolean load_app_info (const gchar * filename, const gchar * domain, sqlite3 * db);

#endif /* __LOAD_APP_INFO_H__ */
