/* $Id: os_timer.c 7666 2006-12-30 23:04:14Z Darkvater $ */

#include "stdafx.h"

#undef RDTSC_AVAILABLE

/* rdtsc for MSC_VER, uses simple inline assembly, or _rdtsc
 * from external win64.asm because VS2005 does not support inline assembly */
#if defined(_MSC_VER) && !defined(RDTSC_AVAILABLE)
# if defined (_M_AMD64)
extern uint64 _rdtsc(void);
#	else
uint64 _declspec(naked) _rdtsc(void)
{
	_asm {
		rdtsc
		ret
	}
}
# endif
# define RDTSC_AVAILABLE
#endif

/* rdtsc for OS/2. Hopefully this works, who knows */
#if defined (__WATCOMC__) && !defined(RDTSC_AVAILABLE)
unsigned __int64 _rdtsc( void);
# pragma aux _rdtsc = 0x0F 0x31 value [edx eax] parm nomemory modify exact [edx eax] nomemory;
# define RDTSC_AVAILABLE
#endif

/* rdtsc for all other *nix-en (hopefully). Use GCC syntax */
#if defined(__i386__) || defined(__x86_64__) && !defined(RDTSC_AVAILABLE)
uint64 _rdtsc(void)
{
	uint32 high, low;
	__asm__ __volatile__ ("rdtsc" : "=a" (low), "=d" (high));
	return ((uint64)high << 32) | low;
}
# define RDTSC_AVAILABLE
#endif

/* rdtsc for PPC which has this not */
#if (defined(__POWERPC__) || defined(__powerpc__)) && !defined(RDTSC_AVAILABLE)
uint64 _rdtsc(void)
{
	uint32 high = 0, high2 = 0, low;
	/* PPC does not have rdtsc, so we cheat by reading the two 32-bit time-counters
	 * it has, 'Move From Time Base (Upper)'. Since these are two reads, in the
	 * very unlikely event that the lower part overflows to the upper part while we
	 * read it; we double-check and reread the registers */
	asm volatile (
				  "mftbu %0\n"
				  "mftb %1\n"
				  "mftbu %2\n"
				  "cmpw %3,%4\n"
				  "bne- $-16\n"
				  : "=r" (high), "=r" (low), "=r" (high2)
				  : "0" (high), "2" (high2)
				  );
	return ((uint64)high << 32) | low;
}
# define RDTSC_AVAILABLE
#endif

/* In all other cases we have no support for rdtsc. No major issue,
 * you just won't be able to profile your code with TIC()/TOC() */
#if !defined(RDTSC_AVAILABLE)
#warning "(non-fatal) No support for rdtsc(), you won't be able to profile with TIC/TOC"
uint64 _rdtsc(void) {return 0;}
#endif
