/************************************************************************/
/*                                                                      */
/*             CP/M Hardware Emulator Card Support Program              */
/*                       CPM-BIOS.C Ver 1.10                            */
/*                 Copyright (c) By C.J.Chen NTUEE 1988                 */
/*                        All Right Reserved                            */
/*                                                                      */
/************************************************************************/
#include <stdio.h>
#include <conio.h>
#include <bios.h>
#define		CHANGE_PORT	0x2f0
#define		ReturnZ80	{ outp(CHANGE_PORT,0); in_bios = 0; }
extern int test;
extern char far *bioscode;
extern char far *bdoscall;
extern char far *eop;
extern char far *fcbdn;
extern char far *fcbfn;
extern char far *fcbft;
extern char far *fcbrl;
extern char far *fcbrc;
extern char far *fcbcr;
extern char far *fcbln;
extern unsigned int dmaaddr;
extern char far *rega;
extern char far *regl;

extern int far *regaf;
extern int far *regbc;
extern int far *regde;
extern int far *reghl;
extern int far *regix;
extern int far *regiy;
extern int far *regsp;
extern int tmp;
char in_bios;
/*----------------------------------------------------------------------*/
initialbios()
{
}
/*----------------------------------------------------------------------*/
cpmbios()
{
	in_bios = 1;
	switch (*bioscode) {
		case 1:bios01();break;       /* coldboot */
		case 2:bios02();break;       /* warmboot */
		case 3:bios03();break;       /* console status */
		case 4:bios04();break;       /* console input */
		case 5:bios05();break;       /* console output */
		case 6:bios06();break;       /* lister output */
		case 7:bios07();break;       /* punch output */
		case 8:bios08();break;       /* reader input */
		case 9:bios09();break;       /* home disk */
		case 10:bios10();break;      /* select disk */
		case 11:bios11();break;      /* set track */
		case 12:bios12();break;      /* set sector */
		case 13:bios13();break;      /* set dma */
		case 14:bios14();break;      /* read sector */
		case 15:bios15();break;      /* write sector */
		case 16:bios16();break;      /* list status */
		default:ReturnZ80;
	}
}
/*----------------------------------------------------------------------*/
bios01()        /* cold boot */
{
	dmaaddr = 0x0080;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios02()        /* warm boot */
{
	dmaaddr = 0x0080;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios03()        /* console status */
{
	*rega = (char)_bios_keybrd(_KEYBRD_READY);
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios04()        /* console input */
{
	static char keybuf = 0;
	unsigned keypress;
	char *ptr1;

	if (keybuf == 0x00)
		{
			keypress = _bios_keybrd(_KEYBRD_READ);
			if ((keypress) == 0x2e03) *rega = 0x03;
			else if ((keypress) == 0x0300) *rega = 0x00;
			else
			{
				ptr1 = (char *)&keypress;
				*rega = *ptr1++;
				if (*rega == 0x00) keybuf = *ptr1;
			}
		}
	else
		{
			*rega = keybuf;
			keybuf = 0x00;
		}

		/*  *rega = getch();*/
		ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios05()            /* console output */
{
register test;
	/*putch(*rega);*/
	union REGS regs;

	regs.h.dl = *rega;
	ReturnZ80;
	regs.h.ah = 0x02;
	/* if (regs.h.dl > 0x80) regs.h.dl -= 0x80;*/
	int86(0x21,&regs,&regs);
}
/*----------------------------------------------------------------------*/
bios06()        /* lister output */
{
	_bios_printer(_PRINTER_WRITE,0,(unsigned)*rega);
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios07()        /* punch output */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios08()        /* reader input */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios09()        /* home disk */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios10()        /* select disk */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios11()        /* set track */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios12()        /* set sector */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios13()        /* set dma */
{
	dmaaddr = *regbc;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios14()        /* read sector */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios15()        /* write sector */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bios16()        /* list status */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
