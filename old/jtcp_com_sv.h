/** 
 * @file  jtcp_com_sv.h
 * @brief JTCP communication server header file
 *
 * @author Katayama
 * @date 2018-10-17
 * @version 1.00  2018/10/17 katayama
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#ifndef __JTCP_COM_SV_H__
#define __JTCP_COM_SV_H__

extern int JTCP_open(void);
extern int JTCP_close(void);
extern int JTCP_set_mode(int mode);

#endif  // #ifndef __JTCP_COM_SV_H__
