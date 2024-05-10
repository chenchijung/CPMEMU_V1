/************************************************************************/
/*									*/
/*	     CP/M Hardware Emulator Card Support Program		*/
/*			     BDOS Part					*/
/*			 CPM-BDOS.C Ver 1.20				*/
/*	       Copyright (c) By Chen Chi-Jung NTUEE 1988		*/
/*			All Right Reserved				*/
/*									*/
/************************************************************************/
#define	 MAXFILE	 20      /* CP/M Max File definition */
#define  BASEADDR        0XE0000000
#define	 BASESEG	 (BASEADDR / 0X10000)
#define  CHANGE_PORT     0X2F0
#define  ReturnZ80       outp(CHANGE_PORT,0)
/*
** Include File Defination
*/
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <conio.h>
#include <bios.h>
#include <dos.h>
#include <io.h>
/*
**  External Grobol Variable Used
*/
extern char far *bioscode; /* Variable to indicate Bios call code */
extern char far *bdoscall; /* Variable to indicate This call is bdos-call*/
extern char far *eop;      /* End-of-program flag  */
extern char far *fcbdn;    /* CP/M FCB Drive Name  */
extern char far *fcbfn;    /* CP/M FCB Filename    */
extern char far *fcbft;    /* CP/M FCB Filetype    */
extern char far *fcbrl;
extern char far *fcbrc;
extern char far *fcbcr;
extern char far *fcbln;
extern unsigned int dmaaddr;  /* CP/M Dma address */

extern char far *rega;  /* Z80 register A backup address pointer */
extern char far *regb;  /* Z80 register B backup address pointer */
extern char far *regc;  /* Z80 register C backup address pointer */
extern char far *regd;  /* Z80 register D backup address pointer */
extern char far *rege;  /* Z80 register E backup address pointer */
extern char far *regh;  /* Z80 register H backup address pointer */
extern char far *regl;  /* Z80 register L backup address pointer */

extern int far *regaf;  /* Z80 register AF backup address pointer */
extern int far *regbc;  /* Z80 register BC backup address pointer */
extern int far *regde;  /* Z80 register DE backup address pointer */
extern int far *reghl;  /* Z80 register HL backup address pointer */
extern int far *regix;  /* Z80 register IX backup address pointer */
extern int far *regiy;  /* Z80 register IY backup address pointer */
extern int far *regip;  /* Z80 register IP backup address pointer */
extern int far *regsp;  /* Z80 register SP backup address pointer */
extern int tmp;
extern jmp_buf ctrl_c;  /* Jump struct for longjump when ctrl-c happen */
extern jmp_buf end_of_execution;

extern FILE *lpt;       /* Debug File pointer  */
extern char DebugFlag;  /* Debug status flag */

