## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(INTERNET_ICON_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = internet_icon


internet_icon_SOURCES =  \
	main.c \
	socket.c \
	socket.h \
	findtask.c \
	findtask.h \
	pixbufs.c \
	xml.c \
	xml.h \
	main.h \
	mylocale.h \
	utils.c \
	utils.h

internet_icon_LDFLAGS = \
	-Wl,--export-dynamic

internet_icon_LDADD = $(INTERNET_ICON_LIBS)




EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

