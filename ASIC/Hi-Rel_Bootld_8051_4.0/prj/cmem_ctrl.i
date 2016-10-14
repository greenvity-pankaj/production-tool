
#line 1 "..\src\cmem_ctrl.c" /0












 
 
 void EnableWrCRam ()
 {
 #pragma asm





 
 MOV		  09FH,    #0	 
 MOV       0f1h,    #00h  
 MOV       0f2h,    #20h  
 MOV       0f3h,    #0ffh 
 MOV       0f4h,    #0ffh 
 MOV       0F8H,    #0h	  
 
 #pragma endasm
 }
 
 void DisableWrCRam ()
 {
 #pragma asm







 
 MOV		  09FH,    #0	  
 MOV       0f1h,    #00h   
 MOV       0f2h,    #20h   
 MOV       0f3h,    #0ffh  
 MOV       0f4h,    #0ffh  
 MOV       0F8H,    #1h	
 
 
#line 53 "..\src\cmem_ctrl.c" /1
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
#line 69 "..\src\cmem_ctrl.c" /0
 #pragma endasm
 }
