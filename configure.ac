AC_INIT([indicator-appmenu],
        [0.3.94],
        [http://bugs.launchpad.net/indicator-appmenu],
        [indicator-appmenu],
        [http://launchpad.net/indicator-appmenu])

AC_COPYRIGHT([Copyright 2010 Canonical])

AC_PREREQ(2.53)

AM_CONFIG_HEADER(config.h)
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE([1.11 -Wall foreign subdir-objects])
AM_MAINTAINER_MODE([enable])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AM_PROG_CC_C_O

# Initialize libtool
LT_PREREQ([2.2.6])
LT_INIT

AC_PATH_PROG([GLIB_MKENUMS], [glib-mkenums])
AC_PATH_PROG([GLIB_GENMARSHAL], [glib-genmarshal])

PKG_PROG_PKG_CONFIG

AC_ARG_ENABLE([deprecations],
  [AS_HELP_STRING([--enable-deprecations],
    [allow deprecated API usage @<:@default=yes@:>@])],
  [],
  [enable_deprecations=yes])
AS_IF([test "x$enable_deprecations" = xno],
  [CFLAGS="$CFLAGS -DG_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGDK_PIXBUF_DISABLE_DEPRECATED -DGTK_DISABLE_DEPRECATED -DGSEAL_ENABLE -DGTK_DISABLE_SINGLE_INCLUDES"]
)

###########################
# GTK Doc
###########################

GTK_DOC_CHECK([1.9], [--flavour=no-tmpl])
AC_CONFIG_MACRO_DIR(m4)

###########################
# GLib GSettings
###########################

GLIB_GSETTINGS

###########################
# Dependencies
###########################

GTK_REQUIRED_VERSION=2.12
GTK3_REQUIRED_VERSION=3.0
GIO_REQUIRED_VERSION=2.26
INDICATOR_REQUIRED_VERSION=0.3.90
DBUSMENUGTK_REQUIRED_VERSION=0.5.90
BAMF_REQUIRED_VERSION=0.2.53

AC_ARG_WITH([gtk],
  [AS_HELP_STRING([--with-gtk],
    [Which version of gtk to use @<:@default=3@:>@])],
  [],
  [with_gtk=3])

AS_IF([test "x$with_gtk" = x3],
        [PKG_CHECK_MODULES(INDICATOR,  gtk+-3.0 >= $GTK3_REQUIRED_VERSION
                                       gio-2.0 >= $GIO_REQUIRED_VERSION
                                       indicator3-0.4 >= $INDICATOR_REQUIRED_VERSION
                                       dbusmenu-gtk3-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                       libbamf3 >= $BAMF_REQUIRED_VERSION)
        ],
      [test "x$with_gtk" = x2],
        [PKG_CHECK_MODULES(INDICATOR,  gtk+-2.0 >= $GTK_REQUIRED_VERSION
                                       gio-2.0 >= $GIO_REQUIRED_VERSION
                                       indicator-0.4 >= $INDICATOR_REQUIRED_VERSION
                                       dbusmenu-gtk-0.4 >= $DBUSMENUGTK_REQUIRED_VERSION
                                       libbamf >= $BAMF_REQUIRED_VERSION)
        ],
      [AC_MSG_FAILURE([Value for --with-gtk was neither 2 nor 3])]
)

AC_SUBST(INDICATOR_CFLAGS)
AC_SUBST(INDICATOR_LIBS)

###########################
# Dependencies
###########################

DBUSMENU_GLIB_REQUIRED_VERSION=0.4.0
BAMF_REQUIRED_VERSION=0.2.53
SQLITE_REQUIRED_VERSION=0.0

PKG_CHECK_MODULES([HUD],[
		dbusmenu-glib-0.4 >= $DBUSMENU_GLIB_REQUIRED_VERSION
		libbamf3 
		sqlite3 >= $SQLITE_REQUIRED_VERSION
])

###########################
# CLI Dependencies
###########################

AC_CHECK_HEADER("readline/readline.h",
	[has_readline_h="yes"])
AC_CHECK_HEADER("readline/history.h",
	[has_history_h="yes"])
AC_CHECK_HEADER("curses.h",
	[has_curses_h="yes"])

AM_CONDITIONAL([BUILD_CLI], [test "x$has_readline_h" == "xyes" && test "x$has_history_h" == "xyes" && test "x$has_curses_h" == "xyes"]),

###########################
# Test Dependencies
###########################

DBUSMENU_JSONLOADER_REQUIRED_VERSION=0.3.3
LIBAPPINDICATOR_REQUIRED_VERSION=0.0.0

AC_ARG_ENABLE([tests],
	AC_HELP_STRING([--disable-tests], [Disable test scripts and tools]),,
	[enable_tests=auto])

if test x"$enable_tests" != x"no" ; then
   PKG_CHECK_MODULES(INDICATORTEST,
                     dbusmenu-jsonloader-0.4 >= $DBUSMENU_JSONLOADER_REQUIRED_VERSION
                     appindicator3-0.1 >= $LIBAPPINDICATOR_REQUIRED_VERSION,
                     [have_dbusmenu_jsonloader=yes],
                     [have_dbusmenu_jsonloader=no]
                    )
   if test x${enable_tests} = xyes && test x${have_dbusmenu_jsonloader} = xno; then
     AC_MSG_ERROR([Test scripts and tools configured but dbusmenu-jsonloader not found])
   fi
else
   have_dbusmenu-jsonloader=no
fi
AM_CONDITIONAL(BUILD_TESTS, test x${have_dbusmenu_jsonloader} = xyes)

AC_SUBST(INDICATORTEST_CFLAGS)
AC_SUBST(INDICATORTEST_LIBS)

###########################
# gcov coverage reporting
###########################

m4_include([m4/gcov.m4])
AC_TDD_GCOV
AC_SUBST(COVERAGE_CFLAGS)
AC_SUBST(COVERAGE_CXXFLAGS)
AC_SUBST(COVERAGE_LDFLAGS)

###########################
# XSLT Processor
###########################

AC_PATH_PROG([XSLT_PROC], [xsltproc])

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
elif test "x$with_gtk" = x2; then
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator-0.4`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator-0.4`
else
	INDICATORDIR=`$PKG_CONFIG --variable=indicatordir indicator3-0.4`
	INDICATORICONSDIR=`$PKG_CONFIG --variable=iconsdir indicator3-0.4`
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

##############################
# Custom Junk
##############################

AC_DEFUN([AC_DEFINE_PATH], [
	test "x$prefix" = xNONE && prefix="$ac_default_prefix"
	test "x$exec_prefix" = xNONE && exec_prefix='${prefix}'
	ac_define_path=`eval echo [$]$2`
	ac_define_path=`eval echo [$]ac_define_path`
	$1="$ac_define_path"
	AC_SUBST($1)
	ifelse($3, ,
		AC_DEFINE_UNQUOTED($1, "$ac_define_path"),
		AC_DEFINE_UNQUOTED($1, "$ac_define_path", $3))
])

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=indicator-appmenu

IT_PROG_INTLTOOL([0.41.0])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the gettext package name.])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])

###########################
# Files
###########################

AC_OUTPUT([
Makefile
src/Makefile
scripts/Makefile
data/Makefile
tools/Makefile
po/Makefile.in
tests/Makefile
tests/manual/Makefile
docs/Makefile
docs/man/Makefile
docs/devel/Makefile
])

###########################
# Results
###########################

AC_MSG_NOTICE([

App Menu Indicator Configuration:

	Prefix:        $prefix
	Indicator Dir: $INDICATORDIR
	gcov:          $use_gcov
	Local Install: $with_localinstall
	Test tools:    $have_dbusmenu_jsonloader
])
