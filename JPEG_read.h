/** 
 * @file  JPEG_read.h
 * @brief JPEG read program header file
 *
 * @author  Katayama
 * @date    2018-10-12
 * @version 1.00 2018/10/12
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#ifndef __JPEG_READ_H__
#define __JPEG_READ_H__

// Display resolution
#define QVGA (0)
#define VGA  (1)
#define SVGA (2)
#define XGA  (3)
#define HD   (4)
#define SXGA (5)
#define UXGA (6)
#define FHD  (7)
#define DISP_RESOL_MAX  8

#include <sys/types.h>
#include <time.h>
typedef struct STR_JPEG_INFO {
	int index;
	u_char frm_no;
	u_char cam_no;
	u_char vga;
	u_long itv_tm;
	struct timespec st_ts;
	struct timespec ed_ts;
} t_jpeg_info;


extern int JPEG_open(char* dev_name, int _bps, int _vga, int _cam_no);
extern int JPEG_close(void);

extern int JPEG_read(unsigned char** buf_p, int buf_size, t_jpeg_info* info);
extern int JPEG_write_file(char* f_name, unsigned char* buf_p, int image_size);
extern int JPEG_write_rq(char* f_name);

extern int JPEG_status(void);
extern int JPEG_set_bps(int _bps);
extern int JPEG_chg_cam(int _cam_no);

#endif // #ifndef __JPEG_READ_H__
