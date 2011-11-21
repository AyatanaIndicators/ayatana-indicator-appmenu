/*
Prints out application info for debugging and CLI tools

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

#include "dump-app-info.h"

void
dump_app_info (const gchar * app, sqlite3 * db)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(db != NULL);

	g_print("<hudappinfo>\n");

	g_print("\t<desktopfile>%s</desktopfile>\n", app);

	g_print("</hudappinfo>\n");
	return;
}
