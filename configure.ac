dnl Process this file with autoconf to produce a configure script.

AC_INIT([ultracam], [8.1.21])

AC_CONFIG_SRCDIR([src/reduce.cc])

AM_INIT_AUTOMAKE

dnl Get rid of all the -D stuff
AC_CONFIG_HEADERS([config.h])

AC_PROG_MAKE_SET

dnl Extra libraries

PGPLOT_LIBS=`./pgplot_libs`

AC_MSG_CHECKING([for curl-config in path])
if which curl-config > /dev/null
then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([curl-config must be in your path])
fi

CURL_LIBS=`curl-config --libs`

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC

dnl Make C++ the default
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

dnl libtool
AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

dnl third-party software checks

AC_CHECK_LIB([pcrecpp], [main], [], AC_MSG_ERROR(cannot find the pcre C++ library))

AC_CHECK_LIB([xerces-c], [main], [], AC_MSG_ERROR(cannot find the xerces-c library))

AC_CHECK_LIB([curl], [main], [LIBS="$CURL_LIBS $LIBS"], AC_MSG_ERROR(cannot find the curl library), [$CURL_LIBS])

AC_CHECK_LIB([csla], [main], [], AC_MSG_ERROR(cannot find the sla C library))

AC_CHECK_LIB([cfitsio], [main], [], AC_MSG_ERROR(cannot find the cfitsio library))

AC_CHECK_LIB([cpgplot],  [main], [LIBS="$PGPLOT_LIBS $LIBS"], [AC_MSG_ERROR(cannot link to the pgplot libraries)], [$PGPLOT_LIBS])

AC_CHECK_HEADERS([stdlib.h float.h math.h sstream string fstream iostream], [], AC_MSG_ERROR(missing header; please fix))

AC_CHECK_HEADERS([pcrecpp.h slalib.h cpgplot.h fitsio.h curl/curl.h xercesc/dom/DOM.hpp], [], AC_MSG_ERROR(missing header; please fix))

dnl Installation program
AC_PROG_INSTALL

dnl The Makefiles to create
AC_CONFIG_FILES([Makefile include/Makefile src/Makefile scripts/Makefile doc/Makefile]) 

AC_OUTPUT
