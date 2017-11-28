dnl -*- mode: Autoconf;-*-

dnl -- PadicoTM
dnl --
AC_DEFUN([AC_NMAD_PADICOTM],
	 [
	   AC_ARG_WITH([nmad],
	     [AS_HELP_STRING([--without-padicotm], [do not use PadicoTM @<:@default=with-padicotm@:>@] )] )
           if test "x${with_padicotm}" != "xno"; then
    	     with_padicotm=yes
    	     PKG_CHECK_MODULES([PadicoTM], [ PadicoTM ])
             AC_MSG_CHECKING([whether PadicoTM needs pioman])
             if pkg-config PadicoTM --print-requires | grep -q -- '^pioman$'; then
               need_pioman=yes
	     else
	       need_pioman=no
             fi
	     AC_MSG_RESULT([$need_pioman])
           fi
           AC_SUBST(with_padicotm)
	 ])

dnl -- PIOMan
dnl --
AC_DEFUN([AC_NMAD_PIOMAN],
	 [
	   AC_ARG_WITH([pioman],
	     [AS_HELP_STRING([--with-pioman], [use pioman I/O manager @<:@default=no@:>@] )])
           AC_MSG_CHECKING([for PIOMan])
	   if test "x${need_pioman}" = "xyes"; then
	     with_pioman=yes
	   fi
           if test "x${with_pioman}" = "xyes"; then
             AC_MSG_RESULT([yes])
             PKG_CHECK_MODULES([pioman], [ pioman ])
             requires_pioman="pioman"
             AC_DEFINE([NMAD_PIOMAN], [1], [use Pioman progression engine])
	   else
             AC_MSG_RESULT([no])
	     requires_pioman=
           fi
	   AC_SUBST([requires_pioman])
	   AC_SUBST(with_pioman)
 	 ])
