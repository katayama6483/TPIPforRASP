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

typedef struct STR_JTP_INFO {
  u_short v_type;	// Video type 0=QVGA 1=VGA 2=SVGA 3=XGA 4=SXGA 5=UXGA 6=FHD(Full-HD)
  u_short q_fact;	// Q-factor
  u_char  cam_no;	// Camera No.
  u_char  v_fps;    // fps 0:30fps 1:15fps 2:10fps 3:5fps (未使用)
} t_jtp_param;

extern int JTP_open(long IPadr, int vType, int kbps, int cam_no);
extern int JTP_close(void);
extern int JTP_set_param(int vType, int kbps, int cam_no);
extern int JTP_get_param(t_jtp_param* param);

#endif  // #ifndef __JTP_COM_SV_H__
