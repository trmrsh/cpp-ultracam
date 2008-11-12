dnl Process this file with autoconf to produce a configure script.

AC_INIT([src/reduce.cc])

dnl Get rid of all the -D stuff

AM_CONFIG_HEADER(config.h)

dnl automake stuff

AM_INIT_AUTOMAKE([ultracam], [8.1.17])

AC_PROG_MAKE_SET

dnl Extra libraries

PGPLOT_LIBS=`./pgplot_libs`

CURL_LIBS=`curl-config --libs`

dnl Checks for programs.
AC_PROG_CXX
AC_PROG_CC

dnl Make C++ the default
AC_LANG_CPLUSPLUS
AC_REQUIRE_CPP

dnl Initialise libtool
AC_PROG_LIBTOOL

dnl library checks

AC_CHECK_LIB([pcrecpp], [main], [], AC_MSG_ERROR(cannot find the pcre C++ library))

AC_CHECK_LIB([xerces-c], [main], [],    AC_MSG_ERROR(cannot find the xerces-c library))

AC_CHECK_LIB([curl], [main], [LIBS="$CURL_LIBS $LIBS"], AC_MSG_ERROR(cannot find the curl library), [$CURL_LIBS])

AC_CHECK_LIB([csla], [main], [], AC_MSG_ERROR(cannot find the sla C library))

AC_CHECK_LIB([cfitsio], [main], [], AC_MSG_ERROR(cannot find the cfitsio library))

AC_CHECK_LIB([cpgplot],  [main], [LIBS="$PGPLOT_LIBS $LIBS"], [AC_MSG_ERROR(cannot link to the pgplot libraries)], [$PGPLOT_LIBS])

AC_CHECK_LIB([subs], [main], [], [AC_MSG_ERROR(cannot find the subs library)])

AC_CHECK_LIB([colly], [main], [], AC_MSG_WARN(cannot find the C++ molly library; dump to molly disabled in 'sreduce'))

dnl Check for some header files.
AC_CHECK_HEADERS([stdlib.h float.h math.h sstream string fstream iostream], [], AC_MSG_ERROR(missing header; please fix))

AC_CHECK_HEADERS([pcrecpp.h slalib.h cpgplot.h fitsio.h curl/curl.h xercesc/dom/DOM.hpp], [], AC_MSG_ERROR(missing header; please fix))

AC_CHECK_HEADERS([trm_subs.h trm_position.h trm_constants.h trm_array1d.h],[],AC_MSG_ERROR(cannot find headers associated with subs))

AC_CHECK_HEADERS([trm_colly.h],[],AC_MSG_WARN(cannot find the header for C++ molly library; dump to molly disabled in 'sreduce'))

dnl Installation program
AC_PROG_INSTALL

dnl The Makefiles to create

AC_OUTPUT([Makefile include/Makefile src/Makefile scripts/Makefile doc/Makefile]) 
