/** 
 * @file  v4l2_capture.h
 * @brief V4L2 video capture header file
 *
 * @author  Katayama
 * @date    2018-10-11
 * @version 1.00 2018/10/11
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#ifndef __V4L2_CAPTURE_H__
#define __V4L2_CAPTURE_H__


extern int get_v4l2_capability(int fd);
extern int PRT_v4l2_capability(void);
extern int get_v4l2_input(int fd);
extern int PRT_v4l2_input(void);
extern int set_video_rate(int fd, int bps);
extern int get_video_rate(int fd);
extern int set_video_format(int fd, int width, int height, int pix_fmt);
extern int get_video_format(int fd);
extern int get_v4l2_fmtList(int fd);
extern void PRT_v4l2_fmtList(void);

extern int QBUF(int fd, int index, int cam_no);
extern int DQBUF(int fd);
extern int query_buf(int fd, int index);

extern int init_mmap(int fd, int cam_no);
extern int release_mmap(int fd);
extern int v_capture(int fd);
extern int v_capture_dummy(int fd);
extern int get_size_v_capture(int index);
extern unsigned char* get_buf_p_v_capture(int index);
extern int get_cam_no_v_capture(int index);

extern int STREAM_on(int fd);
extern int STREAM_off(int fd);

#endif // #ifndef __V4L2_CAPTURE_H__
