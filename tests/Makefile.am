DISTCLEANFILES =
EXTRA_DIST =

TESTS =
check_PROGRAMS =
XFAIL_TESTS =

DBUS_RUNNER=dbus-test-runner
XVFB_RUN=". $(srcdir)/run-xvfb.sh"

EXTRA_DIST += \
	run-xvfb.sh

SUBDIRS = \
	manual

######################
# Test Distance
######################

TESTS += \
	test-distance-test

check_PROGRAMS += \
	test-distance

DISTANCE_XML_REPORT = test-distance.xml

test-distance-test: test-distance Makefile.am
	@echo "#!/bin/bash" > $@
	@echo $(XVFB_RUN) >> $@
	@echo gtester --verbose -k -o $(DISTANCE_XML_REPORT) $(builddir)/test-distance >> $@
	@chmod +x $@

test_distance_SOURCES = \
	test-distance.c

test_distance_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_distance_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

DISTCLEANFILES += $(DISTANCE_XML_REPORT)

######################
# Test Usage DB Simple
######################

TESTS += \
	test-usage-db-simple-test

check_PROGRAMS += \
	test-usage-db-simple

USAGE_DB_SIMPLE_XML_REPORT = test-usage-db-simple.xml

test-usage-db-simple-test: test-usage-db-simple test-usage-db-simple.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-simple.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo cat $(abs_srcdir)/test-usage-db-simple.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo export GSETTINGS_BACKEND=memory >> $@
	@echo export GSETTINGS_SCHEMA_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(abs_builddir)/$(USAGE_DB_SIMPLE_XML_REPORT) $(abs_builddir)/test-usage-db-simple >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-simple.dir.\* >> $@
	@chmod +x $@

test_usage_db_simple_SOURCES = \
	test-usage-db-simple.c

test_usage_db_simple_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_usage_db_simple_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

DISTCLEANFILES += $(USAGE_DB_SIMPLE_XML_REPORT)
EXTRA_DIST += test-usage-db-simple.sql

######################
# Test Usage DB Old
######################

TESTS += \
	test-usage-db-old-test

check_PROGRAMS += \
	test-usage-db-old

USAGE_DB_OLD_XML_REPORT = test-usage-db-old.xml

test-usage-db-old-test: test-usage-db-old test-usage-db-old.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-old.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo cat $(abs_srcdir)/test-usage-db-old.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo export GSETTINGS_BACKEND=memory >> $@
	@echo export GSETTINGS_SCHEMA_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(abs_builddir)/$(USAGE_DB_SIMPLE_XML_REPORT) $(abs_builddir)/test-usage-db-old >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-old.dir.\* >> $@
	@chmod +x $@

test_usage_db_old_SOURCES = \
	test-usage-db-old.c

test_usage_db_old_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_usage_db_old_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

DISTCLEANFILES += $(USAGE_DB_OLD_XML_REPORT)
EXTRA_DIST += test-usage-db-old.sql

#######################
# Test Usage DB Ancient
#######################

TESTS += \
	test-usage-db-ancient-test

check_PROGRAMS += \
	test-usage-db-ancient

