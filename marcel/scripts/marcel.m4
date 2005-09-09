m4_changequote([[, ]])m4_dnl
m4_define([[dnl]], [[m4_dnl]])dnl
m4_changecom dnl
dnl
dnl
dnl
m4_define([[PRINT_MARCEL]], [[dnl
/* PART MARCEL */
//#line m4___line__ "m4___file__"
m4_patsubst([[m4_patsubst([[$1]], [[prefix]],[[marcel]])]], 
	    [[PREFIX]],[[MARCEL]])dnl
]])dnl
dnl
dnl
dnl
m4_define([[PRINT_PMARCEL]], [[dnl
/* PART PMARCEL */
//#line m4___line__ "m4___file__"
m4_patsubst([[m4_patsubst([[$1]], [[prefix]],[[pmarcel]])]], 
	    [[PREFIX]],[[PMARCEL]])dnl
]])dnl
dnl
dnl
dnl
m4_define([[PRINT_LPT]], [[dnl
/* PART LPT */
//#line m4___line__ "m4___file__"
#ifdef MA__POSIX_FUNCTIONS_NAMES
m4_patsubst([[m4_patsubst([[$1]], [[prefix]],[[lpt]])]],
	    [[PREFIX]],[[LPT]])dnl
#endif
]])dnl
dnl
dnl
dnl
m4_define([[PRINT_PTHREAD]], [[dnl
/* PART PTHREAD */
#ifdef MA__PTHREAD_FUNCTIONS
//#line m4___line__ "m4___file__"
m4_patsubst([[m4_patsubst([[$1]], [[prefix]],[[pthread]])]],
	    [[PREFIX]],[[PTHREAD]])dnl
#endif
]])dnl
dnl
dnl
dnl
m4_define([[REPLICATE]], [[
m4_pushdef([[PARTS]],m4_ifelse([[$2]],,[[ MARCEL PMARCEL LPT ]],[[ $2 ]]))
/******************************************
 * Partie dupliquée (orig: m4___file__, parts= PARTS )
 */
m4_ifelse(m4_index( PARTS ,[[ MARCEL ]]),-1,,[[PRINT_MARCEL([[$1]])
]])dnl
m4_ifelse(m4_index( PARTS ,[[ PMARCEL ]]),-1,,[[PRINT_PMARCEL([[$1]])
]])dnl
m4_ifelse(m4_index( PARTS ,[[ LPT ]]),-1,,[[PRINT_LPT([[$1]])
]])dnl
m4_ifelse(m4_index( PARTS ,[[ PTHREAD ]]),-1,,[[PRINT_PTHREAD([[$1]])
]])dnl
/*
 * Fin de la partie dupliquée
 ******************************************/dnl
m4_popdef([[PARTS]])
]])dnl
dnl
dnl
dnl
m4_define([[DEFINE_MODES]], [[dnl
#define MA__MODE_MARCEL 1
#define MA__MODE_PMARCEL 2
#define MA__MODE_LPT 3
#define MA__MODE_PTHREAD 4
]])dnl
dnl
dnl
dnl
m4_define([[REPLICATE_CODE]], [[dnl
m4_pushdef([[PARTS]],m4_ifelse([[$2]],,[[MARCEL PMARCEL LPT]],[[$2]]))
REPLICATE([[dnl
#undef MA__MODE
#define MA__MODE MA__MODE_PREFIX
$1]],[[PARTS]])
m4_popdef([[PARTS]])
]])dnl
