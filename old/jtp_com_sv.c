/** 
 * @file  jtp_com_sv.c
 * @brief JTP communication server program
 *
 * @author Katayama
 * @date 2018-10-17
 * @version 1.00  2018/10/17 katayama
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "lnx_UDP_pl.h"


#include "def_jtp.h"

#define MAX_JPEG_DATA     (90*1024)
#define MAX_BLOCK_SIZE    (sizeof(jtp_t) - sizeof(jtp_hd_t))


#define ERR_JTP_IP   (-4)

static int                    t_jtp_Flg   = 0;
static int                    t_jtp_stat  = 0;
static pthread_t              t_jtp_Thread;
pthread_mutex_t               mtx_jtp;

static int    connect_IPaddr = 0;
static int    JPEG_vType     = 0;	// Video type     0=QVGA    1=VGA
static int    rq_JPEG_vType  = 0;	// request Video type
static int    JPEG_kbps      = 0;	// mJpeg bit rate [Kbps]
static int    rq_JPEG_kbps   = 0;	// request mJpeg bit rate
static int    JPEG_cam_no    = 0;	// Camera Number [0 - 3]
static int    rq_JPEG_cam_no = 0;	// request Camera Number
static jtp_t  rcv_buf;
static jtp_t  snd_buf;
static char   jpeg_data[MAX_JPEG_DATA];
static u_long TMstmp = 0;

#if (0)
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <unistd.h>
/** open JPEG read program
 *  @fn     int JPEG_open(char* dev_name, int _bps)
 *  @param  _dev_name  : camera device name(ex. "/dev/vodeo0")
 *  @param  _bsp       : first Transmission speed(bsp)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
static int JPEG_open(char* dev_name, int _bps)
{
	return 0;
}

/** close JPEG read program
 *  @fn     int JPEG_close(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
static int JPEG_close(void)
{
	return 0;
}

/** read JPEG data
 *  @fn     int JPEG_read(unsigned char* buf_p, int buf_size, t_jpeg_info* info)
 *  @param  buf_p    : read buffer pointer
 *  @param  buf_size : buffer size
 *  @param  info     : get jpeg capture infomation pointer <NULL: Do not acquire>
 *  @retval >  0     : JPEG data size
 *  @retval <= 0     : error
 *
 */
static int JPEG_read(unsigned char* buf_p, int buf_size, void* dummy)
{
	int fd;
	int ans = 0;
	long fl_size = -1;
	
	fd = open("j_vga.jpg", O_RDONLY);
	if (fd < 0) ans = -1;
	
	if (fd >= 0) {
		fl_size = lseek(fd, 0, SEEK_END);
//		printf("lseek()=%d\n", fl_size);
		ans     = lseek(fd, 0, SEEK_SET);
		if (ans < 0) fl_size = 0;
	}
	if ((fl_size > 0)&&(fl_size <= buf_size)) {
		ans = read(fd, buf_p, buf_size);
		printf("read()=%d(%d)\n", ans, fl_size);
	}
	if (fd >= 0) {
		close(fd);
	}
	return ans;
}
#else
#include "JPEG_read.h"
#endif



#include <time.h>
static struct timespec st_ts;
static struct timespec ed_ts;

static t_jpeg_info jpeg_info;

/** calculate interval time
 *  @fn     double calc_itv_time(struct timespec* st_time, struct timespec* ed_time)
 *  @param  st_ts : start time (timespec)
 *  @param  ed_ts : end   time (timespec)
 *  @retval interval time[nsec]
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


/** Print IP address
 * @fn     void PRT_IPaddr(long ip)
 * @param  ip  : IP address (32bit)
 *
 */
static void PRT_IPaddr(long ip)
{
	printf("JTP connct IP addrss %d.%d.%d.%d\n", 
		(ip         & 0xFF),
		((ip >>  8) & 0xFF),
		((ip >> 16) & 0xFF),
		((ip >> 24) & 0xFF));
}

/** Print Header
 * @fn     void PRT_JTP_header(jtp_t* hd, int err)
 * @param  hd  : JTP header pointer
 * @param  err : error number(0:OK, < 0: error)
 *
 */
