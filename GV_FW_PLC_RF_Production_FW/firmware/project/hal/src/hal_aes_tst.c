
#include <REG51.H>                /* special function register declarations   */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "hal.h"
#include "hal_hpgp.h"

extern int strcmp_nocase(const char *str1, const char *str2);
void base64_output(u8 *pData, u8 len);

u8  aes_init_val[16] = {
    0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x00,0x00,0x00,0x00
};

u8 aes_key[16] = {
    0xC5,0x80,0xC2,0xF6,0x32,0xDD,0xD9,0x69,0x68,0x6E,0x73,0x85,0x8B,0xA8,0x5F,0xFD,
};

u8  XDATA aes_plaindata[128] = 
{
    0x85,0x00,0x00,0x31,0x32,0x33,0x34,0x35,0x00,0x46,0x47,0x48,0x49,0x50,0x88,0x7B,                // 1 * 16
    0x00,0x00,0x00,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,                // 2 * 16
    0x10,0x11,0x12,0x13,0xDE,0xD0,0x34,0xAC,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 3 * 16
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 4 * 16
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 5 * 16
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 6 * 16
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 7 * 16
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,                // 8 * 16
};

u8  XDATA aes_output_buff[128];

void Write4BytesReg(u8  *ptr, u8 *pByte)
{
    u8    v1, v2, v3, v4;
	u8 xdata *p;

    v1 = *pByte++;
    v2 = *pByte++;
    v3 = *pByte++;
    v4 = *pByte++;

	p = ptr;
    *p = v1;
    *(p+1) = v2;
    *(p+2) = v3;
    *(p+3) = v4;
}

void aes_access_req(u8 enc_dec_flag)
{
    uAesCpuCmdStatReg  aesStatus;

    aesStatus.reg = 0;
    if (enc_dec_flag == 1)
        aesStatus.s.enc = 1;
    aesStatus.s.reqAes = 1;

    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesStatus.reg);

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    } while (aesStatus.s.gnt == 0);

    aesStatus.reg = 0;
    if (enc_dec_flag == 1)
        aesStatus.s.enc = 1;
    aesStatus.s.reqAes = 0;

    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesStatus.reg);
}

void reorder(u8 *pData)
{
    u8    i, j;
    u8    chval, chval_1, chval_2, new_val;
    u8    mask_1, mask_2;
    u8    shift;

    for (i=0; i < 16; i++)
    {
        chval = *(pData+i);
        printf("0x%02bX ", chval);
    }
    printf("\n");

    for (i=0; i < 8; i++)
    {
        chval = *(pData+i);
        *(pData+i) = *(pData + 15 - i);
        *(pData + 15 - i) = chval;
    }

    for (i=0; i < 16; i++)
    {
        chval = *(pData+i);
        new_val = 0;
        mask_1 = 0x80;
        mask_2 = 0x01;

        for (j=0, shift = 7; j < 4; j++, shift -= 2, mask_1 >>= 1, mask_2 <<= 1)
        {
            chval_1 = chval & mask_1;
            chval_2 = chval & mask_2;

            chval_1 >>= shift;
            chval_2 <<= shift;
            new_val |= chval_1 | chval_2;
        }
        *(pData+i) = new_val;

//        printf("old:0x%02bx new:0x%02bx\n", chval, new_val);
    }

    for (i=0; i < 16; i++)
    {
        chval = *(pData+i);
        printf("0x%02bX ", chval);
    }
    printf("\n\n");
}

void aes_iv_write(u8  *init_val)
{
    u32   *pU32;
    uAesCpuCmdStatReg  aesStatus;
//	u32    val1, val2;

    reorder(init_val);

//    do {
    aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
	printf("iv write status:%08lX\n", aesStatus.reg);
//    } while (aesStatus.s.ivRdy == 0);


//    Write4BytesReg(PLC_AESINITVECT0_REG, init_val);
//    Write4BytesReg(PLC_AESINITVECT1_REG, init_val + 4);    
//    Write4BytesReg(PLC_AESINITVECT2_REG, init_val + 8);    
//    Write4BytesReg(PLC_AESINITVECT3_REG, init_val + 12);

    pU32 = (u32 *)init_val;

#if 0
    WriteU32Reg(PLC_AESINITVECT0_REG, (*pU32++));    
    WriteU32Reg(PLC_AESINITVECT1_REG, (*pU32++));    
    WriteU32Reg(PLC_AESINITVECT2_REG, (*pU32++));    
    WriteU32Reg(PLC_AESINITVECT3_REG, (*pU32++));    
#else
    WriteU32Reg(PLC_AESINITVECT0_REG, ctorl(*pU32++));    
    WriteU32Reg(PLC_AESINITVECT1_REG, ctorl(*pU32++));    
    WriteU32Reg(PLC_AESINITVECT2_REG, ctorl(*pU32++));    
    WriteU32Reg(PLC_AESINITVECT3_REG, ctorl(*pU32++));    
#endif
}

void aes_key_write(u8 *pKey)
{
    union {
        u8 chval[4];
        u32 u32val;
    } val;
    u8      i;
    uAesCpuCmdStatReg  aesStatus;

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		printf("key write status:%08lX\n", aesStatus.reg);
    } while (aesStatus.s.keyRdy == 0);

    reorder(pKey);

    val.u32val = 0;

    for (i=0; i < 16; i++)
    {
        val.chval[0] = *pKey++;
        WriteU32Reg(PLC_AESENCDECDATA_REG, val.u32val);    
    }

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		printf("key write done status:%08lX\n", aesStatus.reg);
    } while (aesStatus.s.keyRdy == 1);
}

