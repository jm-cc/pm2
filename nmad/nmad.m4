dnl -*- mode: Autoconf;-*-

dnl -- PadicoTM
dnl --
AC_DEFUN([AC_NMAD_PADICOTM],
	 [
	   AC_ARG_WITH([padicotm],
	     [AS_HELP_STRING([--with-padicotm], [use PadicoTM as launcher @<:@default=yes@:>@] )] )
           if test "x${with_padicotm}" != "xno"; then
    	     with_padicotm=yes
             AC_DEFINE([NMAD_PADICOTM], [1], [use PadicoTM as launcher])
    	     PKG_CHECK_MODULES([PadicoTM], [ PadicoTM ])
             AC_MSG_CHECKING([whether PadicoTM needs pioman])
             if pkg-config PadicoTM --print-requires | grep -q -- '^pioman$'; then
               need_pioman=yes
	     else
	       need_pioman=no
             fi
	     AC_MSG_RESULT([$need_pioman])
           fi
           AC_SUBST([with_padicotm])
	   AC_MSG_CHECKING([for PadicoTM root])
	   padicotm_root="`pkg-config --variable=prefix PadicoTM`"
	   if test ! -r ${padicotm_root}/bin/padico-launch; then
	     AC_MSG_ERROR([cannot find PadicoTM in ${padicotm_root}])
	   fi
	   AC_MSG_RESULT([${padicotm_root}])
	   AC_SUBST([padicotm_root])
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

dnl -- PukABI
dnl --
AC_DEFUN([AC_NMAD_PUKABI],
	 [
	   AC_ARG_WITH([pukabi],
	     [AS_HELP_STRING([--with-pukabi], [use PukABI ABI interceptor @<:@default=yes@:>@] )])
 	   AC_MSG_CHECKING([for PukABI])
	   if test "x${with_pukabi}" = "xno"; then
	     AC_MSG_RESULT([no])
	   else
	     AC_MSG_RESULT([yes])
	     PKG_CHECK_MODULES([PukABI], [ PukABI ])
	     requires_pukabi="PukABI"
	     AC_DEFINE([NMAD_PUKABI], [1], [interface with PukABI interposer])
	   fi
	   AC_SUBST([requires_pukabi])
	   AC_SUBST(with_pukabi)
	 ])

dnl -- MX
dnl --
AC_DEFUN([AC_NMAD_CHECK_MX],
	 [
	   if test "x$MX_DIR" = "x" ; then
    	     if test "x${MX_ROOT}" = "x"; then
               if test -r /opt/mx/include/myriexpress.h ; then
    	         MX_DIR=/opt/mx
               fi
    	     else
	       MX_DIR=${MX_ROOT}
     	     fi
	   fi
	   HAVE_MX=no
	   with_mx=no
	   AC_ARG_WITH([mx],
	     [AS_HELP_STRING([--with-mx], [use Myrinet MX @<:@default=no@:>@] )])
	   if test "x${with_mx}" != "xno" ; then
     	     save_CPPFLAGS="${CPPFLAGS}"
    	     CPPFLAGS="${CPPFLAGS} -I${MX_DIR}/include"
    	     AC_CHECK_HEADER(myriexpress.h, [ HAVE_MX=yes ], [ HAVE_MX=no ])
    	     CPPFLAGS="${save_CPPFLAGS}"
    	     if test ${HAVE_MX} = yes; then
               if test "x${MX_DIR}" != "x" -a "x${MX_DIR}" != "x/usr"; then
      	         mx_CPPFLAGS="-I${MX_DIR}/include"
      	   	 mx_LIBS="-Wl,-rpath,${MX_DIR}/lib -L${MX_DIR}/lib"
               fi
	       mx_LIBS="${mx_LIBS} -lmyriexpress"
	       AC_SUBST([mx_CPPFLAGS])
	       AC_SUBST([mx_LIBS])
    	     elif test "x${with_mx}" != "xcheck"; then
               AC_MSG_FAILURE([--with-mx was given, but myrinetexpress.h is not found])
    	     fi
	   fi
	   AC_SUBST(HAVE_MX)
	 ])

dnl -- DCFA
dnl --
AC_DEFUN([AC_NMAD_CHECK_DCFA],
	 [
	   HAVE_DCFA=no
	   AC_ARG_WITH([dcfa],
	     [AS_HELP_STRING([--with-dcfa], [use Direct Communication Facility for Accelerators @<:@default=check@:>@])],
	     [], [ with_dcfa=check ])
	   if test "x${with_dcfa}" != "xno" ; then
    	     save_CPPFLAGS="${CPPFLAGS}"
    	     if test "x${with_dcfa}" != "xyes"; then
               DCFA_DIR="${with_dcfa}"
	       CPPFLAGS="${CPPFLAGS} -I${DCFA_DIR}/include"
    	     fi
    	     AC_CHECK_HEADER(dcfa.h, [ HAVE_DCFA=yes ], [ HAVE_DCFA=no  ])
    	     CPPFLAGS="${save_CPPFLAGS}"
    	     if test ${HAVE_DCFA} = yes; then
               if test "x${DCFA_DIR}" != "x" -a "x${DCFA_DIR}" != "x/usr"; then
      	         dcfa_CPPFLAGS="-I${DCFA_DIR}/include"
      	   	 dcfa_LIBS="-Wl,-rpath,${DCFA_DIR}/lib -L${DCFA_DIR}/lib"
               fi
	       dcfa_LIBS="${dcfa_LIBS} -ldcfa"
	       AC_SUBST([dcfa_CPPFLAGS])
	       AC_SUBST([dcfa_LIBS])
    	     elif test "x${with_dcfa}" != "xcheck"; then
               AC_MSG_FAILURE([--with-dcfa was given, but dcfa.h is not found])
	     fi
	   fi
    	   AC_SUBST(HAVE_DCFA)
	 ])

dnl -- CCI
dnl --
AC_DEFUN([AC_NMAD_CHECK_CCI],
	 [
	   HAVE_CCI=no
	   AC_ARG_WITH([cci],
	     [AS_HELP_STRING([--with-cci], [use Common Communication Interface (CCI) @<:@default=check@:>@] )],
	     [], [ with_cci=check ])
	   if test "x${with_cci}" != "xno" ; then
    	     save_CPPFLAGS="${CPPFLAGS}"
    	     if test "x${with_cci}" != "xyes"; then
               CCI_DIR="${with_cci}"
	       CPPFLAGS="${CPPFLAGS} -I${CCI_DIR}/include"
    	     fi
    	     AC_CHECK_HEADER(cci.h, [ HAVE_CCI=yes ], [ HAVE_CCI=no  ])
    	     CPPFLAGS="${save_CPPFLAGS}"
    	     if test ${HAVE_CCI} = yes; then
               if test "x${CCI_DIR}" != "x" -a "x${CCI_DIR}" != "x/usr"; then
      	         cci_CPPFLAGS="-I${CCI_DIR}/include"
      	   	 cci_LIBS="-Wl,-rpath,${CCI_DIR}/lib -L${CCI_DIR}/lib"
               fi
	       cci_LIBS="${cci_LIBS} -lcci"
	       AC_SUBST([cci_CPPFLAGS])
	       AC_SUBST([cci_LIBS])
    	     elif test "x${with_cci}" != "xcheck"; then
               AC_MSG_FAILURE([--with-cci was given, but cci.h is not found])
    	     fi
	   fi
	   AC_SUBST(HAVE_CCI)
         ])
