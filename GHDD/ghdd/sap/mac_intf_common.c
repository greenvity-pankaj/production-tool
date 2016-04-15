/*******************************************************************************
 *
 * File:       hybrii_mac_intf.c
 * Contact:    pankaj_razdan@greenvity.com
 *
 * Description: Host Interface to Zigbee MAC (802.15.4)
 *
 * Copyright (c) 2011 by Greenvity Communication.
 *
 ******************************************************************************/
#include <linux/module.h>
#include "papdef.h"
#include "mac_intf_common.h"

/* Buffer, which holds frame received from the peer entity Host/Hybrii
  * Note :- This buffer can be used for ethernet/HPGP frames 
  */
u8 ufrm[MAX_MAC_IF_FRAME_LENGTH];


/* Utility APIs */

/* This API extracts all primitive TLVs from frame received, and stores all fields in the params_t. 
 * @Parameters
 * p - parameter structure to store TLV value fields of the primitive
 * p_frm - pointer to the Hybrii frame
 * frm_len - length of the Hybrii frame
 * le - Boolean for checking Endiannes
 * Note :- All parameters in the Hybrii frame are in little endian format
 */
bool zmac_hostIf_get_tlvs (params_t *p, u8 *p_frm, u8 frm_len, bool le)
{
	u8 idx = 0;

	while (idx + 2 < frm_len)
	{
		/* Skip type and length fields of TLV */
		idx += 2;
		/* Identify TLV type */
		switch (p_frm[idx-2])
		{
			/* Single byte Data Type */
			case DEST_ADDR_MODE:
        	case SRC_ADDR_MODE:	
            case TX_OPTIONS:	
            case LOGICAL_CHANNEL:	
            case CHANNEL_PAGE:				
            case MSDU_HANDLE:			
            case CAPABILITY_INFO:	
            case COORD_ADDR_MODE:	
			case PRIVM_STATUS:	
            case DISSOCIATE_REASON:	
            case TRANSMIT_INDIRECT:
            case PIB_ATTRIBUTE:		
            case PIB_ATTRIBUTE_INDEX:	
            case SCAN_TYPE:	
            case BEACON_ORDER:
            case SUPER_FRAME_ORDER:	
            case PAN_COORDINATOR:	
            case BLE_ID:		
            case COORD_REALIGNMENT:	
            case MPDU_LINK_QUALITY:		
            case DSN_ID:		
            case BSN_ID:	
            case PAN_ADDR_SPEC:	
            case MSDU_LENGTH:	
            case RESULT_LIST_SIZE:		
            case LOSS_REASON:	
            case SET_DEFAULT_PIB:
            case ASSOCIATED_MEMBER:	
            case PEND_ADDRESS:	
			case DEFER_PERMIT:
			case SCAN_DURATION: 					
			{
				p->byte_data[p->idx_b++] = p_frm[idx];
				break;
			}				
			
			/* Two bytes Data Type */			
            case DEST_PAN_ID:				
            case SRC_PAN_ID:
			case COOR_PAN_ID:	
			case ASSOC_SHORT_ADDR:	
			case PIB_ATTRIBUTE_LENGTH:				
			{
				if (le)
				{
					memcpy((u8 *)&p->short_data[p->idx_s++], 
								&p_frm[idx], sizeof (u16));
				}
				else
				{
					rev_memcpy((u8 *)&p->short_data[p->idx_s++], 
							&p_frm[idx], sizeof (u16));
				}
				break;
			}	

			/* Four bytes Data Type */			
            case SCAN_CHANNELS:					
            case TX_ONTIME:						
            case RX_ONDURATION:					
            case START_TIME:	
			case TIME_STAMP:	
			case UNSCANNED_CHANNELS:					
			{
				if (le)
				{				
					memcpy((u8 *)&p->word_data[p->idx_w++], 
								&p_frm[idx], sizeof (u32));
				}
				else
				{
					rev_memcpy((u8 *)&p->word_data[p->idx_w++], 
							&p_frm[idx], sizeof (u32));
				}
				break;
			}		

			
			/* Address type Data Type */
            case DEST_ADDR:					
            case SRC_ADDR:	
			case COOR_ADDR: 	
			case DEVICE_ADDR:
			case ORPHAN_ADDRESS:	
			{
				if (le)
				{
					memcpy((u8 *)&p->address_data[p->idx_a++], 
							&p_frm[idx], sizeof (addr_t));
				}
				else
				{					
					rev_memcpy((u8 *)&p->address_data[p->idx_a++], 
							&p_frm[idx], sizeof (addr_t));
				}
				break;
			}

			/* Security type data type */
            case BEACON_SECURITY:	
			case SECURITY_ID:	
			case COORD_REALIGNMENT_SECURITY:				
			{
				if (le)
				{
					memcpy((u8 *)&p->security_data[p->idx_sec++], 
								(u8 *)&p_frm[idx], sizeof (sec_t));
				}
				else
				{
					rev_memcpy((u8 *)&p->security_data[p->idx_sec++], 
							(u8 *)&p_frm[idx], sizeof (sec_t));
				}
				break;
			}		
			
            case PAN_DESCRIPTOR:
			{
				if (le)
				{
					zmac_hostIf_copy_pan_descriptor (
						(pand_t *)&p->pand, 
						(pand_t *)&p_frm[idx]);
				}
				else
				{					
					zmac_hostIf_convert_pand_endianess (
						(pand_t *)&p->pand, 
						(pand_t *)&p_frm[idx]);
				}
				p->idx_p++; 
				break;
			}		
		
			
            case PIB_ATTRIBUTE_VALUE:	
			/* Address List */
            case ADDR_LIST:
			{	
				p->len1 =  p_frm[idx-1];
				
				if (le)
				{
					p->ptr1 = &p_frm[idx];
				}
				else
				{
					p->ptr1 = &p_frm[idx];
				}
				break;
            }	

			/* SDU filed */
            case MSDU_ID:
			{
				if (le)
				{
					p->ptr2 = &p_frm[idx];
				}
				else
				{
					p->ptr2 = &p_frm[idx];
				}
				break;
            }
				
			default:
				return FALSE;
		}
		idx += p_frm[idx-1];
	};

	return TRUE;
}

