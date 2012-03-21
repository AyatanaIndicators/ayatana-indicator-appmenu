CLEANFILES =
DISTCLEANFILES =
BUILT_SOURCES =
EXTRA_DIST = \
	hud-list-applications

bin_PROGRAMS = \
	hud-dump-application \
	hud-verify-app-info \
	hud-gtk

bin_SCRIPTS = \
	hud-list-applications

libexec_PROGRAMS = \
	hud-service

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
libappmenu_la_CFLAGS = \
	$(INDICATOR_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror -DG_LOG_DOMAIN=\"Indicator-Appmenu\"
libappmenu_la_LIBADD = $(INDICATOR_LIBS) -lX11
libappmenu_la_LDFLAGS = \
	$(COVERAGE_CFLAGS) \
	-module -avoid-version

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

####################
# HUD Service
####################

noinst_LIBRARIES = libhud-service.a

libhud_service_a_SOURCES = \
	create-db.h \
	create-db.c \
	distance.h \
	distance.c \
	dump-app-info.h \
	dump-app-info.c \
	shared-values.h \
	hudsettings.h		\
	hudsettings.c		\
	hudappmenuregistrar.h	\
	hudappmenuregistrar.c	\
	huddbusmenucollector.h	\
	huddbusmenucollector.c	\
	hudmenumodelcollector.h	\
	hudmenumodelcollector.c	\
	hudappindicatorsource.h	\
	hudappindicatorsource.c	\
	hudwebappsource.h \
	hudwebappsource.c \
	hudindicatorsource.h	\
	hudindicatorsource.c	\
	hudwindowsource.h	\
	hudwindowsource.c	\
	huddebugsource.h	\
	huddebugsource.c	\
	hudsourcelist.h		\
	hudsourcelist.c		\
	hudsource.h		\
	hudsource.c		\
	hudquery.h		\
	hudquery.c		\
	hudstringlist.h		\
	hudstringlist.c		\
	huditem.h		\
	huditem.c		\
	hudresult.h		\
	hudresult.c		\
	hud.interface.h \
	hud.interface.c \
	load-app-info.h \
	load-app-info.c \
	usage-tracker.h \
	usage-tracker.c

hud_service_SOURCES = hud-service.c

libhud_service_a_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-Wall -Werror
hud_service_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-DDATADIR=\""$(datadir)"\" \
	-Wall -Werror
hud_service_LDADD = \
	libhud-service.a	\
	$(HUD_LIBS)

create-db.c: create-db.sql
	echo "#include \"create-db.h\"" > $@
	echo -n "const gchar * create_db = \"" >> $@
	cat $< | xargs printf "%s " >> $@
	echo -n "\";" >> $@

CLEANFILES += \
	create-db.c


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
	$(COVERAGE_CFLAGS) \
	-Wall -Werror

hud_cli_LDFLAGS = \
	$(HUD_LIBS) \
	-lreadline -lcurses


if BUILD_CLI
bin_PROGRAMS += \
	hud-cli
else
EXTRA_DIST += \
	$(hud_cli_SOURCES)
endif

####################
# hud-gtk
####################

hudgtkdatadir = $(datadir)/hud-gtk
hudgtkdata_DATA = hud-gtk.ui
EXTRA_DIST += hud-gtk.ui

hud_gtk_SOURCES = \
	hud-gtk.vala

hud_gtk_VALAFLAGS = --pkg gtk+-3.0
hud_gtk_CFLAGS = \
	-DHUD_GTK_DATADIR=\""$(hudgtkdatadir)"\"	\
	$(gtk_CFLAGS)

hud_gtk_LDADD = \
	$(gtk_LIBS)

#######################
# HUD Dump Application
#######################

hud_dump_application_SOURCES = \
	dump-app-info.h \
	dump-app-info.c \
	hud-dump-application.c

hud_dump_application_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall -Werror
hud_dump_application_LDFLAGS = \
	$(HUD_LIBS)

#######################
# HUD Verify App Info
#######################

hud_verify_app_info_SOURCES = \
	create-db.h \
	create-db.c \
	load-app-info.h \
	load-app-info.c \
	hud-verify-app-info.c

hud_verify_app_info_CFLAGS = \
	$(HUD_CFLAGS) \
	$(COVERAGE_CFLAGS) \
	-Wall -Werror
hud_verify_app_info_LDFLAGS = \
	$(HUD_LIBS)

####################
# HUD DBus Interface
####################

EXTRA_DIST += \
	hud.xml \
	create-db.sql

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

EXTRA_DIST += $(DBUS_SPECS) clean-namespaces.xslt
