AC_INIT([hud],
        [0.0.4],
        [http://bugs.launchpad.net/hud],
        [hud],
        [http://launchpad.net/hud])

AC_PREREQ([2.64])

AC_CONFIG_HEADERS([config.h])
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

###########################
# Dependencies
###########################

DBUSMENU_GLIB_REQUIRED_VERSION=0.4.0
BAMF_REQUIRED_VERSION=0.2.53
SQLITE_REQUIRED_VERSION=0.0

PKG_CHECK_MODULES([HUD],[
		dbusmenu-glib-0.4 >= $DBUSMENU_GLIB_REQUIRED_VERSION
		libbamf3 >= $BAMF_REQUIRED_VERSION
		sqlite3 >= $SQLITE_REQUIRED_VERSION
])

###########################
# Check to see if we're local
###########################

with_localinstall="no"
AC_ARG_ENABLE([localinstall],
              [AS_HELP_STRING([--enable-localinstall],
                              [install all of the files localy instead of system directories (for distcheck)])],
              [with_localinstall=$enableval],
              [with_localinstall=no])

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
# DBus Service Info
###########################

if test "x$with_localinstall" = "xyes"; then
	DBUSSERVICEDIR="${datadir}/dbus-1/services/"
else
	DBUSSERVICEDIR=`$PKG_CONFIG --variable=session_bus_services_dir dbus-1`
fi
AC_SUBST(DBUSSERVICEDIR)

###########################
# Internationalization
###########################

GETTEXT_PACKAGE=hud

IT_PROG_INTLTOOL([0.41.0])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.17])

AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [Define to the gettext package name.])
AC_DEFINE_PATH(GNOMELOCALEDIR, "${datadir}/locale", [locale directory])


###########################
# Files
###########################

AC_CONFIG_FILES([
Makefile
po/Makefile.in
service/Makefile
data/Makefile
tests/Makefile
])
AC_OUTPUT

###########################
# Results
###########################

AC_MSG_NOTICE([

HUD Configuration:

	Prefix:        $prefix
	Local Install: $with_localinstall
])
