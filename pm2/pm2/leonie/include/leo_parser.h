typedef union
{
  char                   *str;
  tbx_bool_t              bool;
  p_tbx_list_t            list;
  p_leo_app_makefile_t    app_makefile;
  p_leo_app_channel_t     app_channel;
  p_leo_app_application_t app_appli;
  p_leo_clu_adapter_t     clu_adapter;
  p_leo_clu_cluster_t     clu_cluster;
  p_leo_pro_protocol_t    pro_protocol;
  leo_pro_adapter_selection_t pro_adapter_select;
  p_leo_parser_result_t   leo_parser_result;
} YYSTYPE;
#define	CLUSTER	257
#define	HOSTS	258
#define	PROTOCOL	259
#define	ALIAS	260
#define	SELECTOR	261
#define	SUFFIX	262
#define	ADAPTERS	263
#define	ADAPTER	264
#define	APPLICATION	265
#define	EXECUTABLE	266
#define	PATH	267
#define	CHANNELS	268
#define	LOADER	269
#define	MACRO	270
#define	MODULE	271
#define	NFS	272
#define	REGISTER	273
#define	YES	274
#define	NO	275
#define	MAKEFILE	276
#define	NAME	277
#define	RULE	278
#define	NONE	279
#define	ID	280
#define	EID	281
#define	STRING	282
#define	INCONNU	283


extern YYSTYPE yylval;
