## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src

gstranslatordocdir = ${prefix}/doc/gstranslator
gstranslatordoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(gstranslatordoc_DATA)


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(gstranslatordocdir)
