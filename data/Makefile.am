gsettings_SCHEMAS = \
    com.canonical.indicator.appmenu.gschema.xml \
    com.canonical.indicator.appmenu.hud.gschema.xml \
    com.canonical.indicator.appmenu.hud.search.gschema.xml

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