static void PRT_JTP_header(jtp_t* hd, int rcv_cnt, int err)
{
	
	if (err == 0) {
		printf("(%d)[%d,%d,sz=%4d,tm=%08X] ", rcv_cnt,
			hd->jtp_h.JTP_flag,
			hd->jtp_h.seq,
			hd->jtp_h.size,
			hd->jtp_h.TMstmp);
		printf("\n");
	}
}

/** check recive JTP_Header
 * @fn     int check_JTP_Header(long IPaddrss, jtp_t* rcv, int rcv_sz)
 * @param  com     : UDP communication parameter
 * @param  rcv     : JTP recive data pointer
 * @param  rcv_sz  : JTP recive data size
 * @retval == 0 : OK
 * @retval != 0 : NG
 *
 */
static int check_JTP_Header(long IPaddrss, jtp_t* rcv, int rcv_sz)
{
	int err = 0;
	
	if (rcv_sz != JTP_H_SZ) return -1;
	
	if (connect_IPaddr){
		if (connect_IPaddr != IPaddrss) {
			err = ERR_JTP_IP;
			PRT_IPaddr(connect_IPaddr);
		}
	}
	else if (!connect_IPaddr){
		connect_IPaddr = IPaddrss;
		PRT_IPaddr(IPaddrss);
	}
	return err;
}

/** set JTP_Header
 * @fn     int set_JTP_data(jtp_t* snd, u_char flag, u_char seq, char* data, u_long size, u_long TMstmp, u_char cam_no)
 * @param  snd     : JTP send data pointer
 * @param  flag    : JTP communication flag<b0:start of block b1:end of block>
 * @param  seq     : communication sequence number
 * @param  data    : JPEG block data top address
 * @param  size    : JPEG block data size
 * @param  TMstamp : time stamp
 * @param  cam_no  : camera no <0 - 3>
 * @retval send data size
 *
 */
static int set_JTP_data(jtp_t* snd, u_char flag, u_char seq, char* data, u_long size, u_long TMstmp, u_char cam_no)
{
//	static unsigned char cmm[6] = {0xFE,0xFF,0x04,0x00,0x30,0x00};
	
	if (size > sizeof(snd->body)) size = sizeof(snd->body);
	
	if ((data)&&(size > 0)) {
		memcpy(&snd->body, data, size);
//		snd->body[size] = 0x30;
//		size++;
	}
	
	snd->jtp_h.JTP_flag = (flag & 0x03)|((cam_no<<4) & 0x30);
	snd->jtp_h.seq      = seq;
	snd->jtp_h.size     = size;
	snd->jtp_h.TMstmp   = TMstmp;
	
	return (JTP_H_SZ + size); 
}

/**
 * @brief  retry JPEG read
 * @fn     void  retry_JPEG(void)
 *
 */
void  retry_JPEG(void)
{
	int open_err = 0;;

	JPEG_close();
//	if (open_err == 0) open_err = init_cam_chg();
//	if (open_err == 0) open_err = chg_cam(JPEG_cam_no);
	if (open_err == 0) open_err = JPEG_open("/dev/video0", JPEG_kbps*1000, JPEG_vType, JPEG_cam_no);
}

/**
 * @brief  JTP communication thread
 * @fn     void  t_jtp( unsigned long arg )
 * @param  arg  : parameter of task (not use)
 *
 */
