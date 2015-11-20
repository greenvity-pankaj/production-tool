/*
* $Id: ehal_tst.c,v 1.3 2014/11/11 14:52:58 ranjan Exp $
*
* $Source: /home/cvsrepo/Hybrii_B_OSFW_Dev/firmware/project/hal/src/ehal_tst.c,v $
*
* Description : ETH HAL Test module.
*
* Copyright (c) 2010-2011 Greenvity Communications, Inc.
* All rights reserved.
*
* Purpose :
*     Defines ETH Tx/Rx test functions.
*
*
*/

#include <stdio.h>
#include <string.h>
#include <intrins.h>
#include "fm.h"
#include "hal_common.h"
#include "hal.h"
#include "hal_eth.h"
#include "hal_tst.h"
#include "hal_cfg.h"
#include "hal_spi.h"
#include "hpgpdef.h"
#ifdef UM
#include "ctrll.h"
#include "linkl.h"
#endif
#include "hpgpapi.h"
#include "utils_fw.h"

int getline(char *s, int lim);

#ifdef UM 
#ifdef LINK_STATUS
u16 linkTestCnt = 0;
static u8  bcAddr[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#endif
#endif

#ifdef HYBRII_ETH

extern sConnState  ConnState[MAX_NUM_STATIONS];
u8 ETH_TxRxTest = FALSE;
u8 ethDebugON = FALSE;
u8  myStation = UNDEFINED_STATION;
u8  numSlaves = 0;  // # of test stations
u8  stationType = UNDEFINED_STATION;
sEthSimTxTestParams testParams;
extern u8 gEthMacAddrDef[];
#ifndef HPGP_MAC_SAP
void EHT_SimulateTx(sEthSimTxTestParams* pTestParams)
{
    u16             tmpFrmLen;
    u16             minFrmLen;
    u16             maxFrmLen;
    u16             curFrmLen;
    u8              curOffsetDW;
    u8              curDescLen;
    eStatus         status;
    u8              i;
    u8              j;
    u8              quit;

    quit         = 0;
    curOffsetDW  = 31;///0;
    curDescLen   = 3;///HYBRII_DEFAULT_SNID;

    // Incremental/alternating length modes
    if(pTestParams->frmLen == 0)
    {
        minFrmLen  = 60;//40;
        maxFrmLen  = 1536;
        curFrmLen = minFrmLen;
        printf("\nStarting from len = %u\n",curFrmLen);
    }
    else
    {
        // fixed length test
        curFrmLen = pTestParams->frmLen;
    }

    curOffsetDW  = pTestParams->offsetDW;///0;
    curDescLen   = pTestParams->descLen;///HYBRII_DEFAULT_SNID;
    gEthHalCB.CurTxTestFrmCnt = 0;
    gEthHalCB.CurTxTestBytesCnt = 0;

    //for each frame
    while(1)
    {
        sSwFrmDesc             ethTxFrmSwDesc;
        volatile sEth2Hdr*          pEth2Hdr;
        sEth2Hdr                    eth2Hdr;
        u8*                         ethHdrArr;
        u8                          ethHdrLenSoFar;
        u8                          ethHdrLenCurCP;

        u8  frmData    = gEthHalCB.TotalTxFrmCnt;//0;
        tmpFrmLen      = 0;
        ethHdrLenSoFar = 0;
        ethTxFrmSwDesc.frmLen  = curFrmLen;
        ethTxFrmSwDesc.cpCount = 0;

        pEth2Hdr = &eth2Hdr;//(sEth2Hdr xdata*) (cellAddr + tmpOffsetByte);
        ethHdrArr = (u8*)pEth2Hdr;
        memset(pEth2Hdr->dstaddr, 0xFF, MAC_ADDR_LEN);
        memcpy(pEth2Hdr->srcaddr, gEthMacAddrDef, MAC_ADDR_LEN);
        pEth2Hdr->ethtype = htons(ETH_TYPE_ARP);


        // create cp descriptors
        //for( i=0 ; i<plcTxFrmSwDesc.cpCount ; i++)
//      printf("pTestParams->frmLen: %d\n", pTestParams->frmLen);
        while(tmpFrmLen < curFrmLen)
        {
            u8        cp;
            volatile u8 xdata *       cellAddr;
            u8        tmpOffsetDW;
            u8        tmpOffsetByte;
            u8        tmpDescLen;
            u8        actualDescLen;

            ethHdrLenCurCP = 0;
            // Fetch CP
            do
            {
                status = CHAL_RequestCP(&cp);
                if(CHT_Poll() == 'q')
                {
                    // Realease CPs fetched so far for current frame -- tbd
                    quit = 1;
                    break;
                }
            }while (status != STATUS_SUCCESS);
            i = ethTxFrmSwDesc.cpCount;
            // check for user initiated exit task
            if(quit)
            {
                break;
            }

            tmpOffsetDW =      curOffsetDW; //
            tmpDescLen  =      curDescLen; //
            // test offset and desc len - only for first CPs
            if(i==0 || i==1)
            {
                if(pTestParams->altOffsetDescLenTest)
                {
                    curOffsetDW--;
                    curDescLen+=4;

                    //tmpOffsetDW =        pTestParams->offsetDW; // curOffsetDW; //
                    //tmpDescLen  =      pTestParams->descLen; //     curDescLen;  //

                    tmpOffsetDW =      curOffsetDW; //
                    tmpDescLen  =      curDescLen; //
                    if(curOffsetDW == 0)
                    {
                        curOffsetDW  = pTestParams->offsetDW;///0;
                        curDescLen   = pTestParams->descLen;///HYBRII_DEFAULT_SNID;
                        printf("OffsetDW & DescLen resetting to %bu & %bu respectively.\n", 
                                curOffsetDW, curDescLen);
                    }
                }//printf("curOffsetDW = %bu, tempDescLen=%bu\n", tmpOffsetDW, tmpDescLen);
            }
            else
            {
                tmpOffsetDW = 0;
                tmpDescLen  = HYBRII_CELLBUF_SIZE;
            }

            tmpOffsetByte = tmpOffsetDW << 2;
            actualDescLen =  (curFrmLen-tmpFrmLen)>tmpDescLen?tmpDescLen:(curFrmLen-tmpFrmLen);
            if((i==0 || i==1 ) && tmpOffsetDW != 0)
            {
                FM_Printf(FM_LINFO,"curFrmLen = %u, curOffsetDW = %bu, curDescLen=%bu, free CPCnt = %bu\n", 
                    curFrmLen, tmpOffsetDW, actualDescLen, CHAL_GetFreeCPCnt());
            }

            FM_Printf(FM_LINFO, "curOffsetByte = %bu, curDescLen=%bu\n", tmpOffsetByte, actualDescLen);

            // Fill Buffer with pattern
            cellAddr = CHAL_GetAccessToCP(cp);
            FM_Printf(FM_LINFO, "cp = %bu, cellAddr=%08lX\n", cp, (u32)cellAddr);
            if(ethHdrLenSoFar < sizeof(sEth2Hdr))
            {
                ethHdrLenCurCP = actualDescLen > (sizeof(sEth2Hdr)-ethHdrLenSoFar) ? 
                    (sizeof(sEth2Hdr)-ethHdrLenSoFar) : actualDescLen;
                //memcpy(cellAddr+tmpOffsetByte, ethHdrArr + ethHdrLenSoFar, ethHdrLenCurCP);
                for( j=0 ; j<ethHdrLenCurCP ; j++)
                {
                    cellAddr[j+tmpOffsetByte] = * ( ethHdrArr + ethHdrLenSoFar + j);
                    //WriteU8Reg(&cellAddr[j], * (u8*)( ethHdrArr + ethHdrLenSoFar + j ));
                }
                ethHdrLenSoFar += ethHdrLenCurCP;
                tmpOffsetByte+=ethHdrLenCurCP;
            }
            if(ethHdrLenCurCP<actualDescLen)
            {
                for(j=tmpOffsetByte ; j<tmpOffsetByte+actualDescLen-ethHdrLenCurCP ; j++)
                {
                    cellAddr[j] = ++frmData;
                    //WriteU8Reg(&cellAddr[j],frmData++);
                }
            }
            ethTxFrmSwDesc.cpArr[i].offsetU32 = tmpOffsetDW;
            ethTxFrmSwDesc.cpArr[i].len  = actualDescLen;

            tmpFrmLen += ethTxFrmSwDesc.cpArr[i].len;
            ethTxFrmSwDesc.cpArr[i].cp = cp;
            ethTxFrmSwDesc.cpCount++;
        }

        // check for user initiated exit task
        if(quit)
        {
            break;
        }
        else//if(status == STATUS_SUCCESS)
        {
            do
            {
                // write to
                status = EHAL_EthTxQWrite(&ethTxFrmSwDesc);

                // check for user initiated exit task from infinite loop
//              if(CHT_Poll() == 'q')
//              {
//                  // if TxQWrite failed, release CPs for current frame -- tbd
//                    quit = 1;
//                  break;
//              }
            }while(status == STATUS_FAILURE);

            gEthHalCB.CurTxTestFrmCnt++;
            gEthHalCB.CurTxTestBytesCnt+= curFrmLen;

            // check for incremental length
            if(pTestParams->frmLen == 0)
            {
                curFrmLen++;
                // restart inc len test
                if(curFrmLen > maxFrmLen)
                {
                    printf("\nCur Frame Len = %u, Starting over from len %u\n",curFrmLen-1,minFrmLen);
                    curFrmLen = minFrmLen;
                }
            }
        }
        if((gEthHalCB.CurTxTestFrmCnt & (u32)(0x3FF)) == 0)
        {
            printf("Sent %lu ETH frames.\n", gEthHalCB.CurTxTestFrmCnt);
        }
        if(!pTestParams->contMode)
        {
            pTestParams->numFrames--;
            if(!pTestParams->numFrames)
            {
                quit = 1;
                break;
            }
        }

        if(!quit && pTestParams->delay != 0xFF)
        {
            //u32 delay = pTestParams->delay<<6;
            u32 delay64ticks = pTestParams->delay;
            if(delay64ticks == 0)
            {
                u8 userInput = 0;
                printf("press c to continue\n");
                while(1)
                {
                    userInput = CHT_Poll();
                    if( userInput == 'c' || userInput == 'C')
                    {
                        // exit delay loop and resume transmission
                        break;
                    }
                    else    if( userInput == 'q' || userInput == 'Q')
                    {
                        // exit delay loop and quite transmission
                        quit = 1;
                        break;
                    }
                }
            }
            else
            while(delay64ticks--)
            {
                CHAL_DelayTicks(64);
            }

        }
        // getkey
        if(quit )//|| CHAL_GetFreeCPCnt() < 116 || EHAL_GetEthTxQFreeDescCnt() < 60 )
        {
            printf("Quit Tx: Free CP Cnt = %bu, curFrmLen = %u\n", CHAL_GetFreeCPCnt(), curFrmLen);
            break;
        }
    } // while(1)
    printf("\nSent %lu ETH frames, %lu bytes.\n", 
            gEthHalCB.CurTxTestFrmCnt, gEthHalCB.CurTxTestBytesCnt);
}

eStatus ETH_allocOneCP(u8 *cp)
{
    u8  i =0;
    eStatus status;

    do
    {
        status = CHAL_RequestCP(cp);
        i++;
    }while ((status != STATUS_SUCCESS) && (++i < 10));

    if(status != STATUS_SUCCESS)
    {
        printf("EHT_allocOneCP: cannot allocate a CP. Exit !!!\n");
    }
    return(status);
}

eStatus ETH_alloc1stCP(u8 *srcaddr, u8 *dstaddr, sEthGCIHdr *pSrcGCIHdr, 
                       sSwFrmDesc *pEthTxFrmSwDesc)
{
    u8  cp;
    volatile sEth2Hdr   *pEth2Hdr;
    volatile u8 XDATA * cellAddr;
    volatile sEthGCIHdr *pGCIHdr;

    if (ETH_allocOneCP(&cp) == STATUS_FAILURE)
        return(STATUS_FAILURE);

    cellAddr = CHAL_GetAccessToCP(cp);

    // Fill MAC header
    pEth2Hdr = (sEth2Hdr *) cellAddr;
    memcpy(pEth2Hdr->srcaddr, srcaddr, MAC_ADDR_LEN);
    memcpy(pEth2Hdr->dstaddr, dstaddr, MAC_ADDR_LEN);
    pEth2Hdr->ethtype = ETH_TYPE_GREENVITY;

    // Fill Greenvity header
    // remember that not all GCI fields are used by all commands
    pGCIHdr = (sEthGCIHdr *) (cellAddr + sizeof(sEth2Hdr));
    pGCIHdr->pktType = pSrcGCIHdr->pktType;
    pGCIHdr->testType = pSrcGCIHdr->testType;
    memcpy(pGCIHdr->slaveMACaddr, pSrcGCIHdr->slaveMACaddr, MAC_ADDR_LEN);

    // set up CP for xmit
    pEthTxFrmSwDesc->cpCount = 1;
    pEthTxFrmSwDesc->cpArr[0].offsetU32 = 0;
    // len of pkt is the same of len of 1st cp
    pEthTxFrmSwDesc->cpArr[0].len  = sizeof(sEth2Hdr) + sizeof(sEthGCIHdr);
    pEthTxFrmSwDesc->cpArr[0].cp = cp;
    return(STATUS_SUCCESS);
}

eStatus ETH_xmitCmdPkt(u8 *srcaddr, u8 *dstaddr, sEthGCIHdr *pSrcGCIHdr)
{
    sSwFrmDesc ethTxFrmSwDesc;
    eStatus status;
    u8  i;

    // this is a command frame, a frame consists of only ETH and GCI headers, so only 1 cp is needed
    ethTxFrmSwDesc.frmLen = sizeof(sEth2Hdr) + sizeof(sEthGCIHdr);
    status = ETH_alloc1stCP(srcaddr, dstaddr, pSrcGCIHdr, &ethTxFrmSwDesc);
    if (status == STATUS_FAILURE)
    {
        printf("EHT_xmitCmdPkt: cannot allocate a CP. Packet type = %buExit !!!\n", 
                pSrcGCIHdr->pktType);
        return(status);
    }

    // xmit the packet
    i = 0;
    do
    {
        status = EHAL_EthTxQWrite(&ethTxFrmSwDesc);
        if (status == STATUS_FAILURE)
        {
            CHT_Poll();
            i++;
        }
    }while((status == STATUS_FAILURE) && (++i < 10));

    if(status == STATUS_FAILURE)
    {
        u8 i;

   //     printf("EHT_xmitCmdPkt: cannot xmit a packet. Packet type = %buExit !!!\n", 
   //             pSrcGCIHdr->pktType);
        // free the CP and return
        for (i = 0; i < ethTxFrmSwDesc.cpCount; i++)
            CHAL_DecrementReleaseCPCnt(ethTxFrmSwDesc.cpArr[i].cp);
    }
    return(status);
}
extern u8 gEthMacAddrBrdcast[];

eStatus EHT_OpenConn(sConnState *pConnState, sEthSimTxTestParams *pTestParams)
{
    sEthGCIHdr GCIHdr;
    eStatus status;
    u16 i;

    memset(&GCIHdr, 0, sizeof(sEthGCIHdr));
    memcpy(pConnState->slaveMACaddr, pTestParams->slaveMACaddr, MAC_ADDR_LEN);
    memcpy(pConnState->myMACaddr, pTestParams->myMACaddr, MAC_ADDR_LEN);
    pConnState->numFrames = pTestParams->numFrames;

    memcpy(GCIHdr.slaveMACaddr, pTestParams->slaveMACaddr, MAC_ADDR_LEN);
    GCIHdr.pktType =  CMD_CONN_REQ_PKT;
    GCIHdr.testType =  pTestParams->testType;
    status = ETH_xmitCmdPkt(pTestParams->myMACaddr, gEthMacAddrBrdcast, &GCIHdr);
    if (status == STATUS_FAILURE)
        return(status);

    // wait for a ConnResp from the slave
    i = 0;
    do
    {
        // give the ETH RCV ISR a chance to be called
        CHT_Poll();
        i++;
    } while ((pConnState->state == GCI_STATE_CLOSED) && (i<10000));

    if (pConnState->state == GCI_STATE_CLOSED)
    {
  //      printf("EHT_estConn: Station %bx did not return a Conn Response from a CONNECT REQUEST.\n",
   //             pConnState->slaveMACaddr[MAC_ADDR_LEN-1]);
        memset(pConnState, 0, sizeof(sConnState));  // clear ConnState
        return(STATUS_FAILURE);
    }

    return(STATUS_SUCCESS);
}

eStatus EHT_CloseConn(sConnState *pConnState)
{
    sEthGCIHdr GCIHdr;
    eStatus status;
    u16 i=0;

    if (pConnState->state != GCI_STATE_OPEN)
        return(STATUS_FAILURE);

    memset(&GCIHdr, 0, sizeof(sEthGCIHdr));
    GCIHdr.pktType =  CMD_DISCON_REQ_PKT;
    status = ETH_xmitCmdPkt(pConnState->myMACaddr, pConnState->slaveMACaddr, &GCIHdr);
    if (status == STATUS_FAILURE)
        return(status);

    // we just sent out a Disconnect request
    // wait for an ACK from the client
    do
    {
        CHT_Poll();
        i++;
    } while ((pConnState->state == GCI_STATE_OPEN) && (i< 10000));
    if (pConnState->state == GCI_STATE_OPEN)
    {
      //  printf("EHT_CloseConn: Station %bx did not return a DISCON ACK from a DISCON REQUEST. Exit !!!\n",
      //          pConnState->slaveMACaddr[MAC_ADDR_LEN-1]);
        memset(pConnState, 0, sizeof(sConnState));  // clear ConnState
        return(STATUS_FAILURE);
    }
   // printf("\n\nMaster sent %ld ETH Data frames, received %ld ETH Data frames.\n"
   //        "Station %bu sent %ld ETH Data frames, received %ld ETH Data frames\n\n\n",
   //         pConnState->my_numPktTx, pConnState->my_numPktRx, pConnState->slaveMACaddr[5],
   //         pConnState->slave_numPktTx, pConnState->slave_numPktRx);
    memset(pConnState, 0, sizeof(sConnState));  // clear ConnState
//  stationType = UNDEFINED_STATION;        // set it to unknown
    return(STATUS_SUCCESS);
}

eStatus EHT_xmitData(sConnState *pConnState, sEthSimTxTestParams *pTestParms)
{
    u16             curFrmLen;
    u16             tmpFrmLen;
    u8              i, j;
    u8              quit=FALSE;
    u32             numFrames2xmit;
    u32             maxNumFrames2xmit;
    eStatus         status=STATUS_SUCCESS;

    curFrmLen = pTestParms->frmLen;

    // # of frames to xmit per iteration
    if (pTestParms->numFrames2xmit == 0)
        maxNumFrames2xmit = pTestParms->numFrames;
    else maxNumFrames2xmit = pTestParms->numFrames2xmit;

    for (numFrames2xmit = 0; numFrames2xmit < maxNumFrames2xmit; numFrames2xmit++)  // for each DATA frame
    {
        sSwFrmDesc             ethTxFrmSwDesc;
        u8                          frmData    =  '0';
        u8                          userInput=0;

        memset(&ethTxFrmSwDesc, 0, sizeof(sEthTxFrmSwDesc));
        tmpFrmLen      = 0;
        ethTxFrmSwDesc.frmLen  = curFrmLen;
        ethTxFrmSwDesc.cpCount = 0;

        while(tmpFrmLen < curFrmLen)        // for each CP in a DATA frame
        {
            u8        tmpCpCount;
            u8        tmpOffsetByte;
            volatile u8 xdata *cellAddr;
            u16       numBytes2Write;
            volatile sEthGCIHdr GCIHdr;

            tmpCpCount = ethTxFrmSwDesc.cpCount;

            if (tmpCpCount == 0)
            {
                // first CP contains the ETH header and GCI header
                // for DATA pkt, GCI header only contains the pktType
                memset(&GCIHdr, 0, sizeof(sEthGCIHdr));
                GCIHdr.pktType =  CMD_DATA_PKT;

                // send a DATA pkt
                status = ETH_alloc1stCP(pConnState->myMACaddr, pConnState->slaveMACaddr, 
                                        &GCIHdr, &ethTxFrmSwDesc);
                if (status == STATUS_FAILURE)
                {
                    printf("EHT_xmitData: Cannot allocate 1st CP. Client's MAC address (last byte) %bx. Exit !!!\n",
                        pConnState->slaveMACaddr[MAC_ADDR_LEN-1]);
                    return(status);
                }
                tmpOffsetByte = sizeof(sEth2Hdr) + sizeof(sEthGCIHdr);
            } else
            {
                // subsequent CP contains only data
                u8 cp;

                status = ETH_allocOneCP(&cp);
                if (status == STATUS_FAILURE)
                {
                    printf("EHT_xmitData: Cannot allocate a CP. Client's MAC address (last byte) %bx. Exit !!!\n",
                            pConnState->slaveMACaddr[MAC_ADDR_LEN-1]);
                    return(status);
                }
                ethTxFrmSwDesc.cpArr[tmpCpCount].offsetU32 = 0;
                ethTxFrmSwDesc.cpArr[tmpCpCount].len  = 0;
                ethTxFrmSwDesc.cpArr[tmpCpCount].cp = cp;
                ethTxFrmSwDesc.cpCount++;
                tmpOffsetByte = 0;
            }

            // fill frame with data
            cellAddr = CHAL_GetAccessToCP(ethTxFrmSwDesc.cpArr[tmpCpCount].cp);
            // # of bytes to write
            numBytes2Write = (curFrmLen - tmpFrmLen) > HYBRII_CELLBUF_SIZE ? 
                              HYBRII_CELLBUF_SIZE:(curFrmLen - tmpFrmLen);

            // if 1st CP, minimal len is the headers
            if (numBytes2Write < tmpOffsetByte)
                 numBytes2Write = tmpOffsetByte;

            for(j=tmpOffsetByte; j<numBytes2Write; j++)
            {
                cellAddr[j] = frmData;
                if (++frmData == 'z')
                    frmData = 'a';
            }
            ethTxFrmSwDesc.cpArr[tmpCpCount].len = numBytes2Write;
            tmpFrmLen += numBytes2Write;
        } // while tmp..

        do
        {
            // xmit the frame
            status = EHAL_EthTxQWrite(&ethTxFrmSwDesc);
            if (status == STATUS_FAILURE)
            {
                userInput = CHT_Poll();
                i++;
            }
        }while((status == STATUS_FAILURE) && (++i < 10));
        if(status == STATUS_FAILURE)
        {
            printf("EHT_xmitData: cannot xmit a packet. Exit !!!\n");
            return(STATUS_FAILURE);
        }

        // the tx counter is only for DATA pkts
        pConnState->my_numPktTx++;
        if(((pConnState->my_numPktTx % 64) == 0) && (pConnState->my_numPktTx > 0))
        {
            printf("\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b"
                   "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
            if(pConnState->testType == HALF_DUPLEX_TEST)
                printf("Sent %ld ETH Data frames", pConnState->my_numPktTx);
            else
                printf("Sent %ld Data frames, Rcvd %ld Data frames", 
                        pConnState->my_numPktTx, pConnState->my_numPktRx);
        }

        // If not continuous mode, decrement total # of pkts
        if(!pTestParms->contMode)
        {
            pConnState->numFrames--;
            if(!pConnState->numFrames)
            {
                quit = 1;
            }
        }

        if(pTestParms->delay != 0xFF)
        {
            u32 delay64ticks = pTestParms->delay;
            if(delay64ticks == 0)
            {
                printf("press c to continue or q to quit\n");
                while(1)
                {
                    userInput = CHT_Poll();
                    if( userInput == 'c' || userInput == 'C')
                    {
                        // exit delay loop and resume transmission
                        break;
                    }
                    else    if( userInput == 'q' || userInput == 'Q')
                    {
                        // exit delay loop and quit transmission
                        quit = 1;
                        break;
                    }
                }
            }
            else
                while(delay64ticks--)
                {
                    CHAL_DelayTicks(64);
                }
        }

        // Check if user has typed 'q' for quit
        if (!userInput)
            userInput = CHT_Poll();
        if( userInput == 'q' || userInput == 'Q')
            // user wants to quit test
            quit = 1;

        if (quit)
        {
            // if quit, a DISCON REQ is sent
            return(EHT_CloseConn(pConnState));
        }
    } // for each frame...
    return(STATUS_SUCCESS);
}

void EHT_testTxRxStations(sEthSimTxTestParams *pTestParms)
{
    u8  i, numStations, numClosedStations, numOpenStations=0;
    eStatus         status;
    sConnState *pConnState;

    memset(ConnState, 0, sizeof(sConnState)*MAX_NUM_STATIONS);

    numStations = pTestParms->numSlaves;
    // first, establish connections with all available stations
    for (i = 0; i < numStations; i++)
    {
        // assign individual MAC addr to each station
        pTestParms->slaveMACaddr[MAC_ADDR_LEN-1] = i;   
        status = EHT_OpenConn(&ConnState[i], pTestParms);
        if (status == STATUS_SUCCESS)
        {
            printf("Station %bu is ready for the test\n", i);
            numOpenStations++;
        }
    }

    if (!numOpenStations)
    {
        printf("No Test Station on the Network ! Exit\n");
        return;
    }

    // now xmit data to each slave station
    while (TRUE)
    {
        numClosedStations = 0;
        for (i = 0; i < numStations; i++)
        {
            pConnState = &ConnState[i];
            if (pConnState->state == GCI_STATE_CLOSED)
            {
                numClosedStations++;
                continue;
            }

            status = EHT_xmitData(pConnState, pTestParms);
            if (status == STATUS_FAILURE)
                printf("Failed to xmit data to station %bu\n", i);
        }

        if (numClosedStations == numStations)
            // all stations are done with test
            break;
    }
}

void EHT_BasicTxRxMenu(u8 txrxtest)
{
    // test mode variables
    u16             frmLen;
    u32             numFrames;
    u32             numFrames2xmit;
    u8              alterDescLenNOffset;
    char            input[100];
    u8              testType;
    u8              numbytes;
    u8              MACaddr[MAC_ADDR_LEN];
    u32             delay;
    u8 defMasterMACaddr[MAC_ADDR_LEN] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x01};
    u8 defSlaveMACaddr[MAC_ADDR_LEN] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x00};

    if (txrxtest)
    {
        do
        {
            printf("Enter number of test stations  : 1-%bu (default is 1)        :: ", MAX_NUM_STATIONS);
            numbytes = getline(input, sizeof(input));
            if (numbytes == 1)
            {
                // user hits <CR>: take the default value
                numSlaves = 1;
                break;
            } else
            {
                sscanf(input,"%bu",&numSlaves);
                printf("numslaves=%bu\n", numSlaves);
                if((numSlaves >= 1) && (numSlaves <= MAX_NUM_STATIONS))
                {
                    break;
                }
            }
        } while (TRUE);
        testParams.numSlaves = numSlaves;
    }

    if (txrxtest == 2)
    {
        // quick txrx test: set everything else default values
        testParams.testType  = FULL_DUPLEX_TEST;
        memcpy(testParams.myMACaddr, defMasterMACaddr, MAC_ADDR_LEN);
        memcpy(testParams.slaveMACaddr, defSlaveMACaddr, MAC_ADDR_LEN);
        testParams.frmLen = 1500;
        testParams.numFrames  = 2000;
        testParams.contMode = 0;
        testParams.numFrames2xmit = 100;
        testParams.delay = 50;
        printf("Quick Transmit/Receive Test's default values: \n");
        printf("    - Test method: Full Duplex\n");
        printf("    - Master MAC Address: %bx %bx %bx %bx %bx %bx\n",
                testParams.myMACaddr[0], testParams.myMACaddr[1], testParams.myMACaddr[2], 
                testParams.myMACaddr[3], testParams.myMACaddr[4], testParams.myMACaddr[5]);
        printf("    - Slave MAC Multicast Address: %bx %bx %bx %bx %bx \n",
                testParams.slaveMACaddr[0], testParams.slaveMACaddr[1], testParams.slaveMACaddr[2], 
                testParams.slaveMACaddr[3], testParams.slaveMACaddr[4]);
        printf("    - # of bytes per frame: %d\n", testParams.frmLen);
        printf("    - # of total frames to transmit: %ld\n", testParams.numFrames);
        printf("    - # of frames per iteration: %ld\n", testParams.numFrames2xmit);
        printf("    - Delay (in ticks) after each transmission: %ld\n", testParams.delay);
    }

    if (txrxtest == 1)
    {
        do
        {
            printf("Enter Test Type  : 1-Half Duplex, 2-Full Duplex (Default is Half-Duplex)        :: ");
            numbytes = getline(input, sizeof(input));
            if (numbytes == 1)
            {
                // user hits <CR>: take the default value
                testType = HALF_DUPLEX_TEST;
                break;
            } else
            {
                if((sscanf(input,"%bx",&testType) == 1) && ((testType == HALF_DUPLEX_TEST) || 
                    (testType == FULL_DUPLEX_TEST)))
                {
                    break;
                }
            }
        } while (TRUE);
        testParams.testType  = testType;

        do
        {

            printf("Enter Master MAC address  : (Default is 0x10 0x10 0x10 0x10 0x10 0x01)        :: ");
            if ((numbytes = getline(input, sizeof(input))) == 1)
            {
                // user hits <CR>: take the default value
                memcpy(MACaddr, defMasterMACaddr, MAC_ADDR_LEN);
                break;
            } else
            {
                if (sscanf(input,"%bx %bx %bx %bx %bx %bx", &MACaddr[0], 
                        &MACaddr[1], &MACaddr[2], &MACaddr[3], &MACaddr[4], 
                        &MACaddr[5]) == MAC_ADDR_LEN)
                {
                    break;
                }
            }
        } while (TRUE);
        memcpy(testParams.myMACaddr, MACaddr, MAC_ADDR_LEN);

        do
        {

            printf("Enter Slave MAC multicast address (the first 5 bytes)  :",
                   "(Default is 0x20 0x20 0x20 0x20 0x20)        :: ");
            if ((numbytes = getline(input, sizeof(input))) == 1)
            {
                // user hits <CR>: take the default value
                memcpy(MACaddr, defSlaveMACaddr, MAC_ADDR_LEN);
                break;
            } else
            {
                if (sscanf(input,"%bx %bx %bx %bx %bx", &MACaddr[0], &MACaddr[1], &MACaddr[2], 
                           &MACaddr[3], &MACaddr[4]) == (MAC_ADDR_LEN-1))
                {
                    MACaddr[MAC_ADDR_LEN-1] = 0;
                    break;
                }
            }
        } while (TRUE);
        memcpy(testParams.slaveMACaddr, MACaddr, MAC_ADDR_LEN);

        // Make sure that if master and slave have 
        // the same subnet then master's last byte MAC addr 
        // does not fall into the slave's range 
        // of addrs, ie. the last byte has to be
        // different. Otherwise, master's addr will 
        // be the same with one of slave's
        if (!memcmp(MACaddr, testParams.myMACaddr, MAC_ADDR_LEN-1))
        {
            printf("Master's last bye=%bx, numSlaves=%bu\n", testParams.myMACaddr[MAC_ADDR_LEN-1], testParams.numSlaves);
            if ((testParams.myMACaddr[MAC_ADDR_LEN-1] >= 0) && 
                (testParams.myMACaddr[MAC_ADDR_LEN-1] < testParams.numSlaves))
            {
                testParams.myMACaddr[MAC_ADDR_LEN-1] =  testParams.numSlaves;
                printf("Master's MAC address is in conflict with Slave's\n"
                       "Master's MAC address is changed to %bx %bx %bx %bx %bx %bx\n",
                       testParams.myMACaddr[0], testParams.myMACaddr[1], testParams.myMACaddr[2],
                       testParams.myMACaddr[3], testParams.myMACaddr[4], testParams.myMACaddr[5]);
            }
        }                   
    } //if txrxtest == 1

    if ((txrxtest == 0) || (txrxtest == 1))
    {
        do
        {
            printf("Enter FrameLen  : 0-IncLen, 40 to 1536-Fixed Len        :: ");
            while (getline(input, sizeof(input)) > 0)
            {
                if(sscanf(input,"%d",&frmLen) >= 1)
                break;
            }
        }while (frmLen<40 && frmLen>1536);

        printf("Enter number of frames: 0-Continuous Mode, N-NumOfFrames :: ");
        while (getline(input, sizeof(input)) > 0)
        {
            if(sscanf(input,"%ld",&numFrames) >= 1)
            break;
        }
    }

    if (txrxtest == 1)
    {
        u32 limit;

        // if xmit continuously, we limit the #of frames to xmit each iteration to 10000
        if (!numFrames)
            limit = 10000;
        else limit = numFrames;

        do
        {
            printf("Enter number of frames to transmit each iteration  : 0-transmit ",
                   "the maximum, 1 to numofFrames-1 (Default is 1)        :: ");
            numbytes = getline(input, sizeof(input));
            if (numbytes == 1)
            {
                // user hits <CR>: take the default value
                numFrames2xmit = 1;
                break;
            } else sscanf(input,"%ld",&numFrames2xmit);
        } while (numFrames2xmit<0 || numFrames2xmit>limit);
        testParams.numFrames2xmit = numFrames2xmit;
    }

    if (txrxtest == 0)
    {
        // TXRX does not do the DescLen and Offset test
        do
        {
            printf("DescLen & Offset Test Enable? : 0-Disable, 1-Enable      :: ");
            while (getline(input, sizeof(input)) > 0)
            {
                if(sscanf(input,"%bd",&alterDescLenNOffset) >= 1)
                break;
            }
        }while(alterDescLenNOffset != 0 && alterDescLenNOffset!= 1);
    }

    if ((txrxtest == 0) || (txrxtest == 1))
    {
        // fill tx test params structure
        printf("Enter delay (unit of 64 timer ticks)    :: ");
        while (getline(input, sizeof(input)) > 0)
        {
            if(sscanf(input,"%lu",&delay) >= 1)
            break;
        }
        testParams.delay = delay;

        testParams.numFrames  = numFrames;
        if(!testParams.numFrames)
        {
            testParams.contMode = 1;
        }
        else
        {
            testParams.contMode = 0;
        }

        testParams.frmLen = frmLen;
        if (txrxtest == 0)
        {
            testParams.altOffsetDescLenTest = alterDescLenNOffset;

            if(alterDescLenNOffset)
            {
                testParams.offsetDW  = 31;
                testParams.descLen   = 3;
            }
            else
            {
                testParams.offsetDW  = 0;
                testParams.descLen   = HYBRII_CELLBUF_SIZE;
            }
        }
    }

    if (txrxtest)
        // tx/rx test
        EHT_testTxRxStations(&testParams);
    else
        // tx test
        EHT_SimulateTx(&testParams);
}
#endif
#endif
#ifdef DOTHIS
void testThis()
{
    eStatus status;
    u16 i, frmLen;
    u8 defMasterMACaddr[MAC_ADDR_LEN] = {0x10, 0x10, 0x10, 0x10, 0x10, 0x01};
    u8 defSlaveMACaddr[MAC_ADDR_LEN] = {0x20, 0x20, 0x20, 0x20, 0x20, 0x00};
    char            input[100];

        do
        {
            printf("Enter FrameLen  : 0-IncLen, 40 to 1536-Fixed Len        :: ");
            while (getline(input, sizeof(input)) > 0)
            {
                if(sscanf(input,"%d",&frmLen) >= 1)
                break;
            }
        }while (frmLen<40 && frmLen>1536);

        testParams.frmLen = frmLen;
        testParams.numSlaves = 1;
        testParams.testType  = FULL_DUPLEX_TEST;
        memcpy(testParams.myMACaddr, defMasterMACaddr, MAC_ADDR_LEN);
        memcpy(testParams.slaveMACaddr, defSlaveMACaddr, MAC_ADDR_LEN);
        testParams.numFrames  = 2000;
        testParams.contMode = 0;
        testParams.numFrames2xmit = 100;
        testParams.delay = 50;
        printf("Quick Transmit/Receive Test's default values: \n");
        printf("    - Test method: Full Duplex\n");
        printf("    - Master MAC Address: %bx %bx %bx %bx %bx %bx\n",
                testParams.myMACaddr[0], testParams.myMACaddr[1], 
                testParams.myMACaddr[2], testParams.myMACaddr[3], 
                testParams.myMACaddr[4], testParams.myMACaddr[5]);
        printf("    - Slave MAC Multicast Address: %bx %bx %bx %bx %bx \n",
                testParams.slaveMACaddr[0], testParams.slaveMACaddr[1], 
                testParams.slaveMACaddr[2], 
                testParams.slaveMACaddr[3], testParams.slaveMACaddr[4]);
        printf("    - # of bytes per frame: %d\n", testParams.frmLen);
        printf("    - # of total frames to transmit: %ld\n", testParams.numFrames);
        printf("    - # of frames per iteration: %ld\n", testParams.numFrames2xmit);
        printf("    - Delay (in ticks) after each transmission: %ld\n", testParams.delay);


        memcpy(ConnState[0].myMACaddr, testParams.myMACaddr, MAC_ADDR_LEN);
        memcpy(ConnState[0].slaveMACaddr, testParams.slaveMACaddr, MAC_ADDR_LEN);
        ConnState[0].testType == HALF_DUPLEX_TEST;
        ConnState[0].state = GCI_STATE_OPEN;
        ConnState[0].my_numPktTx = 0;
        ConnState[0].my_numPktRx = 0;

        for (i = 0; i < testParams.numFrames; i++)
            status = EHT_xmitData(&ConnState[0], &testParams);
}
#endif


