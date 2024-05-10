/************************************************************************/
/*                                                                      */
/*             CP/M Hardware Emulator Card Support Program              */
/*                         CPMEMU.C Ver 1.51                            */
/*                 Copyright (c) By C.J.Chen NTUEE 1988                 */
/*                         All Right Reserved                           */
/*                                                                      */
/************************************************************************/
#include <stdio.h>
#include <ctype.h>
#include <bios.h>
#include <dos.h>
#include <stdlib.h>
#include <direct.h>
#include <process.h>
#include <setjmp.h>
#include <string.h>
#include <memory.h>
#include <conio.h>

#define   PROGRAM_NAME	  "\nCP/M 2.2 and Z80 Hardware Emulator Ver 1.51\n"
#define   COPYRIGHT	  "Copyright(c) By Chen Chi-Jung NTUEE 1988,1989,"
#define   COPYRIGHT1	  "All Right Reserved\n"
#define   MAXFILE         20
#define   BASEADDR        0xE0000000
#define   BASESEG         (unsigned)(BASEADDR / 0x10000)
#define   CHANGE_PORT     0x2F0
#define   ReturnZ80       outp(CHANGE_PORT,0)
#define   GotoPC          inp(CHANGE_PORT)
#define   ResetZ80        outp(RESET_PORT,0)
#define   RESET_PORT      0X2F2
#define   STATUS_PORT     0X2F3
#define   NULLSIZE        0x40
#define   REGA            (BASEADDR+0X0000FF83)
#define   REGB            (BASEADDR+0X0000FF85)
#define   REGC            (BASEADDR+0X0000FF84)
#define   REGD            (BASEADDR+0X0000FF87)
#define   REGE            (BASEADDR+0X0000FF86)
#define   REGH            (BASEADDR+0X0000FF89)
#define   REGL            (BASEADDR+0X0000FF88)

#define   REGAF           (BASEADDR+0X0000FF82)
#define   REGBC           (BASEADDR+0X0000FF84)
#define   REGDE           (BASEADDR+0X0000FF86)
#define   REGHL           (BASEADDR+0X0000FF88)
#define   REGIX           (BASEADDR+0X0000FF8A)
#define   REGIY           (BASEADDR+0X0000FF8C)
#define   REGSP           (BASEADDR+0X0000FF8E)
#define   REGIP           (BASEADDR+0X0000FF90)
#define   BIOSCODE        (BASEADDR+0X0000FF92)
#define   BDOSCALL        (BASEADDR+0X0000FF81)
#define   EOP             (BASEADDR+0X0000FF80)
/*   CP/M SYSTEM FILE CONTROL BLOCK  */
#define   FCBDN           (BASEADDR+0X00000000)
#define   FCBFN           (BASEADDR+0X00000001)
#define   FCBFT           (BASEADDR+0X00000009)
#define   FCBRL           (BASEADDR+0X0000000C)
#define   FCBRC           (BASEADDR+0X0000000F)
#define   FCBCR           (BASEADDR+0X00000020)
#define   FCBLN           (BASEADDR+0X00000021)

void cdecl interrupt far handle_ctrl_c();

char far *bioscode;     char far *bdoscall;     char far *eop;
char far *fcbdn;        char far *fcbfn;        char far *fcbft;
char far *fcbrl;        char far *fcbrc;        char far *fcbcr;
char far *fcbln;        char far *rega;         char far *regb;
char far *regc;         char far *regd;         char far *rege;
char far *regh;         char far *regl;

int far *regaf;         int far *regbc;         int far *regde;
int far *reghl;         int far *regix;         int far *regiy;
int far *regsp;         int far *regip;

char submitstring[128];
char *submitptr[10];
char halt;
char doscommand=1;
char *cpmhex,*hexptr;
int tmp;
unsigned char test;
unsigned int dmaaddr=0x80;
unsigned ctrl_c_seg, ctrl_c_offset;
unsigned stack_segment;
unsigned char checknum;
jmp_buf ctrl_c, end_of_execution;
jmp_buf cold_start;
FILE *subfile=NULL;
FILE *cpmhexptr;
FILE *fp;
extern FILE *fcbfile[MAXFILE];
extern unsigned int fcbfilelen[MAXFILE];
extern unsigned char fcbused[MAXFILE][15];
/*----------------------------------------------------------------------*/
char *nullptr = NULL;
char nulldata[NULLSIZE];

