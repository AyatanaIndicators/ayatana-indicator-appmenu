
AC_INIT(indicator-appmenu, 0.0.10, ted@canonical.com)
AC_COPYRIGHT([Copyright 2010 Canonical])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(indicator-appmenu, 0.0.10)

AM_MAINTAINER_MODE

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_C_O
AC_STDC_HEADERS
AC_PROG_LIBTOOL

AC_SUBST(VERSION)
AC_CONFIG_MACRO_DIR([m4])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])

PKG_PROG_PKG_CONFIG

###########################
# GTK Doc
###########################

GTK_DOC_CHECK([1.9])
AC_CONFIG_MACRO_DIR(m4)


###########################
# Dependencies
###########################

GTK_REQUIRED_VERSION=2.12
INDICATOR_REQUIRED_VERSION=0.3.5
DBUSMENUGTK_REQUIRED_VERSION=0.3.3
DBUS_GLIB_REQUIRED_VERSION=0.82
BAMF_REQUIRED_VERSION=0.2.47

PKG_CHECK_MODULES(INDICATOR, gtk+-2.0 >= $GTK_REQUIRED_VERSION
                             indicator >= $INDICATOR_REQUIRED_VERSION
                             dbus-glib-1 >= $DBUS_GLIB_REQUIRED_VERSION
                             dbusmenu-gtk >= $DBUSMENUGTK_REQUIRED_VERSION
                             libbamf >= $BAMF_REQUIRED_VERSION)

AC_SUBST(INDICATOR_CFLAGS)
AC_SUBST(INDICATOR_LIBS)

###########################
# Test Dependencies
###########################

DBUSMENU_JSONLOADER_REQUIRED_VERSION=0.3.3

PKG_CHECK_MODULES(INDICATORTEST,
                  dbusmenu-jsonloader >= $DBUSMENU_JSONLOADER_REQUIRED_VERSION
                 )

AC_SUBST(INDICATORTEST_CFLAGS)
AC_SUBST(INDICATORTEST_LIBS)

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE(localinstall, AS_HELP_STRING([--enable-localinstall], [install all of the files localy instead of system directories (for distcheck)]), with_localinstall=$enableval, with_localinstall=no)

###########################
# Indicator Info
###########################

if test "x$with_localinstall" = "xyes"; then
	INDICATORDIR="${libdir}/indicators/3/"
	INDICATORICONSDIR="${datadir}/libindicate/icons/"
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator`
fi
AC_SUBST(INDICATORDIR)
AC_SUBST(INDICATORICONSDIR)

###########################
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
scripts/Makefile
tools/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

App Menu Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
])
