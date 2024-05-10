/************************************************************************/
/*                                                                      */
/*             CP/M Hardware Emulator Card Support Program              */
/*                       CPMEMUINT.C Ver 1.45 (QC 2.00 only)            */
/*                 Copyright (c) By C.J.Chen NTUEE 1988                 */
/*                         All Right Reserved                           */
/*                                                                      */
/************************************************************************/

#include <dos.h>
#include <setjmp.h>
extern unsigned stack_segment;
extern jmp_buf ctrl_c;
extern char in_bios;
void cdecl interrupt far handle_ctrl_c()
{
	_enable();
	if (in_bios == 1) return;
	else {
		_asm    mov     ax,word ptr stack_segment
		_asm    mov     ss,ax
		longjmp(ctrl_c,0);
	}
}
