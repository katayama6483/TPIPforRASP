/** 
 * @file  jtcp_com_sv.c
 * @brief JTCP communication server program
 *
 * @author Katayama
 * @date 2018-10-17
 * @version 1.00  2018/10/17 katayama
 * @version 1.01  2018/10/19 katayama com_st反映
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#include <stdio.h>
#include <pthread.h>

#include "lnx_UDP_pl.h"


#include "def_jtp.h"
#include "ctrl_mng.h"
#include "jtp_com_sv.h"
#include "wlan_mng.h"


#define UDP_PORT_JTCP (7070)
#define JTCP_VER      (130)

#define ERR_JTCP_MARK (-1)
#define ERR_JTCP_VER  (-2)
#define ERR_JTCP_SIZE (-3)
#define ERR_JTCP_IP   (-4)

static int                    t_jtcp_Flg   = 0;
static int                    t_jtcp_stat  = 0;
static pthread_t              t_jtcp_Thread;
pthread_mutex_t               mtx_jtcp;

static jtcp_t  rcv_buf;
static jtcp_t  snd_buf;

// ---- For actuator / sensor communication
static long C_connect_IPaddr = 0;	// conncted IP address
static int  C_connct_on;
static int  C_com_cnt;
static int  C_err;
static int  C_err_cnt;

// ---- For video communication
static long V_connect_IPaddr = 0;	// conncted IP address
static int  V_connct_on;
static int  V_com_cnt;
static int  V_err;
static int  V_err_cnt;

static int  JTCP_com_rq    = 0;	// communication request bit0:JPEG bit1:制御
static int  JTCP_com_st    = 0;	// communication status  bit0:JPEG bit1:制御
static int  JTCP_com_mode  = 0; // communication mode    0= normal, 1= command mode
static int  JPEG_vType     = 0;	// Video type     0=QVGA    1=VGA
static int  JPEG_kbsp      = 0;	// mJpeg bit rate [Kbps]
static int  JPEG_cam_no    = 0;	// camera number [0 - 3]

/** Print IP address
 * @fn     void PRT_IPaddr(long ip)
 * @param  ip  : IP address (32bit)
 *
 */
static void PRT_IPaddr(long ip)
{
	printf("JTCP connct IP addrss %d.%d.%d.%d\n", 
		(ip         & 0xFF),
		((ip >>  8) & 0xFF),
		((ip >> 16) & 0xFF),
		((ip >> 24) & 0xFF));
}

/** Print Header
 * @fn     void PRT_JTCP_header(jtcp_t* hd, int err)
 * @param  hd  : JTCP header pointer
 * @param  err : error number(0:OK, < 0: error)
 *
 */
static void PRT_JTCP_header(jtcp_t* hd, int rcv_cnt, int err)
{
	mctrl_hd_t* body;
	
	if (err >= 0) {
		printf("(%d)[%c,%d,sz=%3d] ", rcv_cnt,
			hd->jtcp_h.mark,
			hd->jtcp_h.ver,
			hd->jtcp_h.size);
	
		printf("v[type %d,Q %d,cam %d,fps %d] ",
			hd->v_ctrl.v_type,
			hd->v_ctrl.q_fact,
			hd->v_ctrl.cam_no,
			hd->v_ctrl.v_fps);
	
		if (hd->jtcp_h.size) {
			body = (mctrl_hd_t*)&hd->body[0];
			printf("[Ver %d,MsgNo %d,ID %02X,Info %d]",
				body->ver,
				body->msg_no,
				body->d_id,
				body->info);
		}
		printf("\n");
	}
	else {
		if (err == ERR_JTCP_MARK) printf("JTCP mark error[%c] \n", hd->jtcp_h.mark);
		if (err == ERR_JTCP_VER ) printf("JTCP support version error(>=%d)[%d] \n", 110, hd->jtcp_h.ver);
		if (err == ERR_JTCP_SIZE) printf("JTCP recive size error[%d(body size=%d)] \n", rcv_cnt, hd->jtcp_h.size);
	}
}

/** check recive JTCP_Header
 * @fn     int check_JTCP_Header(jtcp_t* rcv)
 * @param  rcv     : JTCP recive data pointer
 * @param  rcv_sz  : JTCP recive data size
 * @retval == 0 : OK
 * @retval == 1 : OK(TLX)
 * @retval != 0 : NG
 *
 */
