PM2_NMAD_LIBNAME=nmad
PM2_NMAD_MODULE_DEPEND_LIB="${PM2_NMAD_MODULE_DEPEND_LIB} tbx ntbx marcel"

PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/core/include"

# 'session' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/interfaces/session/include"
# 'sendrecv' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/interfaces/sendrecv/include"
# 'pack' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/interfaces/pack/include"
# 'launcher' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/interfaces/launcher/include"
# 'bmi' interface
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/interfaces/bmi/include"
# need Datatype per default 
PM2_NMAD_CFLAGS="$PM2_NMAD_CFLAGS -I${PM2_SRCROOT}/nmad/ccs"

PM2_DEFAULT_LOADER=${PM2_SRCROOT}/nmad/bin/nmadload_conf_not_needed

PM2_NMAD_EARLY_LDFLAGS="$PM2_NMAD_EARLY_LDFLAGS -Wl,--export-dynamic"

PM2_NMAD_LIBS="$PM2_NMAD_LIBS -lrt"

# list of component. Most of work is done in nmad-post.sh.
PM2_NMAD_COMPONENTS="NewMad_Launcher_cmdline"
