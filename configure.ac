AC_INIT([PhotoSelect], [0.1], [brucekarsh@gmail.com], [PhotoSelect])
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE
AC_PROG_CC
AC_PROG_CXX

AC_CHECK_PROGS([DOXYGEN], [doxygen])
if test -z "$DOXYGEN";
   then AC_MSG_WARN([Doxygen not found - continuing without Doxygen support])
fi
AM_CONDITIONAL([HAVE_DOXYGEN], 
[test -n "$DOXYGEN"])AM_COND_IF([HAVE_DOXYGEN], [AC_CONFIG_FILES([docs/Doxyfile docs/Makefile])])

PKG_CHECK_MODULES([PhotoSelect], [gtk+-3.0 gmodule-2.0 >= 2.0 json-c exiv2 xerces-c])
PKG_CHECK_MODULES([LIBCRYPTO], [libcrypto++],,[CXXFLAGES="${CXXFLAGS} -lcryptopp])
AC_PROG_INSTALL
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
