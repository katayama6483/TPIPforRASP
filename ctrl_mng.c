/**
 * @file  ctrl_mng.c
 * @brief UDP : Control data manegement module (for Raspverry Pi)
 *
 * @author Katayama
 * @date 2018-11-02
 * @version 1.00 : 2018/11/02 katayama
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */


#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include "time_sub.h"
#include "ctrl_mng.h"
#include "ctrl_com.h"
#include "def_jtp.h"
#include "def_mctrl_com16.h"
#include "que_buf.h"
#include "data_pack.h"

#define  JTCP_VER_NUM (130)


#ifdef __DUMP__
#include "dump.h"
static int dump_f = 0;
#endif

#define ___TRACE_INIT___
#include "trace.h"

/**==========================================================================================
 *   #define
 **==========================================================================================*/
#define MOTOR_PWM_ONBOARD_NUM  (4)   // 制御ユニットオンボード PWM ch 数
#define MOTOR_PWM_ADD_NUM      (16)  // 制御ユニット増設 PWM ch 数
#define MOTOR_CTRLDATA_SIZE    (sizeof(mctrl_hd_t)+sizeof(short)+(sizeof(short)*(1+1+4))) // 18byte      -> hd(4)+b_ptn(2)+ctrl_do(2)+nouse(2)+pwm4ch(2*4)
#define MOTOR_ADDPWM_SIZE(n)   (sizeof(mctrl_hd_t)+sizeof(short)+(sizeof(short)*(n)))     // 6byte+(2*n) -> hd(4)+b_ptn(2)+pwm*n
#define MOTOR_ADDPWM_OFFSET    (6)                                                        // ctrl_do(1) + nouse(1) + pwm4ch(4)

#define CTRL_DATA_MAX          (1024)

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
struct ctrl_data_STR {
    struct timeval     e2s_tm;                        // Ether->SIO time stamp
    unsigned short     owne_mask[CTRL_BOARD_MAX];     // ctrl data mask bit ('1'= USER set)
    unsigned short     owne_addmask[CTRL_BOARD_MAX];  // ctrl data mask bit ('1'= USER set)
    unsigned short     owne_do_mask[CTRL_BOARD_MAX];  // DO & Ctrl bit mask ('1'= USER set)
    int                e2s_sz;                        // data size
    char               e2s_data[CTRL_DATA_MAX];       // control data(JTCP)
    char               u2s_data[CTRL_DATA_MAX];       // control data(USER)
    char               eu2s_data[CTRL_DATA_MAX];      // control data(JTCP+USER)
    char               eu2s_predata[CTRL_DATA_MAX];   // previous control data(JTCP+USER)
    struct timeval     s2e_tm;                        // SIO->Ether time stamp
    int                s2e_sz;                        // data size
    char               s2e_data[CTRL_DATA_MAX];       // data
};

typedef struct {
    mctrl_dt16_t ctrl;
    mctrl_dt16_t addPwm;
} CTRL_DATA_TEMP;

/**==========================================================================================
 *   グローバル変数形宣言
 **==========================================================================================*/


/**==========================================================================================
 *   スタティック変数形宣言
 **==========================================================================================*/
static int                    ctrlUdpRcvFlg   = 0;

static pthread_mutex_t        ctrlUdpE2sMutex;

static struct ctrl_data_STR   ctrlUdpCtrlData;


/* 送信パラメータ */
static unsigned char          ctrlUdpSeq;
static unsigned long          ctrlUdpTMstmp;
static struct timeval         ctrlUdpLastTime;

typedef struct {
    def_que_t    eu2s;            // Ether,USER -> SIO
    def_que_t    eu2s_can;        // Ether,USER -> SIO(CAN)
    def_que_t    s2e;             // SIO -> Ether
} CONTROL_MESSAGE_QUEUES;

static CONTROL_MESSAGE_QUEUES  ctrlMsgQueue[CTRL_BOARD_MAX];

