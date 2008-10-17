# add default interfaces
PM2_NMAD_INTERFACES="sendrecv launcher pack $PM2_NMAD_INTERFACES"

# add default strategies
PM2_NMAD_STRATEGIES="default aggreg aggreg_extended aggreg_autoextended split_balance $PM2_NMAD_STRATEGIES"

# compute list of components
PM2_NMAD_COMPONENTS=
for d in $PM2_NMAD_DRIVERS; do
    PM2_NMAD_COMPONENTS="$PM2_NMAD_COMPONENTS NewMad_Driver_${d}"
done
for s in $PM2_NMAD_STRATEGIES; do
    PM2_NMAD_COMPONENTS="$PM2_NMAD_COMPONENTS NewMad_Strategy_${s}"
done

# force inclusion of components in binary (they may seem unused for ld)
for c in $PM2_NMAD_COMPONENTS; do
    PM2_NMAD_EARLY_LDFLAGS="$PM2_NMAD_EARLY_LDFLAGS -Wl,-upadico_module__${c}"
done
