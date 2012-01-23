
gsettings_in_files = \
    com.canonical.indicator.appmenu.gschema.xml.in \
    com.canonical.indicator.appmenu.hud.gschema.xml.in \
    com.canonical.indicator.appmenu.hud.search.gschema.xml.in

gsettings_SCHEMAS = \
	$(gsettings_in_files:.xml.in=.xml)

@INTLTOOL_XML_NOMERGE_RULE@
@GSETTINGS_RULES@

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = com.canonical.hud.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
	$(dbus_services_DATA:.service=.service.in) \
	$(gsettings_in_files)

CLEANFILES = \
	$(dbus_services_DATA) \
	$(gsettings_SCHEMAS:.xml=.valid) \
	$(gsettings_SCHEMAS)