static unsigned short add_pwm_bptn[16+1] = {
                                             0x0000,
                                             0x0001, 0x0003, 0x0007, 0x000F,
                                             0x001F, 0x003F, 0x007F, 0x00FF,
                                             0x01FF, 0x03FF, 0x07FF, 0x0FFF,
                                             0x1FFF, 0x3FFF, 0x7FFF, 0xFFFF
                                           };

/**==========================================================================================
 *   外部参照関数プロタイプ宣言
 **==========================================================================================*/

/**==========================================================================================
 *   スタティック関数プロタイプ宣言
 **==========================================================================================*/



/**
 * @brief  制御データ管理情報 初期化関数
 * @fn     int init_ctrl_mng()
 * @retval == 0 : OK
 * @retval ==-1 : error
 *
 */
int init_ctrl_mng()
{
    int  ix;
	int  err;

	pthread_mutex_init(&ctrlUdpE2sMutex, NULL);         // ctrlUdpCtrlDataアクセス制御(mutex)生成

	if (ctrlUdpRcvFlg == 0) {
		for(ix=0;ix<CTRL_BOARD_MAX;ix++){
			Init_que(&ctrlMsgQueue[ix].eu2s);
			Init_que(&ctrlMsgQueue[ix].eu2s_can);
			Init_que(&ctrlMsgQueue[ix].s2e);
		}
		ctrlUdpRcvFlg = 1;
		err = init_ctrl_com();
        return(err);
	}
    return(-1);
}

/**
 * @brief  制御データ管理情報  削除関数
 * @fn     int del_ctrl_mng()
 * @retval == 0 : OK
 * @retval == 1 : error
 *
 */
int del_ctrl_mng()
{
	int err;
	
	ctrlUdpRcvFlg = 0;
	pthread_mutex_destroy(&ctrlUdpE2sMutex);
	err = del_ctrl_com();

    return(err);
}

/**
 * @brief  ctrlUdpCtrlData（Ether -> SIO) 情報格納 関数
 * @fn     int set_ctrl_e2sdata(char *buf, int size)
 * @param  buf  : hostPC -> SEB9512 制御情報 pointer
 * @param  size : 制御情報サイズ
 * @retval == 0 : error
 * @retval != 0 : OK 制御情報サイズ
 *
 */

