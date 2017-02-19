/*
* Description : Functions to control Palmchip's code and data memory
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Author      : Peter Nguyen
*
* Purpose :
*     Control PalmChip's memory mode 
*
* File: cmem_ctrl.c
*/

void EnableWrCRam ()
{
#pragma asm
/*#ifdef 8051_V5
    MOV       0f3h,    #0ffh  
    MOV       0f4h,    #0ffh  
    MOV       0F8H,    #0h	  
#else  //8051 v7
*/
	MOV		  09FH,    #0	 
	MOV       0f1h,    #00h  
	MOV       0f2h,    #20h  
    MOV       0f3h,    #0ffh 
    MOV       0f4h,    #0ffh 
    MOV       0F8H,    #0h	  
//#endif
#pragma endasm
}

void DisableWrCRam ()
{
#pragma asm
/*
#ifdef 8051_V5
    //Download size
    MOV       0f3h,    #0FFh
    MOV       0f4h,    #0FFh
    MOV       0F8H,    #01h
#else
*/
	MOV		  09FH,    #0	  
	MOV       0f1h,    #00h   
	MOV       0f2h,    #20h   
    MOV       0f3h,    #0ffh  
    MOV       0f4h,    #0ffh  
    MOV       0F8H,    #1h	
//#endif
#if 0
	mov		r0,#0;
	mov		a,#0;
test_mem_lp1:
	mov 	@r0,a;
	inc		r0;
	inc		a;
	cjne	a,#0,test_mem_lp1;
	mov		r0,#0;
	mov		a,#0;	
test_mem_lp2:
	mov		a,@r0;
	mov		p1,a;
	inc		a;
	inc		r0;
	cjne	a,#0,test_mem_lp2;
#endif
#pragma endasm
}
