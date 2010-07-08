AC_PREREQ([2.64])
AC_INIT([soto], [0.0.1], [simgidacav@gmail.com])

AM_INIT_AUTOMAKE([-Wall])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADER([src/headers/config.h])

AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC

AC_ARG_ENABLE([debug],
              AS_HELP_STRING([--enable-debug], [Enable verbosity]),
              AC_DEFINE([DEBUG_ENABLED], [], [Debugging log enabled]))

# Checks for header files.
AC_CHECK_HEADER([alsa/asoundlib.h], [],
                [AC_MSG_ERROR(libasound2 headers not found.)])

# Checks for libraries.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])

AC_OUTPUT