test-usage-db-ancient-test: test-usage-db-ancient test-usage-db-ancient.sql test-usage-dump-entries.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-ancient.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo cat $(abs_srcdir)/test-usage-db-ancient.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo export GSETTINGS_BACKEND=memory >> $@
	@echo export GSETTINGS_SCHEMA_DIR=\`pwd\` >> $@
	@echo $(abs_builddir)/test-usage-db-ancient >> $@
	@echo if test \`cat $(abs_srcdir)/test-usage-dump-entries.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite \| wc -l\` -gt 0\; then >> $@
	@echo echo Database has entries >> $@
	@echo exit 1 >> $@
	@echo fi >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-ancient.dir.\* >> $@
	@chmod +x $@

test_usage_db_ancient_SOURCES = \
	test-usage-db-ancient.c

test_usage_db_ancient_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_usage_db_ancient_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

EXTRA_DIST += \
	test-usage-db-ancient.sql \
	test-usage-dump-entries.sql

#######################
# Test Usage DB Testapp
#######################

TESTS += \
	test-usage-db-testapp-test

check_PROGRAMS += \
	test-usage-db-testapp

USAGE_TESTAPP_XML_REPORT = test-usage-db-testapp.xml

test-usage-db-testapp-test: test-usage-db-testapp good-app-info/testapp.desktop.hud-app-info good-app-info/testapp100.desktop.hud-app-info $(top_srcdir)/src/create-db.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-testapp.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo cat $(abs_top_srcdir)/src/create-db.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo export HUD_APP_INFO_DIR=$(abs_srcdir)/good-app-info >> $@
	@echo export GSETTINGS_BACKEND=memory >> $@
	@echo export GSETTINGS_SCHEMA_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(abs_builddir)/$(USAGE_TESTAPP_XML_REPORT) $(abs_builddir)/test-usage-db-testapp >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-testapp.dir.\* >> $@
	@chmod +x $@

test_usage_db_testapp_SOURCES = \
	test-usage-db-testapp.c

test_usage_db_testapp_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_usage_db_testapp_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

EXTRA_DIST += \
	good-app-info/testapp100.desktop.hud-app-info \
	good-app-info/testapp.desktop.hud-app-info

DISTCLEANFILES += $(USAGE_TESTAPP_XML_REPORT)

######################
# Test Usage DB Off
######################

TESTS += \
	test-usage-db-off-test

USAGE_OFF_XML_REPORT = $(abs_builddir)/test-usage-db-off.xml

test-usage-db-off-test: test-usage-db-testapp good-app-info/testapp.desktop.hud-app-info good-app-info/testapp100.desktop.hud-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-off.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo export HUD_APP_INFO_DIR=$(abs_srcdir)/good-app-info >> $@
	@echo export HUD_NO_STORE_USAGE_DATA=1 >> $@
	@echo export GSETTINGS_BACKEND=memory >> $@
	@echo export GSETTINGS_SCHEMA_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(abs_buildir)/$(USAGE_OFF_XML_REPORT) $(abs_builddir)/test-usage-db-testapp >> $@
	@echo test \! -e indicator-appmenu/hud-usage-log.sqlite >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-off.dir.\* >> $@
	@chmod +x $@

DISTCLEANFILES += $(USAGE_OFF_XML_REPORT)

#########################
# Test Bad App Info
#########################

TESTS += \
	test-bad-app-info-dual-header \
	test-bad-app-info-item-no-count \
	test-bad-app-info-item-no-name \
	test-bad-app-info-menu-no-name \
	test-bad-app-info-missing-desktop \
	test-bad-app-info-missing-menus \
	test-bad-app-info-multiple-menus

test-bad-app-info-dual-header: bad-app-info/dual-headers.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/dual-headers.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-item-no-count: bad-app-info/item-no-count.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/item-no-count.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-item-no-name: bad-app-info/item-no-name.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/item-no-name.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-menu-no-name: bad-app-info/menu-no-name.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/menu-no-name.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-missing-menus: bad-app-info/missing-menus.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/missing-menus.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-missing-desktop: bad-app-info/missing-desktop.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/missing-desktop.hud-app-info >> $@
	@chmod +x $@

test-bad-app-info-multiple-menus: bad-app-info/multiple-menus.hud-app-info test-bad-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(builddir)/test-bad-app-info $(srcdir)/bad-app-info/multiple-menus.hud-app-info >> $@
	@chmod +x $@

EXTRA_DIST += \
	bad-app-info/dual-headers.hud-app-info \
	bad-app-info/item-no-count.hud-app-info \
	bad-app-info/item-no-name.hud-app-info \
	bad-app-info/menu-no-name.hud-app-info \
	bad-app-info/missing-desktop.hud-app-info \
	bad-app-info/missing-menus.hud-app-info \
	bad-app-info/multiple-menus.hud-app-info

check_PROGRAMS += \
	test-bad-app-info

test_bad_app_info_SOURCES = \
	test-bad-app-info.c

test_bad_app_info_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_bad_app_info_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

#########################
# Test Good App Info
#########################

TESTS += \
	test-good-app-info-tons-of-entries

check_PROGRAMS += \
	test-load-app-info

test_load_app_info_SOURCES = \
	test-load-app-info.c

test_load_app_info_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src \
	-Wall -Werror

test_load_app_info_LDADD = \
	../src/libhud-service.a		\
	$(HUD_LIBS)

test-good-app-info-tons-of-entries: good-app-info/tons-of-entries.hud-app-info test-load-app-info test-usage-dump-entries.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-good-app-info-tons-of-entries.dir.XXXXXX\` >> $@
	@echo mkdir -p indicator-appmenu >> $@
	@echo $(abs_builddir)/test-load-app-info indicator-appmenu/hud-usage-log.sqlite $(abs_srcdir)/good-app-info/tons-of-entries.hud-app-info >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo if test \`cat $(abs_srcdir)/test-usage-dump-entries.sql \| sqlite3 indicator-appmenu/hud-usage-log.sqlite \| wc -l\` -ne 540\; then >> $@
	@echo echo Database has wrong number of entries >> $@
	@echo exit 1 >> $@
	@echo fi >> $@
	@echo cd .. >> $@
	@echo rm -rf test-good-app-info-tons-of-entries.dir.\* >> $@
	@chmod +x $@

EXTRA_DIST += \
	good-app-info/tons-of-entries.hud-app-info \
	test-usage-dump-entries.sql

#########################
# Test Dbus Messages
#########################

#TESTS += \
#	test-dbus-message-count

test-dbus-message-count: test-dbus-message-count.in
	@sed \
		-e "s|\@top_builddir\@|$(top_builddir)|" \
		-e "s|\@builddir\@|$(builddir)|" \
		-e "s|\@srcdir\@|$(srcdir)|" \
		$< > $@
	@chmod +x $@

EXTRA_DIST += \
	test-dbus-message-count-send-query \
	test-dbus-message-count.in \
	test-dbus-message-count.json

DISTCLEANFILES += test-dbus-message-count.bustle

#########################
# Footer
#########################

DISTCLEANFILES += $(TESTS)
