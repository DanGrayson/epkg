dnl ### Normal initialization. ######################################
AC_INIT([epkg], [2.3.9])
AC_PREREQ([2.57])
AC_CONFIG_AUX_DIR([autoconf])
AC_CONFIG_HEADER([config.h])
AC_COPYRIGHT([[
Copyright (c) 1998-2003 University of Illinois Board of Trustees
Copyright (c) 1998-2004 Mark D. Roth
All rights reserved.
]])
AC_CONFIG_SRCDIR([epkg/epkg.c])
if test "${enable_encap:-unset}" = "unset"; then
  enable_encap=yes;
fi
ENCAP_PKG(["-E 2.0"], [postinstall-encap], [postinstall-no-encap])
if test "${enable_encap}" = "yes"; then
  if test "${EPKG}" != ":"; then
    EPKG='${DESTDIR}${bindir}/epkg';
  fi
  AC_SUBST([EPKG])
  MKENCAP='${DESTDIR}${bindir}/mkencap';
  AC_SUBST([MKENCAP])
fi


dnl ### Set some option defaults. ###################################
if test -z "$CFLAGS"; then
  CFLAGS="-O"
fi
MKDIR="mkdir -p -m 755"
AC_SUBST([MKDIR])


dnl ### Check for compiler et al. ###################################
AC_PROG_CC
AC_CHECK_PROG([AR], [ar], [ar])
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET


dnl ### Compiler characteristics. ##################################
AC_AIX
AC_C_CONST


dnl ### Checks for header files. ###################################
AC_HEADER_STDC
AC_CHECK_HEADERS([unistd.h sys/time.h])
AC_CHECK_HEADERS([odmi.h sys/systemcfg.h sys/systeminfo.h])
AC_HEADER_SYS_WAIT
AC_TYPE_SIZE_T


dnl ### Check for needed functions. ################################
PSG_MODULE([compat])
COMPAT_FUNC_BASENAME
COMPAT_FUNC_DIRNAME
COMPAT_FUNC_FNMATCH
COMPAT_FUNC_GETHOSTNAME
COMPAT_FUNC_SNPRINTF
COMPAT_FUNC_STRDUP
COMPAT_FUNC_STRLCAT
COMPAT_FUNC_STRLCPY
COMPAT_FUNC_STRRSTR
COMPAT_FUNC_STRSEP


dnl ### Check for needed libraries. #################################
if test "$ac_cv_header_odmi_h" = "yes"; then
  AC_CHECK_LIB([odm], [odm_initialize])
fi


dnl ### Check for user-specified options. ###########################
AC_ARG_WITH([libtar],
  [  --without-libtar        Use external tar binary instead of libtar],
  [],
  [with_libtar=yes])
if test "$with_libtar" = "yes"; then
  PSG_LIB_TAR
fi

AC_ARG_WITH([zlib],
  [  --without-zlib          Use external gzip binary instead of zlib],
  [],
  [with_zlib=yes])
if test "$with_zlib" = "yes"; then
  AC_CHECK_LIB([z], [gzread])
fi

AC_ARG_WITH([libfget],
  [  --without-libfget       Disable FTP ability],
  [],
  [with_libfget=yes])
if test "$with_libfget" = "yes"; then
  PSG_LIB_FGET
fi

AC_ARG_WITH([libcurl],
  [  --without-libcurl       Disable HTTP ability],
  [],
  [with_libcurl=yes])
if test "$with_libcurl" = "yes"; then
  AC_PATH_GENERIC([curl], [7.9.8], [AC_DEFINE([HAVE_LIBCURL], 1,
                                              [Define if you have libcurl])])
fi

AC_ARG_WITH([libexpat],
  [  --without-libexpat      Disable package profile code],
  [],
  [with_libexpat=yes])
if test "$with_libexpat" = "yes"; then
  AC_CHECK_LIB([expat], [XML_ParserCreate])
fi

