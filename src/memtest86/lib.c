/* lib.c - MemTest-86  Version 4.1
 *
 * Released under version 2 of the Gnu Public License.
 * By Chris Brady
 * Copyright 2016-2017 Reneo, Inc; Author: Minesh B. Amin
 */
#include "io.h"
#include "serial.h"
#include "test.h"
#include "config.h"
#include "stdint.h"
#include "cpuid.h"
#include "smp.h"
#include "rlib.h"

int slock = 0, lsr = 0;
#if ((115200%SERIAL_BAUD_RATE) != 0)
#error Bad default baud rate
#endif
const short         serial_tty          = SERIAL_TTY;
const short         serial_base_ports[] = {0x3f8, 0x2f8};
const int           serial_baud_rate    = SERIAL_BAUD_RATE;
const unsigned char serial_parity       = 0;
const unsigned char serial_bits         = 8;

struct ascii_map_str {
        int ascii;
        int keycode;
};

inline void reboot(void)
{
	/* tell the BIOS to do a warm start */
	*((unsigned short *)0x472) = 0x1234;
	outb(0xfe,0x64);
}

void *memmove(void *dest, const void *src, ulong n)
{
	long i;
	char *d = (char *)dest, *s = (char *)src;

	/* If src == dest do nothing */
	if (dest < src) {
		for(i = 0; i < n; i++) {
			d[i] = s[i];
		}
	}
	else if (dest > src) {
		for(i = n -1; i >= 0; i--) {
			d[i] = s[i];
		}
	}
	return dest;
}

void memcpy(void *dst, void *src, int len)
{
	char *s = (char*)src;
	char *d = (char*)dst;
	int i;

	if (len <= 0) {
		return;
	}
	for (i = 0 ; i < len; i++) {
		*d++ = *s++;
	} 
}

const char * const codes[] = {
  iInt_00,
  iInt_01,
  iInt_02,
  iInt_03,
  iInt_04,
  iInt_05,
  iInt_06,
  iInt_07,
  iInt_08,
  iInt_09,
  iInt_10,
  iInt_11,
  iInt_12,
  iInt_13,
  iInt_14,
  iInt_15,
  iInt_16,
  iInt_17,
  iInt_18,
  iInt_19,
};

struct eregs {
	ulong ss;
	ulong ds;
	ulong esp;
	ulong ebp;
	ulong esi;
	ulong edi;
	ulong edx;
	ulong ecx;
	ulong ebx;
	ulong eax;
	ulong vect;
	ulong code;
	ulong eip;
	ulong cs;
	ulong eflag;
};
	
/* Handle an interrupt */
void inter(struct eregs *trap_regs)
{
	ulong address = 0;

	/* Get the page fault address */
	if (trap_regs->vect == 14) {
		__asm__("movl %%cr2,%0":"=r" (address));
	}
#ifdef PARITY_MEM

	/* Check for a parity error */
	if (trap_regs->vect == 2) {
		parity_err(trap_regs->edi, trap_regs->esi);
		return;
	}
#endif

   print_iInterrupt(trap_regs, address);
   halt();
}

void set_cache(int val) 
{
	switch(val) {
	case 0:
		cache_off();	
		break;
	case 1:
		cache_on();
		break;
	}
}

void serial_echo_init(void)
{
	int comstat, serial_div;
	unsigned char lcr;	

	/* read the Divisor Latch */
	comstat = serial_echo_inb(UART_LCR);
	serial_echo_outb(comstat | UART_LCR_DLAB, UART_LCR);
	serial_echo_inb(UART_DLM);
	serial_echo_inb(UART_DLL);
	serial_echo_outb(comstat, UART_LCR);

	/* now do hardwired init */
	lcr = serial_parity | (serial_bits - 5);
	serial_echo_outb(lcr, UART_LCR); /* No parity, 8 data bits, 1 stop */
	serial_div = 115200 / serial_baud_rate;
	serial_echo_outb(0x80|lcr, UART_LCR); /* Access divisor latch */
	serial_echo_outb(serial_div & 0xff, UART_DLL);  /* baud rate divisor */
	serial_echo_outb((serial_div >> 8) & 0xff, UART_DLM);
	serial_echo_outb(lcr, UART_LCR); /* Done with divisor */

	/* Prior to disabling interrupts, read the LSR and RBR
	 * registers */
	comstat = serial_echo_inb(UART_LSR); /* COM? LSR */
	comstat = serial_echo_inb(UART_RX);	/* COM? RBR */
	serial_echo_outb(0x00, UART_IER); /* Disable all interrupts */

	return;
}

// ----------------------------------------------------------------------------

#include "rlib.c"

// ----------------------------------------------------------------------------
