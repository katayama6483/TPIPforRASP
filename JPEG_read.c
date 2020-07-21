/** 
 * @file JPEG_read.c
* @brief JPEG read program module
 *
 * @author  Katayama
 * @date    2018-10-12
 * @version 1.00 2018/10/12
 * @version 1.10 2018/10/22
 * @version 1.20 2020/01/14 katayama SANRITZ 4ch camera prototype board
 * @version 1.21 2020/02/12 katayama ch4 camera 未接続時の対応 <open()呼出タイミング変更>
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

#include <time.h>
#include <pthread.h>

#include "v4l2_capture.h"
#include "JPEG_read.h"
#include "camera_chg.h"
#include "set_config.h"
#include "MEM_mng.h"


static int                    t_jpeg_Flg   = 0;
static int                    t_jpeg_stat  = 0;
static pthread_t              t_jpeg_Thread;
pthread_mutex_t               mtx_jpeg     = PTHREAD_MUTEX_INITIALIZER;

static int capture_size = 0;
static unsigned char* capture_data_p = 0;

static int fd = -1;	// File descriptor of video device
static int bps;
static int vga;
static int cam_no;
static int rq_cam_no;
static int auto_cam_chg;
static int _auto_cnt;
static char device_name[32];

static int rq_jpeg_write     = 0;
static int jpeg_write_size   = 0;
static char jpeg_f_name[128] = "";

#define CAP_CNT    200
/*
struct {
	int    img_sz;
	double itv_tm;
} reslt[CAP_CNT];
*/

typedef struct{
	int   fl_size;
	char* buf_p;
} fl_image_t;

static fl_image_t VGA_err;
static fl_image_t QVGA_err;

static struct timespec st_ts;
static struct timespec ed_ts;
static struct timespec st_ts1;
static struct timespec ed_ts1;
static u_long itv_tm1;
static int  prt_on = 0;

static t_jpeg_info jpeg_info;

/** calculate interval time
 *  @fn     double calc_itv_time(struct timespec* st_time, struct timespec* ed_time)
 *  @param  st_ts : start time (timespec)
 *  @param  ed_ts : end   time (timespec)
 *  @retval interval time[sec]
 *
 */
static double calc_itv_time(struct timespec* st_ts, struct timespec* ed_ts)
{
	long int itv_stime; // sec
	long int itv_ntime; // nano-sec
	double   itv_time;
	
	if (ed_ts->tv_nsec < st_ts->tv_nsec) {
		itv_stime = (ed_ts->tv_sec - st_ts->tv_sec - 1);
		itv_ntime = (ed_ts->tv_nsec + 1000000000 - st_ts->tv_nsec);
	} else {
		itv_stime = (ed_ts->tv_sec  - st_ts->tv_sec);
		itv_ntime = (ed_ts->tv_nsec - st_ts->tv_nsec);
	}
	
	itv_time = (double)itv_stime + (double)itv_ntime / 1000000000.0;
	return itv_time;
}

static struct {
	int width;
	int height;
} disp_resol_table[] = {
//  width  height
	  320,    240,		// 0:QVGA
	  640,    480,		// 1:VGA
	  800,    600,		// 2:SVGA
	 1024,    768,		// 3:XGA
	 1280,    720,		// 4:HD
	 1280,   1024,		// 5:SXGA
	 1600,   1200,		// 6:UXGA
	 1920,   1080		// 7:FHD(Full-HD)
};
/**
 * @brief  set Display resolution
 * @fn     int  set_dsp_resolution(int resol)
 * @param  resol : Display resolution [0:QVGA, 1:VGA, 2:SVGA, 3:XGA, 4:HD, 5:SXGA, 6:UXGA, 7:FHD]
 * @retval == 0  : OK
 * @retval <  0  : error
 */
int  set_dsp_resolution(int fd, int resol)
{
	int err = -1;
	
	if ((resol >= 0)&&(resol <  DISP_RESOL_MAX)) {
		err = set_video_format(fd, disp_resol_table[resol].width,   disp_resol_table[resol].height, 0);
	}
	printf("set_dsp_resolution(%d) = %d\n", resol, err);
	return err;
}
/**
 * @brief  read image file
 * @fn     int  read_image(char* f_name, unsigned char** buf_p)
 * @param  f_name: file name
 * @param  buf_p : buffer pointer address(image data)
 * @param  image_size : image data size
 * @retval >  0  : OK <read data size>
 * @retval == 0  : error
 */
