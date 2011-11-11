DISTCLEANFILES =
EXTRA_DIST =

TESTS = \
	test-distance-test \
	test-usage-db-simple-test

check_PROGRAMS = \
	test-distance \
	test-usage-db-simple

DISTCLEANFILES += $(TESTS)

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
# Test Usage DB
######################

USAGE_DB_SIMPLE_XML_REPORT = test-usage-db-simple.xml
DOLLAR_SIGN=

test-usage-db-simple-test: test-usage-db-simple test-usage-db-simple.sql Makefile.am
	@echo "#!/bin/bash -e" > $@
	@echo TEMP_DIR=\`mktemp -u $(builddir)/test-usage-db-simple.XXXXXX\` >> $@
	@echo mkdir -p $${TEMP_DIR}/hud >> $@
	@echo cat $(srcdir)/test-usage-db-simple.sql \| sqlite3 $${TEMP_DIR}/hud/usage-log.sqlite >> $@
	@echo export HUD_CACHE_DIR=$${TEMP_DIR} >> $@
	@echo gtester --verbose -k -o $(USAGE_DB_SIMPLE_XML_REPORT) $(builddir)/test-usage-db-simple >> $@
	@echo rm -rf $${TEMP_DIR} >> $@
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