FILE *lpt = NULL;
char DebugFlag = 0;
extern unsigned char lastcall;
extern unsigned repeats;
extern char debugmess1[50];
extern char debugmess2[50];
/*----------------------------------------------------------------------*/
printtitle()
{
        printf(PROGRAM_NAME);
	printf(COPYRIGHT);
	printf(COPYRIGHT1);
	printf("\nZ80 Card Segment : %04XH\n",BASESEG);
	printf("Control I/O Port : %03XH\n",CHANGE_PORT);
	printf("TPA Area: 0100H - FDFFH\n");
}
/*----------------------------------------------------------------------*/
char getchfromcpmhex()
{
        char ch;

	ch = *hexptr++;
        return(ch);
}
/*----------------------------------------------------------------------*/
char fgetc1()
{
        char tmp;

	tmp = getchfromcpmhex();
	if (tmp > '9') tmp -= 55;
	else tmp -= 48;
        return(tmp);
}
/*----------------------------------------------------------------------*/
unsigned char readbyte()
{
        unsigned char result;

	result = fgetc1() * 16;
	result += fgetc1();
	checknum += result;
        return(result);
}
/*----------------------------------------------------------------------*/
readline()
{
	char ch;
	unsigned int addr=0;
	unsigned char data_type;
	unsigned char data_number;
	unsigned char data;
	unsigned char i,tmp;
	unsigned char far *ptr;

	checknum = 0;
	if ((ch = getchfromcpmhex()) == EOF) {
		printf("\ninvalid EOF \n"); quit();
	}
	if (ch != ':') { 
		printf("\nInvalid HEX file!\n");
		quit();
	}
	if (( data_number = readbyte() ) == 0) return(EOF);
	addr = readbyte();
	tmp = readbyte() ;
	addr = addr * 256 + tmp;
	if (( data_type = readbyte() ) == 1) return(EOF);
	else if (data_type != 0) { 
		printf("\nInvalid data type char.\n");
		quit();
	}
	ptr = (char far *)BASEADDR + addr;
	for (i=1 ; i <= data_number ; i++) *ptr++ = readbyte();
	readbyte();             /* read checknumber */
	getchfromcpmhex();      /* read Line Feed char */
	if (checknum != 0) { printf("\nCheck Sum error!\n"); quit(); }
	return(0);
}
/*----------------------------------------------------------------------*/
loadcpmhex()
{
	int result;

	hexptr = cpmhex;
	do {
        	result = readline();
	} while (result != EOF);
	return(0);
}
/*----------------------------------------------------------------------*/
loadcom(char *filename)
{
	char far *addr = (char far *)(BASEADDR+0x00000100);
	char buffer[128];
	char *ptr1;
	unsigned char i;
	unsigned filelen = 0;
	unsigned loadlen = 0;

	if ((fp = fopen(filename,"rb"))==NULL) {
	        printf("Z80 Execution File not found\n");
        	return(1);
	}
	fseek(fp,0L,SEEK_END);
	filelen = (unsigned)ftell(fp);
	fseek(fp,0L,SEEK_SET);
	do {
		ptr1 = buffer;
        	fread(buffer,128,1,fp);
	        for (i = 0 ; i < 128 ; i++) *addr++ = *ptr1++;
        	loadlen += 128;
	} while (loadlen < filelen);
	fclose(fp);
	return(0);
}
/*----------------------------------------------------------------------*/
clearmem()
{
	long far *ptr;
	unsigned int addr;

	GotoPC;
	ptr = (long far *)BASEADDR;
	*ptr = 0x55aa55aa;
	ptr++;
	if (*(--ptr) != 0x55aa55aa ) {
	        printf("\n\7Z80 Card Bad or not Implimented!\n");
        	quit();
	}
	for (addr = 0 ; addr <= 0x4000 ; addr++) *ptr++=0L;
}
/*----------------------------------------------------------------------*/
fillfcb(char far *addr,char *ptr)
{
	char count = 0;
	char far *ptr3;
	char i;

	ptr3 = addr;
	*ptr3++ = 0x00;
	for (i = 0 ; i < 11 ; i++) *ptr3++ = 0x20;

	while (*ptr == ' ') ptr++;
	if (*(ptr+1) == ':') {*addr = *ptr - 'A' + 1; ptr += 2; }
	addr++;
	while (*ptr != '.' && *ptr != 0x00 && count < 8)
		{*addr++ = *ptr++; count++; }
	if (*ptr != 0x00) {
		 while (count < 8) { addr++; count++; }
	        if (*ptr='.') {
        	        ptr++;
                	count = 0;
	                while(*ptr != 0x00 && count < 3) {
        	                *addr++ = *ptr++;
                	        count++;
                	}
        	}
    	}
}
/*----------------------------------------------------------------------*/
submit(char *ptr1)
{
	char *ptr2;
	char filename[20];
	char submitcount = 1;

	ptr1 += 6;
	while (*ptr1 == ' ') ptr1++;
	ptr2 = filename;
	while (*ptr1 != ' '&& *ptr1 != '.'&& *ptr1 != 0x00) *ptr2++ = *ptr1++;
	switch (*ptr1) {
		case '.': while (*ptr1 != ' ' && *ptr1 != 0x00)
				 *ptr2++ = *ptr1++; *ptr2=0x00; break;
		case ' ': strcpy(ptr2,".SUB"); break;
		case  0 : strcpy(ptr2,".SUB"); break;
	}
	if ((subfile = fopen(filename,"r"))==NULL) printf("File not found\n");
	else if (*ptr1 != 0x00) {
	        ptr1++;
        	strcpy(submitstring,ptr1);
	        ptr1 = submitstring;
	        while (submitcount < 10) {
    		    	if (*ptr1 == 0x00) break;
			submitptr[submitcount++] = ptr1;
			while (*ptr1 != ' ' && *ptr1 != 0x00) *ptr1++;
			if (*ptr1 == ' ') *ptr1++ = 0x00;
		}
	}
}
/*----------------------------------------------------------------------*/
debug(char *ptr1)
{
	if (strnicmp(ptr1,"DEBUG ON",8) == 0) {
		printf("Debug is set on\n");
	        DebugFlag = 1;
	        lastcall = 0xff;
	        if (lpt == NULL) {
	                lpt = fopen("btrace.dat","w");
	                fputs("Times  Z80PC    Z80DE    Z80DMA   FUNCTION\n"
				,lpt);
	                fputs("-----  -----    -----    ------   --------\n"
				,lpt);
	        }
	}
	else if (strnicmp(ptr1,"DEBUG OFF",9) == 0) {
	        printf("Debug is set off\n");
	        DebugFlag = 0;
	        if (lpt != NULL) {
	                if (lastcall != 0xFF) {
	                        fprintf(lpt,"% 5d  %s%s\n", repeats, debugmess1
					, debugmess2);
	                }
	                fputs("--------- END OF BDOS TRACE TABLE -------- "
				,lpt);
	                fclose(lpt);
	                lpt = NULL;
	        }
	}
	else {
		if (DebugFlag == 0) printf("Debug is off\n");
	        else printf("Debug is on\n");
	}
}
/*----------------------------------------------------------------------*/
CheckDosCommand(char *ptr1)
{
	doscommand = 1;
	if (strnicmp(ptr1,"EXIT",4) == 0)         quit();
	else if (strnicmp(ptr1,"DIR",3) == 0)     system(ptr1);
	else if (strnicmp(ptr1,"EDIT ",5) == 0)   system(ptr1);
	else if (strnicmp(ptr1,"COPY ",5) == 0)   system(ptr1);
	else if (strnicmp(ptr1,"REN ",4) == 0)    system(ptr1);
	else if (strnicmp(ptr1,"RENAME ",7) == 0) system(ptr1);
	else if (strnicmp(ptr1,"CD ",3) == 0)     system(ptr1);
	else if (strnicmp(ptr1,"CD",2) == 0 && strlen(ptr1) == 2) system(ptr1);
	else if (strnicmp(ptr1,"DEL ",4) == 0)    system(ptr1);
	else if (strnicmp(ptr1,"TYPE ",5) == 0)   system(ptr1);
	else if (strnicmp(ptr1,"VER",3) == 0 && strlen(ptr1) == 3)
		printtitle();
	else if (strnicmp(ptr1,"SUBMIT ",7) == 0) submit(ptr1);
	else if (strnicmp(ptr1,"!",1) == 0)       system(++ptr1);
	else if (strnicmp(ptr1,"DEBUG",5) == 0)   debug(ptr1);
	else if (strnicmp(ptr1,"COLD!",5) == 0) {
		clearmem();
	        loadcpmhex();
	        longjmp(cold_start,0);
	}
	else if (strlen(ptr1) == 0);
	else if (strlen(ptr1) == 2 && ptr1[1] == ':') system(ptr1);
	else if (*ptr1 == ';');
	else doscommand = 0;
}
/*----------------------------------------------------------------------*/
upcase(char *ptr1)
{
	while (*ptr1 != 0x00) { 
		if ( *ptr1 > 0x60 && *ptr1 < 0x7b) *ptr1 -= 32;
		*ptr1++;
	}
}
/*----------------------------------------------------------------------*/
getstring(char *filename)
{
	char *ptr1,*ptr2;
	char tempname[129];
	unsigned char ch,tmp;
	int i;

	ptr1 = filename;
	if (subfile != NULL) {
	        if (feof(subfile) != 0) {
	                fclose(subfile);
        	        subfile = NULL;
	                strncpy(filename,"\n\0",2);
            	}
	        else {
                	do {
	                        ch = fgetc(subfile);
        	                if (ch == 0x0a) ch=0x00;
                	        else if (ch == 0xff) {
                                	ch = 0x00;
	                                fclose(subfile);
        	                        subfile = NULL;
	                                for (i = 1 ; i < 10 ;
						submitptr[i++] = NULL);
	                                *submitstring = 0x00;
				}
	                        else if (ch == '$') {
	                                tmp = getc(subfile);
         	                      	if (!isdigit(tmp)) ungetc(tmp,subfile);
	        	        	else {
                                        	ptr2 = submitptr[tmp-'0'];
	                                        if (ptr2 != NULL) {
        	                                	while (*ptr2 != 0x00)
							    *ptr1++ = *ptr2++;
                        	                	ch = *(--ptr2);
	                                        	ptr1--;
        	                                }
	                                        else ch = getc(subfile);
        	                        }
                	        }
	                        *ptr1++ = ch;
			} while( ch != 0x00 );
	                printf("%s\n",filename);
                }
	}
	else {
		/* gets(filename); */
		tempname[0] = 127;
		strncpy( filename , cgets(tempname) ,127);
		printf("\n");
	}
}
/*----------------------------------------------------------------------*/
getcommand()
{
	char filename[128], first_file[20], second_name[30], third_name[30];
	char *ptr1,*ptr2,*ptr4;
	char far *ptr3;
	char i,count=0;
	char dir_now[50];

	do {
        	do {
        		getcwd(dir_now,50);
	                printf("\nZ80 %s>",dir_now);
        	        getstring(filename);
                	ptr1 = filename;
	                while (*ptr1 == ' ') ptr1++;
	                CheckDosCommand(ptr1);
        	        upcase(ptr1);
		} while (doscommand == 1);

	        ptr2 = first_file;
	        for (i = 0 ; i < 20 ; i++) first_file[i] = 0x00;
		for (i = 0 ; i < 30 ; i++)
			second_name[i] = third_name[i] = 0x00;
        	while (!isalpha(*ptr1) && *ptr1 != 0x00) ptr1++;
	        while (*ptr1 != ' '&& *ptr1 != '.'&& *ptr1 != 0x00
			 && *ptr1 != '\n') *ptr2++ = *ptr1++;
		switch (*ptr1) {
		        case '.' : while (*ptr1 != ' '&& *ptr1 != 0x00)
					 *ptr2++ = *ptr1++;
				   break;
            		case '\n':
		        case ' ' : strncpy(ptr2,".COM",4);break;
		        case  0  : strncpy(ptr2,".COM",4);break;
	        }
	} while (loadcom(first_file) != 0);

	ptr3 = (char far *)(BASEADDR + 0x00000080);
	*ptr3++ = 0x00;
	*ptr3++ = 0x00;

	if (*ptr1 != 0x00) {
	        ptr2 = ptr1;
        	ptr3 = (char far *)(BASEADDR + 0x00000081);
	        while (*ptr2 != 0x00) { *ptr3++ = *ptr2++; count++; }
	        *ptr3++ = 0x0D;
        	*ptr3 = 0x00;
	        ptr3 = (char far *)(BASEADDR + 0x00000080);
	        *ptr3 = count;

        	ptr2 = ptr1;
	        ptr4 = second_name;
        	*second_name = 0x00;         /* clear name */
	        while (*ptr2 == ' ') ptr2++;
	        while (*ptr2 != 0x00 && *ptr2 != ' ') *ptr4++ = *ptr2++;
		*third_name = *ptr4 = 0x00;
	        if (*ptr2 == ' ') {
	                ptr4 = third_name;
        	        while (*ptr2 == ' ') ptr2++;
                	while (*ptr2 != 0x00) {*ptr4++ = *ptr2++;}
	                *ptr4 = 0x00;
        	}
	        fillfcb((char far *)(BASEADDR + 0x0000005c),second_name);
	        fillfcb((char far *)(BASEADDR + 0x0000006c),third_name);
	} 

	while ((i = _bios_keybrd(_KEYBRD_READY)) != 0)
	        _bios_keybrd(_KEYBRD_READ);    /* clear keybrd buffer */
	ResetZ80;        /* pull reset line to low */
	ReturnZ80;        /* set z80 to raad RAM */
	ResetZ80;   /* pull reset line to high ,z80 begin running*/
	waitz80();
	ReturnZ80;
	waitz80();
	halt = *eop = 0;
}
/*----------------------------------------------------------------------*/
initpointer()               /* initial pointers value */
{
	rega = (char far *)REGA;  /* initial 8 bits register pointers */
	regb = (char far *)REGB;
	regc = (char far *)REGC;
	regd = (char far *)REGD;
	rege = (char far *)REGE;
	regh = (char far *)REGH;
	regl = (char far *)REGL;

	regaf = (int far *)REGAF; /* initial 16 bits register pointers */
	regbc = (int far *)REGBC;
	regde = (int far *)REGDE;
	reghl = (int far *)REGHL;
	regix = (int far *)REGIX;
	regiy = (int far *)REGIY;
	regip = (int far *)REGIP;
	regsp = (int far *)REGSP;

	eop = (char far *)EOP;
	bioscode = (char far *)BIOSCODE;
	bdoscall = (char far *)BDOSCALL;

	fcbdn = (char far *)FCBDN; /* initial FCB block pointers */
	fcbfn = (char far *)FCBFN;
	fcbft = (char far *)FCBFT;
	fcbrl = (char far *)FCBRL;
	fcbrc = (char far *)FCBRC;
	fcbcr = (char far *)FCBCR;
	fcbln = (char far *)FCBLN;
}
/*----------------------------------------------------------------------*/
getcpmhex()
{
	char ptr[40];
	int test;

	if ((cpmhexptr = fopen("cpmemu.hex","r")) == NULL) {
	        getcwd(ptr,40);
        	test = chdir("c:\\util");
	        if ((cpmhexptr = fopen("cpmemu.hex","r")) == NULL) {
	        	test = chdir("c:\\system");
			system("c:");
		        if ((cpmhexptr = fopen("cpmemu.hex","r")) == NULL) {
	                	printf("Can't find 'Cpmemu.hex'\n");
	        	        quit();
			}
	        }
	}
	cpmhex = (char *)malloc(1400);
	fread(cpmhex,1400,1,cpmhexptr);
	fclose(cpmhexptr);
        test = chdir(ptr);
	system("d:");
}
/*----------------------------------------------------------------------*/
set_ctrl_c()
{
	void far *ptr1;

	ptr1 = (void far *)_dos_getvect(35);
	ctrl_c_seg = FP_SEG(ptr1);
	ctrl_c_offset = FP_OFF(ptr1);
	_dos_setvect(35,handle_ctrl_c);
}
/*----------------------------------------------------------------------*/
initial()
{
	unsigned far *total_memory = (unsigned far *)0x00000413;

	set_ctrl_c();
	store_stack_segment();
	initpointer();
	initialbdos();
	initialbios();
	printtitle();
	if (*total_memory > BASESEG) {
		printf("\nPC memory conflict with Z80 in segment %4X!\7\n"
			,BASESEG);
	quit();
	}
	_bios_printer(_PRINTER_INIT,0,0);

	getcpmhex();
	subfile = fopen("$$$.SUB","r");
}
/*----------------------------------------------------------------------*/
waitz80()
{
	register test,test1;

	_enable();
	do {
	        test = (unsigned char)inp(STATUS_PORT);
        	if ((test & 0x02) == 0x00) {
			printf("Program halted!\n"); halt = 1;
		}
		test1 = _bios_keybrd(_KEYBRD_READY);
		if ( test1 != 0 ) {
			if (test1 == 0xffff) {
				puts("^Break");
				GotoPC;
				longjmp(ctrl_c,0);
			}
			else if ((test1 & 0x00ff) == 0x0003 );
			else _bios_keybrd(_KEYBRD_READ);
		}
	} while (((test & 0x01) != 0) | halt == 1 ) ;
}
/*----------------------------------------------------------------------*/
quit()
{
	union REGS regs;
	struct SREGS sregs;

	if (DebugFlag ==  1 && lpt !=  NULL) {
		if (lastcall  !=  0xFF) {
		        fprintf(lpt,"% 5d  %s%s\n", repeats, debugmess1
				, debugmess2);
		}
		fputs("----- END OF BDOS TRACE TABLE ----- ",lpt);
		fclose(lpt);
	}
	regs.h.al = 35;
	regs.h.ah = 0x25;
	sregs.ds = ctrl_c_seg;
	regs.x.dx = ctrl_c_offset;
	int86x(0x21,&regs,&regs,&sregs);
	checknullptr();
	free(cpmhex);
	exit(0);
}
/*----------------------------------------------------------------------*/
closeall()
{
	char fcbptr;

	for (fcbptr = 0 ; fcbptr < MAXFILE ; fcbptr++) {
	        if (fcbfile[fcbptr] != NULL) fclose(fcbfile[fcbptr]);
        	fcbfile[fcbptr] = NULL;
	        fcbused[fcbptr][0] = 0x00;
	        fcbfilelen[fcbptr] = 0x0000;
	}
}
/*----------------------------------------------------------------------*/
store_stack_segment()
{
        struct SREGS sregs;

        segread(&sregs);
        stack_segment = sregs.ss;
}
/*----------------------------------------------------------------------*/
printnull()
{
	char *temp,i;
	char linebuf[17];

	printf("NULL Pointer value:\n");
	for ( temp = nullptr, i = 0 ; temp < (nullptr + NULLSIZE) ; temp++) {
		linebuf[i++] = (*temp >=  32) ? *temp : '.';
        	printf( "%02X ", *temp);
	        if (i == 16) {
	                linebuf[i] = '\0';
        	        printf("| %16s\n",linebuf);
                	i = 0;
	        }
	}
}
/*----------------------------------------------------------------------*/
savenullptr()
{
	char i,*tmp;

        tmp = nullptr;
        for( i = 0 ; i < NULLSIZE ; i++) nulldata[i] = *tmp++;
}
/*----------------------------------------------------------------------*/
checknullptr()
{
	char i,*tmp,flag;

        tmp = nullptr;
        flag = 0;
        for(i = 0 ; i < NULLSIZE ; i++) {
                if (nulldata[i]  !=  *tmp++) flag = 1;
        }
	if (flag ==  1) {
                printf("\nNull Pointer assignment error!\7\n");
                printnull();
                tmp = nullptr;
                for( i = 0 ; i < NULLSIZE ; i++) *tmp++ = nulldata[i];
        }
}
/*----------------------------------------------------------------------*/
main()
{
	savenullptr();
	initial();
	setjmp(cold_start);
	while(1) {
		clearmem();
		setjmp(ctrl_c);
	        checknullptr();
        	if (subfile  !=  NULL) {
	                fclose(subfile);
        	        subfile = NULL;
                	for (tmp = 1 ; tmp < 10 ; submitptr[tmp++] = NULL);
	                *submitstring = 0x00;
                }
	        setjmp(end_of_execution);
		GotoPC;
        	loadcpmhex();
	        closeall();
        	getcommand();
	        waitz80();
		dmaaddr = 0x0080;
        	while (*eop != 1 && halt != 1) {
	                if (*bdoscall != 0) cpmbdos();
        	        else cpmbios();
                	waitz80();
                }
	        halt = 0;
	        checknullptr();
        	longjmp(end_of_execution,0);
	}
}
