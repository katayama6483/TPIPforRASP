/** 
 * UDP communication class Driver(for Linux) 
 * @file W32_UDP_pl.h
 * @brief UDP communication �Υѥ�᡼�����ɥ饤�С���Linux�ǡ�
 *
 * @author Katayama
 * @date 2007-11-21
 * @version 1.00 : 2007/11/21 katayama
 * @version 2.00 : 2018/10/03 katayama
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */

/** \mainpage
 * ���Υ⥸�塼���UDP���̿����륯�饹�ؿ��Ǥ���
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
	int  rcv_tm;				//	�����Ԥ�����(ms)
	int  init_flg;				//	�������λ�ե饰(0:̤����1:��λ)
	SOCKET destSocket;
	struct sockaddr_in destSockAddr;
};

extern void lnx_udp_init(struct LNX_UDP* p, char* sv_cl);		// ����� sv_cl= "UDP_C":client "UDP_S":server
extern int  lnx_udp_open(struct LNX_UDP* p, char* host_adr, int port_no, int rcv_tm); //  open & ������ؿ�
extern int  lnx_udp_retry(struct LNX_UDP* p);			// ������ƻ�Դؿ�
extern void lnx_udp_close(struct LNX_UDP* p);			// ��λ�ؿ�
extern int  lnx_udp_send(struct LNX_UDP* p, void* send_buf, unsigned int send_size);	//	�����ؿ�
extern int  lnx_udp_recv(struct LNX_UDP* p, void* rcv_buf , unsigned int r_cnt);		// �����ؿ�
extern int  lnx_udp_recv_rt(struct LNX_UDP* p, void* rcv_buf ,unsigned int r_cnt ,int rcv_tm);	// �����ؿ��ʼ��������ޡ���


#endif /* ifndef ___LNX_UDP_PL_H___ */