#ifndef HPGP_MAC_SAP
void EHAL_CmdHALProcess(char* CmdBuf)
{
#ifdef HYBRII_ETH
    u8  cmd[20];

    if (sscanf(CmdBuf+1, "%s", &cmd) < 1 || strcmp(cmd,"") == 0)
    {
        printf("HAL Test Commands:\n");
        printf("e xmitTest              - Data transmit test\n");
        printf("e xmitrcvTest/txrx      - Data transmit/receive test\n");
        printf("e quicktxrxTest/qtxrx   - Data transmit/receive test\n");
        printf("e stat                  - Display sw statistics\n");
        printf("e rstStat               - Reset sw statistics\n");
        printf("e hwstat                - Display hw statistics\n");
        printf("e rsthwStat             - Reset hw statistics\n");
        printf("e debug                 - Toggle debug flag\n\n");
        return;
    }

    if((strcmp(cmd, "xmitTest") == 0) || (strcmp(cmd, "tx") == 0))
    {
        EHT_BasicTxRxMenu(FALSE);
    }
    else if((strcmp(cmd, "xmitrcvTest") == 0) || (strcmp(cmd, "txrx") == 0))
    {
        EHT_BasicTxRxMenu(TRUE);
    }
    else if((strcmp(cmd, "quickxmitrcvTest") == 0) || (strcmp(cmd, "qtxrx") == 0))
    {
        EHT_BasicTxRxMenu(2);
    }
    else if (strcmp(cmd, "stat") == 0)
    {
        EHAL_DisplayEthStat();

    }
    else if (strcmp(cmd, "rststat") == 0 || strcmp(cmd, "rstStat") == 0)
    {
        EHAL_ResetStat();
    }
    else  if (strcmp(cmd, "hwstat") == 0)
    {
         EHAL_Print_ethHWStat();
    }
    else  if (strcmp(cmd, "rsthwstat") == 0)
    {
         EHAL_Clear_ethHWStat();
    }
    else if (strcmp(cmd, "debug") == 0)
    {
        // toggle the debug flag
        ethDebugON = !ethDebugON;
        printf("\n Debug flag is %s\n", ethDebugON ? "ON":"OFF");
    }
#ifdef DOTHIS
    else if (strcmp(cmd, "testthis") == 0)
    {
        testThis();
    }
#endif
#endif
}

#endif
