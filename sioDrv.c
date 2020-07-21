/**
 * @file  sioDrv.c
 * @brief RS232 SIO Access関数群
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2007-05-18
 * @version $Id: v 1.00 2007/05/23 11:24:00 katayama $
 * @version $Id: v 1.01 2008/08/20 00:00:00 katayama $
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "sioDrv.h"
#include "time_sub.h"	// *** 2006.12
//#include "dump.h"
//#include "led_access.h"

//#define ___TRACE_ON___
//#include "trace.h"

/**==========================================================================================
 *   #define
 **==========================================================================================*/
#define MAX_COM_PORT_NUM     (16)
#define FALSE                 (0)
#define TRUE                  (1)
#define TRANS_MAX_SIZE        (40)

#define SIO_DRIVE_ESC_CODE    (0xFC)
#define SIO_DRIVE_START_CODE  (0xFE)
#define SIO_DRIVE_END_CODE    (0xFF)

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/

static struct {
    int           baudrate;
    unsigned int  bit;
} gSerialBaudRate[] = {
    {   1200, B1200 },
    {   2400, B2400 },
    {   4800, B4800 },
    {   9600, B9600 },
    {  19200, B19200 },
    {  38400, B38400 },
    {  57600, B57600 },
    { 115200, B115200 },
    { 0, 0 },
};

/**==========================================================================================
 *   スタティック変数形宣言
 **==========================================================================================*/
typedef int  BOOL;

static  int  rcv_cnt;
static  int  rcv_mode;
static  int  esc_f;

static  int  tr_n = 0;
static  char tr[256];
static  char snd_dt[256];

/**
 * @brief Get baudrate bit
 * @param  BaudRate : 9600, 19200, ..... ,115200
 * @retval bit      : B9600,B19200, .....
 * @retval == 0xFFFF: error
 */
unsigned int get_baudrate_bit(int BaudRate)
{
    unsigned int res;
    int          i;

    res = 0xffff;
    for ( i = 0; gSerialBaudRate[i].baudrate != 0; i++) {
        if (BaudRate == gSerialBaudRate[i].baudrate) {
            res = gSerialBaudRate[i].bit;
            break;
        }
    }
    return res;
}

/**
 * @brief  Get ByteSize bit
 * @param  ByteSize : 5, 6, 7, 8
 * @retval bit      : CS5, CS6, CS7, CS8
 * @retval == 0xFFFF: error
 */
unsigned int get_bytesize_bit(int ByteSize)
{
    unsigned int res;

    res = 0xffff;
    switch(ByteSize){
        case 5 :
            res = CS5;
            break;
        case 6 :
            res = CS6;
            break;
        case 7 :
            res = CS7;
            break;
        case 8 :
            res = CS8;
            break;
        default :
            res = 0xffff;
            break;
    }
    return res;
}

/**
 * @brief  Get Parity bit
 * @param  Parity   : 0:NOPARITY  1:ODDPARITY 2:EVENPARITY
 * @retval bit      : 0, PARENB|PARODD, PARENB
 * @retval == 0xFFFF: error
 */
unsigned int get_parity_bit(int Parity)
{
    unsigned int res;

    res = 0xffff;
    switch(Parity){
        case NOPARITY :
            res = 0;
            break;
        case ODDPARITY :
            res = (PARENB|PARODD);
            break;
        case EVENPARITY :
            res = PARENB;
            break;
        default :
            res = 0xffff;
            break;
    }
    return res;
}

/**
 * @brief  Get StopBits bit
 * @param  StopBits : 0:1 1:1.5 2:2
 * @retval bit      : 0, 0, CSTOPB
 * @retval == 0xFFFF: error
 */
unsigned int get_stopbits_bit(int StopBits)
{
    unsigned int res;

    res = 0xffff;
    switch(StopBits){
        case 0 :
            res = 0;
            break;
        case 1 :
            res = 0;
            break;
        case 2 :
            res = CSTOPB;
            break;
        default :
            res = 0xffff;
            break;
    }
    return res;
}

void reset_tr()
{
    tr_n =0;
}

