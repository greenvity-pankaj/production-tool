#ifndef _DATAPATH_H
#define _DATAPATH_H

typedef enum _queue_id_e
{
   HOST_DATA_QUEUE,
   PLC_DATA_QUEUE,
   MAX_DATA_QUEUES
}queue_id_e;
typedef struct _dqueue_
{
   sSwFrmDesc desc[MAX_Q_BUFFER];
   u8 head;
   u8 tail;

}dqueue_t;

extern dqueue_t gDqueue[MAX_DATA_QUEUES];
void datapath_init();

void datapath_handlePlcTxDone();
eStatus datapath_queueToHost (sSwFrmDesc*  pPlcRxFrmSwDesc,
                              u16            frameSize);


void datapath_queue(queue_id_e id,
						   sSwFrmDesc *pPlcTxFrmSwDesc);

#if defined(POWERSAVE) || defined(LLP_POWERSAVE)
void datapath_transmitDataPlc(u8 from);
#else
void datapath_transmitDataPlc();
#endif

void datapath_transmitDataHost();

void datapath_queue_depth(queue_id_e id);

void datapath_hostTransmitFrame(u8* TxByteArr, u16 frameSize);

void datapath_resetQueue(queue_id_e id);

bool datapath_IsQueueFull(queue_id_e id);

sSwFrmDesc *datapath_getHeadDesc(queue_id_e id, u8 pop);

bool datapath_IsQueueEmpty(queue_id_e id);

#ifdef UM
eStatus  datapath_HostTransmitEvent();

bool datapath_transmitMgmtPlc();

#endif



#endif //end of  define _DATAPATH_H

