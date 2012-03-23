
EXTRA_DIST = 
bin_PROGRAMS = hud-gtk


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
	-DHUD_GTK_DATADIR=\""$(hudgtkdatadir)"\" \
	$(gtk_CFLAGS)

hud_gtk_LDADD = \
	$(gtk_LIBS)