void set_tr(char dt)
{
    if (tr_n < 256) {
        tr[tr_n++] = dt;
    }
}



/**
 * @brief  SIO初期化処理
 * @param  ComNM    : オープン通信ポート名　/dev/ttySC
 * @param  BaudRate : 9600, 19200, .....
 * @param  ByteSize : 7, 8
 * @param  Parity   : 0:NOPARITY  1:ODDPARITY 2:EVENPARITY
 * @param  StopBits : 0:1 1:1.5 2:2
 * @param  TimeOut  : 0:完了待ち  1縲鰀:ms
 * @retval TRUE     : 成功
 * @retval FALSE    : 失敗
 */

int sioInit( char *ComNM, int BaudRate, int ByteSize, int Parity, int StopBits, int TimeOut )
{
    BOOL            result;
    int             fd;
    struct termios  newtio;
    unsigned int    B_speed, B_CS, B_parity, B_stopbits;

    result = FALSE;

    fd = open(ComNM, O_RDWR | O_NOCTTY, 0 );
    if (fd <0){
        return(FALSE);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = CLOCAL | CREAD;

    result = TRUE;
    B_speed = get_baudrate_bit(BaudRate);
    if (B_speed == 0xffff) {
        result = FALSE;
    }

    if(result){
        B_CS = get_bytesize_bit(ByteSize);
        if (B_CS == 0xffff) {
            result = FALSE;
        }
    }

    if(result){
        B_parity = get_parity_bit(Parity);
        if (B_parity == 0xffff) {
            result = FALSE;
        }
    }

    if(result){
        B_stopbits = get_stopbits_bit(StopBits);
        if (B_stopbits == 0xffff) {
            result = FALSE;
        }
    }

    if(result){
        newtio.c_cflag |= B_speed;
        newtio.c_cflag |= B_CS;
        newtio.c_cflag |= B_parity;
        newtio.c_cflag |= B_stopbits;
        newtio.c_iflag = 0;
        newtio.c_oflag = 0;
        /* set input mode (non-canonical, no echo,...) */
        newtio.c_lflag = 0;

        newtio.c_cc[VTIME]    = 1;   /* 受信待ちタイマは100ms */
        newtio.c_cc[VMIN]     = 0;   /* Ｎバイト受信待ち*/

        tcflush(fd, TCIFLUSH);
        tcsetattr(fd,TCSANOW,&newtio);

    }
    if (result) {
        return(fd);
    }
    else {
        return(result);
    }
}

/**
 * @brief  SIOクローズ処理
 * @param  ComNM    : オープン通信ポート名　/dev/ttySC
 * @param  fd       : file descriptor
 * @retval TRUE     : 成功
 * @retval FALSE    : 失敗
 */
int  sioClose( int fd )
{
    BOOL result;

    result = close(fd);
    return(result);
}


/**
 * @brief  時間制限付 SIO １バイト受信
 * @param  fd       : file descriptor
 * @param  dt       : １バイト受信格納エリア
 * @param  timeout  : 0:完了待ち  1縲鰀:ms
 * @retval TRUE     : 成功
 * @retval FALSE    : 失敗
 */
int  sioTimedRecvByte( int fd, char *dt, int timeout )
{
    BOOL   result = FALSE;
    int    res;

    res = read(fd,dt,1);
    if(res > 0){
        result = TRUE;
    }

    return(result);
}

/**
 * @brief  時間制限付 SIO １バイト受信
 * @param  fd       : file descriptor
 * @param  buffer   : 受信格納バッファ ポインタ
 * @param  Size     : 受信格納バッファ サイズ
 * @param  ActualSize : 受信サイズ 格納エリア　ポインタ
 * @param  timeout  : 0:完了待ち  1縲鰀:ms
 * @retval TRUE     : 成功
 * @retval FALSE    : 失敗
 */
int  sioTimedRecvBuffer( int fd, void *buffer, int Size, int *ActualSize, int timeout )
{
    BOOL           result = FALSE;
    int            actual, res, size;
    char           *bpBuffer;
    fd_set         readfs;
    int            maxfd;
    struct timeval Timeout;

    if( ActualSize != 0){
        *ActualSize = 0;
    }

    bpBuffer  = (char *) buffer;
    actual    = 0;
    size      = Size;

    FD_ZERO(&readfs);                    // revision 2008.08.20
    maxfd = MAX(0, fd)+1;

    do{
        res = 0;

        Timeout.tv_usec = 1000*timeout;  /* ミリ秒 */
        Timeout.tv_sec  = 0;             /* 秒 */
        FD_SET(fd, &readfs);

        if(0 < select(maxfd, &readfs, NULL, NULL, &Timeout) ){
            if (FD_ISSET(fd, &readfs)){
                res = read( fd, bpBuffer, size );
                if(res > 0){
                    actual   += res;
                    size     -= res;
                    bpBuffer += res;
                }
            }
        }
    }while( (actual < Size)&&(0<res) );

    if( actual == Size ){
        result = TRUE;
    }
    if( ActualSize != 0){
        *ActualSize = actual;
    }

    return( result );
}


/**
 * @brief  Transparent 受信処理(Local)
 * @param  rcv_char : 受信データ
 * @param  buffer   : 受信格納バッファ ポインタ
 * @retval          : 受信格納サイズ
 */
int recvTransparent(int rcv_char, void *buffer)
{

    static int     cnt;
    int            ret;
    unsigned char  *rcv_buf;

    rcv_buf = (unsigned char *) buffer;

    ret = 0;
    if ( rcv_char > -1 )
    {
        switch(rcv_mode)
        {
            case 0:                 // start code 受信待ち
                switch(rcv_char)
                {
                    case SIO_DRIVE_START_CODE:      // start code 受信
                        cnt = 0 ;
                        rcv_mode = 1;
                        esc_f = 0 ;
                        break;
                    default:
                        break;
                }
                break;
            case 1:                 // Message 受信待ち
                switch(rcv_char)
                {
                    case SIO_DRIVE_START_CODE:      // start code 受信
                        cnt = 0 ;
                        rcv_mode = 1;
                        esc_f = 0 ;
                        break;
                    case SIO_DRIVE_ESC_CODE:        // ESC code 受信
                        esc_f = 1 ;
                        break;
                    case SIO_DRIVE_END_CODE:        // end code 受信
                        rcv_cnt = cnt;
                        rcv_mode = 0 ;
                        cnt = 0 ;
                        esc_f = 0 ;
                        ret = rcv_cnt;
                        break;
                    default:
                        if (cnt >= TRANS_MAX_SIZE)	// Message over
                        {
                            rcv_mode = 0;
                            break;
                        }
                        if (esc_f)		// ESC next code 待ち
                        {
                            esc_f = 0;
                            if (rcv_char < 4)
                            {
                                rcv_buf[cnt] = rcv_char + SIO_DRIVE_ESC_CODE ;
                                if (cnt < TRANS_MAX_SIZE) {
                                    cnt++;
                                }
                            }
                            else {
                                rcv_mode = 0;
                            }
                        }
                        else {
                            rcv_buf[cnt] = rcv_char ;
                            if (cnt < TRANS_MAX_SIZE) {
                                cnt++;
                            }
                        }
                        break;
                }
                break;
            default:
                rcv_mode = 0;
                break;
        }
        //		set_tr(rcv_mode);		//############
    }
    return (ret);
}

/**
 * @brief  Transparent 受信処理（メイン）
 * @param  buffer   : 受信格納バッファ ポインタ
 * @param  Size     : 受信格納バッファ サイズ
 * @param  ActualSize : 受信サイズ 格納エリア　ポインタ
 * @param  timeout  : 0:完了待ち  1縲鰀:ms
 * @retval          : 受信格納サイズ
 */
int sioRecvTransparent( int fd, void *buffer, int Size, int *ActualSize, int timeout )
{
    int                  Ret;
    BOOL                 ans;
    int                  rcv_char;
    char                 dt;
    static unsigned long TM_prv,TM_now,TM_itv;
    static unsigned long TM_st;
    int                  _cnt;


	TM_prv = GetTickCount();
	rcv_mode      = 0;
	(*ActualSize) = 0;
	Ret           = 0;
	_cnt          = 0;

	while( Ret == 0 )
	{
		ans = sioTimedRecvByte(fd, &dt, 100);
		TM_now = GetTickCount();
		if ( TM_prv ) {
			TM_itv = (TM_now - TM_prv) ;
		}
		else {
			TM_itv = 0;
		}
		if(ans == TRUE )
		{
			if (rcv_mode == 1) {
//				if ((TM_now - TM_st) > 2) printf("T");
			}
			_cnt++;
			rcv_char = dt;
//			if (rcv_char == SIO_DRIVE_START_CODE) printf("<");
			set_tr(dt);
//			CHECK_POINT(510, (short)rcv_char);
			Ret = recvTransparent( rcv_char, buffer );  // 受信処理
			TM_st = TM_now;
		}
		else  {
			rcv_char = -1;
			usleep(1000);
		}

		if ( TM_itv > timeout )        // Check Time out
		{
//			printf("sioRecvTransparent() --> timeout<%d,mode:%d,cnt:%d>\r\n", (int)TM_itv, rcv_mode, _cnt);
			rcv_mode = 0;
			Ret = -1;
		}
	}
	if (Ret > 0) {
		(*ActualSize) = Ret;
	}
	return(Ret);
}


/**
 * @brief  時間制限付 SIO 送信処理
 * @param  fd       : file descriptor
 * @param  buffer   : 送信バッファ ポインタ
 * @param  Size     : 送信データ サイズ
 * @param  ActualSize : 送信結果サイズ 格納エリア　ポインタ
 * @retval TRUE     : 成功
 * @retval FALSE    : 失敗
 */
int  sioSendBuffer( int fd, void *buffer, int Size, int *ActualSize )
{
    BOOL   result = FALSE;
    int    actual;


    if( ActualSize != 0 ){
        *ActualSize = 0;
    }

    actual = write( fd, (char*)buffer, Size );
    if( actual > 0 ){
        if( ActualSize != 0 ){
            *ActualSize = actual;
        }
        if( actual == Size ){
            result = TRUE;
        }
    }

    return(result);
}

/**
 * @brief  CreateTransparentData（Local)
 * @param  snd_buf  : Transparent mode 変換データ格納先 ポインタ
 * @param  snd_msg  : 送信バッファ ポインタ
 * @param  snd_cnt  : 送信データ サイズ
 * @param  ActualSize : 送信結果サイズ 格納エリア　ポインタ
 * @retval          : 変換後のデータサイズ
 */
int Create_sd_data(char* snd_buf,char* snd_msg,int snd_cnt)
{
    int           i,ix;
    unsigned char rv_c;

    ix = 0;
    snd_buf[ix] = (unsigned char)SIO_DRIVE_START_CODE ;
    ix++;
    for ( i = 0; i < snd_cnt; i++ )
    {
        rv_c        = snd_msg[i];
        if (rv_c >= SIO_DRIVE_ESC_CODE)
        {
            snd_buf[ix] = (unsigned char)SIO_DRIVE_ESC_CODE;
            ix++;
            rv_c = rv_c - SIO_DRIVE_ESC_CODE;
        }
        snd_buf[ix] = rv_c ;
        ix++;
    }
    snd_buf[ix] = (unsigned char)SIO_DRIVE_END_CODE ;
    ix++;
    return (ix);
}

/**
 * @brief  Transparent mode send routine（main)
 * @param  fd       : file descriptor
 * @param  snd_buf  : 送信データバッファ　ポインタ
 * @param  snd_cnt  : 送信データ サイズ
 * @retval          : 送信結果　サイズ
 */

int sioSendTransparent( int fd, char *snd_buf, int snd_cnt)
{
	int snd_sz;
	int res;

	snd_sz = Create_sd_data(snd_dt, snd_buf, snd_cnt);
//	CHECK_POINT(550, (short)snd_sz);
	res = sioSendBuffer( fd, snd_dt, snd_sz, &snd_sz);
	return(res);
}


