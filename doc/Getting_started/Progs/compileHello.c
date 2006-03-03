# Assuming you use csh...
setenv CC       "`pm2-config --cc`"
setenv CFLAGS   "`pm2-config --cflags`"
setenv LIBS     "`pm2-config --libs`" 
$CC $CFLAGS hello.c $LIBS -o hello
