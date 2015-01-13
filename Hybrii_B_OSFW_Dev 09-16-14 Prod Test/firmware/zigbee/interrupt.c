#include <stdlib.h>
#include <string.h>	
#include "ism.h"
#include "hal_reg.h"

#ifdef HPGP_HAL_TEST
/*extern  void CHAL_Tim1Isr(void) interrupt 3
{
    //u32  intStatus;
    uInterruptReg intStatus;

    // Read interrupt status.
    intStatus.reg = ReadU32Reg(CPU_INTSTATUS_REG);
}

void CHAL_Ext0Isr(void) interrupt 0
{
       // gHalCB.extIntCnt++;
} */

#endif 