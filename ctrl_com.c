/**
 * @file  ctrl_com.c
 * @brief 制御ボードとの通信処理プログラム
 *
 * @author Katayama
 * @date 2014-02-05
 * @version ver 1.00 2014/02/05 asanuma
 * @version ver 1.10 2016/12/21 katayama
 * @version ver 1.11 2017/01/25 katayama(115200bps)
 * @version ver 1.20 2018/11/02 katayama(for Raspberry Pi)
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

// ver 1.00 : ctrl_sio16.c をベースにI2C通信に対応

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#include "ctrl_mng.h"
#include "time_sub.h"
#include "sioDrv.h"
#include "data_pack.h"
#include "set_config.h"
#include "def_mctrl_com16.h"
//#include "sio_udp9512.h"
//#include "ctrl_spi.h"
#include "ctrl_com.h"
//#include "sio_stat.h"
//#include "led_access.h"


//#define ___TRACE_ON___
#include "trace.h"

//#define __DUMP__  1
#ifdef __DUMP__
#include "dump.h"
char dump_msg[256];
#endif

//#define  SIO_COMPORT   (2)

//#define __LOG__
#ifdef __LOG__
static FILE* err_log;
#endif

//#define DEBUG_MESG


/**==========================================================================================
 *   #define
 **==========================================================================================*/
//#define CTRL_COM_SIO_DEVNAME          ("/dev/ttymxc2")
//#define CTRL_COM_SIO_DEVNAME          ("/dev/ttyUSB0")	// raspberry Pi USB port rs232 converter
#define CTRL_COM_SIO_DEVNAME            ("/dev/ttyS0")	// raspberry Pi GPIO UART port
#define CTRL_COM_SIO_DEFAULT_SPEED      (38400)
#define CTRL_COM_SIO_HIGH_SPEED         (115200)		// ver 1.11

//#define CTRL_COM_I2C_MASTER_DEV       ("/dev/i2c-2")  /* i2c master device name */
#define CTRL_COM_I2C_MASTER_DEV       ("/dev/i2c-1")  /* i2c master device name(Raspberry Pi) */
#define CTRL_COM_I2C_SLAVE_ADDR       (0x20)          /* i2c slave address base */
#define CTRL_COM_I2C_RSIZE_MASK       (0x7F)
#define CTRL_COM_I2C_RNEXT_FLAG       (0x80)

#define CTRL_COM_I2C_CYC_WAIT         (10*1000*1000)   // 10ms
#define CTRL_COM_I2C_RECV_COUNT_MAX   (4)
#define CTRL_COM_NSEC_MAX             (1000*1000*1000) // 1s

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
typedef struct {
    char            ver_buf[16];
    unsigned char   ver9512;        // SEB9512 network version
    int             set_ver_f;      // get version no
    int             set_para_f;     // set parameter
    int             set_para2_f;    // set parameter2
    int             set_can_f;      // set can parameter
    int             set_SIO_f;      // set rs232 parameter
    int             set_i2c_f;      // set i2c parameter
    int             set_spi_f;      // set spi parameter

    int             set_send_one;   // 1= ctlr_para -1= リモートSIO
    int             set_PWMaddNum;  // 0-16
    int             fw_stat;
} CTRL_COM_BOARD_INFO;


/**==========================================================================================
 *   グローバル変数形宣言
 **==========================================================================================*/
int                         f_ctrl_com = 0;     //main.cで使用(デバッグ用)
int                         SIO_com_cnt;        //main.cで使用(デバッグ用)
int                         SIO_com_err;        //main.cで使用(デバッグ用)
int                         SIO_deftime;        //main.cで使用(デバッグ用)
int                         SIO_deftime_min;    //main.cで使用(デバッグ用)
int                         SIO_deftime_max;    //main.cで使用(デバッグ用)
int                         Ctrl_com_stat = 0;  //ctrl_udp.cにて使用


/**==========================================================================================
 *   スタティック変数形宣言
 **==========================================================================================*/
static pthread_t            ctrl_sio_send_info;
static pthread_t            ctrl_sio_recv_info;
static pthread_t            ctrl_i2c_com_info;

static sem_t                sem_send;

static short                COM_snd_bf[20];     // SIO送信バッファ
static char                 COM_rcv_buf[80];    // SIO受信バッファ
//static int                  FW9512_stat  = 0;

static int                  set_ComType    = 0;  // 0:rs232c 1:I2C
static int                  set_BoardNum   = 0;  // 0:rs232c 1:I2C
static int                  Ctrl_SIO_speed = CTRL_COM_SIO_DEFAULT_SPEED;

static int                  fd_com         = 0;

static int                  init_rq_flg    = 0;
static int                  del_rq_flg     = 0;
static int                  ctrl_com_flg   = 0;

static SET_CONFIG_PARAM     ctrlCom_initParam[CTRL_BOARD_MAX];
static CTRL_COM_BOARD_INFO  ctrlCom_boardInfo[CTRL_BOARD_MAX];