static int read_image(char* f_name, char** buf_p)
{
	int   ans     = 0;
	int   fd      = -1;
	off_t fl_size = 0;
	char* buf     = 0;

	fd = open(f_name, O_RDONLY);
	if (fd >= 0) {
		fl_size = lseek(fd, 0, SEEK_END);
		lseek(fd, 0, SEEK_SET);
		if (fl_size > 0) buf = malloc(fl_size);
		if ((fl_size > 0)&&(buf)) {
			ans = read(fd, buf, fl_size);
		}
		close(fd);
	}
	*buf_p = buf;
	return ans;
}

/**
 * @brief  write image file
 * @fn     int  write_image(char* f_name, unsigned char* buf_p, unsigned long image_size)
 * @param  f_name: file name
 * @param  buf_p : buffer pointer(image data)
 * @param  image_size : image data size
 * @retval == 0  : OK <write data size>
 * @retval <  0  : error
 */
static int  write_image(char* f_name, unsigned char* buf_p, int image_size)
{
	int outfd = -1;
	int ans = -1;
	
	if (image_size <= 0) return -2;
	outfd = creat(f_name, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
//	printf("  open file(%d) -> ", outfd);
	if (outfd >= 0) {
	    int n = write(outfd, buf_p, image_size);
//	    printf("write image[%d]\n -> ",n);
		fdatasync(outfd);
	    close(outfd);
		ans = n;
	}
	return ans;
}

/**
 * @brief  lock JPEG data
 * @fn     int  lock_data(void)
 * @retval == 0  : OK
 * @retval != 0  : error
 */
static int  lock_data(void)
{
	return pthread_mutex_lock(&mtx_jpeg);
}

/**
 * @brief  unlock JPEG data
 * @fn     int  unlock_data(void)
 * @retval == 0  : OK
 * @retval != 0  : error
 */
static int  unlock_data(void)
{
	return pthread_mutex_unlock(&mtx_jpeg);
}

/**
 * @brief  JPEG read task
 * @fn     void  t_jpeg( unsigned long arg )
 * @param  arg  : parameter of task (not use)
 *
 */
void  t_jpeg( unsigned long arg )
{
	int err = -1;
	int index = 0;
	int    img_size = 0;
	u_char frm_cnt  = 0;
	int bit_rate;
	int _cam_no;
	
	pthread_mutex_init(&mtx_jpeg, NULL);
	printf("--- jpeg thread start ---\n");
	
	capture_data_p = 0;
	capture_size   = 0;
	fd = -1;
	
	printf("---- get Multi CAM mode : [%02X] -----\n", get_multi_cam_mode());
	if (get_multi_cam_mode()) {
		_cam_no = init_cam_chg(cam_no);
		printf("[%s:%s]init_cam_chg(%d) = %d \n", __FILE__,__func__, cam_no, _cam_no);
		
		if (_cam_no >= 0) {
			cam_no    = _cam_no;
			rq_cam_no = _cam_no;
			err = 0;
		}
	}
	else err = 0;

	fd = open(device_name, O_RDWR, 0);
	printf("camera device open(%s)=%d \n", device_name, fd);

	if (fd >= 0){
		if (err == 0) {
			err = get_v4l2_capability(fd);
//			PRT_v4l2_capability();
		}
		if (err == 0) {
			err = get_v4l2_input(fd);
//			PRT_v4l2_input();
		}
		if (err == 0) {
			err = get_v4l2_fmtList(fd);
//			PRT_v4l2_fmtList();
		}

		set_video_rate(fd, bps);
// --- get JPEG Compression Quality ---
		bit_rate = get_video_rate(fd);
		if (bit_rate >= 0) {
			printf("    MJPEG Bitrate    :[%d]\n",   bit_rate);
		}
		if (err == 0) err = set_dsp_resolution(fd, vga);	// VGA
		if (err == 0) err = get_video_format(fd);
		if (err == 0) err = init_mmap(fd, cam_no);

		if (err == 0) {
			err = STREAM_on(fd);
			if (err == 0) t_jpeg_stat = 0x01;
		}
	}
	else err = -1;
	
	printf("ERROR = %d \n", err);
	t_jpeg_Flg = 2;
	if (err == 0) {
		t_jpeg_Flg = 1;
	}
	
	index = 0;
//	int _skip = 0;
	static int _m_cam = 0;
	static int _s_cam = 0;
	static int _alt   = 0;
	int q_fg = 0;
	frm_cnt  = 0;
	while (t_jpeg_Flg == 1) {
		clock_gettime(CLOCK_REALTIME, &st_ts1);
		if ((index = v_capture(fd)) < 0) {
			t_jpeg_Flg = 2;
			break;
		}
		
		if (index >= 0) {
			lock_data();
			t_jpeg_stat = 0x01;
			if ((capture_size = get_size_v_capture(index)) < 0) {
				printf("[%s:%s]capture_size=%d\n", __FILE__,__func__, capture_size);
				t_jpeg_Flg = 2;
				break;
			}
			
//			if (_skip == 1) {
//				capture_size = 0;
//				_skip = 0;
//			}
			if (_auto_cnt > 0) _auto_cnt--;
			
			capture_data_p = get_buf_p_v_capture(index);
		
			clock_gettime(CLOCK_REALTIME, &ed_ts1);
			jpeg_info.index  = index;
			jpeg_info.frm_no = frm_cnt;
			jpeg_info.cam_no = get_cam_no_v_capture(index);
			jpeg_info.vga    = (u_char)(vga & 0xff);
			jpeg_info.st_ts  = st_ts1;
			jpeg_info.ed_ts  = ed_ts1;
			itv_tm1 = (u_long)(calc_itv_time(&st_ts1, &ed_ts1) * 1000);
			jpeg_info.itv_tm = itv_tm1;
			
			if (itv_tm1 > 50) prt_on = 1;
			if (prt_on) printf("--- capture log frm_cnt=%3d vga=%d itv_tm=%3d capture_size=%d \n", frm_cnt, vga, itv_tm1, capture_size);
			
			frm_cnt++;
			if (frm_cnt >= CAP_CNT) frm_cnt = 0;
			
			if ((capture_size > 0)&&(rq_jpeg_write)) {
				if ((strnlen(jpeg_f_name, sizeof(jpeg_f_name)) > 0)&&(strnlen(jpeg_f_name, sizeof(jpeg_f_name)) < sizeof(jpeg_f_name))) {
					jpeg_write_size = write_image(jpeg_f_name, capture_data_p, capture_size);
					jpeg_f_name[0] = 0;
				}
				rq_jpeg_write = 0;
			}
			unlock_data();
			
			if (auto_cam_chg) {
				_m_cam = auto_cam_chg & 0x03;
				if (_auto_cnt == 0) {
					if (_alt) {
						_s_cam = (_s_cam + 1) & 0x03;
						if (_m_cam == _s_cam) {
							_s_cam = (_s_cam + 1) & 0x03;
						}
						rq_cam_no = _s_cam;
						_auto_cnt = 2;
					}
					else {
						rq_cam_no = _m_cam;
						_auto_cnt = 2;
					}
					
					_alt ^= 1;
				}
			}
			
			if ((rq_cam_no != cam_no)&&(get_multi_cam_mode())) {
				printf ("--- Stream OFF ---\n");
				err = STREAM_off(fd);
				err = chg_cam(rq_cam_no);
				if (!err) {
					err = STREAM_on(fd);
					if (!err) {
						cam_no = rq_cam_no;
//						_skip = 1;
					}
				}
				else {
					printf ("--- chg_cam() ERROR --> Return camera No ----\n");
					rq_cam_no = cam_no;
					chg_cam(rq_cam_no);
					STREAM_on(fd);
				}
			}
			if ((err = QBUF(fd, index, cam_no)) < 0) break;
		}
/*
		if (rq_cam_no != cam_no) {
			err = chg_cam(rq_cam_no);
			if (!err) cam_no = rq_cam_no;
			else rq_cam_no = cam_no;
		}
*/
	}
	while (t_jpeg_Flg == 2) {
		usleep(30*1000);

		lock_data();
		if (vga == 1) {
			capture_size   = VGA_err.fl_size;
			capture_data_p = VGA_err.buf_p;
		}
		else {
			capture_size   = QVGA_err.fl_size;
			capture_data_p = QVGA_err.buf_p;
		}
		unlock_data();

		if (rq_cam_no != cam_no) {
			cam_no = rq_cam_no;
		}
	}
	
	if (fd > -1) {
		printf ("--- Stream OFF ---\n");
		err = STREAM_off(fd);
		capture_data_p = 0;
		capture_size   = 0;
		
		if (!err) err = release_mmap(fd);
		
		printf(" ---- Frame cont = %d (t_jpeg_Flg = %d) err = %d----\n", frm_cnt, t_jpeg_Flg, err);
		
		close(fd);
		fd = -1;
	}
	
	pthread_mutex_destroy(&mtx_jpeg);
	t_jpeg_Flg = 0;
	printf("--- jpeg thread end ---\n");
	pthread_exit(NULL);
}



/** open JPEG open program
 *  @fn     int JPEG_open(char* dev_name, int _bps, int _vga, int _cam_no)
 *  @param  _dev_name  : camera device name(ex. "/dev/vodeo0")
 *  @param  _bsp       : first Transmission speed(bsp)
 *  @param  _vga       : first Video type     0=QVGA    1=VGA
 *  @param  _cam_no    : first Camera Number [0 - 3]
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JPEG_open(char* dev_name, int _bps, int _vga, int _cam_no)
{
	int err = -1;
	int i;
	bps = _bps;
	vga = _vga;
	_cam_no   &= 0x03; 
	cam_no    = _cam_no;
	rq_cam_no = _cam_no;
	prt_on = 0;
	
	MEM_init(32*1024);
	set_multi_cam_mode(get_multi_cam());

	if (VGA_err.buf_p)  {
		free(VGA_err.buf_p);
		VGA_err.fl_size = 0;
		VGA_err.buf_p   = NULL;
	}
	if (QVGA_err.buf_p) {
		free(QVGA_err.buf_p);
		QVGA_err.fl_size = 0;
		QVGA_err.buf_p   = NULL;
	}
	VGA_err.fl_size  = read_image("VGA_err.jpg" , &VGA_err.buf_p);
	QVGA_err.fl_size = read_image("QVGA_err.jpg", &QVGA_err.buf_p);
	
	t_jpeg_stat = 0x00;
	if (strlen(dev_name) < sizeof(device_name)){
		strcpy(device_name, dev_name);
		err = 0;
	}
	else err = -2;

	if ((t_jpeg_Flg == 0)&&(err == 0)) {
		t_jpeg_Flg = -3;
		err = pthread_create( &t_jpeg_Thread, NULL, (void(*))t_jpeg, (void *)NULL );
		err = -3;
		for (i = 0; i < 100; i++) {
			if (t_jpeg_Flg > 0) {
				err = 0;
				break;
			}
			usleep(30*1000);
		}
	}
	
	if (err < 0) {
		printf("[%s:%s]JPEG_open error! <t_jpeg_Flg = %d, err = %d>=%d\n", __FILE__,__func__, t_jpeg_Flg, err);
	}

	return err;
}

/** close JPEG close program
 *  @fn     int JPEG_close(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JPEG_close(void)
{
	int i;
	int err = -1;
	
	prt_on = 0;
	t_jpeg_stat = 0x00;
//	printf("[%s:%s]\n", __FILE__,__func__);
	if (t_jpeg_Flg > 0) {
		t_jpeg_Flg = -1;	// About request
		for (i = 0; i < 100; i++) {
			if (t_jpeg_Flg == 0) break;
			usleep(1*1000);		// 1ms sleep
		}
		err = pthread_join(t_jpeg_Thread, NULL);
	}
	
	if (VGA_err.buf_p)  {
		free(VGA_err.buf_p);
		VGA_err.fl_size = 0;
		VGA_err.buf_p   = NULL;
	}
	if (QVGA_err.buf_p) {
		free(QVGA_err.buf_p);
		QVGA_err.fl_size = 0;
		QVGA_err.buf_p   = NULL;
	}
	MEM_end();

	return err;
}

/** read JPEG read program
 *  @fn     int JPEG_read(unsigned char* buf_p, int buf_size, t_jpeg_info* info)
 *  @param  buf_p    : read buffer pointer
 *  @param  buf_size : buffer size
 *  @param  info     : get jpeg capture infomation pointer <NULL: Do not acquire>
 *  @retval >  0     : JPEG data size
 *  @retval == 0     : non data
 *  @retval <= 0     : error
 *
 */
int JPEG_read(unsigned char** buf_pp, int buf_size, t_jpeg_info* info)
{
	int err = -2;
	unsigned char* _buf_p = *buf_pp;
	
	if (t_jpeg_Flg > 0) { // Ready JPEG read task?
		err = lock_data();
		if (!err) {
			err = -1;
//			if ((capture_data_p)&&(capture_size <= buf_size)) {
			if ((capture_data_p)) {
				if (capture_size > 0) {
					if (buf_size == 0) {
						_buf_p  = (unsigned char*)MEM_get(capture_size);
						*buf_pp = _buf_p;
					}
//					printf("[%s:%s] capture_data_p=%p[%p] capture_size=%d\n", __FILE__,__func__, capture_data_p, _buf_p, capture_size);
					memcpy(_buf_p, capture_data_p, capture_size);
					prt_on = 0;
				}
				err = capture_size;
				capture_size = 0;
			}
			if (info) *info = jpeg_info;
			unlock_data();
		}
	}
	return err;
}

/**
 * @brief  JPEG write image file
 * @fn     int JPEG_write_file(char* f_name, unsigned char* buf_p, unsigned long image_size)
 * @param  f_name: file name
 * @param  buf_p : buffer pointer(image data)
 * @param  image_size : image data size
 * @retval == 0  : OK <write data size>
 * @retval <  0  : error
 */
int JPEG_write_file(char* f_name, unsigned char* buf_p, int image_size)
{
	int ans = -1;
	
	if (image_size <= 0) return -2;
	ans = write_image(f_name, buf_p, image_size);
	return ans;
}

/**
 * @brief  request JPEG write 
 * @fn     int JPEG_write_rq(char* f_name)
 * @param  f_name: file name
 * @retval == 0  : OK <write data size>
 * @retval <  0  : error
 */
int JPEG_write_rq(char* f_name)
{
	if (rq_jpeg_write) return -1;
	
	if ((strnlen(f_name, sizeof(jpeg_f_name)) > 0)&&(strnlen(f_name, sizeof(jpeg_f_name)) < sizeof(jpeg_f_name))) {
		strcpy(jpeg_f_name, f_name);
		jpeg_write_size = 0;
		rq_jpeg_write   = 1;
	}
	for (int i = 0; i < 500; i++) {
		if (rq_jpeg_write == 0) break;
		usleep(10*1000);		// 10ms sleep
	}
	if (rq_jpeg_write) {
		jpeg_write_size = -3;	// time over
		rq_jpeg_write   = 0;
	}
	return jpeg_write_size;
}


/** get JPEG read status
 *  @fn     int JPEG_status(void)
 *  @retval : JPEG read status
 *
 */
int JPEG_status(void)
{
	return t_jpeg_stat;
}

/** set JPEG bitrate
 *  @fn     int JPEG_set_bps(int _bps)
 *  @param  _bsp      : first Transmission speed(bsp)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JPEG_set_bps(int _bps)
{
	int err = -1;
	int bit_rate;
	
	bps = _bps;
	err = set_video_rate(fd, bps);
	bit_rate = get_video_rate(fd);
	if (bit_rate >= 0) {
		printf("    MJPEG Bitrate    :[%d]\n",   bit_rate);
	}
	return err;
}

/** set JPEG change camera no
 *  @fn     int JPEG_chg_cam(int _cam_no)
 *  @param  _cam_no   : camera no
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JPEG_chg_cam(int _cam_no)
{
	int err = -1;
	if (_cam_no & 0xf0) {
		printf("[%s:%s] _cam_no=%X\n", __FILE__,__func__, _cam_no);
	}
	
	if ((_cam_no >= 0)&&(_cam_no < 4)) {
		rq_cam_no = _cam_no;
		auto_cam_chg = 0;
		err = 0;
	}
	
	if (_cam_no & 0xf0) {
		auto_cam_chg = _cam_no & 0xff;
		_auto_cnt = 0;
		err = 0;
	}
	
	return err;
}
