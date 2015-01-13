#ifndef FRAMETASK_H
#define FRAMETASK_H


#define MGMT_DATAFRAME     (0x88E1)
#define CTRL_DATAFRAME     (0x88E2)

#define MAX_FRAME_LEN 256
/* Control Layer */
typedef struct frameTask
{
#ifndef P8051
#if defined(WIN32) || defined(_WIN32)
    HANDLE   frmEvntSem;
#else //POSIX
    sem_t    frmEvntSem;
#endif
#endif

    sSlist   eventQueue;     /* external event queue */
    void    (*deliverEvent)(void XDATA *eventcookie, sEvent XDATA *event);
    void     *eventcookie;
} sFrameTask, *psFrameTask;

//eStatus FrameTask_PostEvent(enum eventType evttype, 
//  							u8 evntclass, void* data_ptr, u16 len);

void frame_task_init(void);

void fwdAgent_handleData(sSwFrmDesc  *plcTxFrmSwDesc);

#ifdef UM
void fwdAgent_sendFrame(eHybriiPortNum dstPort,
             					 sEvent *event);


void fwdAgent_sendEvent(eFwdAgentModule mod,
					 			sEvent *event);



void Host_SendIndication(u8 eventId, u8 *payload, u8 length);


#define SEND_HOST_EVENT(x) fwdAgent_sendEvent(FWDAGENT_HOST_EVENT,x)

#endif


#endif