/**==========================================================================================
 *   スタティック変数形宣言
 **==========================================================================================*/
#ifdef DEBUG_MESG
static void log_message( char *name, int bno, void *buf, uint size);
#else
#define log_message(name,bno,buf,size)
#endif

/**
 * @brief パラメータデータセット
 */
void set_para_fl(void)
{
    int  ix;
	int err;

    printf("   -----  set_para_fl()\n");
    init_ctrl_para( &ctrlCom_initParam[0] );
	
//	dump_ConfigParam(0);
    err = set_config("config.txt");
//	printf("set_config()=%d \n", err);
//	dump_ConfigParam(0);

    for(ix=0;ix<CTRL_BOARD_MAX;ix++){
        if (get_io_para_bit(ix))   {
            ctrlCom_boardInfo[ix].set_para_f   = 1;
        }
        if (get_ctrl_para_bit(ix))  {
            ctrlCom_boardInfo[ix].set_para2_f  = 1;
        }
        if (get_can_para_bit(ix))    {
            ctrlCom_boardInfo[ix].set_can_f    = 1;
        }
        if (get_rs232_para_bit(ix))  {
            ctrlCom_boardInfo[ix].set_SIO_f    = 1;
        }
        if (get_i2c_para_bit(ix))  {
            ctrlCom_boardInfo[ix].set_i2c_f    = 1;
        }
        if (get_spi_para_bit(ix))  {
            ctrlCom_boardInfo[ix].set_spi_f    = 1;
        }
        ctrlCom_boardInfo[ix].set_ver_f = 1;

        ctrlCom_boardInfo[ix].set_PWMaddNum = ctrlCom_initParam[ix].ctrlpara.dt[SET_PARA2_PWM_ADD_NUM];
    }

    set_ComType  = get_ctrl_connectType();
    set_BoardNum = get_ctrl_boardNum();
    if (get_com_speed() == CTRL_COM_SIO_DEFAULT_SPEED) Ctrl_SIO_speed = CTRL_COM_SIO_DEFAULT_SPEED;
    if (get_com_speed() == CTRL_COM_SIO_HIGH_SPEED)    Ctrl_SIO_speed = CTRL_COM_SIO_HIGH_SPEED;
}


/**
 *  @brief パラメータデータ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_para(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    CTRL_COM_BOARD_INFO  *pInfo  = &ctrlCom_boardInfo[bno];
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_PARAMETER;
    snd_bf->hd.info   = 0;

    if (pInfo->ver9512 < 4) {
        // disable parameter by low version
        pParam->iopara.bit &= 0x1FFF;
    }
    sz = data_pack(&pParam->iopara, (data_param_t*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);
    return (sz);
}

/**
 *  @brief パラメータ２データ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_para2(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_CTRL_SET;
    snd_bf->hd.info   = 0;

    sz = data_pack(&pParam->ctrlpara, (data_param_t*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);
    return ( sz );
}


/**
 *  @brief CAN パラメータデータ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_can(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_CAN_SET;
    snd_bf->hd.info   = 0;

    sz = data_pack(&pParam->can, (data_param_t*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);
    return (sz);
}

/**
 *  @brief remoteSIO パラメータデータ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_rSIO(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_RS232_SET;
    snd_bf->hd.info   = 0;

    sz = data_pack(&pParam->rs232, (data_param_t*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);

    return (sz);
}

/**
 *  @brief get Version message　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_ver(int bno)
{
    mctrl_dt_t  *snd_bf;

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_VERSION;
    snd_bf->hd.info   = 0;
    return(sizeof(snd_bf->hd));
}

#if (0)
/**
 *  @brief request rSIO data message　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_req_rSIO(int bno)
{
    mctrl_dtsio_t* snd_bf;
    int sz;

    snd_bf = (mctrl_dtsio_t*)COM_snd_bf;
    sz     = get_rSIO_send(bno,snd_bf);

    if (sz == 0) {
        snd_bf->hd.ver    = VER_E2S_MSG;
        snd_bf->hd.msg_no++ ;
        snd_bf->hd.d_id   = CMD_RS232_SEND;
        snd_bf->hd.info   = 0;
        snd_bf->sz        = 0;
        sz = 6;
    }

    return (sz);
}

/**
 *  @brief request SPI data message　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_req_SPI(int bno)
{
    mctrl_dtspi_t* snd_bf;
    int sz;

    snd_bf = (mctrl_dtspi_t*)COM_snd_bf;
    sz     = get_SPI_send(bno,snd_bf);

	snd_bf->sz = (sz - 6);

    if (sz == 0) {
        snd_bf->hd.ver    = VER_E2S_MSG;
        snd_bf->hd.msg_no++ ;
        snd_bf->hd.d_id   = CMD_SPI_SEND;
        snd_bf->hd.info   = 0;
        snd_bf->sz        = 0;
        sz = 6;
    }

    //16bit設定の際に0byteをC基板側に送信しないようにする
    if (snd_bf->sz == 0){
        sz = 0;
    }

    return (sz);
}


/**
 *  @brief I2C パラメータデータ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_i2c(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_I2C_SET;
    snd_bf->hd.info   = 0;

    sz = data_pack(&pParam->i2c, (struct _DATA_F_STR*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);
    return (sz);
}

/**
 *  @brief SPI パラメータデータ　送信バッファセット
 *  @retval    : 送信データサイズ
 */
