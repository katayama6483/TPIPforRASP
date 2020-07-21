/** 
 * UDP communication class Driver(for Linux) 
 * @file W32_UDP_pl.h
 * @brief UDP communication のパラメータ型ドライバー（Linux版）
 *
 * @author Katayama
 * @date 2007-11-21
 * @version 1.00 : 2007/11/21 katayama
 * @version 2.00 : 2018/10/03 katayama
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */

/** \mainpage
 * このモジュールはUDPで通信するクラス関数です。
 */
#ifndef ___LNX_UDP_PL_H___
#define ___LNX_UDP_PL_H___

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>

typedef int SOCKET;
typedef unsigned long DWORD;
#define INVALID_SOCKET -1

struct LNX_UDP {
	char ip_adr[16];
	int  sv_flg;
	int  port_no;
	int  rcv_tm;				//	受信待ち時間(ms)
	int  init_flg;				//	初期化完了フラグ(0:未完、1:完了)
	SOCKET destSocket;
	struct sockaddr_in destSockAddr;
};

extern void lnx_udp_init(struct LNX_UDP* p, char* sv_cl);		// 初期化 sv_cl= "UDP_C":client "UDP_S":server
extern int  lnx_udp_open(struct LNX_UDP* p, char* host_adr, int port_no, int rcv_tm); //  open & 初期化関数
extern int  lnx_udp_retry(struct LNX_UDP* p);			// 初期化再試行関数
extern void lnx_udp_close(struct LNX_UDP* p);			// 終了関数
extern int  lnx_udp_send(struct LNX_UDP* p, void* send_buf, unsigned int send_size);	//	送信関数
extern int  lnx_udp_recv(struct LNX_UDP* p, void* rcv_buf , unsigned int r_cnt);		// 受信関数
extern int  lnx_udp_recv_rt(struct LNX_UDP* p, void* rcv_buf ,unsigned int r_cnt ,int rcv_tm);	// 受信関数（受信タイマー）


#endif /* ifndef ___LNX_UDP_PL_H___ */
