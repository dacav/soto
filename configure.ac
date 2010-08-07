AC_PREREQ([2.64])
AC_INIT([soto], [0.2.1], [simgidacav@gmail.com])

AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([src/headers/config.h])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_C_INLINE

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [Enable verbosity]),
              AC_DEFINE([DEBUG_ENABLED], [], [Debugging log enabled]))

AC_ARG_ENABLE([realtime],
              AS_HELP_STRING([--disable-realtime], [Disable realtime priority]),
              AC_DEFINE([RT_DISABLE], [], [Debugging log enabled]))

# Checks for header files.
AC_CHECK_HEADER([alsa/asoundlib.h], [],
                [AC_MSG_ERROR([Please install libasound2.])])
AC_CHECK_HEADER([dacav/dacav.h], [],
                [AC_MSG_ERROR([Please install libdacav (>= 0.4.2).])])
AC_CHECK_HEADER([plot.h], [],
                [AC_MSG_ERROR([Please install libplot.])])
AC_CHECK_HEADER([fftw3.h], [],
                [AC_MSG_ERROR([Please install libfftw (>= 3.2.1).])])

# Checks for libraries.
AC_CHECK_LIB([dacav], [dlist_sort], [],
             [AC_MSG_ERROR([libdacav version required: >= 0.4.2]).])
AC_CHECK_LIB([fftw3], [fftw_plan_dft_r2c_1d], [],
             [AC_MSG_ERROR([libfftw version required: >= 3.2.1]).])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T

# Checks for library functions.
AC_CHECK_FUNCS([strerror])
AC_FUNC_MALLOC

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 doc/Doxyfile:doc/Doxyfile.in])

AC_OUTPUT