#if 1
void zmac_hostIf_convert_pand_endianess (pand_t *p_dst, pand_t *p_src)
{
	p_dst->coor_addr_mode = p_src->coor_addr_mode;
	rev_memcpy((u8 *)&p_dst->coor_pan_id,
				(u8 *)&p_src->coor_pan_id, sizeof (u16));
	rev_memcpy((u8 *)&p_dst->coor_addr,
				(u8 *)&p_src->coor_addr, sizeof (addr_t));
	rev_memcpy((u8 *)&p_dst->chan, 
				(u8 *)&p_dst->chan, sizeof (chan_t));
	rev_memcpy((u8 *)&p_dst->super_frm_spec, 
				(u8 *)&p_src->super_frm_spec, sizeof (u16));
	p_dst->gts_permit = p_src->gts_permit;
	p_dst->link_quality = p_src->link_quality;
	rev_memcpy((u8 *)&p_dst->timestamp, 
				(u8 *)&p_dst->timestamp, sizeof (u32)); 
	p_dst->sec_failure = p_src->sec_failure;
	rev_memcpy((u8 *)&p_dst->sec, 
				(u8 *)&p_src->sec, sizeof (sec_t));
}

void zmac_hostIf_copy_pan_descriptor (pand_t *p_dst, pand_t *p_src)
{
	p_dst->coor_addr_mode = p_src->coor_addr_mode;
	memcpy((u8 *)&p_dst->coor_pan_id,
				(u8 *)&p_src->coor_pan_id, sizeof (u16));
	memcpy((u8 *)&p_dst->coor_addr,
				(u8 *)&p_src->coor_addr, sizeof (addr_t));
	memcpy((u8 *)&p_dst->chan, 
				(u8 *)&p_dst->chan, sizeof (chan_t));
	memcpy((u8 *)&p_dst->super_frm_spec, 
				(u8 *)&p_src->super_frm_spec, sizeof (u16));
	p_dst->gts_permit = p_src->gts_permit;
	p_dst->link_quality = p_src->link_quality;
	memcpy((u8 *)&p_dst->timestamp, 
				(u8 *)&p_dst->timestamp, sizeof (u32)); 
	p_dst->sec_failure = p_src->sec_failure;
	memcpy((u8 *)&p_dst->sec, 
				(u8 *)&p_src->sec, sizeof (sec_t));
}

