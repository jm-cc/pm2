strat=`echo $1 | tr a-z A-Z`
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -DCONFIG_STRAT_$strat"
PM2_NMAD_STRAT="$strat"
