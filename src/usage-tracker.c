/*
Tracks which menu items get used by users and works to promote those
higher in the search rankings than others.

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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <sqlite3.h>
#include "usage-tracker.h"
#include "load-app-info.h"
#include "utils.h"

struct _UsageTrackerPrivate {
	gchar * cachefile;
	sqlite3 * db;
	guint drop_timer;
	GSettings * settings;
};

#define USAGE_TRACKER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), USAGE_TRACKER_TYPE, UsageTrackerPrivate))

static void usage_tracker_class_init (UsageTrackerClass *klass);
static void usage_tracker_init       (UsageTracker *self);
static void usage_tracker_dispose    (GObject *object);
static void usage_tracker_finalize   (GObject *object);
static void configure_db             (UsageTracker * self);
static void usage_setting_changed    (GSettings * settings, const gchar * key, gpointer user_data);
static void build_db                 (UsageTracker * self);
static gboolean drop_entries         (gpointer user_data);
static void check_app_init (UsageTracker * self, const gchar * application);

G_DEFINE_TYPE (UsageTracker, usage_tracker, G_TYPE_OBJECT);

static void
usage_tracker_class_init (UsageTrackerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (UsageTrackerPrivate));

	object_class->dispose = usage_tracker_dispose;
	object_class->finalize = usage_tracker_finalize;

	return;
}

static void
usage_tracker_init (UsageTracker *self)
{
	self->priv = USAGE_TRACKER_GET_PRIVATE(self);

	self->priv->cachefile = NULL;
	self->priv->db = NULL;
	self->priv->drop_timer = 0;
	self->priv->settings = NULL;

	if (settings_schema_exists("com.canonical.indicator.appmenu.hud")) {
		self->priv->settings = g_settings_new("com.canonical.indicator.appmenu.hud");
		g_signal_connect(self->priv->settings, "changed::store-usage-data", G_CALLBACK(usage_setting_changed), self);
	}

	configure_db(self);

	/* Drop entries daily if we run for a really long time */
	self->priv->drop_timer = g_timeout_add_seconds(24 * 60 * 60, drop_entries, self);
	
	return;
}

static void
usage_tracker_dispose (GObject *object)
{
	UsageTracker * self = USAGE_TRACKER(object);

	if (self->priv->db != NULL) {
		sqlite3_close(self->priv->db);
		self->priv->db = NULL;
	}

	if (self->priv->drop_timer != 0) {
		g_source_remove(self->priv->drop_timer);
		self->priv->drop_timer = 0;
	}

	if (self->priv->settings != NULL) {
		g_object_unref(self->priv->settings);
		self->priv->settings = NULL;
	}

	G_OBJECT_CLASS (usage_tracker_parent_class)->dispose (object);
	return;
}

static void
usage_tracker_finalize (GObject *object)
{
	UsageTracker * self = USAGE_TRACKER(object);

	if (self->priv->cachefile != NULL) {
		g_free(self->priv->cachefile);
		self->priv->cachefile = NULL;
	}

	G_OBJECT_CLASS (usage_tracker_parent_class)->finalize (object);
	return;
}

UsageTracker *
usage_tracker_new (void)
{
	return g_object_new(USAGE_TRACKER_TYPE, NULL);
}

/* Checking if the setting for tracking the usage data has changed
   value.  We'll rebuild the DB */
static void
usage_setting_changed (GSettings * settings, const gchar * key, gpointer user_data)
{
	g_return_if_fail(IS_USAGE_TRACKER(user_data));
	UsageTracker * self = USAGE_TRACKER(user_data);

	configure_db(self);
	return;
}

