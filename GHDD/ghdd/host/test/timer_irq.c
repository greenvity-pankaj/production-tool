#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/timer.h>
#include <linux/interrupt.h>
#include "host.h"
#include "nmm.h"

void short_do_tasklet (unsigned long);
DECLARE_TASKLET(short_tasklet, short_do_tasklet, 0);
struct timer_list hpgp_timer[10];

void 
hpgp_driver_schedule( unsigned long nmm )
{
	tasklet_schedule(&short_tasklet);
}

void 
short_do_tasklet (unsigned long dummy)
{
	sNmm *nmm = (sNmm *)Host_GetNmm();
	CDBG( "tasklet called (%ld).\n", jiffies );    
	NMM_Proc((sNmm*)nmm);
}