static void  t_jtp( unsigned long arg )
{
	struct LNX_UDP com ;
	int open_err;
	int rcv_err;
	int rcv_cnt;
	int snd_sz;
	int snd_cnt;
	u_char  seq  = 0;
	u_char  flag = 0;
	u_short block_no   = 0;
	u_short block_size = 0;
	long    actual_size = 0;
	char*   jpeg_data_p = &jpeg_data[0];
	t_jpeg_info info;
	int rv_tm;
	int jpeg_err_cnt = 0;
	int not_ready_cnt = 0;
	int max_jpeg_err_cnt = 0;
	int wait_flg = 0;
	int cam_no   = 0;


	printf("--- JTP communication thread start ---\n");
	
	pthread_mutex_init(&mtx_jtp, NULL);

	lnx_udp_init(&com,"UDP_S");
	open_err = lnx_udp_open(&com, NULL, UDP_PORT_JPEG, 100);
	printf("lnx_udp_open() <%X>\n", open_err);
	
	if (open_err == 0) open_err = JPEG_open("/dev/video0", JPEG_kbps*1000, JPEG_vType, JPEG_cam_no);
	if (open_err == 0) t_jtp_Flg = 1;	// start loop
	
	rcv_err    = -1;
	rv_tm      = 100;	// connection time
	not_ready_cnt = 0;
	while (t_jtp_Flg > 0) {
		
		while ((rcv_cnt = lnx_udp_recv_rt(&com, &rcv_buf, sizeof(rcv_buf), -1)) > 0) {}
		if (rcv_cnt == 0) rcv_cnt = lnx_udp_recv_rt(&com, &rcv_buf, sizeof(rcv_buf), rv_tm);
		rcv_err = -1;
		if (rcv_cnt > 0) {
			rcv_err = check_JTP_Header(com.destSockAddr.sin_addr.s_addr, &rcv_buf, rcv_cnt);
			if (rcv_err) rcv_cnt = -1;
		}

		if (t_jtp_Flg == 1) {	// 1st Phase [get JPEG data]
			if (rcv_cnt > 0) {
				actual_size = JPEG_read(jpeg_data, sizeof(jpeg_data), &info);
//				printf("JPEG_read()=%d\n", actual_size);
				if (actual_size > 0) {
					not_ready_cnt = 0;
					t_jtp_Flg    = 2;		// goto 2nd phase [send JPEG data]
					block_no     = 0;
					cam_no       = info.cam_no;
					flag         = 0x01;	// start block
					rv_tm        = -1;		// recive timer => non-block 
					jpeg_err_cnt = 0;
/*					if (actual_size == 0) {
						flag         = 0x02;	// end block (0x03にするとJPEG sizeがゼロで通知される為、windows APPに影響を与える)
					}*/
				}
				else if (actual_size < 0) {	// JPEG read error?
					if (actual_size == -2) { // JPEG thread not ready ?
						not_ready_cnt++;
						if (not_ready_cnt > 20) actual_size = -1;
					}
					if (actual_size == -1) {
						printf("[%s:%s] JPEG read error[%d,%d] \n", __FILE__, __func__, actual_size, jpeg_err_cnt);
						if ((jpeg_err_cnt % 10)== 9) retry_JPEG();
//						t_jtp_Flg    = 3;		// goto 3rd Phase [check JTP  parameter]
						t_jtp_Flg    = 2;		// goto 2nd phase [send JPEG data]
						block_no     = 0;
						cam_no       = 0;
						flag         = 0x02;	// end block<error>
//						rv_tm        = -1;		// recive timer => non-block 
						
						jpeg_err_cnt++;
						if (max_jpeg_err_cnt < jpeg_err_cnt) max_jpeg_err_cnt = jpeg_err_cnt;
						if (jpeg_err_cnt > 100) {
//							printf("[%s:%s] goto exit \n", __FILE__, __func__);
//							t_jtp_Flg = -2;
							jpeg_err_cnt = 0;
						}
						
					}
				}
			}
			
/*			if (rq_JPEG_cam_no != JPEG_cam_no) {
				JPEG_cam_no = rq_JPEG_cam_no;
				int err = JPEG_chg_cam(JPEG_cam_no);
			}*/

		}

		if (t_jtp_Flg == 2) {	// 2nd Phase [send JPEG data]
			if (rcv_cnt >= 0) {		// recive or timeout
				block_size = 0;
				if (actual_size > 0) {
					jpeg_data_p = &jpeg_data[MAX_BLOCK_SIZE * block_no];	// set block top position of jpeg data
					if (actual_size > MAX_BLOCK_SIZE) block_size = MAX_BLOCK_SIZE;
					else {
						block_size = actual_size;
						flag |= 0x02;	// end block
					}
				}
				else if (actual_size == 0) {	// not JPEG data
					flag |= 0x02;	// end block
				}
					
				if (block_size >= 0) {
					snd_sz  = set_JTP_data(&snd_buf, flag, seq, jpeg_data_p, block_size, TMstmp, (u_char)(cam_no & 0x03));
//					PRT_JTP_header(&snd_buf, snd_sz, 0);
					snd_cnt = lnx_udp_send(&com, &snd_buf, snd_sz);
					actual_size -= block_size;
					flag         = 0x00;
					block_no++;
					seq++;
					if (actual_size <= 0) {
						t_jtp_Flg = 3;		// goto 3rd Phase [check JTP  parameter]
						wait_flg  = 0;
						rcv_cnt   = 0;
					}
				}
			}
		}
		
		if (t_jtp_Flg == 3) {	// 3rd Phase [check JTP  parameter]
			if (rq_JPEG_kbps != JPEG_kbps) {
				JPEG_kbps = rq_JPEG_kbps;
				int err = JPEG_set_bps(JPEG_kbps*1000);
			}

			if (rq_JPEG_cam_no != JPEG_cam_no) {
//				printf("[%s:%s] chang rq_JPEG_cam_no = %X \n", __FILE__, __func__, rq_JPEG_cam_no);
				JPEG_cam_no = rq_JPEG_cam_no;
				int err = JPEG_chg_cam(JPEG_cam_no);
			}
			
			if (rq_JPEG_vType != JPEG_vType) {
				printf("[%s:%s] chang QVGA/VGA \n", __FILE__, __func__);
				JPEG_close();
				JPEG_vType = rq_JPEG_vType;
				JPEG_open("/dev/video0", JPEG_kbps*1000, JPEG_vType, JPEG_cam_no);
				jpeg_err_cnt = 0;
			}
			t_jtp_Flg = 1;		// goto 1st phase [get JPEG data]
			rv_tm     = 15;		// next JPEG data time
		}
	}
	JPEG_close();
	lnx_udp_close(&com);

	t_jtp_Flg = 0;
	pthread_mutex_destroy(&mtx_jtp);
	printf("--- JTP communication thread end [max_jpeg_err_cnt = %d] ---\n", max_jpeg_err_cnt);
	pthread_exit(NULL);
}


