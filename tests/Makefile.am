DISTCLEANFILES =
EXTRA_DIST =

TESTS =
check_PROGRAMS =
XFAIL_TESTS =

DBUS_RUNNER=dbus-test-runner

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
	-Wall -Werror

test_distance_LDADD = \
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
	@echo mkdir -p hud >> $@
	@echo cat $(abs_srcdir)/test-usage-db-simple.sql \| sqlite3 hud/usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(USAGE_DB_SIMPLE_XML_REPORT) $(abs_builddir)/test-usage-db-simple >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-simple.dir.\* >> $@
	@chmod +x $@

test_usage_db_simple_SOURCES = \
	test-usage-db-simple.c

test_usage_db_simple_CFLAGS = \
	$(HUD_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/service \
	-Wall -Werror

test_usage_db_simple_LDADD = \
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
	@echo mkdir -p hud >> $@
	@echo cat $(abs_srcdir)/test-usage-db-old.sql \| sqlite3 hud/usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo gtester --verbose -k -o $(USAGE_DB_SIMPLE_XML_REPORT) $(abs_builddir)/test-usage-db-old >> $@
	@echo cd .. >> $@
	@echo rm -rf test-usage-db-old.dir.\* >> $@
	@chmod +x $@

test_usage_db_old_SOURCES = \
	test-usage-db-old.c

test_usage_db_old_CFLAGS = \
	$(HUD_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/service \
	-Wall -Werror

test_usage_db_old_LDADD = \
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
	@echo mkdir -p hud >> $@
	@echo cat $(abs_srcdir)/test-usage-db-ancient.sql \| sqlite3 hud/usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo $(abs_builddir)/test-usage-db-ancient >> $@
	@echo if test \`cat $(abs_srcdir)/test-usage-dump-entries.sql \| sqlite3 hud/usage-log.sqlite \| wc -l\` -gt 0\; then >> $@
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
	-DDATADIR=\""$(datadir)"\" \
	-I$(top_srcdir)/service \
	-Wall -Werror

test_usage_db_ancient_LDADD = \
	$(HUD_LIBS)

EXTRA_DIST += \
	test-usage-db-ancient.sql \
	test-usage-dump-entries.sql

#########################
# Test Indicator Tracker
#########################

TESTS += \
	test-indicator-tracker-test

check_PROGRAMS += \
	test-indicator-tracker \
	test-indicator-tracker-owner

test-indicator-tracker-test: test-indicator-tracker test-indicator-tracker-owner Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(XVFB_RUN) >> $@
	@echo $(DBUS_RUNNER) --task $(builddir)/test-indicator-tracker --task-name Tracker --task $(builddir)/test-indicator-tracker-owner --parameter com.canonical.indicator.messages --task-name Messages --ignore-return --task $(builddir)/test-indicator-tracker-owner --parameter com.canonical.indicators.sound --task-name Sound --ignore-return >> $@
	@chmod +x $@

test_indicator_tracker_SOURCES = \
	test-indicator-tracker.c

test_indicator_tracker_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror

test_indicator_tracker_LDADD = \
	$(HUD_LIBS)

test_indicator_tracker_owner_SOURCES = \
	test-indicator-tracker-owner.c

test_indicator_tracker_owner_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror

test_indicator_tracker_owner_LDADD = \
	$(HUD_LIBS)

#########################
# Test Bad App Info
#########################

TESTS += \
	test-bad-app-info-dual-header \
	test-bad-app-info-item-no-count \
	test-bad-app-info-item-no-name \
	test-bad-app-info-menu-no-name \
	test-bad-app-info-missing-desktop \
	test-bad-app-info-missing-menus

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

EXTRA_DIST += \
	bad-app-info/dual-headers.hud-app-info \
	bad-app-info/missing-destkop.hud-app-info \
	bad-app-info/missing-menus.hud-app-info

check_PROGRAMS += \
	test-bad-app-info

test_bad_app_info_SOURCES = \
	test-bad-app-info.c

test_bad_app_info_CFLAGS = \
	$(HUD_CFLAGS) \
	-I$(top_srcdir)/service \
	-Wall -Werror

test_bad_app_info_LDADD = \
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
	-I$(top_srcdir)/service \
	-Wall -Werror

test_load_app_info_LDADD = \
	$(HUD_LIBS)

test-good-app-info-tons-of-entries: good-app-info/tons-of-entries.hud-app-info test-load-app-info test-usage-dump-entries.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-good-app-info-tons-of-entries.dir.XXXXXX\` >> $@
	@echo mkdir -p hud >> $@
	@echo $(abs_builddir)/test-load-app-info hud/usage-log.sqlite $(abs_srcdir)/good-app-info/tons-of-entries.hud-app-info >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo if test \`cat $(abs_srcdir)/test-usage-dump-entries.sql \| sqlite3 hud/usage-log.sqlite \| wc -l\` -ne 540\; then >> $@
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
# Footer
#########################

DISTCLEANFILES += $(TESTS)
