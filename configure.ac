AC_INIT([pounce],[1.0.0],[fredrik.widlund@gmail.com])
AC_CONFIG_AUX_DIR(autotools)
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SUBDIRS([lib/libdynamic lib/libreactor])
AM_INIT_AUTOMAKE([-Wall -Werror foreign no-define])

: ${CFLAGS="-Wall -Wextra -Wpedantic -g -O3 -flto -march=native"}
CFLAGS="$CFLAGS -I${ac_pwd}/lib/libdynamic/src -I${ac_pwd}/lib/libreactor/src"
export CFLAGS

AM_PROG_AR
LT_INIT
AM_PROG_CC_C_O

AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
