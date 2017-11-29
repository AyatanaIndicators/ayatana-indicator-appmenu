libexec_SCRIPTS = \
	ayatana-appmenu-current-menu \
	ayatana-appmenu-current-menu-dump

libexec_PROGRAMS = \
	ayatana-appmenu-mock-json-app

ayatana-appmenu-current-menu-dump: ayatana-appmenu-current-menu-dump.in
	sed \
		-e s:@LIBEXECDIR@:$(libexecdir):g \
		$< > $@
	chmod +x $@

ayatana_appmenu_mock_json_app_SOURCES = \
	mock-json-app.c
ayatana_appmenu_mock_json_app_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	$(INDICATORTEST_CFLAGS) \
	-Wall -Werror -Wno-error=deprecated-declarations
ayatana_appmenu_mock_json_app_LDADD = \
	$(INDICATOR_LIBS) \
	$(INDICATORTEST_LIBS)

EXTRA_DIST = \
	ayatana-appmenu-current-menu \
	ayatana-appmenu-current-menu-dump.in

CLEANFILES = \
	ayatana-appmenu-current-menu-dump
