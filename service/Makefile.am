
BUILT_SOURCES =
CLEANFILES =
DISTCLEANFILES =
EXTRA_DIST =

libexec_PROGRAMS = \
	hud-cli


####################
# HUD CLI
####################

hud_cli_SOURCES = \
	shared-values.h \
	hud-cli.c \
	hud.xml.h \
	hud.xml.c

hud_cli_CFLAGS = \
	$(HUD_CFLAGS) \
	-Wall -Werror
hud_cli_LDFLAGS = \
	$(HUD_LIBS)

####################
# DBus Interface
####################

EXTRA_DIST += \
	hud.xml

BUILT_SOURCES += \
	hud.xml.c \
	hud.xml.h

%.xml.h: %.xml
	echo "extern const char * $(subst -,_,$(subst .,_,$(basename $(notdir $@))));" > $@

%.xml.c: %.xml
	echo "const char * $(subst -,_,$(subst .,_,$(basename $(notdir $@)))) = " > $@
	sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": $< >> $@
	echo ";" >> $@


CLEANFILES += $(BUILT_SOURCES)
