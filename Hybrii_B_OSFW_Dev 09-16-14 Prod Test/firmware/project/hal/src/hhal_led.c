/*
* $Id: hhal_led.c,v 1.2 2014/01/14 23:34:22 son Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/hhal_led.c,v $
*
* Description : HPGP Tx fuctions for LED Demo.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines functions for sending LED Demo command frame.
*
*
*/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "fm.h"
#include "hal_common.h"
#include "hal_hpgp.h"
#include "hal_tst.h"
#ifdef _LED_DEMO_
#include "led_board.h"
#include "utils.h"
#endif

int getline(char *s, int lim);
u8 led_demo_pkt_retry = 10;
u16 led_demo_tx_drop = 0;

#ifdef POWER_SAVE
extern u8 ethDebugON;
#endif
        
bool HHT_DemoModeTx(u8* demoFrm, u16 frmLen)
{
#ifdef _LED_DEMO_
    sTxFrmSwDesc    plcTxFrmSwDesc;
    eStatus         status;
    volatile u8 xdata *       cellAddr;
    u8              cp;
    u16             tmpFrmLen;
	u8              actualDescLen;
    u16             i,j;
    bool            rc = FALSE;
    memset(&plcTxFrmSwDesc, 0, sizeof(sTxFrmSwDesc));

    plcTxFrmSwDesc.frmLen     = frmLen+1;  // account for the i byte seq num
                                           // that will be prepended
    tmpFrmLen                 = 0;

    plcTxFrmSwDesc.frmInfo.plc.mcstMode   = HPGP_UCST;
    plcTxFrmSwDesc.frmInfo.plc.plid       = HPGP_PLID0;
    plcTxFrmSwDesc.frmType    = HPGP_HW_FRMTYPE_MSDU;
    plcTxFrmSwDesc.frmInfo.plc.stdModeSel = 1;
    plcTxFrmSwDesc.frmInfo.plc.dtei       = gHpgpHalCB.remoteTei;
    plcTxFrmSwDesc.frmInfo.plc.stei       = gHpgpHalCB.selfTei;
    plcTxFrmSwDesc.frmInfo.plc.bcnDetectFlag = 1;
    plcTxFrmSwDesc.frmInfo.plc.eks        = HPGP_UNENCRYPTED_EKS;
    plcTxFrmSwDesc.frmInfo.plc.phyPendBlks = HPGP_PPB_CAP0;
    plcTxFrmSwDesc.frmInfo.plc.clst = HPGP_CONVLYRSAPTYPE_RSV;
    plcTxFrmSwDesc.cpCount     = 0;

	while(tmpFrmLen < plcTxFrmSwDesc.frmLen)
	{
        i = plcTxFrmSwDesc.cpCount;
        // Fetch CP
        status = CHAL_RequestCP(&cp);
        if (status == STATUS_FAILURE) 
        {
            return (rc);
        }

        actualDescLen =  (plcTxFrmSwDesc.frmLen-tmpFrmLen)>HYBRII_CELLBUF_SIZE?HYBRII_CELLBUF_SIZE:(plcTxFrmSwDesc.frmLen-tmpFrmLen);

        // Fill Buffer with pattern
        cellAddr = CHAL_GetAccessToCP(cp);
        if( i==0 )
        {
            cellAddr[0] = (u8)gHpgpHalCB.halStats.TxSeqNum+1;
        }
        for( j=1 ; j<actualDescLen ;j++)
        {
            cellAddr[j] = demoFrm[j-1];
        }

		plcTxFrmSwDesc.cpArr[i].offsetU32 = 0;
		plcTxFrmSwDesc.cpArr[i].len  = actualDescLen;

        tmpFrmLen += plcTxFrmSwDesc.cpArr[i].len; 
        plcTxFrmSwDesc.cpArr[i].cp = cp;
        plcTxFrmSwDesc.cpCount++;
    }

    // queue this frame to PLC Tx queue
    status = HHAL_PlcTxQWrite(&plcTxFrmSwDesc);
    if (status == STATUS_FAILURE)
    {
        led_demo_tx_drop++;
        CHAL_FreeFrameCp(plcTxFrmSwDesc.cpArr, plcTxFrmSwDesc.cpCount);
        return (FALSE);
    }

    // gHpgpHalCB.halStats.CurTxTestFrmCnt will be incremented in HHAL_ProcessPlcTxDone()   
    gHpgpHalCB.halStats.TxSeqNum++;
#endif
    return (TRUE);
}

