changequote([[, ]])dnl
changecom dnl
dnl
dnl
dnl
define([[PRINT_MARCEL]], [[dnl
/* PART MARCEL */
/*#line __line__ "__file__"*/
patsubst([[patsubst([[$1]], [[prefix]],[[marcel]])]], 
	    [[PREFIX]],[[MARCEL]])dnl
]])dnl
dnl
dnl
dnl
define([[PRINT_PMARCEL]], [[dnl
/* PART PMARCEL */
/*#line __line__ "__file__"*/
#ifdef MA__IFACE_PMARCEL
patsubst([[patsubst([[$1]], [[prefix]],[[pmarcel]])]], 
	    [[PREFIX]],[[PMARCEL]])dnl
#endif
]])dnl
dnl
dnl
dnl
define([[PRINT_LPT]], [[dnl
/* PART LPT */
/*#line __line__ "__file__"*/
#ifdef MA__IFACE_LPT
patsubst([[patsubst([[$1]], [[prefix]],[[lpt]])]],
	    [[PREFIX]],[[LPT]])dnl
#endif
]])dnl
dnl
dnl
dnl
define([[PRINT_PTHREAD]], [[dnl
/* PART PTHREAD */
#ifdef MA__LIBPTHREAD
/*#line __line__ "__file__"*/
patsubst([[patsubst([[$1]], [[prefix]],[[pthread]])]],
	    [[PREFIX]],[[PTHREAD]])dnl
#endif
]])dnl
dnl
dnl
dnl
define([[REPLICATE]], [[
pushdef([[PARTS]],ifelse([[$2]],,[[ MARCEL PMARCEL LPT ]],[[ $2 ]]))
/******************************************
 * Partie dupliquée (orig: __file__, parts= PARTS )
 */
ifelse(index( PARTS ,[[ MARCEL ]]),-1,,[[PRINT_MARCEL([[$1]])
]])dnl
ifelse(index( PARTS ,[[ PMARCEL ]]),-1,,[[PRINT_PMARCEL([[$1]])
]])dnl
ifelse(index( PARTS ,[[ LPT ]]),-1,,[[PRINT_LPT([[$1]])
]])dnl
ifelse(index( PARTS ,[[ PTHREAD ]]),-1,,[[PRINT_PTHREAD([[$1]])
]])dnl
/*
 * Fin de la partie dupliquée
 ******************************************/dnl
popdef([[PARTS]])
]])dnl
dnl
dnl
dnl
define([[REPLICATE_CODE]], [[dnl
pushdef([[PARTS]],ifelse([[$2]],,[[MARCEL PMARCEL LPT]],[[$2]]))
REPLICATE([[dnl
#undef MA__MODE
#define MA__MODE MA__MODE_PREFIX
$1]],[[PARTS]])
popdef([[PARTS]])
]])dnl
