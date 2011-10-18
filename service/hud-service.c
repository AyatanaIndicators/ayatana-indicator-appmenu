
#include <glib.h>
#include <gio/gio.h>

#include "shared-values.h"
#include "hud-dbus.h"

static GMainLoop * mainloop = NULL;

static void
name_lost_cb (GDBusConnection * connection, const gchar * name, gpointer user_data)
{
	g_error("Unable to get name '%s'", name);
	g_main_loop_quit(mainloop);
	return;
}

int
main (int argv, char * argc[])
{
	g_type_init();

	HudDbus * dbus = hud_dbus_new();

	g_bus_own_name(G_BUS_TYPE_SESSION,
	/* Name */           DBUS_NAME,
	/* Flags */          G_BUS_NAME_OWNER_FLAGS_NONE,
	/* Bus Acquired */   NULL,
	/* Name Acquired */  NULL,
	/* Name Lost */      name_lost_cb,
	/* User Data */      NULL,
	/* Destroy */        NULL);


	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_main_loop_unref(mainloop);
	g_object_unref(dbus);

	return 0;
}
