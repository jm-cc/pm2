/*	fut.h */

#ifndef __FUT_H__
#define __FUT_H__

/*	fut = Fast User Tracing */



/*	Macros for use from within the kernel */

#if defined(CONFIG_FUT) || defined(CONFIG_FUT_TIME_ONLY)

#define FUT_ALWAYS_PROBE0(KEYMASK,CODE)	do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 12 );\
								} while(0)
#define FUT_ALWAYS_PROBE1(KEYMASK,CODE,P1)	do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 16, \
								(unsigned int)(P1) ); \
								} while(0)
#define FUT_ALWAYS_PROBE2(KEYMASK,CODE,P1,P2)	do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 20, \
								(unsigned int)(P1), (unsigned int)(P2) ); \
								} while(0)
#define FUT_ALWAYS_PROBE3(KEYMASK,CODE,P1,P2,P3) \
								do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 24, \
								(unsigned int)(P1), (unsigned int)(P2), \
								(unsigned int)(P3) ); \
								} while(0)
#define FUT_ALWAYS_PROBE4(KEYMASK,CODE,P1,P2,P3,P4) \
								do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 28, \
								(unsigned int)(P1), (unsigned int)(P2), \
								(unsigned int)(P3), (unsigned int)(P4) ); \
								} while(0)
#define FUT_ALWAYS_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5) \
								do { \
								if( KEYMASK & fut_active ) \
								fut_header( (((unsigned int)(CODE))<<8) | 32, \
								(unsigned int)(P1), (unsigned int)(P2), \
								(unsigned int)(P3), (unsigned int)(P4), \
								(unsigned int)(P5) ); \
								} while(0)

#if defined(CONFIG_FUT_TIME_ONLY)

#define FUT_PROBE0(KEYMASK,CODE)				FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE1(KEYMASK,CODE,P1)				FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE2(KEYMASK,CODE,P1,P2)			FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE3(KEYMASK,CODE,P1,P2,P3)		FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE4(KEYMASK,CODE,P1,P2,P3,P4)	FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5)	FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#else	/* CONFIG_FUT_TIME_ONLY */

#define FUT_PROBE0(KEYMASK,CODE)				FUT_ALWAYS_PROBE0(KEYMASK,CODE)

#define FUT_PROBE1(KEYMASK,CODE,P1)				FUT_ALWAYS_PROBE1(KEYMASK,CODE,P1)

#define FUT_PROBE2(KEYMASK,CODE,P1,P2)			FUT_ALWAYS_PROBE2(KEYMASK,CODE,P1,P2)

#define FUT_PROBE3(KEYMASK,CODE,P1,P2,P3)		FUT_ALWAYS_PROBE3(KEYMASK,CODE,P1,P2,P3)

#define FUT_PROBE4(KEYMASK,CODE,P1,P2,P3,P4)	FUT_ALWAYS_PROBE4(KEYMASK,CODE,P1,P2,P3,P4)

#define FUT_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5)	FUT_ALWAYS_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5)

#endif	/* CONFIG_FUT_TIME_ONLY */

#else	/* CONFIG_FUT or CONFIG_FUT_TIME_ONLY */

#define FUT_ALWAYS_PROBE0(KEYMASK,CODE)					do {} while(0)

#define FUT_ALWAYS_PROBE1(KEYMASK,CODE,P1)				do {} while(0)

#define FUT_ALWAYS_PROBE2(KEYMASK,CODE,P1,P2)			do {} while(0)

#define FUT_ALWAYS_PROBE3(KEYMASK,CODE,P1,P2,P3)		do {} while(0)

#define FUT_ALWAYS_PROBE4(KEYMASK,CODE,P1,P2,P3,P4)		do {} while(0)

#define FUT_ALWAYS_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5)	do {} while(0)

#define FUT_PROBE0(KEYMASK,CODE)						do {} while(0)

#define FUT_PROBE1(KEYMASK,CODE,P1)						do {} while(0)