void aes_data_read_one_block(u8 *pData)
{
    union {
        u8 chval[4];
        u32 u32val;
    } val;
    u8      i;
    u8      *pData_2;
    uAesCpuCmdStatReg  aesStatus;

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		printf("data read status:%08lX\n", aesStatus.reg);
    } while (aesStatus.s.dataValid == 0);

    pData_2 = pData;

    for (i=0; i < 16; i++)
    {
        val.u32val = ReadU32Reg(PLC_AESENCDECDATA_REG);
//		printf("rd val:%08lX  [3]:%02bx [0]:%02bx\n", val.u32val, val.chval[3], val.chval[0]);		    
        *pData++ = val.chval[0];
    }

    reorder(pData_2);

    aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
    aesStatus.s.dataValidClr = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesStatus.reg);
    aesStatus.s.dataValidClr = 0;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesStatus.reg);
}

void aes_data_write_one_block(u8 *pData, u8 flag_lastblock)
{
    union {
        u8 chval[4];
        u32 u32val;
    } val;
    u8      i;
    uAesCpuCmdStatReg  aesStatus;

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		printf("data write status:%08lX\n", aesStatus.reg);
    } while (aesStatus.s.dataRdy == 0);

    if (flag_lastblock == 1)
        aesStatus.s.lastBlk = 1;
    WriteU32Reg(PLC_AESCPUCMDSTAT_REG, aesStatus.reg);

    reorder(pData);

    val.u32val = 0;
    for (i=0; i < 16; i++)
    {
        val.chval[0] = *pData++;
        WriteU32Reg(PLC_AESENCDECDATA_REG, val.u32val);    
    }

    do {
        aesStatus.reg = ReadU32Reg(PLC_AESCPUCMDSTAT_REG);
		printf("data write done status:%08lX\n", aesStatus.reg);
    } while (aesStatus.s.dataRdy == 1);
}

void aes_data_write(u8 *pData, u8 *pOutput, u8 len)
{
    u8  i, blkn;
    u8  flag_lastblock;

    blkn = (len + 15) / 16;

    for (i=0; i < blkn - 1; i++, pData += 16, pOutput += 16)
    {
        aes_data_write_one_block(pData, 0);
        aes_data_read_one_block(pOutput);
    }
//	printf("i=%bu blkn=%bu\n", i, blkn);

    aes_data_write_one_block(pData, 1);
//	printf("write done\n");

    aes_data_read_one_block(pOutput);
//	printf("read done\n");
}

void AES_CmdHelp()
{
    printf("AES Test Commands:\n"
           "a encrypt  - encrypt fixed data\n"
           "a decrypt  - decrypt fixed data\n"
           "a encdata  - encrypt user data\n"
           "a decdata  - decrypt user data\n"
           "\n");
    return;
}

void aes_encrypt_test()
{
    u8      i, j;

    aes_iv_write(aes_init_val);

    aes_access_req(1);
    aes_key_write(aes_key);

    aes_data_write(aes_plaindata, aes_output_buff, sizeof(aes_plaindata));

    printf("\n");
    for (i=0; i<8; i++)
    {
        for (j=0; j < 16; j++)
        {
            printf("%02bX  ", aes_output_buff[i * 16 + j]);
        }
        printf("\n");
    }
	base64_output(aes_output_buff, 128);
}

void aes_decrypt_test()
{
}

void aes_process(char* CmdBuf)
{
    u8  cmd[10];

    CmdBuf++;

	if (sscanf(CmdBuf, "%s", &cmd) < 1 || strcmp(cmd, "?") == 0)
	{
		AES_CmdHelp();
        return;
	}
	if(strcmp_nocase(cmd, "encrypt") == 0)
	{
		aes_encrypt_test();		
	}
	else if (strcmp_nocase(cmd, "decrypt") == 0)
	{
		aes_decrypt_test();
	}
    else
    {
        AES_CmdHelp();
    }  
}	  


void base64_output(u8 *pData, u8 len)
{
    u8  loop;
    union {
        u8 chval[4];
        u32 u32val;
    } val;
    u8      i, j, chval64, bytebase64;

    loop = len / 3;

    for (i=0; i < loop; i++)
    {
        val.u32val = 0;

        val.chval[0] = *pData++;
        val.chval[1] = *pData++;
        val.chval[2] = *pData++;

        for (j=0; j < 4; j++)
        {
            bytebase64 = (val.chval[0] >> 2) & 0x3F;
        
            if (bytebase64 < 26)
            {
                chval64 = 'A' + bytebase64;
            }
            else if (bytebase64 < 52)
            {
                bytebase64 -= 26;
                chval64 = 'a' + bytebase64;
            }
			else if (bytebase64 < 62)
			{
                bytebase64 -= 52;
                chval64 = '0' + bytebase64;
			} 
			else if (bytebase64 == 62)
			{
                chval64 = '+';
			}
			else
                chval64 = '/';

//            printf("%08lX  byte=0x%02bx  - %c[0x%02bx]\n", val.u32val, bytebase64, chval64, chval64);
            printf("%c", chval64);
            val.u32val <<= 6;
        }
//        printf("\n");
    }
}


