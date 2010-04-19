
######################################
# Building the indicator
######################################

appmenulibdir = $(INDICATORDIR)
appmenulib_LTLIBRARIES = libappmenu.la
libappmenu_la_SOURCES = \
	indicator-appmenu.c
libappmenu_la_CFLAGS = $(APPLET_CFLAGS) -Wall -Wl,-Bsymbolic-functions -Wl,-z,defs -Wl,--as-needed -Werror
libappmenu_la_LIBADD = $(APPLET_LIBS)
libappmenu_la_LDFLAGS = -module -avoid-version