/** open JTP communication program
 *  @fn     int JTP_open(long IPadr, int vType, int kbps)
 *  @param  IPadr  : connecting IP address
 *  @param  vType  : Video type     0=QVGA    1=VGA
 *  @param  kbps   : mJpeg bit rate [Kbps]
 *  @param  cam_no : camera number  [0: camera1, 1: camera2, 2: camera3, 3: camera4]
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTP_open(long IPadr, int vType, int kbps, int cam_no)
{
	int err = -1;
	
	if (t_jtp_Flg == 0) {
		
		connect_IPaddr = IPadr;
		JPEG_vType     = vType;
		rq_JPEG_vType  = vType;
		JPEG_kbps      = kbps;
		rq_JPEG_kbps   = kbps;
		JPEG_cam_no    = cam_no;
		rq_JPEG_cam_no = cam_no;
		
		
		err = pthread_create( &t_jtp_Thread, NULL, (void(*))t_jtp, (void *)NULL );
	}
	return err;
}

/** close JTP read program
 *  @fn     int JTCP_close(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTP_close(void)
{
	int i;
	int err = -1;
	
	if (t_jtp_Flg == 1) {
		t_jtp_Flg = -1;	// About request
		for (i = 0; i < 100; i++) {
			if (t_jtp_Flg == 0) break;
			usleep(1*1000);		// 1ms sleep
		}
		connect_IPaddr = 0;
		JPEG_vType     = 0;
		JPEG_kbps      = 0;
		err = pthread_join(t_jtp_Thread, NULL);
	}
	return err;
}


/** set JTP parameter
 *  @fn     int JTP_set_param(int vType, int kbps, int cam_no)
 *  @param  vType  : Video type     0=QVGA    1=VGA
 *  @param  kbps   : mJpeg bit rate [Kbps]
 *  @param  cam_no : camera number  [0: camera1, 1: camera2, 2: camera3, 3: camera4]
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTP_set_param(int vType, int kbps, int cam_no)
{
	int err = -1;

	if (t_jtp_Flg > 0) {
		rq_JPEG_vType  = vType;
		rq_JPEG_kbps   = kbps;
		rq_JPEG_cam_no = cam_no;
		err = 0;
	}
	return err;
}