int set_ctrl_eu2sdata(char *buf, int size)
{
    mctrl_hd_t     *pHd;
    jtcp_out_t     *pCtrlMes;
    CTRL_DATA_TEMP *pOwneTmp, *pCtrlTmp;
    int             bno, ix, cpsz;
    unsigned short  mask, do_mask;

    pHd = (mctrl_hd_t*)   buf;

    if ((size == 0)||(size > CTRL_DATA_MAX)) {
        return (0);
    }

    bno = pHd->info;

    if( 0 == pthread_mutex_lock( &ctrlUdpE2sMutex) ){

        if (pHd->d_id == CMD_CTRL_DATA) {

            ctrlUdpCtrlData.e2s_sz = size ;

            pCtrlMes = (jtcp_out_t *)buf;
            pCtrlTmp = (CTRL_DATA_TEMP *)ctrlUdpCtrlData.eu2s_data;
            pOwneTmp = (CTRL_DATA_TEMP *)ctrlUdpCtrlData.e2s_data;

            // data ID = ctrl data(cyclic send data)
            for( ix=0; ix<CTRL_BOARD_MAX;ix++ ){
                mask = ~ctrlUdpCtrlData.owne_mask[ix];

                pCtrlTmp[ix].ctrl.hd.d_id    = CMD_CTRL_DATA;
                pCtrlTmp[ix].ctrl.b_ptn     &= ~mask;
                pCtrlTmp[ix].addPwm.hd.d_id  = 0;
            }

            for( ix=0; (ix<CTRL_BOARD_MAX)&&(size!=0);ix++ ){


                // ヘッダーコピー
                cpsz = sizeof(mctrl_hd_t);
                if( cpsz <= size ){

                    mask    = ~ctrlUdpCtrlData.owne_mask[ix];
                    do_mask = ~ctrlUdpCtrlData.owne_do_mask[ix];

                    memcpy( &pCtrlTmp[ix].ctrl.hd, &pCtrlMes->hd, cpsz);
                    if( ix == 0 ){
                        size -= cpsz; // ヘッダはデータ先頭のみ存在するので、2枚目以降はデータサイズから引かない
                    }
                }else {
                    break;
                }

                // b_ptnコピー
                cpsz = sizeof(pCtrlMes->b_ptn);
                if( cpsz <= size ){
                    pOwneTmp[ix].ctrl.b_ptn = pCtrlMes->b_ptn & mask;
                    if( ix == 0 ){
                        size -= cpsz; // ヘッダとb_ptnはデータ先頭のみ存在するので、2枚目以降はデータサイズから引かない
                    }
                }else {
                    break;
                }


                // DO+NOUSE+オンボードPWM4chコピー
                cpsz = sizeof(pOwneTmp[ix].ctrl.dt[0])*6;
                if( cpsz <= size ){
                    memcpy( &pOwneTmp[ix].ctrl.dt[0], &pCtrlMes->dt[ix].d_out, cpsz );
                    size -= cpsz;

                    data_unpack2unpack_arb( (data_16_t*)&pCtrlTmp[ix].ctrl.b_ptn, (data_16_t*)&pOwneTmp[ix].ctrl.b_ptn, mask );
                    if( ctrlUdpCtrlData.owne_mask[ix] & CTRL_DATA_BPTN_DOCTRL ){
                        pCtrlTmp[ix].ctrl.dt[0] = (pCtrlTmp[ix].ctrl.dt[0] & do_mask)|(pOwneTmp[ix].ctrl.dt[0] & (~do_mask));
                    }

                }else {
                    break;
                }

                // 増設PWM最大16chコピー
                if( sizeof(pOwneTmp[ix].addPwm.dt[0]) <= size ){

                    mask = ~ctrlUdpCtrlData.owne_addmask[ix];

                    memcpy( &pCtrlTmp[ix].addPwm.hd, &pCtrlMes->hd, sizeof(mctrl_hd_t) );
                    pCtrlTmp[ix].addPwm.hd.d_id = CMD_SBUS_DATA;

                    pOwneTmp[ix].addPwm.b_ptn   = mask; // 制御ボードに実際送られる値は、GETで設定する

                    cpsz = size;
                    if( sizeof(pOwneTmp[ix].addPwm.dt) < cpsz ){
                        cpsz = sizeof(pOwneTmp[ix].addPwm.dt);
                    }
                    memcpy( &pOwneTmp[ix].addPwm.dt[0], &pCtrlMes->dt[ix].PWM2[0], cpsz );
                    size -= cpsz;

                    data_unpack2unpack_arb( (data_16_t*)&pCtrlTmp[ix].addPwm.b_ptn, (data_16_t*)&pOwneTmp[ix].addPwm.b_ptn, mask );

                }else {
                    break;
                }
            }

            gettimeofday(&ctrlUdpCtrlData.e2s_tm, NULL);
        }
        else if (pHd->d_id == CMD_CAN_SEND) {
            // data ID = CAN data
            Put_que(&ctrlMsgQueue[bno].eu2s_can, (mctrl_dt_t*)buf, size);
        }
        else {
            // one-time send data
            Put_que(&ctrlMsgQueue[bno].eu2s, (mctrl_dt_t*)buf, size);
        }

        req_send_com();

        pthread_mutex_unlock(&ctrlUdpE2sMutex);
    }
    return ( size );
}