AC_ARG_WITH([encap-source],
  [  --with-encap-source=PATH Set default Encap source directory],
  [],
  [with_encap_source="/usr/local/encap"]
)
AC_DEFINE_UNQUOTED([DEFAULT_SOURCE], ["${with_encap_source}"],
          [Default Encap source directory])

AC_ARG_WITH([encap-target],
  [  --with-encap-target=PATH Set default Encap target directory],
  [],
  [with_encap_target="/usr/local"]
)
AC_DEFINE_UNQUOTED([DEFAULT_TARGET], ["${with_encap_target}"],
          [Default Encap target directory])

AC_ARG_WITH([excludes],
  [  --with-excludes=LIST    Set default exclude directories],
  [],
  [with_excludes="src:lost+found"]
)
AC_DEFINE_UNQUOTED([DEFAULT_EXCLUDES], ["${with_excludes}"],
          [Default exclude directories])

AC_ARG_WITH([overrides],
  [  --with-overrides=LIST   Set default override list],
  [],
  [with_overrides="old_local"]
)
AC_DEFINE_UNQUOTED([DEFAULT_OVERRIDES], ["${with_overrides}"],
          [Default override list])

AC_ARG_WITH([download-dir],
  [  --with-download-dir=DIR  Set default download directory],
  [],
  [with_download_dir="/usr/local/src"]
)
AC_DEFINE_UNQUOTED([DEFAULT_DOWNLOAD_DIR], ["${with_download_dir}"],
          [Default download directory])

AC_ARG_WITH([build-tree],
  [  --with-build-tree=DIR   Set default build tree],
  [],
  [with_build_tree="/usr/local/src"]
)
AC_DEFINE_UNQUOTED([DEFAULT_BUILD_TREE], ["${with_build_tree}"],
          [Default build directory])

AC_ARG_WITH([common-src-tree],
  [  --with-common-src-tree=DIR  Set default common source tree],
  [],
  [with_common_src_tree=""]
)
AC_DEFINE_UNQUOTED([DEFAULT_COMMON_SRC_TREE], ["${with_common_src_tree}"],
          [Default common source tree])


dnl ### Generate info for mkencap_environment file ###########################
AC_PATH_PROG([C_COMPILER], [$CC])
COMPILER_DIR=`AS_DIRNAME([$C_COMPILER])`

for dir in /bin /usr/bin /usr/ccs/bin /usr/bin/X11 /usr/openwin/bin /usr/ucb /usr/bsd; do
  if test -d "${dir}"; then
    if test -n "${ENVIRONMENT_PATH}"; then
      ENVIRONMENT_PATH="${ENVIRONMENT_PATH}:${dir}";
    else
      ENVIRONMENT_PATH="${dir}";
    fi
    if test "${COMPILER_DIR}" = "${dir}"; then
      GOT_COMPILER_DIR=1;
    fi
  fi
done

if test -z "${GOT_COMPILER_DIR}"; then
  if test "${COMPILER_DIR}" = "${ENCAP_TARGET}/bin"; then
    AC_MSG_WARN([compiler being invoked from Encap target tree])
    AC_MSG_WARN([adding compiler directory to PATH in mkencap_environment file - this is not recommended])
    AC_MSG_WARN([you may want to change the PATH and CC variables in the mkencap_environment file after installation])
    ENVIRONMENT_PATH="${ENVIRONMENT_PATH}:${COMPILER_DIR}";
  else
    ENVIRONMENT_PATH="${COMPILER_DIR}:${ENVIRONMENT_PATH}";
  fi
fi

AC_SUBST([ENVIRONMENT_PATH])


dnl ### Create output files. #################################################
PSG_MODULE([listhash], [encap])
PSG_MODULE([pathcode], [encap])
AC_CONFIG_FILES([Makefile lib/Makefile epkg/Makefile mkencap/Makefile mkencap/mkencap_environment doc/Makefile])
AC_OUTPUT

