#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> //sleep
#include <pthread.h>
#include "papdef.h"
#include "hpgpapi.h"
#include "host.h"
#include "uim.h"
#include "nmm.h"
#include "../../../common/fm.h"
#include "hpgpdef.h"

#include "crm.h"
#include "linkl.h"

u8 dummyKey[16] = {2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

void UIM_Menu()
{
    printf(" UIM  Menu: \n");
    printf("   1:  Set Device Password. \n");
    printf("   2:  Set Default NID.\n");
    printf("   3:  Set Security Mode.\n");
    printf("   4:  Restart Station.\n");
    printf("   5:  Start/Join Network.\n");
    printf("   6:  Leave Network.\n");
    printf("   7:  Exit from Network.\n");
    printf("   8:  Appoint CCo.\n");
    printf("   9:  Authenticate the current NMK using DPW.\n");
    printf("   10: Authenticate a NMK using DPW.\n");
    printf("   11: Authenticate NMK using UKE.\n");
    printf("   21: Get Device Password. \n");
    printf("   22: Get\n");
}

void UIM_DisplayResult(sUim *uim, u8 reqType, void *resultParam)
{
   switch(reqType)
   {
       case APCM_AUTHORIZE_CNF:
       {
           FM_Printf(1, "Request Id: %d\n", ((sNmkAuthCnf *)resultParam)->reqId);
           switch(((sNmkAuthCnf *)resultParam)->result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Authorization success\n");
                   FM_HexDump(FM_UIM, "MAC Address: ", ((sNmkAuthCnf *)resultParam)->macAddr, MAC_ADDR_LEN);
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: No Response\n");
                   break;
               case 2:
                   FM_Printf(FM_UIM, "Result: Protocol Aborted\n");
                   break;
           }
           break;
       }

       case APCM_AUTHORIZE_IND:
       {
           FM_HexDump(FM_UIM, "MAC Address: ", ((sNmkAuthInd *)resultParam)->macAddr, MAC_ADDR_LEN);
           switch(((sNmkAuthInd *)resultParam)->status)
           {
               case 0:
                   FM_Printf(FM_UIM, "Status: Authorization success\n");
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Status: Protocol Aborted\n");
                   break;
           }
           FM_HexDump(FM_UIM, "NID: ", ((sNmkAuthInd *)resultParam)->nid, NID_LEN);
           break;
       }

       case APCM_GET_SECURITY_MODE_CNF:
       {
           switch(((sGetSecModeCnf *)resultParam)->result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Success\n");
                   FM_Printf(FM_UIM, "Security Mode: %d\n", ((sGetSecModeCnf *)resultParam)->secMode);
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: Fail\n");
                   break;
           }
           break;
       }

       case APCM_SET_SECURITY_MODE_CNF:
       {
           u8 *result = (u8 *)resultParam;
           switch(*result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Success\n");
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: Fail\n");
                   break;
           }
           break;
       }

       case APCM_SET_KEY_CNF:
       {
           u8 *result = (u8 *)resultParam;
           switch(*result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Success\n");
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: Fail\n");
                   break;
           }
           break;
       }

       case APCM_GET_KEY_CNF:
       {
           FM_HexDump(FM_UIM, "NID: ", ((sGetKeyCnf *)resultParam)->nid, NID_LEN);
           FM_HexDump(FM_UIM, "NMK: ", ((sGetKeyCnf *)resultParam)->nmk, ENC_KEY_LEN);
           break;
       }

       case APCM_SET_PPKEYS_CNF:
       {
           u8 *result = (u8 *)resultParam;
           switch(*result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Success\n");
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: Fail\n");
                   break;
           }
           break;
       }

       case APCM_SET_AUTH_MODE_CNF:
       {
           u8 *result = (u8 *)resultParam;
           switch(*result)
           {
               case 0:
                   FM_Printf(FM_UIM, "Result: Success\n");
                   break;
               case 1:
                   FM_Printf(FM_UIM, "Result: Fail\n");
                   break;
           }
           break;
       }
   }
}


void UIM_SendGetSecModeReq(sNmm *nmm)
{
    NMM_ProcessRequest(nmm, APCM_GET_SECURITY_MODE_REQ, NULL);
}


void UIM_SendGetKeyReq(sNmm *nmm)
{
    NMM_ProcessRequest(nmm, APCM_GET_KEY_REQ, NULL);
}


#if 0
void UIM_SendSetPPKeysReq(sNmm *nmm)
{
    sSetPPKeysReq reqParam;
    memcpy(reqParam.ppEks, dummyKey, ATTR_LEN_PP_EKS); //TODO: Get PPEKs from somewhere
    memcpy(reqParam.ppek, dummyKey, ATTR_LEN_PPEK); //TODO: Get ppek from somewhere
    memcpy(reqParam.macAddr, dummyKey, ATTR_LEN_MACADDR); //TODO: Get MAC address from somewhere
    NMM_ProcessRequest(nmm, APCM_SET_PPKEYS_REQ, &reqParam);
}

#endif

static void UIM_PrintMACAddress(char *dbg, u8 *macAddr)
{
    printf("%s %02X-%02X-%02X-%02X-%02X-%02X\n", dbg,
    macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
}


void UIM_DisplayCrm()
{
    u8             i, j;
    sScb          *scb = NULL;
    sLinkLayer    *linkl = (sLinkLayer *)HPGPCTRL_GetLayer(HP_LAYER_TYPE_LINK);
    sCrm          *crm = LINKL_GetCrm(linkl);


    scb = CRM_GetNextScb(crm, scb);
    i = 0;
    while(scb)
    {
        FM_Printf(FM_ERROR, "== SCB %d == \n", i);
        UIM_PrintMACAddress("\tMAC Addr:", scb->macAddr);
        FM_Printf(FM_ERROR, "\ttei: %d \n", scb->tei);
        FM_Printf(FM_ERROR, "\tCCo cap: %d\n",  scb->staCap.fields.ccoCap);
        FM_Printf(FM_ERROR, "\tDisc STA list (%d):\n", scb->numDiscSta);

        if(scb->numDiscSta)
        {
            for(j = 0; j< DISC_STA_LIST_MAX; j++)
            {
                if(scb->discStaInfo[j].valid)
                {
                    UIM_PrintMACAddress("\t", scb->discStaInfo[j].macAddr);
                    FM_Printf(FM_ERROR, "\ttei: %d. \n", scb->discStaInfo[j].tei);
                    FM_Printf(FM_ERROR, "\tSTA CAP: 0x%.2x. \n",
                                    scb->discStaInfo[j].staCap.byte);
                    FM_Printf(FM_ERROR, "\tSTA STATUS: 0x%.2x. \n",
                                    scb->discStaInfo[j].staStatus.byte);
                }
            }

        }
        FM_Printf(FM_ERROR, "\t#Disc Net: %d.\n", scb->numDiscNet);

        scb = CRM_GetNextScb(crm, scb);
        i++;
    }
}







#if defined(WIN32) || defined(_WIN32)
DWORD WINAPI UIM_Proc( LPVOID lpParam )
#else
void* UIM_Proc( void* lpParam )
#endif
{
/*
    sUim *uim = (sUim *)lpParam;
*/
    sNmm *nmm = (sNmm *)Host_GetNmm();
    int  len = 0;
    u8   passwd[PASSWORD_LEN+1];
    int  sl;
    int  opt;
    char line[128];

    while(1)
    {
        UIM_Menu();
        printf(" Please select an operation: ");
        fflush(stdout);

//    printf(" ==== YD ===== (0)\n ");
        if(fgets(line, sizeof line, stdin) == NULL)
            continue;
//    printf(" ==== YD ===== (1)\n ");
        sscanf(line, "%d", &opt);
//    printf(" ==== YD ===== (2)\n ");
    printf(" your selection: %d\n ", opt);

        switch(opt)
        {
           case 1:
           {
                printf(" Please enter the Device Password (8-64 characters):\n");
                fflush(stdout);
                if(fgets(line, sizeof line, stdin) == NULL)
                    continue;

                if (sscanf(line, "%s", passwd))
                {
                    passwd[PASSWORD_LEN] = '\0';
                    len = strlen((char *)passwd);

                    if(len < 8 || len > 64)
                    {
                        printf("ERROR: Password should be 8 to 64 characters long.\n");
                        continue;
                    }
                    NMM_SetDevicePassword(nmm, passwd, len); 
                }
                break;
           }
           case 2:
           {
               printf(" Please enter the Network Password (8-64 characters):\n");
               if(fgets(line, sizeof line, stdin) == NULL)
                    continue;
               
               if (sscanf(line, "%s", passwd))
               {
                   passwd[PASSWORD_LEN] = '\0';
                   len = strlen((char *)passwd);

                   if(len < 8 || len > PASSWORD_LEN)
                   {
                       printf("ERROR: Password should be 8 to 64 characters long.\n");
                       continue;
                   }

                   printf(" Please enter Security Level (0: HS. 1: SC):\n");

                   if(fgets(line, sizeof line, stdin) == NULL)
                       continue;

                   if (sscanf(line, "%d", (int *)&sl))
                   {
                       if( (sl != 0) && (sl != 1))
                       {
                           printf("ERROR: Security Level should be either 0 or 1. \n");
                           continue;
                       }

                       if (NMM_SetDefaultNetId(nmm, passwd, len, sl) 
                           == STATUS_FAILURE)
                       {
                           printf("Failed: the device is operating in a network.\n");
                       }
                   }
               }
               break;
           }
           case 3:
           {
               printf(" Security Mode: \n");
               printf("   0: Secure.\n");
               printf("   1: Simple-Connect.\n");
               printf("   2: SC-Add. \n");
               printf("   3: SC-Join.\n");
               printf(" Please enter the Security Mode: \n");
               if (scanf("%d", (int *)&opt))
               {
                   if(opt > 3)
                   {
                       printf("ERROR: Security Mode should be 0-3;\n");
                       return NULL;
                   }
                   NMM_SetSecurityMode(nmm, opt);
               }
               break;
           }
           case 4:
           {
               NMM_RestartSta(nmm);
               break;
           }
           case 5:
           {
               printf(" Start/join Network:\n");
               printf("   0: Start the netowrk as a CCo.\n");
               printf("   1: Join the network as a STA.\n");
               printf(" Please select one: \n");

               if (scanf("%d", (int *)&opt))
               {
                   if( (opt != 0) && (opt != 1))
                   {
                       printf("ERROR: the entered number should be either 0 or 1. \n");
                   }
                   NMM_SetNetworks(nmm, opt);
               }
               break;
           }

           case 6:
           {
               NMM_SetNetworks(nmm, NETWORK_LEAVE);
               break;
           }
           case 7:
           {
               NMM_NetExit(nmm);
               //UIM_SendSetPPKeysReq(nmm);
               break;
           }
           case 8:
           {
               u8   macAddr[MAC_ADDR_LEN];

               printf(" Please enter the new CCo MAC address: ");
               if(fgets(line, sizeof line, stdin))
               {
//                   if(sscanf(line, "%02x:%02x:%02x:%02x:%02x:%02x",
                   if(sscanf(line, "%c:%c:%cx:%c:%c:%c",
                               &macAddr[0],
                               &macAddr[1],
                               &macAddr[2],
                               &macAddr[3],
                               &macAddr[4],
                               &macAddr[5]) == 6)
                   {
                       NMM_AppointCco(nmm, macAddr);
                   }
                   else
                   {
                       printf(" failed in entering the new CCo MAC address\n");
                   }
               }
               break;
           }
           case 9:
           {
               printf(" Please enter the DPW for the STA authenticated:\n");
               if (scanf("%s", passwd))
               {
                   passwd[PASSWORD_LEN] = '\0';
                   len = strlen((char *)passwd);

                   if(len < 8 || len > 64)
                   {
                       printf("ERROR: Password should be 8 to 64 characters long.\n");
                       return NULL;
                   }
                   NMM_AuthSta(nmm, passwd, len, NULL, NULL, 0); 
               }
               break;
           }
           case 10:
           {
               u8   nmk[ENC_KEY_LEN+1];
               printf(" Please enter the DPW for the STA authenticated:\n");
               if (scanf("%s", (char *)passwd))
               {
                   passwd[PASSWORD_LEN] = '\0';
                   len = strlen((char *)passwd);

                   if(len < 8 || len > 64)
                   {
                       printf("ERROR: Password should be 8 to 64 characters long.\n");
                       continue;
                   }
                   printf(" Please enter the NMK for authenticatiion");
                   if (scanf("%s", (char *)nmk))
                   {
                       nmk[ENC_KEY_LEN] = '\0';
                       len = strlen((char *)nmk);
                       if(len > ENC_KEY_LEN)
                       {
                          printf("ERROR: NMK is too long.\n");
                          continue;
                       }
                       printf(" Please enter Security Level (0: HS. 1: SC):\n");
                       if (scanf("%d", (int *)&sl))
                       {
                           if( (sl != 0) && (sl != 1))
                           {
                               printf("ERROR: Security Level should be either 0 or 1. \n");
                           }
                           NMM_AuthSta(nmm, passwd, len, nmk, NULL, sl); 
                       }
                   }
               }
               break;
           }
           case 21:
           {
               len = NMM_GetDevicePassword(nmm, passwd);
               if (len <= PASSWORD_LEN)
               {
                   passwd[len] = '\0';
                   printf(" Device Password: %s\n", passwd);
               }
               break;
           }
           case 31:
           {
               UIM_DisplayCrm();
               break;
           }
           default:
           {
           }
        }
    } // end of while
}

eStatus UIM_Init(sUim *uim)
{
    eStatus status = STATUS_SUCCESS;

#if defined(WIN32) || defined(_WIN32)
    uim->uimThread = CreateThread(NULL, 0, UIM_Proc, uim, 0, NULL);
    if(uim->uimThread == NULL)
#else
    if( pthread_create(&uim->uimThread, NULL, UIM_Proc, uim ))
#endif
    {
        status = STATUS_FAILURE;
    }

    FM_Printf(FM_MINFO, "UIM: Initialized\n");
    return status;
}
