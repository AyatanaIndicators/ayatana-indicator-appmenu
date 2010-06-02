
libexec_SCRIPTS = \
	current-menu \
	current-menu-dump

current-menu-dump: current-menu-dump.in
	sed \
		-e s:@LIBEXECDIR@:$(libexecdir):g \
		$< > $@

EXTRA_DIST = \
	current-menu \
	current-menu-dump.in
