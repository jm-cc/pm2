/*	sysmap.h

	Copyright (C) 2003 Samuel Thibault -- samuel.thibault@fnac.net

*/

/* symbols come from 2 main places: /proc, ie the current running kernel, which
 * is a reliable source, and sysmap files, /boot/System.map or nms of modules
 *
 * the same symbols might be statically defined in several places, but symboles
 * from /proc *must* be found in sysmap files, with at least one definition with
 * the same address. */


typedef enum { SYSMAP_FILE, SYSMAP_MODLIST, SYSMAP_PROC } sysmap_source;

void get_sysmap( char *systemmap, sysmap_source source, unsigned long base );
void get_mysymbols( void );
void record_sysmap( int fd );
void load_sysmap( int fd );
char *lookup_symbol( unsigned long address );
