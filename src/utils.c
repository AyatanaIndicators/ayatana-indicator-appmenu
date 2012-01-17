
#include "utils.h"

/* Check to see if a schema exists */
gboolean
settings_schema_exists (const gchar * schema)
{
	const gchar * const * schemas = g_settings_list_schemas();
	int i;

	for (i = 0; schemas[i] != NULL; i++) {
		if (g_strcmp0(schemas[i], schema) == 0) {
			return TRUE;
		}
	}

	return FALSE;
}

/* Checks to see if we can get the setting, and if we can use that,
   otherwise use the fallback value we have here */
guint
get_settings_uint (GSettings * settings, const gchar * setting_name, guint fallback)
{
	if (settings != NULL) {
		return g_settings_get_uint(settings, setting_name);
	} else {
		return fallback;
	}
}
