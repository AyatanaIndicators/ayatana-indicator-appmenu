
man_MANS = \
	hud-cli.1 \
	hud-dump-application.1 \
	hud-list-applications.1 \
	hud-verify-app-info.1

.xml.1:
	@env -u XML_CATALOG_FILES $(XSLT_PROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

EXTRA_DIST = \
	$(man_MANS) \
	$(man_MANS:.1=.xml) 

CLEANFILES = \
	$(man_MANS)
