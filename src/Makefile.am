

SUBDIRS = \
	config \
	kbflags \
	icon
## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(datadir)/gstranslator/ui
ui_DATA = gstranslator.ui \
	properties.ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	$(GSTRANSLATOR_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = gstranslator

gstranslator_SOURCES = \
	main.c \
	xml_parser.c \
	xml_parser.h \
	request.c \
	request.h \
	properties.c \
	properties.h \
	data_logger.c \
	data_logger.h

gstranslator_CFLAGS = -lcurl

gstranslator_LDFLAGS = \
	-Wl,--export-dynamic

gstranslator_LDADD = $(GSTRANSLATOR_LIBS)


EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(datadir)/gstranslator
