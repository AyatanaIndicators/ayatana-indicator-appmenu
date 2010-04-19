CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

######################################
# Building the indicator
######################################

appmenulibdir = $(INDICATORDIR)
appmenulib_LTLIBRARIES = libappmenu.la
libappmenu_la_SOURCES = \
	application-menu-registrar-server.h \
	indicator-appmenu.c
libappmenu_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
libappmenu_la_LIBADD = $(APPLET_LIBS)
libappmenu_la_LDFLAGS = -module -avoid-version

######################################
# DBus Love
######################################

DBUS_SPECS = \
	application-menu-registrar.xml

%-client.h: %.xml
	dbus-binding-tool \
		--prefix=_$(notdir $(subst -,_,$(<:.xml=)))_client \
		--mode=glib-client \
		--output=$@ \
		$<

%-server.h: %.xml
	dbus-binding-tool \
		--prefix=_$(notdir $(subst -,_,$(<:.xml=)))_server \
		--mode=glib-server \
		--output=$@ \
		$<

BUILT_SOURCES += \
	$(DBUS_SPECS:.xml=-client.h) \
	$(DBUS_SPECS:.xml=-server.h)

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST += $(DBUS_SPECS)
