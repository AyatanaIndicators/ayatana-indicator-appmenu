CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST =

include $(top_srcdir)/Makefile.am.marshal

######################################
# Building the indicator
######################################

appmenulibdir = $(INDICATORDIR)
appmenulib_LTLIBRARIES = libappmenu.la
libappmenu_la_SOURCES = \
	application-menu-registrar-server.h \
	dbus-shared.h \
	gdk-get-func.h \
	gdk-get-func.c \
	MwmUtil.h \
	indicator-appmenu.c \
	indicator-appmenu-marshal.c \
	window-menus.c \
	window-menus.h
libappmenu_la_CFLAGS = $(INDICATOR_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
libappmenu_la_LIBADD = $(INDICATOR_LIBS) -lX11
libappmenu_la_LDFLAGS = -module -avoid-version

######################################
# Build your own marshaller
######################################

glib_marshal_list = indicator-appmenu-marshal.list
glib_marshal_prefix = _indicator_appmenu_marshal

######################################
# DBus Love
######################################

DBUS_SPECS = \
	application-menu-renderer.xml \
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
