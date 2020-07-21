/**
 * UDP communication function(for Linux) 
 * @file  lnx_UDP.c
 * @brief UDP communication のパラメータ型ドライバー（Linux版）
 *
 * @author Katayama
 * @date 2007-11-21
 * @version 1.00 : 2007/11/21 katayama
 * @version 2.00 : 2018/10/03 katayama
 *
 * Copyright (C) 2007-2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#include  "lnx_UDP_pl.h"


/**
 * @brief  ソケット受信待ち関数
 * @fn     int wait_socket_readable(int sd, long msec)
 * @param  sd   : receive socket file descriptor
 * @param  msec : wait time [ milli sec ] (-1:non-blockig, 0:永久待ち, >0: wait time)
 * @retval == 1 : readable
 * @retval == 0 : timeout
 * @retval ==-1 : error
 */
int wait_socket_readable(int sd, long msec)
{
	int            ans =-1;
	fd_set         rset;
	struct timeval tv;

	FD_ZERO(&rset);
	FD_SET(sd, &rset);

	if (msec == 0) {
		// msec == 0 （永久待ち）
		ans = select(sd + 1, &rset, NULL, NULL, NULL);
	}
	else {
		if (msec > 0) {
			tv.tv_usec = msec * 1000;
			tv.tv_sec  = 0;
			if (tv.tv_usec >= 1000*1000) {
				tv.tv_sec  = tv.tv_usec / (1000*1000);
				tv.tv_usec = tv.tv_usec % (1000*1000);
			}
		}
		else {
			// usec == -1 （即時リターン）
			tv.tv_sec  = 0;
			tv.tv_usec = 0;
		}

		ans = select(sd + 1, &rset, NULL, NULL, &tv);
	}
	return (ans);
}


/** 初期化
 *  lnx_udp(struct LNX_UDP* p,char* sv_cl)
 * @param  sv_cl    : "UDP_C":client "UDP_S":server
 */
void lnx_udp_init(struct LNX_UDP* p,char* sv_cl)
{
	p->destSocket = 0;
	p->ip_adr[0]  = 0;
	p->sv_flg     = 0;
	if (strcmp(sv_cl,"UDP_S")==0) p->sv_flg = 1;
	p->rcv_tm     = 0;
	p->init_flg   = 0;
}

/** UDP通信ポートOPEN 処理
 *  int lnx_udp_open(char* host_adr, int port_no, int rcv_tm)
 * @param  host_adr  : Host IP address
 * @param  port_no   : port no
 * @param  ms_rcv_tm : 受信タイマー値(-1:即時 , 0:無限待 , N: N ms待）
 * @retval == 0  : 正常終了
 * @retval ==-1  : エラー終了
 */
int lnx_udp_open(struct LNX_UDP* p,char* host_adr, int port_no, int ms_rcv_tm)
{
	int ans;

	if (ms_rcv_tm > 0) p->rcv_tm = ms_rcv_tm;
	else p->rcv_tm = ms_rcv_tm;


	/* sockaddr_in 構造体のセット */
	memset(&p->destSockAddr, 0, sizeof(p->destSockAddr));
	printf("sv_flg = %d \n", p->sv_flg);
	if (p->sv_flg) p->destSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else p->destSockAddr.sin_addr.s_addr = inet_addr(host_adr);
	p->destSockAddr.sin_port = htons(port_no);
	p->destSockAddr.sin_family = AF_INET;

	p->destSocket = socket(AF_INET, SOCK_DGRAM, 0);
	ans = 0;
	if (p->destSocket == INVALID_SOCKET) {
		ans = -1;
		p->destSocket = 0;
	}
	if (p->sv_flg) ans = bind(p->destSocket, (struct sockaddr *) &p->destSockAddr, sizeof(p->destSockAddr));
	return( ans ) ;
}


/** UDP通信ポート終了処理
 *  void lnx_udp_close()
 */
void  lnx_udp_close(struct LNX_UDP* p)
{
	close(p->destSocket);
	return;
}


/** UDP通信ポート 送信処理
 *  int lnx_udp_send( void* send_buf, unsigned int send_size )
 * @param  send_buf    : 送信バッファアドレス
 * @param  send_size   : 送信データサイズ
 * @retval == 0        : 送信エラー
 * @retval == SendByte : 送信したデータバイト数
 */
int  lnx_udp_send( struct LNX_UDP* p, void* send_buf, unsigned int send_size)
{
	DWORD	SendByte ;

	SendByte = sendto(p->destSocket, (char*)send_buf, send_size, 0,
				(struct sockaddr *)&p->destSockAddr, sizeof(p->destSockAddr));
	return ( SendByte );
}


/** UDP通信ポート 受信処理
 *  int lnx_udp_recv( void* rcv_buf ,unsigned int r_cnt )
 * @param  rcv_buf     : 受信バッファアドレス
 * @param  r_cnt       : 受信バッファサイズ
 * @retval == 0        : 受信エラー
 * @retval == RecvByte : 受信したデータバイト数
 */
int  lnx_udp_recv(struct LNX_UDP* p, void* rcv_buf ,unsigned int r_cnt)
{
	int   res;
	DWORD RecvByte ;
	int   destSockAddrLen;

	res = wait_socket_readable(p->destSocket, p->rcv_tm);
	if (res <= 0) return(res);

	destSockAddrLen = sizeof(p->destSockAddr);
	RecvByte = recvfrom(p->destSocket, (char*)rcv_buf, r_cnt, 0,
					(struct sockaddr *)&p->destSockAddr, (socklen_t*)&destSockAddrLen);

	return ( RecvByte );
}

/** UDP通信ポート 受信処理（受信待ちタイム指定）
 *  int lnx_udp_recv_rt( void* rcv_buf ,unsigned int r_cnt ,int rcv_tm )
 * @param  rcv_buf     : 受信バッファアドレス
 * @param  r_cnt       : 受信バッファサイズ
 * @param  rcv_tm      : 受信タイマー値(-1:即時 , 0:無限待 , N: N ms待）
 * @retval == 0        : 受信エラー
 * @retval == RecvByte : 受信したデータバイト数
 */
int  lnx_udp_recv_rt(struct LNX_UDP* p, void* rcv_buf ,unsigned int r_cnt ,int rcv_tm)
{
	int   res;
	DWORD RecvByte ;
	int   destSockAddrLen;

	if (rcv_tm > 0) rcv_tm = rcv_tm;
	res = wait_socket_readable(p->destSocket, rcv_tm);
	if (res <= 0) return(res);

	destSockAddrLen = sizeof(p->destSockAddr);
	RecvByte = recvfrom(p->destSocket, (char*)rcv_buf, r_cnt, 0,
					(struct sockaddr *)&p->destSockAddr, (socklen_t*)&destSockAddrLen);

	return ( RecvByte );
}
