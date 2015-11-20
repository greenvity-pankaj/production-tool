/**
 * @file mac_utils.h
 *
 * MAC Utillity functions 
 *
 * $Id: ihal_tst.h,v 1.1 2013/12/18 17:06:22 yiming Exp $
 *
 * Copyright (c) 2011, Greenvity Communication All rights reserved.
 *
 */
#ifndef _IHAL_TST_H_
#define _IHAL_TST_H_

extern void ihal_tst_slave_tx_dma(u16 max_data_size,
                                  u16 inc_bytes,
                                  u16 num_pkts_in);
extern void ihal_tst_rx(u16 data_size, u16 num_pkts_in);


#endif /* _IHAL_TST_H_ */
