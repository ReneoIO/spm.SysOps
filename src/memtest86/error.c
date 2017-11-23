/* error.c - MemTest-86  Version 4.1
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * Copyright 2016-2017 Reneo, Inc; Author: Minesh B. Amin
 */
#include "stdint.h"
#include "test.h"
#include "config.h"
#include "cpuid.h"
#include "smp.h"
#include "rlib.h"

extern struct cpu_ident cpu_id;
extern struct barrier_s *barr;
extern int test_ticks, nticks;
extern struct tseq tseq[];
extern volatile int test;
extern int smp_ord_to_cpu(int me);
void poll_errors();

static void update_err_counts(void);

#define MAX_ERRORS 0xFFFF

/*
 * Display data error message. Don't display duplicate errors.
 */
void error(ulong *adr, ulong good, ulong bad)
{
	ulong xor;

	spin_lock(&barr->mutex);
	xor = good ^ bad;
#ifdef USB_WAR
	/* Skip any errrors that appear to be due to the BIOS using location
	 * 0x4e0 for USB keyboard support.  This often happens with Intel
         * 810, 815 and 820 chipsets.  It is possible that we will skip
	 * a real error but the odds are very low.
	 */
	if ((ulong)adr == 0x4e0 || (ulong)adr == 0x410) {
		return;
	}
#endif
	common_err(adr, good, bad, xor, 0);
	spin_unlock(&barr->mutex);
}

/*
 * Display address error message.
 * Since this is strictly an address test, trying to create BadRAM
 * patterns does not make sense.  Just report the error.
 */
void ad_err1(ulong *adr1, ulong *mask, ulong bad, ulong good)
{
	spin_lock(&barr->mutex);
	common_err(adr1, good, bad, (ulong)mask, 1);
	spin_unlock(&barr->mutex);
}

/*
 * Display address error message.
 * Since this type of address error can also report data errors go
 * ahead and generate BadRAM patterns.
 */
void ad_err2(ulong *adr, ulong bad)
{
	spin_lock(&barr->mutex);
	common_err(adr, (ulong)adr, bad, ((ulong)adr) ^ bad, 0);
	spin_unlock(&barr->mutex);
}

static void update_err_counts(void)
{
	if (v->ecount < MAX_ERRORS)
		++(v->ecount);

	if (tseq[test].errors < MAX_ERRORS)
		tseq[test].errors++;
}

/*
 * Print an individual error
 */
void common_err( ulong *adr, ulong good, ulong bad, ulong xor, int type) 
{
	update_err_counts();
   print_iErrors(v->ecount);
}

#ifdef PARITY_MEM
/*
 * Print a parity error message
 */
void parity_err( unsigned long edi, unsigned long esi) 
{
	unsigned long addr;

	if (test == 5) {
		addr = esi;
	} else {
		addr = edi;
	}
	common_err((ulong *)addr, addr & 0xFFF, 0, 0, 3);
}
#endif

/*
 * Show progress by displaying elapsed time and update bar graphs
 */
short spin_idx[MAX_CPUS];
char spin[4] = {'|','/','-','\\'};

void do_tick(int me)
{
	int i, pct, cpu;
	extern int mstr_cpu;

	/* Get the cpu number */
	cpu = smp_ord_to_cpu(me);
	if (++spin_idx[cpu] > 3) {
		spin_idx[cpu] = 0;
	}
	cplace(8, cpu+7, spin[spin_idx[cpu]]);
	
	/* Check for keyboard input */
	if (me == mstr_cpu) {
		check_input();
	}
	/* A barrier here holds the other CPUs until the configuration
	 * changes are done */
	s_barrier();

	/* Only the first selected CPU does the update */
	if (me !=  mstr_cpu) {
		return;
	}

	/* FIXME only print serial error messages from the tick handler */
	if (v->ecount) {
     print_iErrors(v->ecount);
	}
	
	nticks++;
	v->total_ticks++;

	if (test_ticks) {
		pct = 100*nticks/test_ticks;
		if (pct > 100) {
			pct = 100;
		}
	} else {
		pct = 0;
	}
   print_pTest(pct);
	i = (BAR_SIZE * pct) / 100;
	while (i > v->tptr) {
		if (v->tptr >= BAR_SIZE) {
			break;
		}
		cprint(2, COL_MID+9+v->tptr, "#");
		v->tptr++;
	}
	
	if (v->pass_ticks) {
		pct = 100*v->total_ticks/v->pass_ticks;
		if (pct > 100) {
			pct = 100;
		}
	} else {
		pct = 0;
        }
   print_pPass(pct);
	i = (BAR_SIZE * pct) / 100;
	while (i > v->pptr) {
		if (v->pptr >= BAR_SIZE) {
			break;
		}
		cprint(1, COL_MID+9+v->pptr, "#");
		v->pptr++;
	}
}
