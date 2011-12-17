/*
Small utility to excersise the HUD from the command line

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

#include <glib.h>
#include <gio/gunixinputstream.h>
#include <unistd.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <stdlib.h>
#include <curses.h>

#include "shared-values.h"
#include "hud.interface.h"


static void print_suggestions(const char * query);
static GDBusProxy * proxy = NULL;
static GVariant * last_key = NULL;
static void update(char *string);
void sighandler(int);

char blank[80];
WINDOW *twindow = NULL;

int
main (int argv, char * argc[])
{

	g_type_init();

	int c;
	int pos = 0;
	int run = 1;
	char line[256]; 
	
	memset( blank, ' ', 80 );
	blank[79] = '\0';

	signal(SIGINT, sighandler);

	GDBusConnection * session = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
	g_return_val_if_fail(session != NULL, 1);

	GDBusNodeInfo * nodeinfo = g_dbus_node_info_new_for_xml(hud_interface, NULL);
	g_return_val_if_fail(nodeinfo != NULL, 1);

	GDBusInterfaceInfo * ifaceinfo = g_dbus_node_info_lookup_interface(nodeinfo, DBUS_IFACE);
	g_return_val_if_fail(ifaceinfo != NULL, 1);

	proxy = g_dbus_proxy_new_sync(session,
	                              G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
	                              ifaceinfo,
	                              DBUS_NAME,
	                              DBUS_PATH,
	                              DBUS_IFACE,
	                              NULL, NULL);
	g_return_val_if_fail(proxy != NULL, 1);

	if (!isatty (STDIN_FILENO)) {
		// FIXME - reading from STDIN seems to not have right dbus context
	}
     	else{
		twindow = initscr(); 		
		cbreak();
		keypad(stdscr, TRUE);	
		noecho();
	
		/* initialize the query screen */
		update( "" );

		/* interactive shell interface */
		while( run ){
		
			c = getch();		
			/* need to go left in the buffer */
			if ( c == KEY_BACKSPACE ){
				/* don't go too far left */
				if( pos > 0 ){
					pos--;
					line[pos] = '\0';
					update( line );
				}
				else
					; /* we are at the beginning of the buffer already */
			}
			/* ENTER will trigger the action for the first selected suggestion */
			else if ( c == '\n' ){

				/* break */ 
				run = 0;

				/* FIXME: execute action on RETURN */
			}
			/* add character to the buffer and terminate string */
			else{
				if ( pos > 256 -1 ){ // -1 for \0
					line[pos] = c;
					line[pos+1] = '\0';
					pos++;
					update( line );
				}
				else {
					
				}
			}
		}

		endwin();
	}

	g_dbus_node_info_unref(nodeinfo);
	g_object_unref(session);

	return 0;
}

static void 
update( char *string ){
	
	werase(twindow);
	mvwprintw(twindow, 7, 10, "Search: %s", string);

	print_suggestions( string );
	
	// move cursor back to input line
	wmove( twindow, 7, 10 + 8 + strlen(string) );

	refresh();
}


static void 
print_suggestions (const char *query)
{
	GError * error = NULL;
	GVariant * suggests = g_dbus_proxy_call_sync(proxy,
	                                             "GetSuggestions",
	                                             g_variant_new("(s)", query),
	                                             G_DBUS_CALL_FLAGS_NONE,
	                                             -1,
	                                             NULL,
	                                             &error);

	if (error != NULL) {
		g_warning("Unable to get suggestions: %s", error->message);
		g_error_free(error);
		return ;
	}

	GVariant * appicon = g_variant_get_child_value(suggests, 0);
	g_variant_unref(appicon);

	GVariant * target = g_variant_get_child_value(suggests, 1);
	g_variant_unref(target);

	GVariant * suggestions = g_variant_get_child_value(suggests, 2);
	GVariantIter iter;
	g_variant_iter_init(&iter, suggestions);
	gchar * suggestion = NULL;
	gchar * icon = NULL;
	GVariant * key = NULL;

	last_key = NULL;

	int i=0;

	while (g_variant_iter_loop(&iter, "(ssv)", &suggestion, &icon, &key)) {
		mvwprintw(twindow, 9 + i, 15, "%s", suggestion);
		i++;

	}

	g_variant_unref(suggestions);

	return;
}

void sighandler(int signal){
	endwin();
	g_object_unref(proxy);
	exit(EXIT_SUCCESS);
}
