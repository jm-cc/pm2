
%{
  /*#define YYSTYPE long */
#define YYDEBUG 1
#define LEO_IN_YACC
#define YYERROR_VERBOSE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "leonie.h"
  /* #define DEBUG */

  /* Lex interface */
  extern FILE *yyin, *yyout;
  extern int yylex(void);

  /* Global variables */
  p_leo_parser_result_t leo_parser_result = NULL;

  /* Prototypes */
  void yyerror(char *s); 
%}

%union
{
  char                        *str;
  tbx_bool_t                   bool;
  p_tbx_list_t                 list;
  p_leo_app_makefile_t         app_makefile;
  p_leo_app_channel_t          app_channel;
  p_leo_app_cluster_t          app_cluster;
  p_leo_app_application_t      app_appli;
  p_leo_clu_host_name_t        clu_host_name;
  p_leo_clu_cluster_t          clu_cluster;
  p_leo_clu_model_adapter_t    clu_model_adapter;
  p_leo_clu_host_model_t       clu_host_model;
  p_leo_clu_cluster_file_t     clu_cluster_file;
  p_leo_pro_protocol_t         pro_protocol;
  leo_pro_adapter_selection_t  pro_adapter_select;
  p_leo_parser_result_t        leo_parser_result;
}

%token LEOP_ADAPTER LEOP_ADAPTERS LEOP_ALIAS LEOP_APPLICATION LEOP_ARCHI              LEOP_CHANNELS LEOP_CLUSTER LEOP_DOMAIN LEOP_EXECUTABLE LEOP_HOST               LEOP_HOSTS LEOP_LOADER LEOP_MACRO LEOP_MAKEFILE LEOP_MODEL LEOP_MODULE         LEOP_NAME LEOP_NFS LEOP_NO LEOP_NONE LEOP_OS LEOP_PATH LEOP_PROTOCOL           LEOP_REGISTER LEOP_RULE LEOP_SELECTOR LEOP_SUFFIX LEOP_YES
%token <str> LEOP_ID LEOP_EID LEOP_STRING 
%token '{' '}' ':' ';' ',' INCONNU
%type <str>                leo_app_adapter leo_app_alias
%type <app_channel>        leo_app_channel
%type <list>               leo_app_channel_list leo_app_host_name_list
%type <list>               leo_app_channels leo_app_hosts
%type <str>                leo_app_path leo_app_executable
%type <str>                leo_app_makefile_name leo_app_makefile_rule
%type <app_makefile>       leo_app_makefile
%type <app_cluster>        leo_app_declaration_application leo_app_cluster
%type <list>               leo_app_cluster_list
%type <app_appli>          leo_app_application

%type <list>               leo_clu_host_host_name_list
%type <clu_host_name>      leo_clu_host_name
%type <list>               leo_clu_host_name_list
%type <list>               leo_clu_hosts
%type <clu_cluster>        leo_clu_declaration_cluster 
%type <clu_cluster>        leo_clu_cluster
%type <str>                leo_clu_model_nfs
%type <clu_model_adapter>  leo_clu_model_adapter
%type <list>               leo_clu_model_adapter_list
%type <list>               leo_clu_model_adapter_list_decl
%type <str>                leo_clu_model_archi
%type <str>                leo_clu_model_os 
%type <str>                leo_clu_model
%type <str>                leo_clu_model_domain_id
%type <list>               leo_clu_model_domain_list 
%type <list>               leo_clu_model_domain 
%type <clu_host_model>     leo_clu_declaration_model
%type <clu_host_model>     leo_clu_host_model
%type <list>               leo_clu_host_model_list leo_clu_hosts
%type <clu_cluster_file>   leo_clu_cluster_file

%type <list>               leo_pro_protocol_table leo_pro_protocol_list
%type <pro_protocol>       leo_pro_protocol leo_pro_protocol_definition
%type <str>                leo_pro_macro leo_pro_module leo_pro_register
%type <pro_adapter_select> leo_pro_adapter 
%type <bool>               leo_pro_loader

%type <leo_parser_result>         entree
%%
entree:
   leo_clu_cluster_file
   {
     LOG("cluster");
     $$ = malloc(sizeof(leo_parser_result_t));
     $$->cluster_file   = $1;
     $$->application    = NULL;
     $$->protocol_table = NULL;
     leo_parser_result  = $$;
   }
  |leo_app_application
   {
     LOG("application");
     $$ = malloc(sizeof(leo_parser_result_t));
     $$->cluster_file   = NULL;
     $$->application    = $1;
     $$->protocol_table = NULL;
     leo_parser_result  = $$;
   }
  |leo_pro_protocol_table
   {
     LOG("protocol");
     $$ = malloc(sizeof(leo_parser_result_t));
     $$->cluster_file   = NULL;
     $$->application    = NULL;
     $$->protocol_table = $1;
     leo_parser_result  = $$;
   }
;

