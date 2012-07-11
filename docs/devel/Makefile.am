include $(top_srcdir)/gtk-doc.make

DOC_MODULE = hud

DOC_MAIN_SGML_FILE = $(DOC_MODULE)-docs.xml

DOC_SOURCE_DIR = $(top_srcdir)/src

MKDB_OPTIONS = --sgml-mode --output-format=xml

HFILE_GLOB = $(DOC_SOURCE_DIR)/*.h
CFILE_GLOB = $(DOC_SOURCE_DIR)/*.c

IGNORE_HFILES = \
	MwmUtil.h				\
	create-db.h				\
	dbus-shared.h				\
	distance.h				\
	dump-app-info.h				\
	gen-application-menu-renderer.xml.h	\
	gen-application-menu-registrar.xml.h	\
	gdk-get-func.h				\
	hud.interface.h				\
	indicator-tracker.h			\
	load-app-info.h				\
	menuitem-collector.h			\
	shared-values.h				\
	usage-tracker.h				\
	utils.h					\
	window-menus.h

GTKDOC_CFLAGS = -I$(top_srcdir)/src $(HUD_CFLAGS)

GTKDOC_LIBS = $(top_builddir)/src/libhud-service.a \
	$(HUD_LIBS) \
	$(COVERAGE_LDFLAGS)