/**
 * @brief  ctrlUdpCtrlData（Ether -> SIO) 情報取出 関数
 * @fn     int get_ctrl_eu2sdata( int bno, char *buf, int buf_sz, int flg)
 * @param  buf  : hostPC -> SEB9512 制御情報格納 pointer
 * @param  size : 格納エリア・サイズ
 * @param  flg  : 通信状態( 0 = OFF Line , 1 = ON Line )
 * @retval == 0 : error
 * @retval != 0 : OK 制御情報サイズ
 *
 */
 
int get_ctrl_eu2sdata( int bno, char *buf, int buf_sz, int flg)
{
    int                      size, com_stat, add_pwmc;
    struct timeval           now;
    mctrl_dt16_t            *s_p;    // Source pointer
    mctrl_dt16_t            *d_p;    // distnation pointer
    mctrl_dt16_t            *p_p;    // previous control data pointer
    CONTROL_MESSAGE_QUEUES  *pQue = &ctrlMsgQueue[bno];
    CTRL_DATA_TEMP          *pCtlTmp, *pPreTmp;
    int prt_flg = 0;

    size = 0;
    gettimeofday(&now,NULL);

    if( 0 == pthread_mutex_lock( &ctrlUdpE2sMutex) ){

        if(Is_get(&pQue->eu2s) ) {
            size =Get_que(&pQue->eu2s, (mctrl_dt_t*)buf, buf_sz);
        }else
        if(Is_get(&pQue->eu2s_can) ) {
            size =Get_que(&pQue->eu2s_can, (mctrl_dt_t*)buf, buf_sz);
        }else
        if( Def_time(&now, &ctrlUdpCtrlData.e2s_tm) < 500 ){
            // 500ms Over?
            com_stat  = (flg & 0xFF);
            add_pwmc  = ((flg>>8) & 0xFF);

            pCtlTmp   = (CTRL_DATA_TEMP*)ctrlUdpCtrlData.eu2s_data;
            pPreTmp   = (CTRL_DATA_TEMP*)ctrlUdpCtrlData.eu2s_predata;
            d_p       = (mctrl_dt16_t*)buf;

            if( pCtlTmp[bno].ctrl.hd.d_id == CMD_CTRL_DATA ) {
                s_p = (mctrl_dt16_t*)&pCtlTmp[bno].ctrl;
                p_p = (mctrl_dt16_t*)&pPreTmp[bno].ctrl;

                s_p->b_ptn &= 0x003F;   // PWMチャンネル最大数４
                size        = MOTOR_CTRLDATA_SIZE;
                prt_flg = 1;
            }else
            if( (pCtlTmp[bno].addPwm.hd.d_id == CMD_SBUS_DATA)&&(add_pwmc != 0) ) {
                s_p = (mctrl_dt16_t*)&pCtlTmp[bno].addPwm;
                p_p = (mctrl_dt16_t*)&pPreTmp[bno].addPwm;

                s_p->b_ptn &= add_pwm_bptn[add_pwmc];
                size        = MOTOR_ADDPWM_SIZE(add_pwmc);
            }else {
                size = 0;
            }

            if( size != 0 ){
                if( com_stat == 0 ){
                	s_p->dt[0] |= 0x0001;		// ctrl off
                    memcpy( d_p, s_p, size );
                    memcpy( p_p, s_p, size );
                    if (s_p->hd.d_id == CMD_CTRL_DATA) {
                        d_p->b_ptn = 0x003F;
                    }
//					chk_point(120,(short)d_p->b_ptn);
                }else {
                    size = data_def_unpack2pack( s_p, d_p, p_p );
//					chk_point(121,(short)d_p->b_ptn);
                    if( (s_p->hd.d_id == CMD_SBUS_DATA)&&(d_p->b_ptn==0) ){
                        size = 0;
                    }
                }
                s_p->hd.d_id = 0;
            }
        }else {
            size = 0;
        }

        pthread_mutex_unlock(&ctrlUdpE2sMutex);
    }
    return ( size );
}

