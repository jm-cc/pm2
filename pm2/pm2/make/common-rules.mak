
# Regle par defaut : aide
#---------------------------------------------------------------------
help:

# Declarations Phony
#---------------------------------------------------------------------
.PHONY: all
.PHONY: help globalhelp bannerhelp targethelp commonoptionshelp
.PHONY: bannertargethelp optionshelp thankshelp tailhelp

# Aide
#--------------------------------------------------------------------
globalhelp: bannerhelp bannertargethelp targethelp commonoptionshelp optionshelp thankshelp tailhelp

# Note: interet ?
bannerhelp:

# Note: interet ?
targethelp:

bannertargethelp:
	@echo ""
	@echo "Targets:"

commonoptionshelp:
	@echo
	@echo "Common options:"
	@echo "  FLAVOR=flavor: build for flavor 'flavor'"
	@echo "  BUILD_STATIC_LIBS=true: build static libraries"
	@echo "  BUILD_DYNAMIC_LIBS=true: build dynamic libraries"
	@echo "  SHOW_FLAGS=true: show CFLAGS used"
	@echo "  SHOW_FLAVOR=true: show flavor target"
	@echo "  VERB=verbose|normal|quiet|silent: vebosity of building process"
	@echo
	@echo "Most of these options can be given with command line or"
	@echo "configurated in $(PM2_ROOT)/make/config.mak"

# Note: interet ?
suboptionshelp:

thankshelp:
	@echo
	@echo "Troubles ? Contact Luc.Bouge@ens-lyon.fr"
	@echo "Have a good day with PM2"

# Note: interet ?
tailhelp:

# Construction du repertoire destination des makefiles generes
#---------------------------------------------------------------------
$(PM2_MAK_DIR):
	@mkdir -p $@
