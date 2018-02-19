
AC_PREREQ([2.57])
AC_INIT(xinfo,[0.0.1], [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg],xinfo)
AM_INIT_AUTOMAKE([dist-bzip2])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_INSTALL

# Checks for pkg-config packages
PKG_CHECK_MODULES(XINFO, x11)
AC_SUBST(XINFO_CFLAGS)
AC_SUBST(XINFO_LIBS)

XORG_MANPAGE_SECTIONS
XORG_RELEASE_VERSION

AC_OUTPUT([src/Makefile
	   Makefile])
