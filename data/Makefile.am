
gsettings_SCHEMAS = \
    com.canonical.indicator.appmenu.gschema.xml

@GSETTINGS_RULES@

dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = com.canonical.hud.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = \
	$(dbus_services_DATA:.service=.service.in) \
	$(gsettings_SCHEMAS)

CLEANFILES = \
	$(dbus_services_DATA) \
	$(gsettings_SCHEMAS:.xml=.valid)

