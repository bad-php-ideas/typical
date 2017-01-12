dnl $Id$
dnl config.m4 for extension typical

PHP_ARG_ENABLE(typical, whether to enable typical support,
[  --disable-typical       Disable typical support], yes)

if test "$PHP_TYPICAL" != "no"; then
  PHP_NEW_EXTENSION(typical, typical.c typical-lexer.c typical-parser.c, $ext_shared)
  PHP_ADD_MAKEFILE_FRAGMENT
fi
