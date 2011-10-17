
#include <glib.h>
#include <gio/gunixinputstream.h>
#include <unistd.h>

void input_cb (GObject * object, GAsyncResult * res, gpointer user_data);

GMainLoop * mainloop = NULL;

int
main (int argv, char * argc[])
{
	g_type_init();

	GInputStream * stdinput = g_unix_input_stream_new(STDIN_FILENO, FALSE);
	g_return_val_if_fail(stdinput != NULL, 1);

	gchar buffer[256];
	g_input_stream_read_async(stdinput,
	                          /* buffer */    buffer,
	                          /* size */      256,
	                          /* priority */  G_PRIORITY_DEFAULT,
	                          /* cancel */    NULL,
	                          /* callback */  input_cb,
	                          /* userdata */  buffer);

	mainloop = g_main_loop_new(NULL, FALSE);
	g_main_loop_run(mainloop);

	g_object_unref(mainloop);
	g_object_unref(stdinput);

	return 0;
}

/* Input coming from STDIN */
void
input_cb (GObject * object, GAsyncResult * res, gpointer user_data)
{
	GInputStream * stdinput = G_INPUT_STREAM(object);
	GError * error = NULL;
	gssize size = 0;

	size = g_input_stream_read_finish(stdinput, res, &error);

	if (error != NULL) {
		g_error("Error from STDIN: %s", error->message);
		g_error_free(error);
		g_main_loop_quit(mainloop);
		return;
	}

	gchar * buffer = (gchar *)user_data;
	g_input_stream_read_async(stdinput,
	                          /* buffer */    buffer,
	                          /* size */      256,
	                          /* priority */  G_PRIORITY_DEFAULT,
	                          /* cancel */    NULL,
	                          /* callback */  input_cb,
	                          /* userdata */  buffer);

	if (size == 0) {
		return;
	}

	buffer[size] = '\0';
	g_debug("New query: %s", buffer);

	return;
}