void HHT_TxLedDemo(u8 *payload_p)
{
#ifdef _LED_DEMO_
    sPlcDemoFrame   plcDemoFrm;
    u8 retry = 0;
    bool rc;

    memset(&plcDemoFrm, 0, sizeof(plcDemoFrm));
    memcpy(plcDemoFrm.hdrStr, PLC_LED_DEMO_CMD, strlen(PLC_LED_DEMO_CMD));
    memcpy(plcDemoFrm.disStr, payload_p, strlen(payload_p));
    printf("\nSend LED command: <%s>\n", plcDemoFrm.disStr);
    while (retry++ < led_demo_pkt_retry) {
        rc = HHT_DemoModeTx( (u8*) &plcDemoFrm, sizeof(sPlcDemoFrame));
        if (rc) {
            gHpgpHalCB.halStats.TxSeqNum--;
        }
        mac_utils_delay_ms(200);
    }
    gHpgpHalCB.halStats.TxSeqNum++;
#endif   
}

void HHT_LedDemoTxMenu(u8* CmdBuf)
{
#ifdef _LED_DEMO_
    u8              i = 0;
    char            input[10];

    //printf("\n\n%s", CmdBuf);

    while (CmdBuf[i++] == 0x20);
    if(i)
        i--;
    CmdBuf = &CmdBuf[i];
    
    if(strncmp(CmdBuf, PLC_DISP_GVC_CMD, strlen(PLC_DISP_GVC_CMD)) == 0)
    {  
        CmdBuf+= strlen(PLC_DISP_GVC_CMD) + 1;
        HHT_TxLedDemo(CmdBuf);
    }
    else if(strncmp(CmdBuf, PLC_LED_DEMO_RETRY_CMD, 
            strlen(PLC_LED_DEMO_RETRY_CMD)) == 0) {
        printf("Enter Retry(%bu) value: ", led_demo_pkt_retry);
        while (getline(input, sizeof(input)) > 0)
        {
            if (sscanf(input,"%bd", &led_demo_pkt_retry) >= 1)
                break;
        }
    }
    else if(strncmp(CmdBuf, PLC_LED_DEMO_STATS_CMD, 
            strlen(PLC_LED_DEMO_STATS_CMD)) == 0) 
    {
        printf("TotalTxLedFrmDrop     = %d\n", led_demo_tx_drop);
    }
    else if(strncmp(CmdBuf, PLC_LED_DEMO_RSTSTATS_CMD, 
            strlen(PLC_LED_DEMO_RSTSTATS_CMD)) == 0) 
    {
        led_demo_tx_drop = 0;
    }
    else
    {
        printf("Command not supported\n");
    }

#else
    printf("Command not supported\n");
#endif
}

u8  HHT_DemoModeRx(u8* demoFrm)
{
     bool bDemoMode = 0;
#ifdef _LED_DEMO_

    if(strncmp(demoFrm, PLC_LED_DEMO_CMD, strlen(PLC_LED_DEMO_CMD)) == 0 )
    {
        sPlcDemoFrame*  pPlcDemoFrame;

        bDemoMode    = 1;
        pPlcDemoFrame = ( sPlcDemoFrame* ) demoFrm;
        printf("\n<%s>\n", pPlcDemoFrame->disStr);
        led_msg_decode(pPlcDemoFrame->disStr);
        printf("\n");
    }
#endif
    return bDemoMode;
}