#endif

/* API returs TRUE if the current processor is Little Endian */
/*
bool is_little_endian (void)
{
	u16 var = 0xcafe;

	if ((((u8 *)&var)[0] == 0xfe) && (((u8 *)&var)[1] == 0xca))
	{
		return TRUE;
	}
	
	return FALSE;
}
*/

/* API to reset frame buffer */
void zmac_hostIf_reset_frm (void)
{
	memset (ufrm, 0x00, MAX_MAC_IF_FRAME_LENGTH);
}

/* API to set Hybrii frame header 
 *  @Parameters 
 *  frm_type - Hybrii frame type
 *  protocol - Protocol for which current frame belongs
 */
void zmac_hostIf_form_hdr (u8 frm_type, u8 protocol)
{
	gv_cmd_hdr_t *cmd = (gv_cmd_hdr_t *)ufrm;

	cmd->fc.proto =  (protocol & 0x03);
	cmd->fc.frm = (frm_type & 0x03);
	cmd->fc.rsvd = 0;
	/* Length can  be added later */
	cmd->len = 0;
	cmd->reserved = 0;
}

/* API to set TLV field in the frame 
*  @Parameters 
*  P_idx - pointer to the index field
*  Type - TLV type field
*  length - TLV length field
*  p_value - void pointer to TLV value field
*/

void zmac_hostIf_set_tlv (u8 *p_idx, u8 type, u8 length, void *p_value)
{
	/* Check if we can accomodate new field */
	if (*p_idx+length > MAX_MAC_IF_FRAME_LENGTH)
	{
		// This should not happen
		while (0); // TBD assert
	}
	
	ufrm[(*p_idx)++] = type;
	ufrm[(*p_idx)++] = length;

	/* All Values should be sent in little endian format, other than variable length byte stream  */
	if (is_little_endian () ||  
		(type == ADDR_LIST) ||
		(type == MSDU_ID))
	{
		/* PAN Descriptor has multiple fields hence need to convert endianess separately */
		if (type == PAN_DESCRIPTOR)
		{
			zmac_hostIf_copy_pan_descriptor (
				(pand_t *)&ufrm[*p_idx], 
				(pand_t *)p_value);
		}
		else
		{
			memcpy (&ufrm[*p_idx], (u8 *)p_value, length);
		}		
	}
	else
	{
		/* PAN Descriptor has multiple fields hence need to convert endianess separately */
		if (type == PAN_DESCRIPTOR)
		{
			zmac_hostIf_convert_pand_endianess (
				(pand_t *)&ufrm[*p_idx], 
				(pand_t *)p_value);
		}
		else
		{
			rev_memcpy (&ufrm[*p_idx], (u8 *)p_value, length);
		}
	}
		
	*p_idx += length;
}

/* API to Calculate CRC and also sets it in the frame 
*  @Parameters 
* p_frm - pointer to the Hybrii frame
* frm_len - length of the Hybrii frame
*/

