DISTCLEANFILES =
EXTRA_DIST =

TESTS = \
	test-bad-app-info-dual-header \
	test-distance-test \
	test-usage-db-ancient-test \
	test-usage-db-old-test \
	test-usage-db-simple-test \
	test-indicator-tracker-test

check_PROGRAMS = \
	test-distance \
	test-usage-db-ancient \
	test-usage-db-old \
	test-usage-db-simple \
	test-indicator-tracker \
	test-indicator-tracker-owner

XFAIL_TESTS = \
	test-bad-app-info-dual-header


DISTCLEANFILES += $(TESTS)

DBUS_RUNNER=dbus-test-runner

######################
# Test Distance
######################

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
	-Wall -Werror

test_usage_db_simple_LDADD = \
	$(HUD_LIBS)

DISTCLEANFILES += $(USAGE_DB_SIMPLE_XML_REPORT)
EXTRA_DIST += test-usage-db-simple.sql

######################
# Test Usage DB Old
######################

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
	-Wall -Werror

test_usage_db_old_LDADD = \
	$(HUD_LIBS)

DISTCLEANFILES += $(USAGE_DB_OLD_XML_REPORT)
EXTRA_DIST += test-usage-db-old.sql

#######################
# Test Usage DB Ancient
#######################

test-usage-db-ancient-test: test-usage-db-ancient test-usage-db-ancient.sql test-usage-db-ancient-test.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo cd \`mktemp -d $(abs_builddir)/test-usage-db-ancient.dir.XXXXXX\` >> $@
	@echo mkdir -p hud >> $@
	@echo cat $(abs_srcdir)/test-usage-db-ancient.sql \| sqlite3 hud/usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=\`pwd\` >> $@
	@echo $(abs_builddir)/test-usage-db-ancient >> $@
	@echo if test \`cat $(abs_srcdir)/test-usage-db-ancient-test.sql \| sqlite3 hud/usage-log.sqlite \| wc -l\` -gt 0\; then >> $@
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
	-Wall -Werror

test_usage_db_ancient_LDADD = \
	$(HUD_LIBS)

EXTRA_DIST += \
	test-usage-db-ancient.sql \
	test-usage-db-ancient-test.sql

#########################
# Test Indicator Tracker
#########################

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

test-bad-app-info-dual-header: bad-app-info/dual-headers.hud-app-info Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo $(top_builddir)/service/hud-verify-app-info $(srcdir)/bad-app-info/dual-headers.hud-app-info >> $@
	@chmod +x $@

EXTRA_DIST += \
	bad-app-info/dual-headers.hud-app-info