#define FUT_PROBE2(KEYMASK,CODE,P1,P2)					do {} while(0)

#define FUT_PROBE3(KEYMASK,CODE,P1,P2,P3)				do {} while(0)

#define FUT_PROBE4(KEYMASK,CODE,P1,P2,P3,P4)			do {} while(0)

#define FUT_PROBE5(KEYMASK,CODE,P1,P2,P3,P4,P5)			do {} while(0)

#endif	/* CONFIG_FUT or CONFIG_FUT_TIME_ONLY */


/*	"how" parameter values, analagous to "how" parameters to sigprocmask */
#define FUT_DISABLE		0		/* for disabling probes with 1's in keymask */
#define FUT_ENABLE		1		/* for enabling probes with 1's in keymask */
#define FUT_SETMASK		2		/* for enabling 1's, disabling 0's in keymask */

/*	Simple keymasks */
#define FUT_KEYMASK0				0x00000001	/* IRQs, exceptions, syscalls */
#define FUT_KEYMASK1				0x00000002
#define FUT_KEYMASK2				0x00000004
#define FUT_KEYMASK3				0x00000008
#define FUT_KEYMASK4				0x00000010
#define FUT_KEYMASK5				0x00000020
#define FUT_KEYMASK6				0x00000040
#define FUT_KEYMASK7				0x00000080
#define FUT_KEYMASK8				0x00000100
#define FUT_KEYMASK9				0x00000200
#define FUT_KEYMASK10				0x00000400
#define FUT_KEYMASK11				0x00000800
#define FUT_KEYMASK12				0x00001000
#define FUT_KEYMASK13				0x00002000
#define FUT_KEYMASK14				0x00004000
#define FUT_KEYMASK15				0x00008000
#define FUT_KEYMASK16				0x00010000
#define FUT_KEYMASK17				0x00020000
#define FUT_KEYMASK18				0x00040000
#define FUT_KEYMASK19				0x00080000
#define FUT_KEYMASK20				0x00100000
#define FUT_KEYMASK21				0x00200000
#define FUT_KEYMASK22				0x00400000
#define FUT_KEYMASK23				0x00800000
#define FUT_KEYMASK24				0x01000000
#define FUT_KEYMASK25				0x02000000
#define FUT_KEYMASK26				0x04000000
#define FUT_KEYMASK27				0x08000000
#define FUT_KEYMASK28				0x10000000
#define FUT_KEYMASK29				0x20000000
#define FUT_KEYMASK30				0x40000000
#define FUT_KEYMASK31				0x80000000
#define FUT_KEYMASKALL				0xffffffff


/*	Fixed parameters of the fut coding scheme */
#define FUT_GENERIC_EXIT_OFFSET		0x100	/* exit this much above entry */

/*	Codes for fut use */
#define FUT_SETUP_CODE				0x210
#define FUT_KEYCHANGE_CODE			0x211
#define FUT_RESET_CODE				0x212
#define FUT_CALIBRATE0_CODE			0x220
#define FUT_CALIBRATE1_CODE			0x221
#define FUT_CALIBRATE2_CODE			0x222
#define FUT_SWITCH_TO_CODE			0x230
#define FUT_MAIN_ENTRY_CODE			0x240
#define FUT_MAIN_EXIT_CODE			0x340

/*	Codes for use with fut items */
#if !defined(PREPROC) && !defined(DEPEND) // for PM2
#include "fut_defines.h"
#endif

extern volatile unsigned int fut_active;
extern volatile unsigned int *fut_next_slot;
extern volatile unsigned int *fut_last_slot;

extern int fut_setup( unsigned int nints, unsigned int keymask,
													unsigned int threadid );
extern int fut_endup( char *filename );
extern int fut_done(void );
extern int fut_keychange( int how, unsigned int keymask,
													unsigned int threadid );
extern int fut_reset( unsigned int keymask, unsigned int threadid );
extern int fut_getbuffer( int *nints, unsigned int **buffer );

extern void fut_header( unsigned int head, ... );


#endif