void zmac_hostIf_set_crc (u8 *p_frm, u16 *p_len)
{
	u8 i;
	u16 crc = 0;
	u16 tb = 0;
	bool is_even;

	/* Frame length can be even or odd */
	is_even = (*p_len % 2)? FALSE : TRUE;
		
	for (i = 0; i < (is_even ? *p_len : *p_len - 1); i = i + MAC_IF_CRC_LENGTH)
	{
	//	crc = *((u16 *)&p_frm[i]) ^ crc;
		memcpy ((u8 *)&tb, &p_frm[i], MAC_IF_CRC_LENGTH);
		crc = tb ^ crc;
	}
	
	/* even case of odd length, get crc of last byte */
	if (!is_even)
	{
		tb = p_frm[*p_len-1];
		crc = tb ^ crc;
	}

//	*((u16 *)&p_frm[*p_len]) = crc;
	memcpy((u8 *)&p_frm[*p_len], (u8 *)&crc, sizeof (crc));
	*p_len += MAC_IF_CRC_LENGTH;
}

/* API to Get CRC of whole frame 
*  @Parameters 
* p_frm - pointer to the Hybrii frame
* frm_len - length of the Hybrii frame
*/
u16 zmac_hostIf_get_crc (u8 *p_frm, u16 frm_len)
{
	u8 i;
	u16 crc = 0;
	u16 tb = 0;
	bool is_even;

	/* Frame length can be even or odd */
	is_even = (frm_len % 2)? FALSE : TRUE;
		
	for (i = 0; i < (is_even ? frm_len : frm_len - 1); i = i + MAC_IF_CRC_LENGTH)
	{
	//	crc = *((u16 *)&p_frm[i]) ^ crc;
		memcpy ((u8 *)&tb, &p_frm[i], MAC_IF_CRC_LENGTH);
		crc = tb ^ crc;
	}
	/* even case of odd length, get crc of last byte */
	if (!is_even)
	{
		tb = p_frm[frm_len-1];
		crc = tb ^ crc;
	}

	return crc;
}

/* API verifies calculates CRC and verifies it with frame CRC 
*  @Parameters 
* p_frm - pointer to the Hybrii frame
* frm_len - length of the Hybrii frame
*/

bool zmac_hostIf_verify_crc (u8 *p_frm, u16 frm_len)
{
	u16 crc = 0, frm_crc;

	/* Get CRC of the received frame */
	crc = zmac_hostIf_get_crc (p_frm, frm_len - MAC_IF_CRC_LENGTH);

#if 0
	if (crc != ((u16 *)&p_frm[len - MAC_IF_CRC_LENGTH])[0])
#else
	frm_crc = 0;
	memcpy ((u8 *)&frm_crc, &p_frm[frm_len - MAC_IF_CRC_LENGTH], sizeof (u16));

	/* Verify received crc and calculate crc */
	if (crc != frm_crc)
#endif
	{
		/* CRC matching failed */
		return FALSE;
	}

	/* CRC success */
	return TRUE;
}


/* API to set uplink direction and primitive ID 
*  @Parameters 
* p_idx - pointer to the index in the frame buffer
* prim_id - primitive ID
*/

void zmac_hostIf_set_primitive_id_uplink ( u8 *p_idx, u8 prim_id)
{
	*p_idx = HYBRII_FRM_HEADER_LENGTH;
	ufrm[(*p_idx)++] = DIR_UPLINK;
	ufrm[(*p_idx)++] = prim_id;
}

/* API to set downlink direction and primitive ID 
*  @Parameters 
* p_idx - pointer to the index in the frame buffer
* prim_id - primitive ID
*/

void zmac_hostIf_set_primitive_id_downlink ( u8 *p_idx, u8 prim_id)
{
	*p_idx = HYBRII_FRM_HEADER_LENGTH;
	ufrm[(*p_idx)++] = DIR_DOWNLINK;
	ufrm[(*p_idx)++] = prim_id;
}

/* API copies count number of bytes data from src to dst  
*  @Parameters 
* p_frm - pointer to the Hybrii frame
* frm_len - length of the Hybrii frame
*/
void *rev_memcpy( u8 *p_dst, const u8 *p_src, unsigned int count )
{ 
    u8 i; 
 
    for (i = 0; i < count; ++i) 
	{
        p_dst[count-1-i] = p_src[i];
    }
	return 0;
} 



