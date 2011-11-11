#include <glib.h>
#include <glib-object.h>

#include "../service/usage-tracker.h"
#include "../service/usage-tracker.c"

gint
main (gint argc, gchar * argv[])
{
	g_type_init();
	UsageTracker * tracker = usage_tracker_new();
	g_object_unref(tracker);
	return 0;
}
