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

	/* SQL Statements */
	sqlite3_stmt * insert_entry;
	sqlite3_stmt * entry_count;
	sqlite3_stmt * delete_aged;
	sqlite3_stmt * application_count;
};

typedef enum {
	SQL_VAR_APPLICATION = 1,
	SQL_VAR_ENTRY = 2
} sql_variables;

#define SQL_VARS_APPLICATION  "1"
#define SQL_VARS_ENTRY        "2"

#define USAGE_TRACKER_GET_PRIVATE(o) \
(G_TYPE_INSTANCE_GET_PRIVATE ((o), USAGE_TRACKER_TYPE, UsageTrackerPrivate))

static void usage_tracker_class_init (UsageTrackerClass *klass);
static void usage_tracker_init       (UsageTracker *self);
static void usage_tracker_dispose    (GObject *object);
static void usage_tracker_finalize   (GObject *object);
static void cleanup_db               (UsageTracker * self);
static void configure_db             (UsageTracker * self);
static void prepare_statements       (UsageTracker * self);
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

	self->priv->insert_entry = NULL;
	self->priv->entry_count = NULL;
	self->priv->delete_aged = NULL;
	self->priv->application_count = NULL;

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

	cleanup_db(self);

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

/* Small function to make sure we get all the DB components cleaned
   up in the spaces we need them */
static void
cleanup_db (UsageTracker * self)
{
	if (self->priv->insert_entry != NULL) {
		sqlite3_finalize(self->priv->insert_entry);
		self->priv->insert_entry = NULL;
	}

	if (self->priv->entry_count != NULL) {
		sqlite3_finalize(self->priv->entry_count);
		self->priv->entry_count = NULL;
	}

	if (self->priv->delete_aged != NULL) {
		sqlite3_finalize(self->priv->delete_aged);
		self->priv->delete_aged = NULL;
	}

	if (self->priv->application_count != NULL) {
		sqlite3_finalize(self->priv->application_count);
		self->priv->application_count = NULL;
	}

	if (self->priv->db != NULL) {
		sqlite3_close(self->priv->db);
		self->priv->db = NULL;
	}

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
	cleanup_db(self);

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

	prepare_statements(self);

	if (self->priv->db != NULL && !db_exists) {
		build_db(self);
	}

	drop_entries(self);

	return;
}

/* Build all the prepared statments */
static void
prepare_statements (UsageTracker * self)
{
	if (self->priv->db == NULL) {
		return;
	}

	/* These should never happen, but let's just check to make sure */
	g_return_if_fail(self->priv->insert_entry == NULL);
	g_return_if_fail(self->priv->entry_count == NULL);
	g_return_if_fail(self->priv->delete_aged == NULL);
	g_return_if_fail(self->priv->application_count == NULL);

	int prepare_status = SQLITE_OK;

	/* Insert Statement */
	prepare_status = sqlite3_prepare_v2(self->priv->db,
	                                    "insert into usage (application, entry, timestamp) values (?" SQL_VARS_APPLICATION ", ?" SQL_VARS_ENTRY ", date('now', 'utc'));",
	                                    -1, /* length */
	                                    &(self->priv->insert_entry),
	                                    NULL); /* unused stmt */

	if (prepare_status != SQLITE_OK) {
		g_warning("Unable to prepare insert entry statement: %s", sqlite3_errmsg(self->priv->db));
		self->priv->insert_entry = NULL;
	}

	/* Entry Count Statement */
	prepare_status = sqlite3_prepare_v2(self->priv->db,
	                                    "select count(*) from usage where application = ?" SQL_VARS_APPLICATION " and entry = ?" SQL_VARS_ENTRY " and timestamp > date('now', 'utc', '-30 days');",
	                                    -1, /* length */
	                                    &(self->priv->entry_count),
	                                    NULL); /* unused stmt */

	if (prepare_status != SQLITE_OK) {
		g_warning("Unable to prepare entry count statement: %s", sqlite3_errmsg(self->priv->db));
		self->priv->entry_count = NULL;
	}

	/* Delete Aged Statement */
	prepare_status = sqlite3_prepare_v2(self->priv->db,
	                                    "delete from usage where timestamp < date('now', 'utc', '-30 days');",
	                                    -1, /* length */
	                                    &(self->priv->delete_aged),
	                                    NULL); /* unused stmt */

	if (prepare_status != SQLITE_OK) {
		g_warning("Unable to prepare delete aged statement: %s", sqlite3_errmsg(self->priv->db));
		self->priv->delete_aged = NULL;
	}

	/* Application Count Statement */
	prepare_status = sqlite3_prepare_v2(self->priv->db,
	                                    "select count(*) from usage where application = ?" SQL_VARS_APPLICATION ";",
	                                    -1, /* length */
	                                    &(self->priv->application_count),
	                                    NULL); /* unused stmt */

	if (prepare_status != SQLITE_OK) {
		g_warning("Unable to prepare application count statement: %s", sqlite3_errmsg(self->priv->db));
		self->priv->application_count = NULL;
	}

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

	sqlite3_bind_text(self->priv->entry_count, SQL_VAR_APPLICATION, application, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(self->priv->entry_count, SQL_VAR_ENTRY, entry, -1, SQLITE_TRANSIENT);

	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(self->priv->insert_entry)) == SQLITE_ROW) {
	}

	if (exec_status != SQLITE_DONE) {
		g_warning("Unknown status from executing insert_entry: %d", exec_status);
	}

	return;
}

guint
usage_tracker_get_usage (UsageTracker * self, const gchar * application, const gchar * entry)
{
	g_return_val_if_fail(IS_USAGE_TRACKER(self), 0);
	check_app_init(self, application);

	sqlite3_bind_text(self->priv->entry_count, SQL_VAR_APPLICATION, application, -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(self->priv->entry_count, SQL_VAR_ENTRY, entry, -1, SQLITE_TRANSIENT);

	int exec_status = SQLITE_ROW;
	guint count = 0;

	while ((exec_status = sqlite3_step(self->priv->entry_count)) == SQLITE_ROW) {
		count = sqlite3_column_int(self->priv->entry_count, 0);
	}

	if (exec_status != SQLITE_DONE) {
		g_warning("Unknown status from executing entry_count: %d", exec_status);
	}

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

	int exec_status = SQLITE_ROW;
	while ((exec_status = sqlite3_step(self->priv->delete_aged)) == SQLITE_ROW) {
	}

	if (exec_status != SQLITE_DONE) {
		g_warning("Unknown status from executing delete_aged: %d", exec_status);
	}

	return TRUE;
}

static void
check_app_init (UsageTracker * self, const gchar * application)
{
	sqlite3_bind_text(self->priv->entry_count, SQL_VAR_APPLICATION, application, -1, SQLITE_TRANSIENT);

	int exec_status = SQLITE_ROW;
	guint count = 0;

	while ((exec_status = sqlite3_step(self->priv->application_count)) == SQLITE_ROW) {
		count = sqlite3_column_int(self->priv->entry_count, 0);
	}

	if (exec_status != SQLITE_DONE) {
		g_warning("Unknown status from executing application_count: %d", exec_status);
	}

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
		g_warning("Unable to load application information for application '%s' at path '%s'", application, app_info);
	}

	g_free(app_info);
	g_free(app_info_filename);
	g_free(app_info_path);
	g_free(basename);

	return;
}
