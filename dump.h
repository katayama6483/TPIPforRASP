/** Header file
 * @file  dump.h
 * @brief 通信data dump
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2008-09-09
 * @version $Id: v 1.00 2008/09/09 00:00:00 katayama $
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */

#ifndef __DUMP_H__
#define __DUMP_H__

extern void dump_enable(int on, int mode);
extern void dump_init(void);
extern void dump_end(void);
extern void dump_out(unsigned short* p, int sz);
extern void dump_out_R(unsigned char *p, int sz);
extern void dump_event(int sz, char* buf);
extern int  Get_dumpON(void);
extern int  Get_dump_mode(void);

#endif  // #ifndef __DUMP_H__
