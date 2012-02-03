
libexec_SCRIPTS = \
	current-menu \
	current-menu-dump

libexec_PROGRAMS = \
	mock-json-app

current-menu-dump: current-menu-dump.in
	sed \
		-e s:@LIBEXECDIR@:$(libexecdir):g \
		$< > $@
	chmod +x $@

mock_json_app_SOURCES = \
	mock-json-app.c
mock_json_app_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	$(INDICATORTEST_CFLAGS) \
	-Wall -Werror
mock_json_app_LDADD = \
	$(INDICATOR_LIBS) \
	$(INDICATORTEST_LIBS)

EXTRA_DIST = \
	current-menu \
	current-menu-dump.in

CLEANFILES = \
	current-menu-dump