leo_clu_cluster_file:
   leo_clu_host_model_list leo_clu_cluster
   {
     $$   = malloc(sizeof(leo_clu_cluster_file_t));
     $$->host_model_list = $1;
     $$->cluster         = $2;
   }
;

leo_clu_host_model_list:
   leo_clu_host_model
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_clu_host_model_list leo_clu_host_model
   {
     tbx_append_list($1, $2);
     $$ = $1;
   }

;

leo_clu_host_model:
   LEOP_HOST LEOP_ID ':' '{' leo_clu_declaration_model '}'
   {
     $$     = $5;
     $$->id = $2;
   }
;

leo_clu_declaration_model:
   leo_clu_model leo_clu_model_domain leo_clu_model_os leo_clu_model_archi leo_clu_model_adapter_list_decl leo_clu_model_nfs
   {
     $$ = malloc(sizeof(leo_clu_host_model_t));
     $$->model        = $1;
     $$->domain_list  = $2;
     $$->os           = $3;
     $$->archi        = $4;
     $$->adapter_list = $5;
     $$->nfs          = $6;
   }
;

leo_clu_model:
   /* empty */ 
   {
      $$ = NULL;
   }
  |LEOP_MODEL ':' LEOP_ID ';' 
   {
     $$ = $3;
   }
;

leo_clu_model_domain:
   /* empty */ 
   {
      $$ = NULL;
   }
  |LEOP_DOMAIN ':' leo_clu_model_domain_list ';' 
   {
     $$ = $3;
   }
;

