
# MAKEFILE_FILE -> nom par defaut des makefiles 
#---------------------------------------------------------------------
MAKEFILE_FILE := Makefile

# CC, AS, LD -> Commandes de construction
#---------------------------------------------------------------------
#CC := gcc #set with "pm2-config --cc"
AS := gcc # needed for some gcc specific flags
LD := gcc # needed for some gcc specific flags

# Categorie(s) de librairies a construire
#---------------------------------------------------------------------
BUILD_STATIC_LIBS := true
#BUILD_DYNAMIC_LIBS := true

# Controle du contenu de l'affichage
#---------------------------------------------------------------------
#SHOW_FLAGS=true
SHOW_FLAVOR=true

# Controle du niveau d'affichage
#---------------------------------------------------------------------
ifdef VERB
MAK_VERB :=  $(VERB)
else
#MAK_VERB :=  verbose
#MAK_VERB :=  normal
MAK_VERB :=  quiet
#MAK_VERB :=  silent
endif

