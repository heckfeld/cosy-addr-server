#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.63])
AC_INIT([addr_serv], [1.0], [mi.simon@fz-juelich.de])
AM_INIT_AUTOMAKE([foreign])
AC_CONFIG_SRCDIR([server.c])
AC_CONFIG_MACRO_DIR([m4])

# Checks for programs.
AC_PROG_CC
AC_PROG_LIBTOOL

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([arpa/inet.h fcntl.h limits.h netdb.h netinet/in.h stdlib.h string.h sys/param.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_PID_T
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_CHECK_FUNCS([alarm gethostbyaddr gethostbyname gethostname inet_ntoa memset select socket])
AC_SUBST([TCL], [$prefix/tcl])
AC_SUBST([NAME], [addr_serv])
AC_SUBST([DAEMON], [$sbindir/addr_serv])

AC_CONFIG_FILES([Makefile init.d/addr_serv default/addr_serv])
AC_OUTPUT
