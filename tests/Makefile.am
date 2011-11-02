DISTCLEANFILES =

TESTS = \
	test-distance-tests

check_PROGRAMS = \
	test-distance

######################
# Test Distance
######################

DISTANCE_XML_REPORT = test-distance.xml

test-distance-test: test-distance Makefile.am
	@echo "#!/bin/bash" > $@
	@echo $(XVFB_RUN) >> $@
	@echo gtester --verbose -k -o $(GTK_PARSER_XML_REPORT) ./test-distance >> $@
	@chmod +x $@

test_distance_SOURCES = \
	test-distance.c

test_distance_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror

test_distance_LDADD = \
	$(HUD_LIBS)

DISTCLEANFILES += $(DISTANCE_XML_REPORT)
