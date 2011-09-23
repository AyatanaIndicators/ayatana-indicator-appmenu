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
	dbus-shared.h \
	gdk-get-func.h \
	gdk-get-func.c \
	MwmUtil.h \
	indicator-appmenu.c \
	indicator-appmenu-marshal.c \
	window-menus.c \
	window-menus.h \
	gen-application-menu-renderer.xml.c \
	gen-application-menu-renderer.xml.h \
	gen-application-menu-registrar.xml.c \
	gen-application-menu-registrar.xml.h
libappmenu_la_CFLAGS = $(INDICATOR_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror -DG_LOG_DOMAIN=\"Indicator-Appmenu\"
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

gen-%.xml.c: %.xml
	@echo "Building $@ from $<"
	@echo "const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<)))) = " > $@
	@$(XSLT_PROC) $(srcdir)/clean-namespaces.xslt $< | \
		sed -e "s:\":\\\\\":g" -e s:^:\": -e s:\$$:\\\\n\": >> $@
	@echo ";" >> $@

gen-%.xml.h: %.xml
	@echo "Building $@ from $<"
	@echo "extern const char * _$(subst -,_,$(subst .,_,$(basename $(notdir $<))));" > $@

BUILT_SOURCES += \
	gen-application-menu-renderer.xml.c \
	gen-application-menu-renderer.xml.h \
	gen-application-menu-registrar.xml.c \
	gen-application-menu-registrar.xml.h

CLEANFILES += $(BUILT_SOURCES)

EXTRA_DIST += $(DBUS_SPECS) clean-namespaces.xslt
