
LIBS := pm2 marcel mad1 mad2 tbx ntbx common

all:

duplique_global:
	@for file in *.sh ; do \
		name=`basename $$file .sh` ; \
		echo "duplicating $$name" ; \
		for lib in $(LIBS); do \
			LIB=`echo $$lib | tr [a-z] [A-Z]`; \
			cp $$name.help \
				$$PM2_ROOT/modules/$$lib/config/options ; \
			[ -f $$name.dep ] && cp $$name.dep \
				$$PM2_ROOT/modules/$$lib/config/options ; \
			sed -e "s,LIB_,$${LIB}_,g" < $$file \
			   > $$PM2_ROOT/modules/$$lib/config/options/$$file ; \
		done ; \
	done