int setbf_spi(int bno)
{
    mctrl_dt_t           *snd_bf;
    int                   sz;
    SET_CONFIG_PARAM     *pParam = &ctrlCom_initParam[bno];

    snd_bf            = (mctrl_dt_t *)COM_snd_bf;
    snd_bf->hd.ver    = VER_E2S_MSG;
    snd_bf->hd.msg_no = 0;
    snd_bf->hd.d_id   = CMD_SPI_SET;
    snd_bf->hd.info   = 0;

    sz = data_pack(&pParam->spi, (struct _DATA_F_STR*)&snd_bf->dt[0]);
    sz = sz + sizeof(snd_bf->hd);
    return (sz);
}
#endif // #if(0)

/**
 *  @brief パラメータ設定チェック（データセット無し）
 *  @retval == 0 : 無し
 *  @retval != 0 : あり
 */
int ch_para(int bno)
{
    int                   res   = 0;
    CTRL_COM_BOARD_INFO  *pInfo = &ctrlCom_boardInfo[bno];

    if( pInfo->set_ver_f == 1)  {
        res |= 1;
    }
    if( pInfo->set_para_f == 1) {
        res |= 0x10;
    }
    if( pInfo->set_para2_f == 1) {
        res |= 0x20;
    }
    if( pInfo->set_can_f == 1) {
        res |= 0x80;
    }
    if( pInfo->set_SIO_f == 1) {
        res |= 0x100;
    }
    if( pInfo->set_i2c_f == 1) {
        res |= 0x200;
    }
    if( pInfo->set_spi_f == 1) {
        res |= 0x800;
    }
    if (res) {
        req_send_com();
    }
    return (res);
}



/**
 *  @brief パラメータ設定（有無）チェック
 *  @retval = 0   : パラメータ設定なし
 *  @retval 0以上 :パラメータ送信データサイズ
 *
 */
