#ifndef APP_SUP_LAYER_H
#define APP_SUP_LAYER_H

gv701x_aps_queue_t* Aps_init(void);
u8 Aps_Proc(void *cookie);
#if 0
u8 Aps_ProcCmd(void *cookie);
#endif
eStatus Aps_PostDataToQueue(u8 src_port, sSwFrmDesc* plcRxFrmSwDesc);
eStatus Aps_PostRspEventToQueue(sEvent* event);
void Aps_TxData(u8 dstn_port, u8* buffDesc, u16 len);
#endif /* APP_SUP_LAYER_H */
