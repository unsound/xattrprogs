# Autoconf
AC_PREREQ(2.59)
AC_INIT([xattrprogs],[1.0],[catacombae@gmail.com])
AC_CONFIG_SRCDIR([getxattr.c])

# Environment
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

# Automake
AM_INIT_AUTOMAKE(no-dist-gzip dist-bzip2)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_MAINTAINER_MODE

# Options
AC_GNU_SOURCE

# Programs
AC_PROG_CC(gcc cc)
AM_PROG_CC_C_O

ifdef(
	[LT_INIT],
	[LT_INIT],
	[AC_PROG_LIBTOOL]
)

AC_PROG_INSTALL

# Environment

# Libraries

# Checks for header files.
AC_HEADER_STDC
#AC_CHECK_HEADERS([ \
#])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for library functions.
#AC_CHECK_FUNCS([ \
#])

# Checks for system services.
AC_SYS_LARGEFILE

if test "$GCC" = "yes" ; then
	# We add -Wall to enable some compiler warnings.
	CFLAGS="${CFLAGS} -Wall"
fi

# Settings

# generate files
AC_CONFIG_FILES([
	Makefile
])
AC_OUTPUT

echo "You can type now 'make' to build xattrprogs."
