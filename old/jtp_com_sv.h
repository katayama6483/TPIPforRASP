/** 
 * @file  jtp_com_sv.h
 * @brief JTP communication server header file
 *
 * @author Katayama
 * @date 2018-10-19
 * @version 1.00  2018/10/17 katayama
 * @version 1.10  2018/10/22 katayama
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#ifndef __JTP_COM_SV_H__
#define __JTP_COM_SV_H__

extern int JTP_open(long IPadr, int vType, int kbps, int cam_no);
extern int JTP_close(void);
extern int JTP_set_param(int vType, int kbps, int cam_no);

#endif  // #ifndef __JTP_COM_SV_H__
