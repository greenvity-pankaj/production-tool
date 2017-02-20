#ifndef APP_SUP_LAYER_H
#define APP_SUP_LAYER_H

extern gv701x_aps_queue_t appSupLayer;
extern sSlist peripheralTxQ;
gv701x_aps_queue_t* Aps_init(void);
u8 Aps_Proc(void *cookie);
void APS_ProcPeripheral();
eStatus Aps_PostDataToQueue(u8 src_port, sSwFrmDesc* plcRxFrmSwDesc);
eStatus Aps_PostRspEventToQueue(sEvent* event);
void Aps_TxData(u8 dstn_port, u8* buffDesc, u16 len);
#endif /* APP_SUP_LAYER_H */