/* Configure which database we should be using */
static void
configure_db (UsageTracker * self)
{
	/* Removing the previous database */
	if (self->priv->db != NULL) {
		sqlite3_close(self->priv->db);
		self->priv->db = NULL;
	}

	if (self->priv->cachefile != NULL) {
		g_free(self->priv->cachefile);
		self->priv->cachefile = NULL;
	}
	
	/* Determine where his database should be built */
	gboolean usage_data = TRUE;
	if (self->priv->settings != NULL) {
		usage_data = g_settings_get_boolean(self->priv->settings, "store-usage-data");
	}

	if (g_getenv("HUD_NO_STORE_USAGE_DATA") != NULL) {
		usage_data = FALSE;
	}

	if (usage_data) {
		g_debug("Storing usage data on filesystem");
	}

	/* Setting up the new database */
	gboolean db_exists = FALSE;

	if (usage_data) {
		/* If we're storing the usage data we need to figure out
		   how to do it on disk */

		const gchar * basecachedir = g_getenv("HUD_CACHE_DIR");
		if (basecachedir == NULL) {
			basecachedir = g_get_user_cache_dir();
		}

		gchar * cachedir = g_build_filename(basecachedir, "indicator-appmenu", NULL);
		if (!g_file_test(cachedir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) {
			g_mkdir(cachedir, 1 << 6 | 1 << 7 | 1 << 8); // 700
		}
		g_free(cachedir);

		self->priv->cachefile = g_build_filename(basecachedir, "indicator-appmenu", "hud-usage-log.sqlite", NULL);
		db_exists = g_file_test(self->priv->cachefile, G_FILE_TEST_EXISTS);
		int open_status = sqlite3_open(self->priv->cachefile, &self->priv->db); 

		if (open_status != SQLITE_OK) {
			g_warning("Error building LRU DB");
			sqlite3_close(self->priv->db);
			self->priv->db = NULL;
		}
	} else {
		/* If we're not storing it, let's make an in memory database
		   so that we can use the app-info, and get better, but we don't
		   give anyone that data. */
		self->priv->cachefile = g_strdup(":memory:");

		int open_status = sqlite3_open(self->priv->cachefile, &self->priv->db); 

		if (open_status != SQLITE_OK) {
			g_warning("Error building LRU DB");
			sqlite3_close(self->priv->db);
			self->priv->db = NULL;
		}
	}

	if (self->priv->db != NULL && !db_exists) {
		build_db(self);
	}

	drop_entries(self);

	return;
}

/* Build the database */
static void
build_db (UsageTracker * self)
{
	g_debug("New database, initializing");

	/* Create the table */
	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	exec_status = sqlite3_exec(self->priv->db,
	                           "create table usage (application text, entry text, timestamp datetime);",
	                           NULL, NULL, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to create table: %s", failstring);
	}

	/* Import data from the system */

	return;
}

void
usage_tracker_mark_usage (UsageTracker * self, const gchar * application, const gchar * entry)
{
	g_return_if_fail(IS_USAGE_TRACKER(self));
	check_app_init(self, application);

	gchar * statement = g_strdup_printf("insert into usage (application, entry, timestamp) values ('%s', '%s', date('now'));", application, entry);
	// g_debug("Executing: %s", statement);

	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	exec_status = sqlite3_exec(self->priv->db,
	                           statement,
	                           NULL, NULL, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to insert into table: '%s'  statement: '%s'", failstring, statement);
	}

	g_free(statement);
	return;
}

static int
count_cb (void * user_data, int columns, char ** values, char ** names)
{
	g_return_val_if_fail(columns == 1, -1);

	guint * count = (guint *)user_data;

	*count = g_ascii_strtoull(values[0], NULL, 10);

	return SQLITE_OK;
}

guint
usage_tracker_get_usage (UsageTracker * self, const gchar * application, const gchar * entry)
{
	g_return_val_if_fail(IS_USAGE_TRACKER(self), 0);
	check_app_init(self, application);

	gchar * statement = g_strdup_printf("select count(*) from usage where application = '%s' and entry = '%s' and timestamp > date('now', 'utc', '-30 days');", application, entry); // TODO: Add timestamp
	// g_debug("Executing: %s", statement);

	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	guint count;
	exec_status = sqlite3_exec(self->priv->db,
	                           statement,
	                           count_cb, &count, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to get count from table: '%s'  statement: '%s'", failstring, statement);
	}

	g_free(statement);
	return count;
}

/* Drop the entries from the database that have expired as they are
   over 30 days old */
static gboolean
drop_entries (gpointer user_data)
{
	g_return_val_if_fail(IS_USAGE_TRACKER(user_data), FALSE);
	UsageTracker * self = USAGE_TRACKER(user_data);

	if (self->priv->db == NULL) {
		return TRUE;
	}

	const gchar * statement = "delete from usage where timestamp < date('now', 'utc', '-30 days');";
	// g_debug("Executing: %s", statement);

	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	exec_status = sqlite3_exec(self->priv->db,
	                           statement,
	                           NULL, NULL, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to drop entries from table: %s", failstring);
	}

	return TRUE;
}

static void
check_app_init (UsageTracker * self, const gchar * application)
{
	gchar * statement = g_strdup_printf("select count(*) from usage where application = '%s';", application);

	int exec_status = SQLITE_OK;
	gchar * failstring = NULL;
	guint count = 0;
	exec_status = sqlite3_exec(self->priv->db,
	                           statement,
	                           count_cb, &count, &failstring);
	if (exec_status != SQLITE_OK) {
		g_warning("Unable to get app count from table: '%s'  statement: '%s'", failstring, statement);
	}

	g_free(statement);

	if (count > 0) {
		return;
	}

	g_debug("Initializing application: %s", application);
	gchar * basename = g_path_get_basename(application);

	gchar * app_info_path = NULL;

	if (g_getenv("HUD_APP_INFO_DIR") != NULL) {
		app_info_path = g_strdup(g_getenv("HUD_APP_INFO_DIR"));
	} else {
		app_info_path = g_build_filename(DATADIR, "indicator-appmenu", "hud", "app-info", NULL);
	}

	gchar * app_info_filename = g_strdup_printf("%s.hud-app-info", basename);
	gchar * app_info = g_build_filename(app_info_path, app_info_filename, NULL);

	if (!load_app_info(app_info, self->priv->db)) {
		if (g_file_test(app_info, G_FILE_TEST_EXISTS)) {
			g_warning("Unable to load application information for application '%s' at path '%s'", application, app_info);
		}
	}

	g_free(app_info);
	g_free(app_info_filename);
	g_free(app_info_path);
	g_free(basename);

	return;
}
