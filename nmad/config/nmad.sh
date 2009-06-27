PM2_NMAD_LIBNAME=nmad
PM2_NMAD_MODULE_DEPEND_LIB="${PM2_NMAD_MODULE_DEPEND_LIB} tbx ntbx marcel"

PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/core/include"

# 'sendrecv' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/interfaces/sendrecv/include"
# 'pack' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/interfaces/pack/include"
# 'launcher' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/interfaces/launcher/include"
# 'bmi' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/interfaces/bmi/include"
# sampling is included by default
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/drivers/sampling/include"
# need Datatype per default 
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_ROOT}/nmad/ccs"

PM2_DEFAULT_LOADER=${PM2_ROOT}/nmad/bin/nmadload_conf_not_needed

PM2_NMAD_EARLY_LDFLAGS="$PM2_NMAD_EARLY_LDFLAGS -Wl,--export-dynamic"

# list of component. Most of work is done in nmad-post.sh.
PM2_NMAD_COMPONENTS=
