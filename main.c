/** 
 * @file main.cpp
 * @brief SIO test program
 *
 * @author Katayama
 * @date 2018-10-30
 * @version 1.00  2018/10/30 katayama
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>


#include "kbhit.h"
#include "sioDrv.h"
#include "def_jtp.h"
#include "def_mctrl_com16.h"
#include "set_config.h"
#include "ctrl_com.h"
#include "ctrl_mng.h"
#include "jtcp_com_sv.h"
#include "wlan_mng.h"
#include "version.h"

#include "JPEG_read.h"
#include "shared_msg.h"

static char ver_buf[16];
static int b_no = 0;
static jtcp_out_t           ctrl_data;	// 制御データ
static jtcp_inp_t           sens_data;	// センサーデータ

#define CTRL_BOARD_MAX                (4)
static SET_CONFIG_PARAM     ctrlCom_initParam[CTRL_BOARD_MAX];

//#define MAX_JPEG_DATA     (90*1024)
//static char   jpeg_data[MAX_JPEG_DATA];

#define FILE_SIZE 256
#define _MMAP_FL_NAME_ "/tmp/tpip_cmd.txt"


extern int setbf_ver(int bno); 

/** set TPIP control head data
 * @fn     int set_ctrl_header(void)
 * @retval == 0 : all
 *
 */
static int set_ctrl_header(void)
{
	ctrl_data.hd.ver    = 0x07;   //2014/02/25 Versionを 2から7に修正
	ctrl_data.hd.msg_no = 0;
	ctrl_data.hd.info   = 0;
	ctrl_data.hd.d_id   = 0x10;
	ctrl_data.b_ptn     = 0xffff; //2014/02/19 hoshino 0x0fff → 0xffff
	return 0;
}

/** set device data
 * @fn     int set_dev_data(char* msg)
 * @retval == 0 : OK
 * @retval != 0 : error
 *
 */
static int set_dev_data(char* msg)
{
	int err = -1;
	int n;
	int m;
	int val = 0;
	int ch  = 0;
	char* p1[2];
	char* p2[2];
	
	n = split_MSG(msg, "=", p1, 2);
	if (n == 2) {
		val = atoi(p1[1]);
		printf("<n=%d> device(%s) = %d \n", n, p1[0], val);
		m = split_MSG(p1[0], "_", p2, 2);
		if (strcmp(p2[0], "DO") == 0) {
			ctrl_data.dt[b_no].d_out = (val << 8) & 0x0f00;
			printf(" ----- DO = %d \n", val);
			err = 0;
		}
		if ((m == 2)&&(strcmp(p2[0], "PWM") == 0)) {
			ch = atoi(p2[1]);
			ctrl_data.dt[b_no].PWM[ch] = val;
			printf(" ----- PWM[%d] = %d \n", ch, val);
			err = 0;
		}
		if ((m == 2)&&(strcmp(p2[0], "PWM2") == 0)) {
			ch = atoi(p2[1]);
			ctrl_data.dt[b_no].PWM2[ch] = val;
			printf(" ----- PWM2[%d] = %d \n", ch, val);
			err = 0;
		}
	}
	
	return err;
}

/**  TPIP control head data
 * @fn     int set_ctrl_header(void)
 * @retval == 0 : all
 *
 */

/** Main Program
 * @fn     int main(int argc, char *argv[])
 * @param  argc : Argument count
 * @param  argv : Argument Value
 * @retval == 0 : all
 *
 */
int main(int argc, char *argv[])
{
	int err;
	int err1;
	int res;
	int fd;
	int loop = 0;
	int key;
	int snd_cnt;
	int snd_sz;
	int rcv_cnt;
	char ver_no[10];
	
	char msg[128];
	char* blk[2];
	char* param[16];
	int n;
	int m;
	t_jpeg_info info;
	long    actual_size = 0;
	int tpip_ctl_on = 0;


	printf("---- TPIP for Raspberry Pi main program start\n");
    ver_buf[0]    = 0;
    set_ver();                  // set version infomation.

	init_wlan_mng();
	err  = JTCP_open();
	err1 = init_share_MSG(_MMAP_FL_NAME_, FILE_SIZE, 1);
	
	if (!err) loop = 1;
	while (loop) {
		if (kbhit()) {
			key = getch();
			if (key == 'q') break;
			if (key == 'w') {
				printf(" wlink : %d\n", get_wlan_info());
			}
		}
		usleep(10*1000);	// sleep 10ms
		res = get_share_MSG(msg, sizeof(msg));
		if (res > 0) {
			n = split_MSG(msg, ",", blk, 2);
			if (n == 2) {
//				printf(" --- split_MSG() = %d [%s][%s] ---\n", n, blk[0], blk[1]);
				if (strcmp(blk[0], "GET_JPEG")==0) {
					res = JPEG_write_rq(blk[1]);
				}
				if (strcmp(blk[0], "QUIT")==0) {
					loop = 0;
				}
				if (strcmp(blk[0], "CTRL_DATA")==0) {
					m = split_MSG(blk[1], ":", param, 16);
					for (int i = 0; i < m; i++) {
						printf(" ctrl data <%s>\n", param[i]);
						set_dev_data(param[i]);
					}
				}
			}
			if (n == 1) {
				if (strcmp(blk[0], "QUIT")==0) {
					loop = 0;
				}
				if (strcmp(blk[0], "CTRL_ON")==0) {
					init_ctrl_mng();
					JTCP_set_mode(1);
					tpip_ctl_on = 1;
				}
				if (strcmp(blk[0], "CTRL_OFF")==0) {
					del_ctrl_mng();
					JTCP_set_mode(0);
					tpip_ctl_on = 0;
				}
			}
			res = clear_share_MSG();
		}
		if (tpip_ctl_on) {
			set_ctrl_header();
			set_ctrl_eu2sdata((char *)&ctrl_data, sizeof(ctrl_data));
		}
		
		if (ver_buf[0]==0) {
			get_ver_fw(0,ver_buf);
			if (ver_buf[0]) {
				set_ver_fw(ver_buf);
			}
		}

	}
	
//	err = JTP_close();
	err = JTCP_close();
	del_wlan_mng();
	printf("---- TPIP for Raspberry Pi program end\n");
	return(0);	return(0);
}
