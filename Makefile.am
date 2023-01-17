AUTOMAKE_OPTIONS = gnu
ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	autogen.sh

MAINTAINERCLEANFILES=\
	$(srcdir)/configure \
	$(srcdir)/Makefile.in \
	$(srcdir)/aclocal.m4 \
	$(srcdir)/compile \
	$(srcdir)/depcomp \
	$(srcdir)/install-sh \
	$(srcdir)/ltmain.sh \
	$(srcdir)/missing \
	$(srcdir)/config.guess \
	$(srcdir)/config.sub \
	$(srcdir)/config.h.in \
	$(srcdir)/config.h.in~ \
	$(srcdir)/INSTALL \
	$(srcdir)/m4/ltsugar.m4 \
	$(srcdir)/m4/libtool.m4 \
	$(srcdir)/m4/ltversion.m4 \
	$(srcdir)/m4/lt~obsolete.m4 \
	$(srcdir)/m4/ltoptions.m4

bin_PROGRAMS = \
	getxattr \
	listxattr \
	removexattr \
	setxattr

man_MANS =

getxattr_LDADD =
getxattr_LDFLAGS = \
	$(AM_LDFLAGS)
getxattr_CFLAGS = \
	$(AM_CFLAGS)
getxattr_SOURCES = \
	getxattr.c

listxattr_LDADD =
listxattr_LDFLAGS = $(AM_LDFLAGS)
listxattr_CFLAGS = \
	$(AM_CFLAGS)
listxattr_SOURCES = \
	listxattr.c

removexattr_LDADD =
removexattr_LDFLAGS = $(AM_LDFLAGS)
removexattr_CFLAGS = \
	$(AM_CFLAGS)
removexattr_SOURCES = \
	removexattr.c

setxattr_LDADD =
setxattr_LDFLAGS = $(AM_LDFLAGS)
setxattr_CFLAGS = \
	$(AM_CFLAGS)
setxattr_SOURCES = \
	setxattr.c

doc_DATA = \
	README

dist-hook:
	$(MKDIR_P) "$(distdir)/m4"

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