int check_para(int bno)
{
    int                   size;
    CTRL_COM_BOARD_INFO  *pInfo = &ctrlCom_boardInfo[bno];

    if( pInfo->set_send_one == 0) {
        if ( pInfo->set_ver_f== 1) {
            pInfo->set_ver_f    = 2;
            size                = setbf_ver(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
        if ( pInfo->set_para_f == 1) {
            pInfo->set_para_f   = 2;
            size                = setbf_para(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
        if ( pInfo->set_para2_f == 1) {
            pInfo->set_para2_f = 2;
            size                = setbf_para2(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
/*
        if ( pInfo->set_can_f == 1) {
            pInfo->set_can_f    = 2;
            size                = setbf_can(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
        if ( pInfo->set_i2c_f == 1) {
            pInfo->set_i2c_f    = 2;
            size                = setbf_i2c(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
        if ( pInfo->set_spi_f == 1) {
            pInfo->set_spi_f    = 2;
            size                = setbf_spi(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
        if ( pInfo->set_SIO_f == 1) {
            pInfo->set_SIO_f    = 2;
            size                = setbf_rSIO(bno);
            pInfo->set_send_one = 1;
            return (size);
        }
		if( 0 != get_rSIO_sz(bno) ){
            size                 = setbf_req_rSIO(bno);
            pInfo->set_send_one  = 0;
            return (size);
        }
        if( 0 != get_SPI_sz(bno) ){
            size                 = setbf_req_SPI(bno);
            pInfo->set_send_one  = 0;
            return (size);
        }*/
    }
    return (0);
}

/**
 *  @brief パラメータ設定応答確認
 *  @retval == 0 : OK
 *  @retval != 0 : NG
 */
int ans_para(int bno)
{
    mctrl_dt_t           *r_buf;
    CTRL_COM_BOARD_INFO  *pInfo = &ctrlCom_boardInfo[bno];


    if( pInfo->set_ver_f == 2)  {

        r_buf          = (mctrl_dt_t*)COM_rcv_buf;
        pInfo->ver9512 = r_buf->hd.ver;

        if( strlen(pInfo->ver_buf) == 0 ) {
            memcpy( pInfo->ver_buf, r_buf->dt, sizeof(pInfo->ver_buf) );
        }

        if (pInfo->ver9512 < 5) {
            // ver 1.10
            pInfo->set_para2_f = 0;
        }

        pInfo->set_ver_f = 0;

    }

    if( pInfo->set_para_f == 2) {
        pInfo->set_para_f  = 0;
    }
    if( pInfo->set_para2_f == 2) {
        pInfo->set_para2_f = 0;
    }
    if( pInfo->set_can_f == 2) {
        pInfo->set_can_f = 0;
    }
    if( pInfo->set_SIO_f == 2) {
        pInfo->set_SIO_f = 0;
    }
    if( pInfo->set_i2c_f == 2) {
        pInfo->set_i2c_f = 0;
    }
    if( pInfo->set_spi_f == 2) {
        pInfo->set_spi_f = 0;
    }
    return (0);
}

/**
 *  @brief パラメータ設定応答確認エラー
 *  @retval == 0 : OK
 *  @retval != 0 : NG
 */
int err_para(int bno)
{
	int ans = 0;
	
//	printf("   -----  err_para(%d)\r\n", bno);
    CTRL_COM_BOARD_INFO  *pInfo = &ctrlCom_boardInfo[bno];

    if( pInfo->set_ver_f == 2)  {
        pInfo->set_ver_f = 1;
    	ans |= 0x001;
    }
    if( pInfo->set_para_f == 2) {
        pInfo->set_para_f = 1;
     	ans |= 0x002;
   }
    if( pInfo->set_para2_f == 2) {
        pInfo->set_para2_f = 1;
    	ans |= 0x004;
    }
    if( pInfo->set_can_f == 2) {
        pInfo->set_can_f = 1;
    	ans |= 0x008;
    }
    if( pInfo->set_SIO_f == 2) {
        pInfo->set_SIO_f = 1;
    	ans |= 0x010;
    }
    if( pInfo->set_i2c_f == 2) {	//-------------------- 2016/06/30
        pInfo->set_i2c_f = 1;
    	ans |= 0x020;
    }
    if( pInfo->set_spi_f == 2) {	//-------------------- 2016/06/30
        pInfo->set_spi_f = 1;
    	ans |= 0x040;
    }
	
    return (ans);
}

/**
 *  @brief  ＳＩＯ通信　送信タスク
 */
void  t_ctrl_sio_send( unsigned long arg )
{
    int                   sndSz;
    CTRL_COM_BOARD_INFO  *pInfo  = &ctrlCom_boardInfo[CTRL_BOARD_SIO];

    // タイマー変数定義
    struct timeval        tv1;
    struct timeval        tv2;


    sem_init( &sem_send, 0,0 );  /// semaphore 生成

    usleep(10*1000);

    ch_para(CTRL_BOARD_SIO);

    while (f_ctrl_com) {

        // 送出時間調整
        CHECK_POINT(300,0);
        gettimeofday(&tv1,NULL);

        sndSz = 0;


        sem_wait(&sem_send);

        // 送信データ取出
        if (f_ctrl_com == 1) {

            // 送信
            sndSz = check_para(CTRL_BOARD_SIO);
            CHECK_POINT(303,(short)sndSz);

            do{

                if( sndSz != 0 ){

                    log_message("232 send", CTRL_BOARD_SIO, COM_snd_bf,sndSz);

//                 if ((COM_snd_bf[1] & 0x00FF)== 0x0010) {
//                      printf("sioSend (%d) [%04X]%04X %04X %04X %04X %04X\r\n",sndSz, COM_snd_bf[1], COM_snd_bf[2]
//                            , COM_snd_bf[3], COM_snd_bf[4], COM_snd_bf[5], COM_snd_bf[6]);
//                  }
                    sndSz = sioSendTransparent( fd_com, (char*)COM_snd_bf, sndSz );
                }
                CHECK_POINT(304,(short)sndSz);

                if( pInfo->set_send_one == 0 ){
                    memset(&COM_snd_bf, 0, sizeof(COM_snd_bf));
					sndSz = get_ctrl_eu2sdata( CTRL_BOARD_SIO, (char*)COM_snd_bf, sizeof(COM_snd_bf), (pInfo->set_PWMaddNum<<8)|(Ctrl_com_stat) );
                }
            	else {
                    break;
                }

            }while( sndSz != 0 );

#ifdef __DUMP__
            if (Get_dump_mode() == 0) {
                dump_out((unsigned short*)COM_snd_bf, sndSz);
            }
            else {
                if (COM_snd_bf[1]!= 0x0010) {
                    dump_out((unsigned short*)COM_snd_bf, sndSz);
                }
            }
#endif

            gettimeofday( &tv2, NULL );
            SIO_deftime = Def_time( &tv2, &tv1 );

            if( sndSz != 0 ){
//				input_sio_stat(SIO_deftime);
            }

            while(pInfo->set_send_one) {
                usleep(1*1000);
            }
            CHECK_POINT(308,0);
        }
    }

    sem_destroy(&sem_send);
    CHECK_POINT(309,0);

}

/**
 * @brief ＳＩＯ通信　受信タスク
 */
void  t_ctrl_sio_recv( unsigned long arg )
{
    int                   res;
    int                   rcv_cnt;
    mctrl_dtcan_t        *buf;
    mctrl_dt16_t         *buf16;
    mctrl_dt16_t          sens_dt;             // sensor  data
    CTRL_COM_BOARD_INFO  *pInfo  = &ctrlCom_boardInfo[CTRL_BOARD_SIO];

    // 変数　初期値設定
    rcv_cnt         = 0;

    reset_mxmi_time();

    f_ctrl_com = 1;

    while (f_ctrl_com > 0) {

        // SIO通信　受信
//        printf("*");
        res = sioRecvTransparent( fd_com, COM_rcv_buf, sizeof(COM_rcv_buf), &rcv_cnt, 200 );
        CHECK_POINT(311,(short)res);

        if( res <= 0 ){


#ifdef __DUMP__
            dump_out_R((unsigned char*)COM_rcv_buf,-1);
#endif
            err_para(CTRL_BOARD_SIO);
            SIO_com_err++;
            Ctrl_com_stat  = 0;
            pInfo->fw_stat = 0;

        }
    	else {
            // 受信データセット
            log_message("232 recv", CTRL_BOARD_SIO, (short*)COM_rcv_buf,rcv_cnt);

#ifdef __DUMP__
            if (Get_dump_mode() == 0) {
                dump_out_R((unsigned char*)COM_rcv_buf,rcv_cnt);
            }
            else {
                if (COM_rcv_buf[2]!= 0x11) {
                    dump_out_R((unsigned char*)COM_rcv_buf,rcv_cnt);
                }
            }
#endif
            ans_para(CTRL_BOARD_SIO);
            ch_para(CTRL_BOARD_SIO);


            buf            = (mctrl_dtcan_t*)COM_rcv_buf;
            buf16          = (mctrl_dt16_t*)COM_rcv_buf;
            pInfo->fw_stat = buf->hd.info;
//    		printf("recive size = %d d-id = %02X \n", rcv_cnt, buf->hd.d_id);

            switch (buf->hd.d_id) {

                case RESP_RS232_RECV:
//					set_rSIO_recv(CTRL_BOARD_SIO,(mctrl_dtsio_t*)buf);
                    break;

                case RESP_SENS_DATA:
                    if( pInfo->set_send_one <= 0 ) {
                        if (pInfo->ver9512 >= 4) {
                            sens_dt.hd = buf16->hd;
                            data_unpack((data_16_t*)&buf16->b_ptn, (data_16_t*)&sens_dt.b_ptn);
                            rcv_cnt = sizeof(mctrl_dt16_t);
                        }
                        else {
                            buf16->b_ptn = 0xF0FF;
                            memcpy(&sens_dt, buf16, rcv_cnt);
                        }
						set_ctrl_s2eudata(CTRL_BOARD_SIO,(char*)&sens_dt, rcv_cnt, 1);
                    }
                    Ctrl_com_stat = 1;
                    break;

                case RESP_SPI_RECV:
//					set_SPI_recv(CTRL_BOARD_SIO,(mctrl_dtspi_t*)buf);
                    break;

                default:
                    if( pInfo->set_send_one <= 0 ){
//						set_ctrl_s2eudata(CTRL_BOARD_SIO,COM_rcv_buf, rcv_cnt, 1);
                    }
                    break;

            }

            CHECK_POINT(312,(short)rcv_cnt);
            SIO_com_cnt++;
        }
        pInfo->set_send_one = 0;
        CHECK_POINT(318,0);
    }

    if (fd_com > 0) {
        sioClose( fd_com );
    }

    f_ctrl_com = 0;
    sem_post(&sem_send);    // Release waitting of send
    CHECK_POINT(319,0);

}


/**
 * @brief I2C通信タスク
 */


void  t_ctrl_i2c_com( unsigned long arg )
{
    int                   i2cAddr;
    int                   sndSz;
    unsigned char         rcvSz;
    int                   result;
    struct timespec       abs_timeout;

    int                   size, count;
    mctrl_dt16_t         *pdt16;
    mctrl_dt16_t          sens_dt[CTRL_BOARD_MAX];             // sensor  data
    int                   status_bit, lastbd_bit;

    int                   bno;
    CTRL_COM_BOARD_INFO  *pInfo;

    // タイマー変数定義
    struct timeval        tv1;
    struct timeval        tv2;

    // 変数　初期値設定
    size         = 0;
    memset( &sens_dt, 0, sizeof(sens_dt));
    lastbd_bit      = 1<<(set_BoardNum-1);

    sem_init( &sem_send, 0,0 );  /// semaphore 生成

    usleep(10*1000);

    for(bno=0;bno<set_BoardNum;bno++){
        ch_para(bno);
    }

    f_ctrl_com = 1;

    clock_gettime(CLOCK_REALTIME, &abs_timeout);

    while (f_ctrl_com > 0) {

        gettimeofday(&tv1,NULL);

        sndSz = 0;

        // 送信データ取出
        if (f_ctrl_com == 1) {

            abs_timeout.tv_nsec += CTRL_COM_I2C_CYC_WAIT;
            if( abs_timeout.tv_nsec >= CTRL_COM_NSEC_MAX ){
                abs_timeout.tv_sec++;
                abs_timeout.tv_nsec -= CTRL_COM_NSEC_MAX;
            }
            sem_timedwait( &sem_send, &abs_timeout );

            if( f_ctrl_com <= 0 ) {
                break;
            }

            for(bno=0;bno<set_BoardNum;bno++){

                pInfo      = &ctrlCom_boardInfo[bno];
                status_bit = (1<<bno);

                i2cAddr = CTRL_COM_I2C_SLAVE_ADDR + bno;
                ioctl( fd_com, I2C_SLAVE, i2cAddr );

                // 送信
                sndSz = check_para(bno);

                do{

                    if( sndSz != 0 ){


                        log_message("i2c send", bno, COM_snd_bf, sndSz );

                        result = write( fd_com, (char*)COM_snd_bf, sndSz );

                    }
                    if( pInfo->set_send_one == 0 ){
                        sndSz = get_ctrl_eu2sdata( bno, (char*)COM_snd_bf, sizeof(COM_snd_bf), (pInfo->set_PWMaddNum<<8)|(Ctrl_com_stat&status_bit) );
                    }else {
                         break;
                    }
                }while( sndSz != 0 );

                // 受信
                count = 0;
                do{
                    rcvSz   = 0;
                    size = sizeof(rcvSz);
                    result  = read( fd_com, &rcvSz, size );
                    //printf("I2C recv1(%3d/%3d) addr:%Xh size:%2d - %02X\n", result, errno, i2cAddr, size, (int)rcvSz );

                    if( (size == result)&&(rcvSz != 0) ){

                        size = (int)(rcvSz & CTRL_COM_I2C_RSIZE_MASK) ;

                        result = read( fd_com, COM_rcv_buf, size );
                        if(size == result ){

                            log_message("i2c recv", bno, COM_rcv_buf,size);

                            ans_para(bno);
                            ch_para(bno);

                            pdt16          = (mctrl_dt16_t *)COM_rcv_buf;
                            pInfo->fw_stat = pdt16->hd.info;

                            switch( pdt16->hd.d_id ){

                                case RESP_RS232_RECV:
//									set_rSIO_recv(bno,(mctrl_dtsio_t*)pdt16);
                                    break;

                                case RESP_SENS_DATA:
                                    if( pInfo->set_send_one <= 0 ){
                                        sens_dt[bno].hd = pdt16->hd;
                                        data_unpack((data_16_t*)&pdt16->b_ptn, (data_16_t*)&sens_dt[bno].b_ptn);
                                        set_ctrl_s2eudata(bno, (char*)&sens_dt[bno], sizeof(mctrl_dt16_t), ((lastbd_bit&status_bit)!=0) );
                                    }
                                    Ctrl_com_stat |= status_bit;
                                    break;

                                case RESP_SPI_RECV:
//									set_SPI_recv(bno,(mctrl_dtspi_t*)pdt16);
                                    break;

                                case CMD_VERSION  :
                                case CMD_PARAMETER:
                                case CMD_CAN_SET  :
                                case CMD_RS232_SET:
                                case CMD_I2C_SET:
                                case CMD_SPI_SET:
                                case CMD_CTRL_SET :
                                case CMD_OFF_SET  :
                                    break;

                                default:
                                    if( pInfo->set_send_one <= 0 ){
                                        set_ctrl_s2eudata(bno, COM_rcv_buf, size, 1);
                                    }
                                    break;

                            }
                        }
                    }
                    if( size != result ){
                        err_para(bno);
                        Ctrl_com_stat &= ~status_bit;
                        pInfo->fw_stat = 0;
#if 0
                        printf("\n");
                        printf("I2C recv1(%3d/%3d) addr:%Xh size:%2d ERROR!! ERROR!! ERROR!! ERROR!! ERROR!! ERROR!! ERROR!! ERROR!!\n", result, errno, i2cAddr, size );
#endif
                        break;
                    }
                    count++;
                } while( (rcvSz & CTRL_COM_I2C_RNEXT_FLAG)&&(count<CTRL_COM_I2C_RECV_COUNT_MAX) );

                pInfo->set_send_one = 0;
            }
            gettimeofday( &tv2, NULL );
            SIO_deftime = Def_time( &tv2, &tv1 );

//			input_sio_stat(SIO_deftime);
        }
    }

    sem_destroy(&sem_send);
    if (fd_com > 0) {
        close( fd_com );
        fd_com = 0;
    }
    f_ctrl_com = 0;

}

/**
 *  @brief 制御ボード通信処理　初期化関数
 */
int init_ctrl_com( void )
{
    int ix;
    int res = -1;

    printf("init_ctrl_com() f_ctrl_com = %d\r\n", f_ctrl_com);
    if (f_ctrl_com == 0) {

        set_para_fl();

#ifdef __DUMP__
        dump_init();
#endif
//		init_sio_stat(10, 10);

        SIO_com_cnt        = 0;
        SIO_com_err        = 1;
        SIO_deftime        = 0;
        Ctrl_com_stat      = 0;

        if( set_ComType == SET_COMTYPE_RS232C ){

            ctrlCom_boardInfo[CTRL_BOARD_SIO].fw_stat = 0;

/*			if( 0 != get_rSIO_udpport(CTRL_BOARD_SIO) ){
                init_sio_udp9512_rq( CTRL_BOARD_SIO, get_rSIO_udpport(CTRL_BOARD_SIO) );     // rSIO #2 initalize
            }
            if( 0 != get_SPI_udpport(CTRL_BOARD_SIO) ){
                init_ctl_spi_rq( CTRL_BOARD_SIO, get_SPI_udpport(CTRL_BOARD_SIO) );          // SPI initalize
            }*/

            // SIO device 初期化
            fd_com = sioInit( CTRL_COM_SIO_DEVNAME, Ctrl_SIO_speed, 8, NOPARITY, ONESTOPBIT, 0 );

            if (fd_com > 0) {
                res = pthread_create( &ctrl_sio_send_info, NULL, (void(*))t_ctrl_sio_send, (void *)NULL );
                res = pthread_create( &ctrl_sio_recv_info, NULL, (void(*))t_ctrl_sio_recv, (void *)NULL );
            }
        }
    	else if( set_ComType == SET_COMTYPE_I2C ){

            for( ix=0; ix<set_BoardNum; ix++ ){

                ctrlCom_boardInfo[ix].fw_stat = 0;

/*				if( 0 != get_rSIO_udpport(ix) ){
                    init_sio_udp9512_rq(ix,get_rSIO_udpport(ix));     // rSIO #2 initalize
                }
                if( 0 != get_SPI_udpport(ix) ){
                    init_ctl_spi_rq(ix,get_SPI_udpport(ix));          // SPI initalize
                }*/
            }

            // I2C device 初期化
            fd_com = open( CTRL_COM_I2C_MASTER_DEV, O_RDWR );

            if (fd_com > 0) {
                res = pthread_create( &ctrl_i2c_com_info, NULL, (void(*))t_ctrl_i2c_com, (void *)NULL );
            }
    		else {
                printf("%s open error %d\n", CTRL_COM_I2C_MASTER_DEV, errno);
            }

        }
    }

    return(res);
}

#if (0)
/**
 *  @brief 制御ボード通信処理　初期化関数
 *  @param[in] flg      : 0x01:UDP経由 0x02:PROC経由
 */
int init_ctrl_com_rq( int flg )
{
    init_rq_flg |= flg;
    return(0);
}

/**
 *  @brief 制御ボード通信処理　初期化関数
 */
void init_ctrl_com_ex()
{
    int  flg;

    if( init_rq_flg != 0 ){

        flg         = init_rq_flg;
        init_rq_flg = 0;

#ifdef __LOG__
        err_log = fopen("/tmp/err_log.txt","a");
        fputs("error log start  \n",err_log);
#endif
        if( ctrl_com_flg == 0 ){
            init_ctrl_com();
        }
        ctrl_com_flg |= flg;
    }
}
#endif // #if (0)

/**
 *  @brief 制御ボード通信処理　終了関数
 */
int del_ctrl_com()
{
    int  ix;

    if (f_ctrl_com == 0) {
        return(-1);
    }

    if (f_ctrl_com > 0)
    {
        f_ctrl_com = -1;
        if( set_ComType == SET_COMTYPE_I2C ){
            sem_post(&sem_send);    // Release waitting of send
        }
        while (f_ctrl_com != 0) {
            usleep(1*1000);
        }
    }

    if( set_ComType == SET_COMTYPE_RS232C ){

//		del_sio_udp9512_rq(CTRL_BOARD_SIO);
//		del_ctl_spi_rq(CTRL_BOARD_SIO);

        if (f_ctrl_com == 0) {
            pthread_join( ctrl_sio_recv_info, NULL );
        }else {
            return(1);
        }

        if (f_ctrl_com == 0) {
            pthread_join( ctrl_sio_send_info, NULL );
        }else {
            return(1);
        }

    }else {
        for( ix=0; ix<set_BoardNum; ix++ ){
//			del_sio_udp9512_rq(ix);
//			del_ctl_spi_rq(ix);
        }

        if (f_ctrl_com == 0) {
            pthread_join( ctrl_i2c_com_info, NULL );
        }else {
            return(1);
        }
    }
//	output_sio_stat();

    Ctrl_com_stat   = 0;

#ifdef __DUMP__
    dump_end();
#endif

    return(0);
}

#if (0)
/**
 * @brief  コントロールSIOタスク削除リクエスト
 *
 * @param flg
 *
 * @return
 */
int del_ctrl_com_rq(int flg)
{
    del_rq_flg |= flg;
    return(0);
}

/**
 * @brief コントロールSIOタスク削除命令
 */
void del_ctrl_com_ex()
{
    int flg;

    if (del_rq_flg != 0) {

        flg           =  del_rq_flg;
        del_rq_flg    =  0 ;
        ctrl_com_flg  &= (~flg);

        if (!ctrl_com_flg) {
            del_ctrl_com();
        }
#ifdef __LOG__
        fprintf(err_log,"error log end \n");
        fclose(err_log);
#endif
    }
}

#endif // #if (0)
/**
 * @brief  deftimeの最大最小の初期化
 */
void reset_mxmi_time()
{
    SIO_deftime_min = 0x7fffffff;
    SIO_deftime_max = 0;
}

/**
 * @brief ファームウェアバージョンの取得
 *
 * @param ver
 *
 * @return
 */
int get_ver_fw(int bno, char* ver)
{
    int                   ans;
    CTRL_COM_BOARD_INFO  *pInfo = &ctrlCom_boardInfo[bno];

    if (pInfo->ver_buf[0]!=0) {
        memcpy(ver,&pInfo->ver_buf[10] ,6);
   }
    ans = strlen(ver);

    return ans;
}


/**
 * @brief  SIO送信シグナルリクエスト
 */
void req_send_com(void)
{
    if (f_ctrl_com == 1) {
        sem_post(&sem_send);	// 送信シグナル
    }
}

/**
 * @brief C基板ステータス取得関数
 *
 * @return
 */
int get_9512FW_stat(void)
{
    return( ctrlCom_boardInfo[CTRL_BOARD_SIO].fw_stat );
}

#ifdef DEBUG_MESG
/**
 * @brief  シリアルへの送信データのダンプ
 *
 * @param size
 */
static unsigned char log_enableTbl[] = {
    CMD_VERSION,       // (0x00)
    CMD_PARAMETER,     // (0x01)
    CMD_CAN_SET,       // (0x02)
    CMD_RS232_SET,     // (0x03)
    CMD_CTRL_SET,      // (0x04)
    CMD_OFF_SET,       // (0x05)
    CMD_SPI_SET,       // (0x06)
    CMD_I2C_SET,       // (0x07)

//    CMD_CTRL_DATA,     // (0x10)
//    CMD_SBUS_DATA,     // (0x12)
//    CMD_CAN_SEND,      // (0x20)
//    CMD_RS232_SEND,    // (0x30)
//    CMD_SPI_SEND,      // (0x40)
//    CMD_I2C_SEND,      // (0x50)
//    CMD_I2C_RECV_REQ,    // (0x52)
//    CMD_PI_REQ,        // (0x80)
//    CMD_ECHO,          // (0xFF)

//    RESP_SENS_DATA,    // (0x11)
//    RESP_CAN_RECV,     // (0x21)
//    RESP_RS232_RECV,   // (0x31)
//    RESP_SPI_RECV,     // (0x41)
//    RESP_I2C_RECV,     // (0x51)
//    RESP_I2C_RECV_REQ, // (0x53)

//    RESP_PI_DATA,      // (0x81)
};

static void log_message( char *name, int bno, void *buf, uint size)
{
    int               ix;
    mctrl_dt_t       *pmsg = (mctrl_dt_t *)buf;
    unsigned short   *pdt  = (unsigned short *)pmsg->dt;
    unsigned char    *pcdt;
    struct timespec   ts;

    //シリアルへの送信データダンプ
    for( ix=0;ix<sizeof(log_enableTbl);ix++ ){
        if( pmsg->hd.d_id == log_enableTbl[ix] ){
            break;
        }
    }
    if( ix == sizeof(log_enableTbl) ){
        return;
    }

    if( (pmsg->hd.d_id == CMD_CTRL_DATA)&&(size==6) ){
        return;
    }

    printf("%s bno(%d) ", name, bno );
    printf("size %2d : ", size);
    if(size!=0){

        clock_gettime(CLOCK_REALTIME, &ts);

        printf("%08d.%03d ", (int)ts.tv_sec, (int)(ts.tv_nsec/1000/1000) );
        printf("id:%02X ", pmsg->hd.d_id );
        size -= sizeof(mctrl_hd_t);

        for(ix =0;ix < size/2 ; ix++){
            if(((ix%32)==0)&&(ix!=0)){
                printf("\n");
            }
            printf("%04X ", pdt[ix] );
        }
        if(size%2){
            pcdt = (unsigned char *)&pdt[ix];
            printf("%02X ", *pcdt );
        }
        printf("\n");
    }

}
#endif /* DEBUG_MESG */
