; CLR_CHAR.A51
EXTRN BIT (?C?charloaded)
PUBLIC clear_getchar

?PR?clear_getchar SEGMENT CODE
RSEG ?PR?clear_getchar

clear_getchar:    CLR  ?C?charloaded
                  RET

                  END