/*
** File System Handler Variable
*/
FILE *fcbfile[MAXFILE];		 /* File Pointer Array */
unsigned char fcbused[MAXFILE][15];     /* Filename array     */
long fcbfilelen[MAXFILE];	       /* File length  array */
/*
**  Grobol Variable Used in This File only
*/
char user_code = 0;		    /* CP/M user code */
struct find_t search_file;	   /* structure for Findfirst & Findnext */
char tmp1[128];		      /* R/W File Data Buffer  */
char filename[15];		   /* Filename Storage space */
long offset = 0L;		      /* File Seek Offset for File R/W  */
ldiv_t result;
unsigned repeats = 1;
unsigned char lastcall = 0xFF;
char debugmess1[50];
char debugmess2[50];
/*----------------------------------------------------------------------*/
/* filloffset: Return Offset Value indicates by FCB pointer by HL       */
/*     offset = (fcbrl * 256 + fcbcr) * 128				*/
/*----------------------------------------------------------------------*/
filloffset(long *offset)
{
	char far *ptr1;
	char far *ptr2;
	char *ptr3;

	*offset = 0L;
	ptr1 = fcbrl + *regde;
	ptr2 = fcbcr + *regde;
	ptr3 = (char *)offset;
	ptr3++;
	*ptr3++ = *ptr2;
	*ptr3 = *ptr1;
	*offset >>= 1;
	return(0);
}
/*----------------------------------------------------------------------*/
/* fcbtofilename:							*/
/*	 Input: ptr1: pointer indicate the filename in a FCB		*/
/*		ptr2: pointer to a buffer				*/
/*	Output: Filename in the buffer pointer by ptr2			*/
/*----------------------------------------------------------------------*/
fcbtofilename(char far *ptr1,char *ptr2)
{
	char i;

	/* if (*ptr1 != 0x00) { *ptr2++ = (*ptr1++)+64; *ptr2++ = ':'; }
	else*/
	ptr1++;
	for (i = 0 ; i < 8 ; i++) 
		if(*ptr1 != ' ') *ptr2++ = *ptr1++;
		else ptr1++;
	*ptr2++ = '.';
	for (i = 0 ; i < 3 ; i++)
		if(*ptr1 != ' ') *ptr2++ = *ptr1++; 
		else ptr1++;
	*ptr2 = 0x00;
}
/*----------------------------------------------------------------------*/
/*  opensuccess: If we open a File required by CP/M Successfully,       */
/*	       Then we call this function to fill the table.	  */
/*----------------------------------------------------------------------*/
opensuccess(char fcbptr)
{
	char far *ptr1;

	ptr1 = fcbcr + (*regde);
	*ptr1 = 0x00;				/* Fill cr  =  0 */
	ptr1 = fcbrl + (*regde);
	*ptr1 = 0x00;				/* Fill rl  =  0 */
	fseek(fcbfile[fcbptr],0L,SEEK_END);	/* Move to EOF */
	fcbfilelen[fcbptr] = ftell(fcbfile[fcbptr]); /* Fill File Length */
	rewind(fcbfile[fcbptr]);		   /* Restore file */
	strcpy(fcbused[fcbptr],filename);	  /* Fill Filename to table */
}
/*----------------------------------------------------------------------*/
PrintDebug()
{
	char name[20],newname[20];
	if (lastcall == *regc) repeats++;
	else {
		if ( *regc >= 15 ) {
			fcbtofilename(fcbdn + *regde, name);
			fcbtofilename(fcbrc + *regde + 1, newname);
		}
		if (lastcall < 39) {
			fprintf(lpt,"% 5d  %s%s\n",
				repeats, debugmess1, debugmess2);
		}
		sprintf(debugmess1,"%04XH    %04XH     %04XH   %02XH "
				,*regip,*regde,dmaaddr,*regc);
		switch ( *regc ) {
			case 0: sprintf(debugmess2,"System reset ");
				break;
			case 1: sprintf(debugmess2,"Console input");
				break;
			case 2: sprintf(debugmess2,"Console output");
				break;
			case 3: sprintf(debugmess2,"Reader input");
				break;
			case 4: sprintf(debugmess2,"Punch output");
				break;
			case 5: sprintf(debugmess2,"List output");
				break;
			case 6: sprintf(debugmess2,"Direct console i/o");
				break;
			case 7: sprintf(debugmess2,"Get iobyte");
				break;
			case 8: sprintf(debugmess2,"Set iobyte");
				break;
			case 9: sprintf(debugmess2,"Print string");
				break;
			case 10: sprintf(debugmess2,"Read console buffer");
				 break;
			case 11: sprintf(debugmess2,"Get console status");
				 break;
			case 12: sprintf(debugmess2,"Get version number");
				 break;
			case 13: sprintf(debugmess2,"Reset disk system");
				 break;
			case 14: sprintf(debugmess2,"Select disk");
				 break;
			case 15: sprintf(debugmess2,"Open file %s",name);
				 break;
			case 16: sprintf(debugmess2,"Close file %s",name);
				 break;
			case 17: sprintf(debugmess2,"Search for first");
				 break;
			case 18: sprintf(debugmess2,"Search for next");
				 break;
			case 19: sprintf(debugmess2,"Delete file %s",name);
				 break;
			case 20: sprintf(debugmess2,"Read sequential %s"
					,name);
				 break;
			case 21: sprintf(debugmess2,"Write sequential %s"
				 	,name);
				 break;
			case 22: sprintf(debugmess2,"Make file %s",name);
				 break;
			case 23: sprintf(debugmess2,"Rename file %s to %s"
					,name,newname);
				 break;
			case 24: sprintf(debugmess2,"Return login vector");
				 break;
			case 25: sprintf(debugmess2,"Return current disk");
				 break;
			case 26: sprintf(debugmess2,"Set DMA address");
				 break;
			case 27: sprintf(debugmess2,"Get alloc address");
				 break;
			case 28: sprintf(debugmess2,"Write protect disk");
				 break;
			case 29: sprintf(debugmess2,"Get r/o vector");
				 break;
			case 30: sprintf(debugmess2,"Set file attributes");
				 break;
			case 31: sprintf(debugmess2,"Get disk parms");
				 break;
			case 32: sprintf(debugmess2,"Get & set user code");
				 break;
			case 33: sprintf(debugmess2,"Read random %s",name);
				 break;
			case 34: sprintf(debugmess2,"Write random %s",name);
				 break;
			case 35: sprintf(debugmess2,"Compute file size");
				 break;
			case 36: sprintf(debugmess2,"Set random record");
				 break;
			case 37: sprintf(debugmess2,"Reset drive");
				 break;
			case 38: sprintf(debugmess2,"Write random (zero)");
				 break;
		}
		repeats = 1;
		lastcall = *regc;
	}
}
/*----------------------------------------------------------------------*/
initialbdos()
{
}
/*----------------------------------------------------------------------*/
cpmbdos()
{
	if (DebugFlag == 1 && lpt != NULL) PrintDebug();
	switch ( *regc ) {
		case 0: bdos00(); break;       /* system reset */
		case 1: bdos01(); break;       /* console input */
		case 2: bdos02(); break;       /* console output */
		case 3: bdos03(); break;       /* reader input */
		case 4: bdos04(); break;       /* punch output */
		case 5: bdos05(); break;       /* list output */
		case 6: bdos06(); break;       /* direct console i/o */
		case 7: bdos07(); break;       /* get iobyte */
		case 8: bdos08(); break;       /* set iobyte */
		case 9: bdos09(); break;       /* print string */
		case 10: bdos10(); break;      /* read console buffer */
		case 11: bdos11(); break;      /* get console status */
		case 12: bdos12(); break;      /* get version number */
		case 13: bdos13(); break;      /* reset disk system */
		case 14: bdos14(); break;      /* select disk */
		case 15: bdos15(); break;      /* open file */
		case 16: bdos16(); break;      /* close file */
		case 17: bdos17(); break;      /* search for first */
		case 18: bdos18(); break;      /* search for next */
		case 19: bdos19(); break;      /* delete file */
		case 20: bdos20(); break;      /* read sequential */
		case 21: bdos21(); break;      /* write sequential */
		case 22: bdos22(); break;      /* make file */
		case 23: bdos23(); break;      /* rename file */
		case 24: bdos24(); break;      /* return login vector */
		case 25: bdos25(); break;      /* return current disk */
		case 26: bdos26(); break;      /* set dma address */
		case 27: bdos27(); break;      /* get alloc address */
		case 28: bdos28(); break;      /* write protect disk */
		case 29: bdos29(); break;      /* get r/o vector */
		case 30: bdos30(); break;      /* set file attributes */
		case 31: bdos31(); break;      /* get disk parms */
		case 32: bdos32(); break;      /* get & set user code */
		case 33: bdos33(); break;      /* read random */
		case 34: bdos34(); break;      /* write random */
		case 35: bdos35(); break;      /* compute file size */
		case 36: bdos36(); break;      /* set random record */
		case 37: bdos37(); break;      /* reset drive */
		case 38: bdos38(); break;      /* write random (zero) */
		default: *reghl = 0; ReturnZ80;
	}
}
/*----------------------------------------------------------------------*/
bdos00()	/* system reset -not ready */
{
	dmaaddr = 0x0080;		 /* Set DMA address to 80h */
	*eop = 1;			 /* End of execution */
	longjmp(end_of_execution,0);	 /* Back to main loop */
}
/*----------------------------------------------------------------------*/
bdos01()	/* console input */
{
/*
	union REGS regs;

	regs.h.ah = 0x01;
	int86(0x21,&regs,&regs);
	*regl = *rega = regs.h.al;
	ReturnZ80;
*/
	unsigned keypress;
	unsigned char ch;

	keypress = _bios_keybrd(_KEYBRD_READ);
	ch = (char)(keypress & 0x00ff);
	*regl = *rega = ch;
	if (*rega == 0x03) {puts("^C");longjmp(ctrl_c,0);}
	ReturnZ80;
	switch (ch) {
		case 0x08:
		case 0x09:
		case 0x0a:
		case 0x0d: putchar(ch); break;
		  default: if (ch < 32) {putchar('^');putchar(ch+64);}
				  else putchar(ch);
	}
}
/*----------------------------------------------------------------------*/
bdos02()	    /* character output */
{
/*
	putchar(*rege);
	ReturnZ80;
*/
	union REGS regs;

	regs.h.dl = *rege;
	ReturnZ80;
	regs.h.ah = 0x02;
	int86(0x21,&regs,&regs);

}
/*----------------------------------------------------------------------*/
bdos03()		/* reader input */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos04()	    /* punch output */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos05()	    /* list output */
{
	unsigned char ch;

	ch = (unsigned)*rege;
	ReturnZ80;
	_bios_printer(_PRINTER_WRITE,0,ch);
}
/*----------------------------------------------------------------------*/
bdos06()	    /* direct console i/o -not check!*/
{
	union REGS regs;

	regs.h.dl = *rege;
	regs.h.ah = 0x06;
	int86(0x21,&regs,&regs);
	*regl = *rega = regs.h.al;
	ReturnZ80;
/*
	if ( (*regde & 0x00ff) == 0xff ) {
		tmp = _bios_keybrd(_KEYBRD_READY);
		if (tmp != 0) tmp = _bios_keybrd(_KEYBRD_READ);
		*ch = (char)tmp;
	}
	else putch( *regde & 0x00ff);
*/
}
/*----------------------------------------------------------------------*/
bdos07()	    /* get i/o byte */
{
	char far *ptr1;

	ptr1 = (char far *)(BASEADDR+0x00000003);
	*rega = *ptr1;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos08()	    /* set i/o byte */
{
	char far *ptr1;

	ptr1 = (char far *)(BASEADDR+0x00000003);
	*ptr1 = *rege;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos09()	    /* print string */
{
/*
	char far *addr;

	addr = BASEADDR + (*regde);
	do { 
		putchar(*addr++);
	} while (*addr != '$');
*/
	union REGS reg;
	struct SREGS segregs;

	segread(&segregs);
	reg.h.ah = 0x09;
	reg.x.dx = *regde;
	segregs.ds = BASESEG;
	int86x(0x21,&reg,&reg,&segregs);
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos10()	    /* read console buffer */
{
	char far *ptr;
	char *ptr2;
	unsigned char strln = 0;
	char tmpbuffer[256];

	gets(tmpbuffer);
	ptr2 = tmpbuffer;
	ptr = (char far *)BASEADDR + *regde+2;
	do {
		*ptr = *ptr2++;
		strln++;
	}while ( *ptr++ != 0x00);
	*(--ptr) = 0X0D;
	*(++ptr) = 0x00;

	ptr = (char far *)BASEADDR + *regde + 1;
	*ptr = strln - 1;
	ReturnZ80;
/*
	union REGS regs;
	struct SREGS segregs;
	char far *ptr;

	ptr = (char far *)BASEADDR + *regde;
	ptr = (char far *)BASEADDR + *regde + 2;
	regs.h.ah = 0x0a;
	regs.x.dx = *regde;
	segregs.ds = BASESEG;
	int86x(0x21,&regs,&regs,&segregs);
	while (*ptr != 0x0D) ptr++;
	*(++ptr) = 0x00; /-- *ptr = 0x00; --/
	ptr = (char far *)BASEADDR + *regde;
	ReturnZ80;
*/
}
/*----------------------------------------------------------------------*/
bdos11()   /* read console status */
{
/*
	int tmp;

	tmp = kbhit();
	if (tmp  != 0) tmp = 0xff;
	*regl = *rega = (char)tmp;
*/
	union REGS regs;

	regs.h.ah = 0x0B;
	int86(0x21,&regs,&regs);
	*regl = *rega = regs.h.al;
	ReturnZ80;

}
/*----------------------------------------------------------------------*/
bdos12()	    /* get version number */
{
	*reghl = 0X0022;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos13()	     /* reset disk system */
{
	dmaaddr = 0x0080;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos14()	     /* select disk */
{
/*
	unsigned disknumber;
	unsigned *drives;

	disknumber = (*regde & 0x00ff)+1;
	_dos_setdrive(disknumber,drives);
*/
	union REGS regs;

	regs.h.ah = 0x0E;
	regs.h.dl = *rege;
	int86(0x21,&regs,&regs);
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos15()	/* open file by fcb filename */
{
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);
	while (stricmp(fcbused[fcbptr],filename) != 0 && 
		fcbused[fcbptr][0] != 0	&& fcbptr <= MAXFILE) fcbptr++;
	if ( fcbptr >MAXFILE ) *regl = *rega = 0xFF;
	else if ( *filename == 0x00 ) *regl = *rega = 0xff; /*Null Filename*/
	else if ( stricmp(fcbused[fcbptr],filename) == 0 )
		{opensuccess(fcbptr);*regl = *rega = 0x00;}
	else if ( (fcbfile[fcbptr] = fopen(filename,"r+b")) == NULL )
		*regl = *rega = 0XFF;
	else {
		*regl = *rega = 0x00;
		opensuccess(fcbptr);
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos16()      /* close file */
{
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);
	while (stricmp(fcbused[fcbptr],filename) != 0 &&
		fcbptr <= MAXFILE) fcbptr++;

	if (fcbptr>MAXFILE) *rega  = 0xff; 
	else if (fcbfile == NULL) *rega = 0xff;
	else {
		if (fclose(fcbfile[fcbptr]) == 0) {
			*rega = 0x00;
			fcbused[fcbptr][0] = 0;
			fcbfile[fcbptr] = NULL;
		}
		else *regaf |= 0xff00;		/* *rega = 0xff */
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/

bdos17()	 /* search for first */
{
	unsigned return_value;

	fcbtofilename(fcbdn + *regde,filename);
	return_value = _dos_findfirst(filename,_A_NORMAL |
		_A_RDONLY | _A_SUBDIR ,&search_file);
	if (return_value != 0) *rega = 0xff;
	else {
		*rega  =  0x00;
		fillfcb((char far *)(BASEADDR+dmaaddr),search_file.name);
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos18()	 /* search for next */
{
	unsigned return_value;

	return_value = _dos_findnext(&search_file);
	if (return_value != 0) *regaf |=  0xff00;       /* *rega = 0xff */
	else {
		*rega = 0x00;
		fillfcb((char far *)(BASEADDR+dmaaddr),search_file.name);
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos19()		 /* delete file */
{

	fcbtofilename(fcbdn + *regde,filename);

	*rega = (char)remove(filename);
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos20()				/* squential read */
{
	char far *ptr1;
	char far *ptr2;
	int i;
	long l;
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);
	while (stricmp(fcbused[fcbptr],filename) != 0 && 
		fcbptr <= MAXFILE) fcbptr++;
	filloffset(&offset);

	ptr1 = (char far *)( BASEADDR + dmaaddr);
	if ( offset  >= fcbfilelen[fcbptr] ) *rega = 0x01;
	else if ( fcbptr >MAXFILE) *rega = 0xff;
	else if ( fseek(fcbfile[fcbptr],offset,SEEK_SET) != 0 ) *rega = 0xff;
							      /* read fail */
	else {
		fread(tmp1,1,128,fcbfile[fcbptr]);
		l =  fcbfilelen[fcbptr] - offset;
		if ( l < 128L )
		tmp1[ (int)l ] = 0x1a;	/* add CP/M EOF char 0x1a */
			for (i = 0;i<128;i++) { *ptr1++ = *(tmp1+i); }
			*regl = *rega = 0x00;
			ptr1 = fcbcr + *regde;
			ptr2 = fcbrl + *regde;
			(*ptr1)++;
			if (*ptr1 == 0) (*ptr2)++;
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos21()		/* write sequential */
{
	char far *ptr1;
	char far *ptr2;
	int i;
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);

	while (stricmp(fcbused[fcbptr],filename) != 0 && 
		fcbptr <= MAXFILE) fcbptr++;

	filloffset(&offset);
	ptr1 = (char far *)(BASEADDR + dmaaddr);
	if ( fcbptr >MAXFILE) { *rega = 0xff; ReturnZ80; }
	else if ( fseek(fcbfile[fcbptr],offset,SEEK_SET)  != 0 ) {
		*rega = 0xff;ReturnZ80;  /* write fail */
	}
	else {
		for (i = 0;i<128;i++) *(tmp1+i) = *ptr1++;
		*rega = 0x00;
		ptr1 = fcbcr + *regde;
		ptr2 = fcbrl + *regde;
		(*ptr1)++;
		if (*ptr1 == 0) (*ptr2)++;
		ReturnZ80;
		if (offset >= fcbfilelen[fcbptr])
			fcbfilelen[fcbptr] = offset+128;
		fwrite(tmp1,1,128,fcbfile[fcbptr]);
	}
}
/*----------------------------------------------------------------------*/
bdos22()		/* make file by fcb filename */
{
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);

	while (stricmp(fcbused[fcbptr],filename) != 0 && 
		fcbused[fcbptr][0] != 0 && fcbptr <= MAXFILE) fcbptr++;
	if ( fcbptr > MAXFILE )  *rega = 0xFF;
	else if ( stricmp(fcbused[fcbptr],filename) == 0 ) {
		opensuccess(fcbptr);*rega = 0x00;
	}
	else if ( (fcbfile[fcbptr] = fopen(filename,"w+b")) == NULL )
		*rega = 0XFF;
	else {
		*rega = 0x00;
		opensuccess(fcbptr);
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos23()		/*  rename file  */
{
	char oldname[15];
	char newname[15];

	fcbtofilename(fcbdn + *regde,oldname);
	fcbtofilename(fcbrc + *regde+1,newname);
	*rega = (char)rename(oldname,newname);
	if (*rega != 0x00) *rega = 0xff;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos24()		/* return login vector ---not checked */
{
	*reghl = 0x0007;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos25()		/* return currect disk -- not checked! */
{
	union REGS regs;

	regs.h.ah = 0x19;
	int86(0x21,&regs,&regs);
	*rega = regs.h.al;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos26()		/* set dma address */
{
	dmaaddr = *regde;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos27()		/* get alloc address */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos28()		/* write protect disk */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos29()		/* get r/o vector */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos30()		/* set file attributes */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos31()		/* get disk parms */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos32()		/* get and set user code --not ready */
{
	if (*rege == (char)0xff) *rega = user_code;
	else user_code = *rege;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos33()		/* read random */
{
	char far *ptr1;
	unsigned far *ptr2;
	int i;
	long l;
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);

	while (stricmp(fcbused[fcbptr],filename) != 0 &&
		fcbptr <= MAXFILE) fcbptr++;

	ptr1 = (char far *)( BASEADDR + dmaaddr);
	ptr2 = (unsigned far *)(fcbln + *regde);
	offset = (long)(*ptr2) * 128L;
	if ( (char)*(fcbln + *regde+2) != 0x00) *rega = 0x06;
	else if ( offset > fcbfilelen[fcbptr] ) *rega = 0x04;
	else if ( fcbptr >MAXFILE) *rega = 0x05;
	else if ( fseek(fcbfile[fcbptr],offset,SEEK_SET) != 0 ) *rega = 0x03;
							      /* seek fail */
	else {
		fread(tmp1,1,128,fcbfile[fcbptr]);
		l =  fcbfilelen[fcbptr] - offset;
		if ( l < 128L )
		tmp1[(int)l] = 0x1a;	/* add CP/M EOF char 0x1a */
		for (i = 0 ; i < 128 ; i++) *ptr1++ = *(tmp1+i);
		if ( offset == fcbfilelen[fcbptr] ) *rega = 0x01;
		else *rega = 0x00;
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos34()		/* write random */
{
	char far *ptr1;
	unsigned far *ptr2;
	int i;
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);
	while (stricmp(fcbused[fcbptr],filename) != 0 && fcbptr <= MAXFILE)
		fcbptr++;
	ptr1 = (char far *)( BASEADDR + dmaaddr);
	ptr2 = (unsigned far *)(fcbln + *regde);
	offset = (long)( *ptr2 ) * 128L;
	if ( (char)*(fcbln + *regde+2) != 0x00) { *rega = 0x06; ReturnZ80;}
	else if ( fcbptr >MAXFILE)  { *rega = 0x05; ReturnZ80; }
	else if ( fseek(fcbfile[fcbptr],offset,SEEK_SET) != 0 )
		{ *rega = 0x03;ReturnZ80; } /* seek fail */
	else {
		for (i = 0 ; i < 128 ; i++) *(tmp1+i) = *ptr1++;
		*rega = 0x00;
		ReturnZ80;
		fwrite(tmp1,1,128,fcbfile[fcbptr]);
		if (offset >= fcbfilelen[fcbptr])
			fcbfilelen[fcbptr] = offset + 128;
	}
}
/*----------------------------------------------------------------------*/
bdos35()		/* compute file size -- not checked*/
{
	char far *ptr1;
	unsigned far *ptr3;
	char fcbptr = 0;

	fcbtofilename(fcbdn + *regde,filename);
	while (stricmp(fcbused[fcbptr],filename) != 0 && 
		fcbused[fcbptr][0] != 0	&& fcbptr <= MAXFILE) fcbptr++;
	if ( (fcbfile[fcbptr] = fopen(filename,"r+b")) != NULL ) {
		opensuccess(fcbptr);
		result = ldiv((long)(fcbfilelen[fcbptr]),128L);
		if (result.rem != 0) result.quot++;
		if (result.quot >65535L ) {
			ptr1 = fcbln + *regde;
			*ptr1++ = 0x00;
			*ptr1++ = 0x00;
			*ptr1++ = 0x01;
		}
		else {
			ptr3 = (unsigned far *)(fcbln + *regde);
			*ptr3++ = (unsigned)result.quot;
			*ptr3 &=  0xff00;
		}
	}
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos36()		/* set random record --not checked--have error!*/
{
	char far *ptr1;
	char far *ptr2;

	ptr1 = fcbln + *regde;
	ptr2 = fcbcr + *regde;
	*ptr1++ = *ptr2;
	ptr2 = fcbrl + *regde;
	*ptr1 = *ptr2;
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos37()		/* reset drive */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
bdos38()		/* write random (zero) */
{
	ReturnZ80;
}
/*----------------------------------------------------------------------*/