/**
 * @brief  set_ctrl_s2eudata（SIO -> Ether) 情報格納 関数
 * @fn     int set_ctrl_s2eudata( int bno, char *buf, int size, int flg)
 * @param  buf  : SEB9512 -> hostPC 情報 pointer
 * @param  size : 情報サイズ
 * @param  flg  : 通信状態( 0 = OFF Line , 1 = ON Line )
 * @retval == 0 : error
 * @retval != 0 : OK 情報サイズ
 *
 */
int set_ctrl_s2eudata( int bno, char *buf, int size, int flg)
{

    mctrl_hd_t              *p    = (mctrl_hd_t*)buf;
    CONTROL_MESSAGE_QUEUES  *pQue = &ctrlMsgQueue[CTRL_BOARD_SIO]; // S->E はQueueをボード毎に分割しない
    jtcp_inp_t              *d_p  = (jtcp_inp_t *)ctrlUdpCtrlData.s2e_data;

    if ( (size == 0)||(size > CTRL_DATA_MAX) ) {
        return (0);
    }

    if( 0 == pthread_mutex_lock( &ctrlUdpE2sMutex) ){

        if( p->d_id == RESP_SENS_DATA ) {
            // data ID = sensor data(cyclic receive data)
            memcpy( &d_p->mes[bno], buf, sizeof(jtcp_inp_mes_t) );
            if( flg != 0 ){
                gettimeofday(&ctrlUdpCtrlData.s2e_tm, NULL);
                ctrlUdpCtrlData.s2e_sz  = sizeof(jtcp_inp_mes_t)*(bno+1);

//				sem_post(&ctrlUdpSndSem);           // 送信シグナル
            }
        }else {

            p->info = bno;

            Put_que(&pQue->s2e, (mctrl_dt_t*)buf, size);

//			sem_post(&ctrlUdpSndSem);           // 送信シグナル
        }

        pthread_mutex_unlock(&ctrlUdpE2sMutex);
    }

    return ( size );
}

/**
 * @brief  ctrlUdpCtrlData（SIO -> Ether) 情報取出 関数
 * @fn     int get_ctrl_s2edata(char *buf, int buf_sz)
 * @param  buf  : SEB9512 -> hostPC 情報格納 pointer
 * @param  size : 格納エリア・サイズ
 * @retval == 0 : error
 * @retval != 0 : OK 制御情報サイズ
 *
 */
int get_ctrl_s2eudata(char *buf, int buf_sz)
{
    int                      size;
    struct timeval           now;
    CONTROL_MESSAGE_QUEUES  *pQue = &ctrlMsgQueue[CTRL_BOARD_SIO]; // S->E はQueueはボード毎に分割しない

    size = 0;
    gettimeofday(&now,NULL);
    if( 0 == pthread_mutex_lock( &ctrlUdpE2sMutex) ){
        if( Is_get(&pQue->s2e) ) {
            size = Get_que(&pQue->s2e, (mctrl_dt_t*)buf, buf_sz);
        }else {
            size = ctrlUdpCtrlData.s2e_sz;
            if ((size == 0)||(size > buf_sz)) {
                size = 0;
            }
            if (Def_time(&now, &ctrlUdpCtrlData.s2e_tm) < 100) {
                // 100ms Over?
                if (size) {
                    memcpy(buf, ctrlUdpCtrlData.s2e_data, size);
                }
            }
            else {
                size = 0;
            }
            ctrlUdpCtrlData.s2e_sz = 0;
        }
        pthread_mutex_unlock(&ctrlUdpE2sMutex);
    }
    return ( size );
}


void set_owner_mask( int bno, unsigned short do_mask, unsigned short dt_mask, unsigned short add_mask )
{
    ctrlUdpCtrlData.owne_do_mask[bno] = do_mask;
    ctrlUdpCtrlData.owne_mask[bno]    = dt_mask;
    ctrlUdpCtrlData.owne_addmask[bno] = add_mask;
}





