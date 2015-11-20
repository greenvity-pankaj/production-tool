/**
 * @file ui_utils.h
 *
 * User Interface Utillity functions 
 *
 * $Id: ui_utils.h,v 1.2 2014/05/28 10:58:58 prashant Exp $
 *
 * Copyright (c) 2012, Greenvity Communication All rights reserved.
 *
 */
#ifndef _UI_UTILS_H_
#define _UI_UTILS_H_

extern bool abort(void);
extern void ui_utils_cmd_get(u8 *cmd_buf_p, u8 max_cmd_buff_size);
extern u8 ui_utils_cmd_get_poll(u8 *cmd_buf_p, u8 max_cmd_buff_size);
extern void ui_utils_cmd_spi(char* cmd_buf_p);
extern void ui_utils_reg_read(uint8_t *cmd_buf_p);
extern void ui_utils_reg_write(uint8_t *cmd_buf_p); 
#endif /* _UI_UTILS_H_ */
