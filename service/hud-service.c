
#include <glib.h>

#include "hud-dbus.h"

int
main (int argv, char * argc[])
{
	g_type_init();

	HudDbus * dbus = hud_dbus_new();

	GMainLoop * mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);
	g_object_unref(dbus);

	return 0;
}