static int check_JTCP_Header(jtcp_t* rcv, int rcv_sz)
{
	int err = 0;
	
	if (rcv->jtcp_h.mark == '*') err = 1;
	else if (rcv->jtcp_h.mark != '@') err = ERR_JTCP_MARK;	// check marker 
	else {
		if (rcv->jtcp_h.ver < 110) err = ERR_JTCP_VER;	// check communication vresion 
		else {
			if ((rcv->jtcp_h.size + JTCP_H_SZ) != rcv_sz) err = ERR_JTCP_SIZE;	// check recive size
		}
	}
	return err;
}

/** set JTCP_Header
 * @fn     int set_JTCP_data(jtcp_t* snd, u_char com_st, u_char seq, u_long TMstmp, char* body, int body_size, int type)
 * @param  snd       : JTCP send data pointer
 * @param  com_st    : communication status<b0:JPEG b1:ctrl>
 * @param  seq       : communication sequence number
 * @param  TMstamp   : time stamp<return>
 * @param  body      : control bourd data 
 * @param  body_size : control bourd data size
 * @param  type      : 0=normal , 1=TPLX
 * @retval send data size
 *
 */
static int set_JTCP_data(jtcp_t* snd, u_char com_st, u_char seq, u_long TMstmp, unsigned char* body, int body_size)
{
	int ctrl_com_size = body_size;	// 制御ボード間通信の受信データサイズ
	
	snd->jtcp_h.mark    = '@';
	snd->jtcp_h.ver     = JTCP_VER;
	snd->jtcp_h.size    = ctrl_com_size;
	snd->jtcp_h.rq_st   = com_st;
	snd->jtcp_h.seq     = seq;
	snd->jtcp_h.w_lnk   = (u_char)get_wlan_info();
	snd->jtcp_h.TMstmp  = TMstmp;
	if (ctrl_com_size > 0) memcpy(&snd->body, body, body_size);
	return (JTCP_H_SZ + snd->jtcp_h.size); 
}


/**
 * @brief  JTCP communication thread
 * @fn     void  t_jtcp( unsigned long arg )
 * @param  arg  : parameter of task (not use)
 *
 */