leo_clu_model_domain_list:
   leo_clu_model_domain_id
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_clu_model_domain_list ',' leo_clu_model_domain_id
   {
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_clu_model_domain_id:
   LEOP_ID
   {
     $$ = $1;
   }
  |LEOP_EID 
   {
     $$ = $1;
   }
;

leo_clu_model_os:
   /* empty */ 
    {
      $$ = NULL;
    }
  |LEOP_OS ':' LEOP_ID ';' 
   {
     $$ = $3;
   }
;

leo_clu_model_archi:
    /* empty */ 
    {
      $$ = NULL;
    }
  |LEOP_ARCHI ':' LEOP_ID ';'
   {
     $$ = $3;
   }
;

leo_clu_model_adapter_list_decl:
     /* empty */ 
    {
      $$ = NULL;
    }
  |LEOP_ADAPTERS ':' leo_clu_model_adapter_list ';'
   {
     $$ = $3;
   }
;

leo_clu_model_adapter_list:
   leo_clu_model_adapter
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_clu_model_adapter_list ',' leo_clu_model_adapter
   {
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_clu_model_adapter:
   '{' LEOP_ALIAS ':' LEOP_ID ';' LEOP_PROTOCOL ':' LEOP_ID ';' '}'
   {
     $$ = malloc(sizeof(leo_clu_model_adapter_t));
     $$->alias    = $4;
     $$->protocol = $8;
   }
;

leo_clu_model_nfs:
    /* empty */ 
    {
      $$ = NULL;
    }
  |LEOP_NFS ':' LEOP_ID ';'
   {
     $$ = $3;
   }
  |LEOP_NFS ':' LEOP_EID ';'
   {
     $$ = $3;
   }
;

leo_clu_cluster:      
   LEOP_CLUSTER LEOP_ID ':' leo_clu_declaration_cluster
   {
     $$     = $4;
     $$->id = $2;
   }
;

leo_clu_declaration_cluster: 
   '{' leo_clu_hosts ';' '}'
   {
     $$           = malloc(sizeof(leo_clu_cluster_t));
     $$->id       = NULL;
     $$->hosts    = $2;
   }
;

leo_clu_hosts: 
   LEOP_HOSTS ':' leo_clu_host_name_list 
   {
     $$ = $3;
   }
;

leo_clu_host_name_list:   
   leo_clu_host_name 
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_clu_host_name_list ',' leo_clu_host_name
   {
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_clu_host_name:
   '{' leo_clu_host_host_name_list ':' LEOP_ID ';' '}'
   {
     $$ = malloc(sizeof(leo_clu_host_name_t));
     $$->name_list = $2;
     $$->model     = $4;
     $$->alias     = NULL;
   }
  |'{' leo_clu_host_host_name_list ':' LEOP_EID ';' '}'
   {
     $$ = malloc(sizeof(leo_clu_host_name_t));
     $$->name_list = $2;
     $$->model     = NULL;
     $$->alias     = $4;
   }
;

leo_clu_host_host_name_list:
   LEOP_ID
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  | leo_clu_host_host_name_list ',' LEOP_ID
   {
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_app_application:      
   LEOP_APPLICATION LEOP_ID ':' '{' leo_app_cluster_list '}'
   {
     $$ = malloc(sizeof(leo_app_application_t));
     $$->id           = $2;
     $$->cluster_list = $5;
   }
;

leo_app_cluster_list:
   leo_app_cluster
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }  
   |leo_app_cluster_list leo_app_cluster
   {
     tbx_append_list($1, $2);
     $$ = $1;
   }
;

leo_app_cluster:
   LEOP_CLUSTER LEOP_ID ':' leo_app_declaration_application
   {
     LOG(".");
     $$     = $4;
     $$->id = $2;
   }
;

leo_app_declaration_application: 
   '{' leo_app_hosts ';' leo_app_executable ';' leo_app_path ';' leo_app_channels ';' leo_app_makefile ';' '}'
   {
     LOG(".");
     $$ = malloc(sizeof(leo_app_cluster_t));
     $$->id         = NULL;
     $$->hosts      = $2;
     $$->executable = $4;
     $$->path       = $6;
     $$->channels   = $8;
     $$->makefile   = $10;
   }
;

leo_app_executable:
   LEOP_EXECUTABLE ':' LEOP_ID
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_path:
   LEOP_PATH ':' LEOP_STRING
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_hosts: 
   LEOP_HOSTS ':' leo_app_host_name_list 
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_channels: 
   LEOP_CHANNELS ':' leo_app_channel_list 
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_host_name_list:   
   LEOP_ID 
   {
     LOG(".");
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_app_host_name_list ',' LEOP_ID 
   {
     LOG(".");
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_app_channel_list:   
   leo_app_channel
   {
     LOG(".");
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
   |leo_app_channel_list ',' leo_app_channel
   {
     LOG(".");
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_app_channel:
   '{' leo_app_alias ';' leo_app_adapter ';' '}'
   {
     LOG(".");
     $$ = malloc(sizeof(leo_app_channel_t));
     $$->alias   = $2; /* alias */
     $$->adapter = $4; /* chapter */
   }
;

leo_app_alias:
  LEOP_ALIAS ':' LEOP_ID
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_adapter:
  LEOP_ADAPTER ':' LEOP_ID
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_makefile:
   LEOP_MAKEFILE ':' '{' leo_app_makefile_name ';' leo_app_makefile_rule ';' '}'
   {
     LOG(".");
     $$ = malloc(sizeof(leo_app_makefile_t));
     $$->name = $4; /* alias */
     $$->rule = $6; /* chapter */
   }
;

leo_app_makefile_name:
   LEOP_NAME ':' LEOP_ID
   {
     LOG(".");
     $$ = $3;
   }
;

leo_app_makefile_rule:
   LEOP_RULE ':' LEOP_ID
   {
     LOG(".");
     $$ = $3;
   }
;


leo_pro_protocol_table:
  leo_pro_protocol_list
   {
     $$ = $1;
   }
;

leo_pro_protocol_list:
  leo_pro_protocol
   {
     $$ = malloc(sizeof(tbx_list_t));
     tbx_list_init($$);
     tbx_append_list($$, $1);
   }
  |leo_pro_protocol_list ';' leo_pro_protocol
   {
     tbx_append_list($1, $3);
     $$ = $1;
   }
;

leo_pro_protocol:
  LEOP_PROTOCOL LEOP_ID ':' leo_pro_protocol_definition
   {
     $$ = $4;
     $$->alias = $2;
   }
;

leo_pro_protocol_definition:
  '{' leo_pro_macro ';' leo_pro_module ';' leo_pro_register ';' leo_pro_adapter ';' leo_pro_loader ';' '}'
  {
    $$ = malloc(sizeof(leo_pro_protocol_t));
    $$->macro          =  $2;
    $$->module_name    =  $4;
    $$->register_func  =  $6;
    $$->adapter_select =  $8;
    $$->external_spawn = $10;
  }
;

leo_pro_macro:
  LEOP_MACRO ':' LEOP_ID
  {
    $$ = $3;
  }
  ;

leo_pro_module:
  LEOP_MODULE ':' LEOP_ID
  {
    $$ = $3;
  }
  ;

leo_pro_register:
  LEOP_REGISTER ':' LEOP_ID
  {
    $$ = $3;
  }
  ;

leo_pro_adapter:
  LEOP_ADAPTER ':' LEOP_NONE
  {
    $$ = leo_pro_adpt_none;
  }
  |LEOP_ADAPTER ':' LEOP_SUFFIX
  {
    $$ = leo_pro_adpt_suffix;
  }
  |LEOP_ADAPTER ':' LEOP_SELECTOR
  {
    $$ = leo_pro_adpt_selector;
  }
  ;

leo_pro_loader:
  LEOP_LOADER ':' LEOP_YES
  {
    $$ = tbx_true;
  }
  |LEOP_LOADER ':' LEOP_NO
  {
    $$ = tbx_false;
  }
  ;

%%

#include <stdlib.h>
#include <stdio.h>
#define DEBUG_YACC

void yyerror(char *s)
{
  fflush(stdout);
  fprintf(stderr, "%s\n", s);
}

