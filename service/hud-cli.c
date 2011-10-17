
#include <glib.h>
#include <gio/gunixinputstream.h>
#include <unistd.h>

GMainLoop * mainloop = NULL;

int
main (int argv, char * argc[])
{
	g_type_init();

	GInputStream * stdinput = g_unix_input_stream_new(STDIN_FILENO, FALSE);
	g_return_val_if_fail(stdinput != NULL, 1);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_object_unref(mainloop);
	g_object_unref(stdinput);

	return 0;
}