static void  t_jtcp( unsigned long arg )
{
	struct LNX_UDP com ;
	int err;
	int rcv_cnt;
	int snd_sz;
	int snd_cnt;
	int com_rq;
	int com_st;
	int type;
	long IPaddrss;
	jtcp_inp_t sens_dt;
	int sens_size;
	int ctrl_size;

	printf("--- JTCP communication thread start ---\n");
	
	pthread_mutex_init(&mtx_jtcp, NULL);

	lnx_udp_init(&com,"UDP_S");
	err = lnx_udp_open(&com, NULL, UDP_PORT_JTCP, 100);
	printf("open <%X>\n",err);
	t_jtcp_Flg = 1;
	com_rq      = 0;
	com_st      = 0;
	IPaddrss    = 0;
	
	C_connct_on = 0;
	C_err_cnt   = 0;
	C_com_cnt   = 0;

	V_connct_on = 0;
	V_err_cnt   = 0;
	V_com_cnt   = 0;

	while (t_jtcp_Flg > 0) {
		type = 0;
		rcv_cnt = lnx_udp_recv(&com, &rcv_buf, sizeof(rcv_buf));
		if (rcv_cnt > 0) {
			err = check_JTCP_Header(&rcv_buf, rcv_cnt);
//			printf("---- JTCP receive(%d) err=%d ----\n", rcv_cnt, err);
//			PRT_JTCP_header(&rcv_buf, rcv_cnt, err);
			if (err == 1) {
				err  = 0;
				type = 1;
			}
			if (!err) {
				IPaddrss = com.destSockAddr.sin_addr.s_addr;
				com_rq   = rcv_buf.jtcp_h.rq_st;	// Get communication request
				C_err = 0;
				V_err = 0;
				
				if (com_rq & 0x02) {	// actuator / sensor receive processing
					if (!C_connect_IPaddr){
						C_connect_IPaddr = IPaddrss;
						printf(" --- ACTUATOR ");
						PRT_IPaddr(IPaddrss);
					}
					else {
						if (C_connect_IPaddr != IPaddrss) C_err = ERR_JTCP_IP;
					}
					
					if (!C_err) {
						// check connecting
						C_err_cnt = 0;
						if (!C_connct_on) {
							C_com_cnt++;
							if (C_com_cnt > 10) {
								C_connct_on = 1;	// 安定した通信を確認
								if (!init_ctrl_mng()) JTCP_com_st |= 0x02;
							}
						}
						/* 制御データセット */
						if ((JTCP_com_mode == 0)&&(rcv_buf.jtcp_h.size)) {
							ctrl_size = set_ctrl_eu2sdata( (char *)&rcv_buf.body[0], rcv_buf.jtcp_h.size );
						}
					}
				}
				
				if (com_rq & 0x01) {	// video receive processing
					if (!V_connect_IPaddr){
						V_connect_IPaddr = IPaddrss;
						printf(" --- VIDEO ");
						PRT_IPaddr(IPaddrss);
					}
					else {
						if (V_connect_IPaddr != IPaddrss) V_err = ERR_JTCP_IP;
					}
					
					if (!V_err) {
						// check connecting
						V_err_cnt = 0;
						JPEG_vType  = rcv_buf.v_ctrl.v_type;
						JPEG_kbsp   = rcv_buf.v_ctrl.q_fact;
						JPEG_cam_no = rcv_buf.v_ctrl.cam_no;
						
						if (!V_connct_on) {
							V_com_cnt++;
							if (V_com_cnt > 10) {
								V_connct_on = 1;	// 安定した通信を確認
								if (!JTP_open(V_connect_IPaddr, JPEG_vType, JPEG_kbsp, JPEG_cam_no )) JTCP_com_st |= 0x01;
							}
						}
					}
				}

			}
			
			if (!err) {
				// send return JTCP data
				com_st  = JTCP_com_st & com_rq;
				sens_size = 0;
				
				if (com_rq & 0x01) {	// video send processing
					if (V_connct_on) JTP_set_param(JPEG_vType, JPEG_kbsp, JPEG_cam_no);
				}
				
				if (com_rq & 0x02) {	// actuator / sensor send processing
					sens_size = get_ctrl_s2eudata( (char *)&sens_dt, sizeof(sens_dt));
				}
				if (sens_size) snd_sz  = set_JTCP_data((jtcp_t*)&snd_buf, com_st, rcv_buf.jtcp_h.seq, rcv_buf.jtcp_h.TMstmp, (unsigned char*)&sens_dt, sens_size);
				else snd_sz  = set_JTCP_data((jtcp_t*)&snd_buf, com_st, rcv_buf.jtcp_h.seq, rcv_buf.jtcp_h.TMstmp, (unsigned char*)NULL, 0);
				
//				printf(" ---- JTCP send(%d) ----\n", snd_sz);
				snd_cnt = lnx_udp_send(&com, &snd_buf, snd_sz);
			}
		}
		else {	// check disconnect
//			printf(" ---- JTCP error ----\n");
			if (com_rq & 0x02) {	// actuator / sensor error processing
				if (C_connct_on) {
					C_err_cnt++;
					if (C_err_cnt > 10) { // disconnect
						printf(" ---- JTCP disconnect(actuator / sensor) ----\n");
						C_connct_on   = 0;
						C_connect_IPaddr = 0;
						JTCP_com_rq = JTCP_com_rq ^ 0x02;
						JTCP_com_st = JTCP_com_st ^ 0x02;
						del_ctrl_mng();
					}
				}
			}
			
			if (com_rq & 0x01) {	// video error processing
				if (V_connct_on) {
					V_err_cnt++;
					if (V_err_cnt > 10) { // disconnect
						printf(" ---- JTCP disconnect(video) ----\n");
						V_connct_on   = 0;
						V_connect_IPaddr = 0;
						JTCP_com_rq = JTCP_com_rq ^ 0x01;
						JTCP_com_st = JTCP_com_st ^ 0x01;
						JTP_close();
					}
				}
			}
		}
	}
	lnx_udp_close(&com);

	pthread_mutex_destroy(&mtx_jtcp);
	t_jtcp_Flg = 0;
	printf("--- JTCP communication thread end ---\n");
	pthread_exit(NULL);
}


/** open JTCP communication program
 *  @fn     int JTCP_open(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTCP_open(void)
{
	int err = -1;
	
	if (t_jtcp_Flg == 0) {
		err = pthread_create( &t_jtcp_Thread, NULL, (void(*))t_jtcp, (void *)NULL );
	}
	return err;
}

/** close JTCP read program
 *  @fn     int JTCP_close(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTCP_close(void)
{
	int i;
	int err = -1;
	
	if (t_jtcp_Flg == 1) {
		t_jtcp_Flg = -1;	// About request
		for (i = 0; i < 100; i++) {
			if (t_jtcp_Flg == 0) break;
			usleep(1*1000);		// 1ms sleep
		}
		err = pthread_join(t_jtcp_Thread, NULL);
	}
	return err;
}

/** set JTCP communication mode
 *  @fn     int JTCP_set_mode(int mode)
 *  @param  mode : communication mode
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int JTCP_set_mode(int mode)
{
	JTCP_com_mode = mode;
	return 0;
}
