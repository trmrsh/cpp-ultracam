dnl Process this file with autoconf to produce a configure script.

AC_INIT([ultracam], [8.1.21])

AC_CONFIG_SRCDIR([src/reduce.cc])

AM_INIT_AUTOMAKE

dnl Get rid of all the -D stuff
AC_CONFIG_HEADERS([config.h])

AC_PROG_MAKE_SET

dnl Compiler checks
AC_PROG_CXX
AC_PROG_CC

dnl Make C++ the default
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

dnl libtool
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

dnl Third-party software checks

dnl First headers

AC_CHECK_HEADERS([stdlib.h float.h math.h sstream string fstream iostream], [], AC_MSG_ERROR(missing header; please fix))

AC_CHECK_HEADERS([pcrecpp.h slalib.h fitsio.h curl/curl.h xercesc/dom/DOM.hpp], [], AC_MSG_ERROR(missing header; please fix))

dnl Now libraries
AC_CHECK_LIB([pcrecpp], [main], [], AC_MSG_ERROR(cannot find the pcre C++ library))

AC_CHECK_LIB([xerces-c], [main], [], AC_MSG_ERROR(cannot find the xerces-c library))

AC_CHECK_LIB([csla], [slaPm], [], AC_MSG_ERROR(cannot find the sla C library))

AC_CHECK_LIB([cfitsio], [main], [], AC_MSG_ERROR(cannot find the cfitsio library))

dnl AC_MSG_CHECKING([for curl-config in path])
dnl if which curl-config > /dev/null
dnl then
dnl 	AC_MSG_RESULT([yes])
dnl else
dnl 	AC_MSG_RESULT([no])
dnl 	AC_MSG_ERROR([curl-config must be in your path])
dnl fi
dnl CURL_LIBS=`curl-config --libs`
dnl AC_CHECK_LIB([curl], [main], [LIBS="$CURL_LIBS $LIBS"], AC_MSG_ERROR(cannot find the curl library), [$CURL_LIBS])

LIBCURL_CHECK_CONFIG

dnl PGPLOT has its own macro 'cos its a pain
TRM_LIB_PGPLOT

dnl Installation program
AC_PROG_INSTALL

dnl The Makefiles to create
AC_CONFIG_FILES([Makefile include/Makefile src/Makefile scripts/Makefile doc/Makefile]) 

AC_OUTPUT
