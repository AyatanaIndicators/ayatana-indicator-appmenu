
man_MANS = \
	hud-cli.1

.xml.1:
	@$(XSLTPROC) -nonet http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl $<

EXTRA_DIST = \
	$(man_MANS) \
	$(man_MANS:.1=.xml) 

CLEANFILES = \
	$(man_MANS)
