
dbus_servicesdir = $(DBUSSERVICEDIR)
dbus_services_DATA = com.canonical.hud.service

%.service: %.service.in
	sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

EXTRA_DIST = com.canonical.hud.service.in

CLEANFILES = com.canonical.hud.service
