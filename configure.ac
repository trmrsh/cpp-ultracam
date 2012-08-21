dnl Process this file with autoconf to produce a configure script.

AC_INIT([ultracam], [8.1.52])

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

dnl check for other trm software

AC_CHECK_HEADERS([trm_subs.h trm_position.h trm_constants.h trm_array1d.h],[],AC_MSG_ERROR([cannot find 'subs' headers]))

AC_CHECK_LIB([subs], [main], [], AC_MSG_ERROR([cannot find 'subs' library]))

AC_CHECK_HEADERS([trm_colly.h],[],AC_MSG_ERROR([cannot find 'colly' header]))

AC_CHECK_LIB([colly], [main], [], AC_MSG_ERROR([cannot find 'colly' library]))

dnl Third-party software checks

dnl First headers

AC_CHECK_HEADERS([stdlib.h float.h math.h sstream string fstream iostream], [], AC_MSG_ERROR(missing header; please fix))

AC_CHECK_HEADERS([pcrecpp.h slalib.h fitsio.h curl/curl.h xercesc/dom/DOM.hpp], [], AC_MSG_ERROR(missing header; please fix))

dnl Now libraries
AC_CHECK_LIB([pcrecpp], [main], [], AC_MSG_ERROR(cannot find the pcre C++ library))

AC_CHECK_LIB([xerces-c], [main], [], AC_MSG_ERROR(cannot find the xerces-c library))

AC_CHECK_LIB([csla], [slaPm], [], AC_MSG_ERROR(cannot find the sla C library))

AC_CHECK_LIB([cfitsio], [main], [], AC_MSG_ERROR(cannot find the cfitsio library))

LIBCURL_CHECK_CONFIG

dnl PGPLOT has its own macro 'cos its a pain
TRM_LIB_PGPLOT

dnl Installation program
AC_PROG_INSTALL

dnl The Makefiles to create
AC_CONFIG_FILES([Makefile include/Makefile src/Makefile scripts/Makefile doc/Makefile]) 

AC_OUTPUT
