# Process this file with autoconf to produce a configure script
AC_PREREQ(2.57)
AC_INIT(konkretcmpi, 0.8.7)
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADER([src/konkret/config.h])
AC_PROG_LIBTOOL
AC_PREFIX_DEFAULT(/usr/local)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AM_PROG_CC_C_O

##=============================================================================
##
## resolve cimschema:
##
##=============================================================================

cimschema=

AC_ARG_WITH(
  cim-schema,
  [  --with-cim-schema=DIR   use the CIM schema located here],
  [cimschema=$withval])

if test ! -z "$cimschema" -a ! -d "$cimschema" ; then
  AC_MSG_ERROR(directory given by --with-cim-schema=$cimschema does not exist)
fi

if test ! -z "$cimschema" ; then
  moffile=`/bin/ls $cimschema/cimv*.mof`
  if test -z "$moffile" ; then
    AC_MSG_ERROR(invalid schema directory given by --with-cim-schema=$cimschema (missing master schema))
  fi
fi

moffile=`/bin/ls /usr/share/sfcb/CIM/cimv*.mof`

if test ! -z "$moffile" ; then
  cimschema="/usr/share/sfcb/CIM"
fi

if test ! -z "$cimschema" ; then
  CXXFLAGS="$CXXFLAGS -DCIMSCHEMA=\\\"$cimschema\\\""
fi

##=============================================================================
##
## resolve --with-cmpi-headers=DIR option
##
##=============================================================================

AC_ARG_WITH(
  cmpi-headers,
  [  --with-cmpi-headers=DIR   use CMPI headers from this diretory],
  [cmpiheaders=$withval])

if test ! -z "$cmpiheaders" -a ! -d "$cmpiheaders" ; then
  AC_MSG_ERROR(directory given by --with-cmpi-headers=$cmpiheaders does not exist)
fi

if test ! -z "$cmpiheaders" ; then
  cmpidt=`/bin/ls $cmpiheaders/cmpidt.h`
  if test -z "$cmpidt" ; then
    AC_MSG_ERROR(directory given by --with-cmpi-headers=$cmpiheaders is missing cmpidt.h)
  fi
fi

if test -z "$cmpiheaders"; then

  AC_CHECK_HEADERS(cmpidt.h cmpidmacs.h cmpift.h, [found=1],[found=0])

  if test "$found" = "0"; then
    if test -f "/usr/include/cmpi/cmpidt.h"; then
        cmpiheaders=/usr/include/cmpi
    fi
    if test -z "$cmpiheaders" -a -f "/usr/local/include/cmpi/cmpidt.h"; then
        cmpiheaders=/usr/local/include/cmpi
    fi
    if test -z "$cmpiheaders" -a -f "/usr/local/include/cmpi/cmpidt.h"; then
      AC_MSG_ERROR(failed to locate CMPI headers. Please specify with --with-cmpi-headers=DIR option)
    fi
  fi
fi

if test ! -z "$cmpiheaders" ; then
  CFLAGS="$CFLAGS -I$cmpiheaders"
  CXXFLAGS="$CXXFLAGS -I$cmpiheaders"
fi

##=============================================================================
##
## CXXFLAGS
## CFLAGS
##
##=============================================================================

CXXFLAGS="$CXXFLAGS -fPIC -Wall"
CFLAGS="$CFLAGS -fPIC -Wall"

##=============================================================================
##
## AC_CONFIG_FILES
##
##=============================================================================

AC_CONFIG_FILES([
    Makefile 
    src/Makefile 
    src/konkret/Makefile
    src/mof/Makefile
    src/program/Makefile
    src/konkretreg/Makefile])

AC_OUTPUT
