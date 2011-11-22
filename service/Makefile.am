
BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =

libexec_PROGRAMS = \
	hud-cli \
	hud-dump-application \
	hud-service

libexec_SCRIPTS = \
	hud-list-applications

####################
# HUD Service
####################

hud_service_SOURCES = \
	dbusmenu-collector.h \
	dbusmenu-collector.c \
	distance.h \
	distance.c \
	dump-app-info.h \
	dump-app-info.c \
	shared-values.h \
	hud-service.c \
	hud-dbus.h \
	hud-dbus.c \
	hud-search.h \
	hud-search.c \
	hud.interface.h \
	hud.interface.c \
	indicator-tracker.h \
	indicator-tracker.c \
	load-app-info.h \
	load-app-info.c \
	usage-tracker.h \
	usage-tracker.c

hud_service_CFLAGS = \
	$(HUD_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-Wall -Werror
hud_service_LDFLAGS = \
	$(HUD_LIBS)

####################
# HUD CLI
####################

hud_cli_SOURCES = \
	shared-values.h \
	hud-cli.c \
	hud.interface.h \
	hud.interface.c

hud_cli_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror
hud_cli_LDFLAGS = \
	$(HUD_LIBS)

####################
# HUD CLI
####################

hud_dump_application_SOURCES = \
	dump-app-info.h \
	dump-app-info.c \
	hud-dump-application.c

hud_dump_application_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror
hud_dump_application_LDFLAGS = \
	$(HUD_LIBS)

####################
# DBus Interface
####################

EXTRA_DIST += \
	hud.xml

BUILT_SOURCES += \
	hud.interface.c \
	hud.interface.h

%.interface.h: %.xml
	echo "extern const char * $(subst -,_,$(subst .,_,$(basename $(notdir $@))));" > $@

%.interface.c: %.xml
	echo "const char * $(subst -,_,$(subst .,_,$(basename $(notdir $@)))) = " > $@
	sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	echo ";" >> $@


CLEANFILES += $(BUILT_SOURCES)
