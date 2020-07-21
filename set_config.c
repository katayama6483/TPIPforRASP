/**
 * @file set_config.c
 * @brief システム構成用の内部変数を設定する関数
 *
 * @author Katayama
 * @date 2007-08-06
 * @version V 1.00 2007/08/06 katayama
 * @version V 1.01 2009/10/20 katayama
 * @version V 1.02 2011/02/04 katayama
 * @version v 1.03 2012/02/21 katayama
 *
 * @version v 1.10 2018/10/31 katayama (for Raspberry Pi)
 * @version v 1.11 2020/01/22 katayama (Multi CAM 対応)
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

// V 1.01 set_config関数：fclose漏れ修正
// v 1.02 PWM_SLOW 追加、 '\r'codeの処置、log file生成追加
// v 1.03 DUMP_ON からTRACE_ON , STAT_ONを分離

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include "data_pack.h"
//#include "dump.h"
//#include "trace.h"
//#include "sio_stat.h"
#include "set_config.h"

/**==========================================================================================
 *   #define
 **==========================================================================================*/

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
typedef struct {
    char  *def_var_nm;
    int   userArg;
    int   (*callback)( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
} SET_CONFIG_ID;


/**==========================================================================================
 *   スタティック関数宣言
 **==========================================================================================*/

static int   set_encode1     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_encode2     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_can         ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_dump_on     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_trace_on    ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_stat_on     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_senc        ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_rs232_2     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_com_off     ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_PWM_mode    ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_PWM_add_port( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_connectType ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_boardNum    ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_com_speed   ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_i2c         ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_spi         ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );
static int   set_multi_cam   ( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s );


/**==========================================================================================
 *   スタティック変数形宣言
 **==========================================================================================*/

static char delimter[] = ",\n";
static char *p1_s;
static char *p2_s;
static char *p3_s;
static char *p4_s;
static char *p5_s;
static char *p6_s;
static char *p7_s;



static SET_CONFIG_ID setConfigTbl[] = {
    { "ENCODE1",      0,  set_encode1       },  // 
    { "ENCODE1@1",    0,  set_encode1       },  // 
    { "ENCODE1@2",    1,  set_encode1       },  // 
    { "ENCODE1@3",    2,  set_encode1       },  // 
    { "ENCODE1@4",    3,  set_encode1       },  // 
    { "ENCODE2",      0,  set_encode2       },  // 
    { "ENCODE2@1",    0,  set_encode2       },  // 
    { "ENCODE2@2",    1,  set_encode2       },  // 
    { "ENCODE2@3",    2,  set_encode2       },  // 
    { "ENCODE2@4",    3,  set_encode2       },  // 
    { "CAN_SET",      0,  set_can           },  // 
    { "CAN_SET@1",    0,  set_can           },  // 
    { "CAN_SET@2",    1,  set_can           },  // 
    { "CAN_SET@3",    2,  set_can           },  // 
    { "CAN_SET@4",    3,  set_can           },  // 
    { "SEN_SET",      0,  set_senc          },  // 
    { "SEN_SET@1",    0,  set_senc          },  // 
    { "SEN_SET@2",    1,  set_senc          },  // 
    { "SEN_SET@3",    2,  set_senc          },  // 
    { "SEN_SET@4",    3,  set_senc          },  // 
    { "RS232_2",      0,  set_rs232_2       },  // RS232(SEB9512)
    { "RS232_2@1",    0,  set_rs232_2       },  // RS232(SEB9512)
    { "RS232_2@2",    1,  set_rs232_2       },  // RS232(SEB9512)
    { "RS232_2@3",    2,  set_rs232_2       },  // RS232(SEB9512)
    { "RS232_2@4",    3,  set_rs232_2       },  // RS232(SEB9512)
    { "COM_OFF",      0,  set_com_off       },  // 通信切断時のPWM出力設定
    { "COM_OFF@1",    0,  set_com_off       },  // 通信切断時のPWM出力設定
    { "COM_OFF@2",    1,  set_com_off       },  // 通信切断時のPWM出力設定
    { "COM_OFF@3",    2,  set_com_off       },  // 通信切断時のPWM出力設定
    { "COM_OFF@4",    3,  set_com_off       },  // 通信切断時のPWM出力設定
    { "PWM_MODE",     0,  set_PWM_mode      },  // PWM mode 切替
    { "PWM_MODE@1",   0,  set_PWM_mode      },  // PWM mode 切替
    { "PWM_MODE@2",   1,  set_PWM_mode      },  // PWM mode 切替
    { "PWM_MODE@3",   2,  set_PWM_mode      },  // PWM mode 切替
    { "PWM_MODE@4",   3,  set_PWM_mode      },  // PWM mode 切替
    { "PWM_ADD",      0,  set_PWM_add_port  },  // PWM 増設数
    { "PWM_ADD@1",    0,  set_PWM_add_port  },  // PWM 増設数
    { "PWM_ADD@2",    1,  set_PWM_add_port  },  // PWM 増設数
    { "PWM_ADD@3",    2,  set_PWM_add_port  },  // PWM 増設数
    { "PWM_ADD@4",    3,  set_PWM_add_port  },  // PWM 増設数
    { "I2C_SET",      0,  set_i2c           },  // I2C通信速度
    { "I2C_SET@1",    0,  set_i2c           },  // I2C通信速度
    { "I2C_SET@2",    1,  set_i2c           },  // I2C通信速度
    { "I2C_SET@3",    2,  set_i2c           },  // I2C通信速度
    { "I2C_SET@4",    3,  set_i2c           },  // I2C通信速度
    { "SPI_SET",      0,  set_spi           },  // SPI設定
    { "SPI_SET@1",    0,  set_spi           },  // SPI設定
    { "SPI_SET@2",    1,  set_spi           },  // SPI設定
    { "SPI_SET@3",    2,  set_spi           },  // SPI設定
    { "SPI_SET@4",    3,  set_spi           },  // SPI設定

    { "CONNECT_TYPE", 0,  set_connectType   },  // 
    { "CBOARD_NUM",   0,  set_boardNum      },  // 
    { "COM_SPEED",    0,  set_com_speed     },  // 制御ボード間通信速度

	{ "MULTI_CAM",    0,  set_multi_cam     },  // マルチ・カメラmode 有効ch指定

    { "DUMP_ON",      0,  set_dump_on       },  // 
    { "TRACE_ON",     0,  set_trace_on      },  // 
    { "STAT_ON",      0,  set_stat_on       },  // 
    { "",             0,  0                 }   // テーブル終端
};



static SET_CONFIG_PARAM  *pSetConfigParam;
//extern char data_bit_num[];


/**
 * @brief  制御パラメータデータ　初期化関数
 *  void init_ctrl_para(struct _DATA_F_STR* para, struct _DATA_F_STR* offset, struct _DATA_F_STR* can_para, struct _DATA_F_STR* rs232_para )
 * @param  para       : パラメータデータエリアのポインタ
 * @param  offset     : オフセットデータエリアのポインタ
 * @param  can_para   : CAN設定データエリアのポインタ
 * @param  rs232_para : RS232設定データエリアのポインタ
 */
void init_ctrl_para( SET_CONFIG_PARAM *pParam )
{
    pSetConfigParam = pParam;
}

/**
 * @brief  制御パラメータ・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_io_para(int bno, int no, int val)
{
    int               p_bit, b_ptn;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    b_ptn = 0x01;
    p_bit = pParam->iopara.bit;
    if (no < 16) {
        p_bit = p_bit | (b_ptn << no);
        pParam->iopara.dt[no]   = val;
        pParam->iopara.bit      = (short)p_bit;
        pParam->set_iopara_rf  |= p_bit;
        pParam->set_iopara_sf  |= p_bit;
    }
}

/**
 * @brief  制御パラメータ２・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_ctrl_para(int bno, int no, int val)
{
    int               p_bit;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    p_bit = pParam->ctrlpara.bit;

    if (no < 16) {
        p_bit                   |= (1 << no);
        pParam->ctrlpara.dt[no]  = val;
        pParam->ctrlpara.bit     = (short)p_bit;
        pParam->set_ctrlpara_rf |= p_bit;
        pParam->set_ctrlpara_sf |= p_bit;
    }
}

/**
 * @brief  CANパラメータ・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_can_para(int bno, int no, int val)
{
    int               p_bit, b_ptn;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    b_ptn = 0x01;
    p_bit = pParam->can.bit;
    if (no < 16) {
        p_bit               = p_bit | (b_ptn << no);
        pParam->can.dt[no]  = val;
        pParam->can.bit     = (short)p_bit;
        pParam->set_can_rf |= p_bit;
        pParam->set_can_sf |= p_bit;
    }
}

/**
 * @brief  RS232パラメータ・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_rs232_para(int bno, int no, int val)
{
    int               p_bit, b_ptn;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    b_ptn = 0x01;
    p_bit = pParam->rs232.bit;
    if (no < 16) {
        p_bit                 = p_bit | (b_ptn << no);
        pParam->rs232.dt[no]  = val;
        pParam->rs232.bit     = (short)p_bit;
        pParam->set_rs232_rf |= p_bit;
        pParam->set_rs232_sf |= p_bit;
    }
}

/**
 * @brief  I2Cパラメータ・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_i2c_para(int bno, int no, int val)
{
    int               p_bit, b_ptn;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    b_ptn = 0x01;
    p_bit = pParam->i2c.bit;
    if (no < 16) {
        p_bit               = p_bit | (b_ptn << no);
        pParam->i2c.dt[no]  = val;
        pParam->i2c.bit     = (short)p_bit;
        pParam->set_i2c_rf |= p_bit;
        pParam->set_i2c_sf |= p_bit;
    }
}

/**
 * @brief  SPIパラメータ・セット関数
 * @param  no      : data no(0〜
 * @param  val     : 値
 */
static void set_spi_para(int bno, int no, int val)
{
    int               p_bit, b_ptn;
    SET_CONFIG_PARAM *pParam;

    if(!pSetConfigParam) {
        return;
    }

    pParam = &pSetConfigParam[bno];

    b_ptn = 0x01;
    p_bit = pParam->spi.bit;
    if (no < 16) {
        p_bit               = p_bit | (b_ptn << no);
        pParam->spi.dt[no]  = val;
        pParam->spi.bit     = (short)p_bit;
        pParam->set_spi_rf |= p_bit;
        pParam->set_spi_sf |= p_bit;
    }
}

/**
 * @brief  SPI port no セット関数
 * @param  val     : 値
 */
static void set_SPI_port(int bno, int val)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].SPI_port_no = val;
}

/**
 * @brief  SPI port no 取出関数
 *  int get_SPI_udpport(void)
 * @retval     : SPI port no
 */
int get_SPI_udpport(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }

    return(pSetConfigParam[bno].SPI_port_no);
}

/**
 * @brief  remote SPI speed(bps) セット関数
 *  void set_SPI_bps(int val)
 * @param  val     : 値
 */
static void set_SPI_bps(int bno, int val)
{
    if(!pSetConfigParam) {
        return;
    }

    pSetConfigParam[bno].SPI_bps = val;
}

/**
 * @brief  @remote SPI speed(bps) 取出関数
 *  int get_SPI_bps(void)
 * @retval     : speed
 */
int get_SPI_bps(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }

    return(pSetConfigParam[bno].SPI_bps);
}

/**
 * @brief  remote SIO port no セット関数
 * @param  val     : 値
 */
static void set_rSIO_port(int bno, int val)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].rSIO_port_no = val;
}

/**
 * @brief  remote SIO port no 取出関数
 *  int get_rSIO_udpport(void)
 * @retval     : UDP port no
 */
int get_rSIO_udpport(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }

    return(pSetConfigParam[bno].rSIO_port_no);
}

/**
 * @brief  remote SIO speed(bps) セット関数
 *  void set_rSIO_bps(int val)
 * @param  val     : 値
 */
static void set_rSIO_bps(int bno, int val)
{
    if(!pSetConfigParam) {
        return;
    }

    pSetConfigParam[bno].rSIO_bps = val;
}

/**
 * @brief  @remote SIO speed(bps) 取出関数
 *  int get_rSIO_bps(void)
 * @retval     : speed
 */
int get_rSIO_bps(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }

    return(pSetConfigParam[bno].rSIO_bps);
}


/**
 * @brief  制御パラメータデータ　更新ビット取出関数
 * int get_ctrl_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_io_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return(pSetConfigParam[bno].set_iopara_rf);
}


/**
 * @brief  制御パラメータデータ　更新ビットリセット関数
 * void reset_ctrl_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
void reset_io_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_iopara_rf = 0;
}

/**
 * @brief  制御パラメータ２データ　更新ビット取出関数
 * int get_ctrl_para2_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_ctrl_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[bno].set_ctrlpara_rf);
}


/**
 * @brief  制御パラメータ２データ　更新ビットリセット関数
 * void reset_ctrl_para2_bit(void)
 * @retval   : パラメータ更新ビット
 */
void reset_ctrl_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_ctrlpara_rf = 0;
}

/**
 * @brief  CANパラメータデータ　更新ビット取出関数
 * int get_ctrl_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_can_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[bno].set_can_rf);
}


/**
 * @brief  CANパラメータデータ　更新ビットリセット関数
 * void reset_can_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
void reset_can_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_can_rf = 0;
}

/**
 * @brief  RS232パラメータデータ　更新ビット取出関数
 * int get_ctrl_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_rs232_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[bno].set_rs232_rf);
}

/**
 * @brief  RS232パラメータデータ　更新ビットリセット関数
 * void reset_rs232ラメータ更新ビット
 */
void reset_rs232_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_rs232_rf = 0;
}

/**
 * @brief  I2Cパラメータデータ　更新ビット取出関数
 * int get_i2c_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_i2c_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[bno].set_i2c_rf);
}

/**
 * @brief  I2Cパラメータデータ　更新ビットリセット関数
 * void reset_i2c_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
void reset_i2c_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_i2c_rf = 0;
}

/**
 * @brief  SPIパラメータデータ　更新ビット取出関数
 * int get_spi_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
int get_spi_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[bno].set_spi_rf);
}

/**
 * @brief  SPIパラメータデータ　更新ビットリセット関数
 * void reset_spi_para_bit(void)
 * @retval   : パラメータ更新ビット
 */
void reset_spi_para_bit(int bno)
{
    if(!pSetConfigParam) {
        return;
    }
    pSetConfigParam[bno].set_spi_rf = 0;
}

/**
 * @brief  
 * 
 * @retval   : 
 */
int get_ctrl_connectType(void)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[0].connectType);
}


/**
 * @brief  
 * 
 * @retval   : 
 */
int get_ctrl_boardNum(void)
{
    if(!pSetConfigParam) {
        return(-1);
    }
    return (pSetConfigParam[0].boardNum);
}

/**
 * @brief  制御ボード間通信(RS232c)の通信速度取得関数
 * int get_com_speed(void)
 * @retval ==  0 : Default speed  38400bps
 * @retval ==  com speed : 38400, 115200
 * @retval == -1 : error
 */
int get_com_speed(void)
{
    if(!pSetConfigParam) {
        return(-1);
    }
	return (pSetConfigParam[0].com_speed);
}

/**
 * @brief  マルチ・カメラのモード（有効ch）取得関数
 * int get_multi_cam(void)
 * @retval ==  0        : 非マルチ・カメラ
 * @retval ==  1 ~ 0x0F : 有効ch(b0:chA, b1:chB, b2:chC, b3:chD)
 */
int get_multi_cam(void)
{
    if(!pSetConfigParam) {
        return(0);
    }
	return (pSetConfigParam[0].multi_cam_mode);
}

/**
 * @brief  文字列内を指定文字列があるか検索して、 有れば指定文字列以降を切り離す。
 * @param  rd_msg    : 入力文字列
 * @param  scan_word : 検索文字列
 * @retval rd_msg    : 出力文字列（入力文字列を編集します）
 */
static void str_cut_(char* rd_msg,char* scan_word)
{
    char* n;

    n = strstr(rd_msg,scan_word);
    if (n != NULL) {
        *n = 0;
    }
}

/**
 * @brief  
 * 
 */
static int get_var_id( char* msg)
{
    char  *token;
    char  var_nm[17];
    int   i;
    int   ans;

    p1_s = "";
    p2_s = "";
    p3_s = "";
    p4_s = "";
    p5_s = "";
    p6_s = "";
    p7_s = "";

    token = strtok( msg , delimter);
    if (token == NULL) {
        return -1;
    }
    sscanf(token,"%16s",var_nm);
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p1_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p2_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p3_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p4_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p5_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p6_s = token;
    }
    token = strtok( NULL, delimter);
    if (token != NULL) {
        p7_s = token;
    }


    i = 0;
    ans = 1;
    while( strlen( setConfigTbl[i].def_var_nm ) != 0 )
    {
        if (strcmp( var_nm, setConfigTbl[i].def_var_nm )== 0) {
            ans = setConfigTbl[i].callback( setConfigTbl[i].userArg, p1_s, p2_s, p3_s, p4_s, p5_s, p6_s, p7_s );
            break;
        }
        i++;
    }
    return(ans);
}


/**
 * @brief  エンコーダ設定（CH1）
 */
static int set_encode1( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1,p2,p3,p4;
    int  ans;

    p1=0; p2=0; p3=0; p4=0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"-1 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);        // sign = -1 , 1
        if (p1 == -1) {
            p1 = 1;
        }
        else {
            p1 = 0;
        }
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"012 \t") == strlen(p2_s))) {
        p2 = atoi(p2_s);		// kind = 0 , 1
        set_io_para( userArg, SET_PARA1_PI1_ENCMODE, p2);
    }
    else {
        ans = 3;
    }
    if ((strlen(p3_s)>0)&&(strspn( p3_s ,"0123456789 \t") == strlen(p3_s))) {
        p3 = atoi(p3_s); // pulse = 0 縲鰀
        set_io_para( userArg, SET_PARA1_PI1_NUM_OF_PLS, p3);
    }
    else {
        ans = 4;
    }
    if ((strlen(p4_s)>0)&&(strspn( p4_s ,"0123456789 \t") == strlen(p4_s))) {
        sscanf(p4_s,"%x",&p4); // 回転比 = 1 縲鰀
        set_io_para( userArg, SET_PARA1_PI1_GEAR_RATIO, p4);
    }
    else {
        ans = 5;
    }

    return ans;
}

/**
 * @brief  エンコーダ設定（CH2）
 */
static int set_encode2( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1,p2,p3,p4;
    int  ans;

    p1=0; p2=0; p3=0; p4=0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"-1 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);         // sign = -1 , 1
        if (p1 == -1) {
            p1 = 1;
        }
        else {
            p1 = 0;
        }
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"012 \t") == strlen(p2_s))) {
        p2 = atoi(p2_s);         // kind = 0 , 1
        set_io_para( userArg, SET_PARA1_PI2_ENCMODE , p2);
    }
    else {
        ans = 3;
    }
    if ((strlen(p3_s)>0)&&(strspn( p3_s ,"0123456789 \t") == strlen(p3_s))) {
        p3 = atoi(p3_s); // pulse = 0 縲鰀
        set_io_para( userArg, SET_PARA1_PI2_NUM_OF_PLS, p3);
    }
    else {
        ans = 4;
    }
    if ((strlen(p4_s)>0)&&(strspn( p4_s ,"0123456789ABCDEFabcdef \t") == strlen(p4_s))) {
        sscanf(p4_s,"%x",&p4); // 回転比 = 1 縲鰀
        set_io_para( userArg, SET_PARA1_PI2_GEAR_RATIO, p4);
    }
    else {
        ans = 5;
    }

    return ans;
}

/**
 * @brief CAN 設定
 */
static int set_can( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1,p2,p3;
    int  ans;

    p1=0; p2=0; p3=0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"012345 \t") == strlen(p1_s)))  {
        p1 = atoi(p1_s);    // 通信速度:0=1Mbps 1=500Kbps 2=250Kbps 3=125Kbps 4=50Kbps 5=83.9Kbps
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"0123456789ABCDEFabcdef \t") == strlen(p2_s))) {
        sscanf(p2_s,"%x",&p2); // 受信ID
        p2 = p2 & 0x7ff;
    }
    else {
        ans = 3;
    }
    if ((strlen(p3_s)>0)&&(strspn( p3_s ,"0123456789ABCDEFabcdef \t") == strlen(p3_s))) {
        sscanf(p3_s,"%x",&p3); // IDマスクビット
        p3 = p3 & 0x7ff;
    }
    else {
        ans = 4;
    }

    if ( ans == 0 ) {
        if ((p1 < 0)||(p1 > 5))	 {
            ans = 2;
        }
    }
    if ( ans == 0 ) {
        set_can_para( userArg, SET_CAN_PARA_COM_SPEED, p1 );  // 通信速度
        set_can_para( userArg, SET_CAN_PARA_RECV_ID,   p2 );  // 受信ID
        set_can_para( userArg, SET_CAN_PARA_ID_MASK,   p3 );  // IDマスクビット
    }
    return ans;
}

/**
 * @brief DUMP ON 設定
 */
static int set_dump_on( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  p2;
    int  ans;

    p1  = 0;
    p2  = 0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s)))  {
        p1 = atoi(p1_s);    // DUMP count
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"0123456789 \t") == strlen(p2_s)))  {
        p2 = atoi(p2_s);    // DUMP mode
    }

    if ( ans == 0 ) {
//        dump_enable(p1, p2);  // DUMP          p1==0 :disable,  p1 > 0 :enable
    }
    return ans;
}

/**
 * @brief TRACE ON 設定
 */
static int set_trace_on( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s)))  {
        p1 = atoi(p1_s);    // TRACE on flag 0==off 0!=on
    }
    else {
        ans = 2;
    }

    if ( ans == 0 ) {
//        trace_enable(p1);   // TRACE         p1==0 :disable,  p1 > 0 :enable
    }
    return ans;
}

/**
 * @brief  STATical data ON 設定
 */
static int set_stat_on( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s)))  {
        p1 = atoi(p1_s);    // TRACE on flag 0==off 0!=on
    }
    else {
        ans = 2;
    }

    if ( ans == 0 ) {
//        stat_enable(p1);    // STATical data p1==0 :disable,  p1 > 0 :enable
    }
    return ans;
}



/**
 * @brief  Sensor Enable bit 設定
 */
static int set_senc( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789ABCDEFabcdef \t") == strlen(p1_s))) {
        sscanf(p1_s,"%x",&p1); // Sensor enable ビット 1: enable 0:disable
        p1 = p1 & 0xffff;
    }
    else {
        ans = 4;
    }
    if (ans == 0) {
        set_io_para( userArg, SET_PARA1_SENS_E_BIT, p1);
    }
    return ans;
}

/**
 * @brief RS232 #2 設定（SEB9512 RS232）<$RS232_2 , port_no, speed, Bit_fmt>
 */
static int set_rs232_2( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1,p2,p3;
    int  ans;
    char tmp_s[16];

    p1=0; p2=0; p3=0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);        // UDP port no =
        set_rSIO_port(userArg, p1);
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"0123456789 \t") == strlen(p2_s))) {
        p2 = atoi(p2_s);        // SIO speed
        switch (p2) {
            case 1200:
            case 2400:
            case 4800:
            case 9600:
            case 19200:
            case 38400:
            case 57600:
                set_rs232_para( userArg, 0, p2/100 );    // 通信速度
                set_rSIO_bps(userArg, p2);
                break;
            default:
                ans = 3;
                break;
        }
    }
    else {
        ans = 3;
    }
    if ((strlen(p3_s)>0)&&(strspn( p3_s ,"1278EON \t") == strlen(p3_s))) {
        p3 = 0;
        sscanf(p3_s, "%s", tmp_s);
        if (strcmp(tmp_s,"8N1")==0) {
            p3 = 0x801; // 8bit non -Parity 1stopbit
        }
        if (strcmp(tmp_s,"8O1")==0) {
            p3 = 0x811; // 8bit odd -Parity 1stopbit
        }
        if (strcmp(tmp_s,"8E1")==0) {
            p3 = 0x821; // 8bit even-Parity 1stopbit
        }
        if (strcmp(tmp_s,"8N2")==0) {
            p3 = 0x802; // 8bit non -Parity 2stopbit
        }
        if (strcmp(tmp_s,"8O2")==0) {
            p3 = 0x812; // 8bit odd -Parity 2stopbit
        }
        if (strcmp(tmp_s,"8E2")==0) {
            p3 = 0x822; // 8bit even-Parity 2stopbit
        }
        if (strcmp(tmp_s,"7N1")==0) {
            p3 = 0x701; // 7bit non -Parity 1stopbit
        }
        if (strcmp(tmp_s,"7O1")==0) {
            p3 = 0x711; // 7bit odd -Parity 1stopbit
        }
        if (strcmp(tmp_s,"7E1")==0) {
            p3 = 0x721; // 7bit even-Parity 1stopbit
        }
        if (strcmp(tmp_s,"7N2")==0) {
            p3 = 0x702; // 7bit non -Parity 2stopbit
        }
        if (strcmp(tmp_s,"7O2")==0) {
            p3 = 0x712; // 7bit odd -Parity 2stopbit
        }
        if (strcmp(tmp_s,"7E2")==0) {
            p3 = 0x722; // 7bit even-Parity 2stopbit
        }

        if (p3) {
            set_rs232_para(userArg, 1, (p3 & 0xf00)>>8);
            set_rs232_para(userArg, 2, (p3 & 0x0f0)>>4);
            set_rs232_para(userArg, 3, (p3 & 0x00f));
        }
        else {
            ans = 4;
        }
    }
    else {
        ans = 4;
    }

    return ans;
}

/**
 * @brief  通信切断時のPWM出力設定
 */
static int set_com_off( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789ABCDEFabcdef \t") == strlen(p1_s))) {
        sscanf(p1_s,"%x",&p1); // 通信off時の
        set_ctrl_para( userArg, SET_PARA2_PWM_CTL_HOLD, p1);
    }
    else {
        ans = 2;
    }
    return ans;
}

/**
 * @brief  PWM mode 設定
 */
static int set_PWM_mode( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789ABCDEFabcdef \t") == strlen(p1_s))) {
        sscanf(p1_s,"%x",&p1); // PWM mode ビット 1: full PWM 0:RC PWM
        p1 = p1 & 0x001f;
    }
    else {
        ans = 4;
    }
    if (ans == 0) {
        set_io_para( userArg, SET_PARA1_PWM_SEL_BIT, p1);
    }
    return ans;
}


/**
 * @brief  PWM増設ポート数設定
 */
static int set_PWM_add_port( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{

    int p1;
    int ans =0;

     if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);
        set_ctrl_para( userArg, SET_PARA2_PWM_ADD_NUM, p1);
    }
     else {
         ans = -1;
     }

    return ( ans ) ;
}

/**
 * @brief I2C 設定
 */
static int set_i2c( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1=0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"01 \t") == strlen(p1_s)))  {
        p1 = atoi(p1_s);    // 通信速度:0=100kbps 1=400Kbps
    }
    else {
        ans = 2;
    }

    if ( ans == 0 ) {
        if ((p1 < 0)||(p1 > 2))	 {
            ans = 2;
        }
    }
    if ( ans == 0 ) {
        set_i2c_para( userArg, SET_I2C_PARA_COM_SPEED, p1 );  // 通信速度
    }

    return ans;
}

/**
 * @brief SPI 設定
 */
static int set_spi( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1,p2,p3,p4;
    int  ans;

    p1=0; p2=0; p3=0; p4=0;
    ans = 0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);        // SPI port no
        set_SPI_port(userArg, p1);
    }
    else {
        ans = 2;
    }
    if ((strlen(p2_s)>0)&&(strspn( p2_s ,"0123456789 \t") == strlen(p2_s))) {
        p2 = atoi(p2_s);     // SPI通信速度設 kbps
    	set_SPI_bps(userArg, p2);
    }
    else {
        ans = 3;
    }
    if ((strlen(p3_s)>0)&&(strspn( p3_s ,"0123456789 \t") == strlen(p3_s))) {
        p3 = atoi(p3_s);     // SPIデータビット数 8 or 16 bits
//        data_bit_num[userArg] = p3;
    }
    else {
        ans = 4;
    }
    if ((strlen(p4_s)>0)&&(strspn( p4_s ,"0123 \t") == strlen(p4_s)))  {
        p4 = atoi(p4_s);    // 動作モード: 0 = 正パルスラッチ先行 1 = 正パルスシフト先行
                            //             2 = 負パルスラッチ先行 3 = 負パルスシフト先行
    }
    else {
        ans = 5;
    }

    if ( ans == 0 ) {
         set_spi_para( userArg, SET_SPI_PARA_COM_SPEED, p2);
         set_spi_para( userArg, SET_SPI_PARA_DATA_BIT_NUM, p3);
         set_spi_para( userArg, SET_SPI_PARA_MODE, p4);
    }

    return ans;
}

/**
 * @brief  
 */
static int set_connectType( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int p1;
    int ans =0;

     if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);
        pSetConfigParam[userArg].connectType = p1;
    }
     else {
         ans = -1;
     }

    return ( ans ) ;
}

/**
 * @brief  
 */
static int set_boardNum( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int p1;
    int ans =0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);
        pSetConfigParam[userArg].boardNum = p1;
    }
    else {
        ans = -1;
    }

    return ( ans ) ;
}

/**
 * @brief  制御ボード間通信速度
 */
static int set_com_speed( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int p1;
    int ans =0;

    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789 \t") == strlen(p1_s))) {
        p1 = atoi(p1_s);
        if ((p1 == 38400)||(p1 == 115200)) {
            pSetConfigParam[userArg].com_speed = p1;
        }
        else ans = -1;
    }
    else {
        ans = -1;
    }

    return ( ans ) ;
}

/**
 * @brief  マルチ・カメラmode (有効ch指定) 設定
 */
static int set_multi_cam( int userArg, char *p1_s, char *p2_s, char *p3_s, char *p4_s, char *p5_s, char *p6_s, char *p7_s )
{
    int  p1;
    int  ans;

    p1  = 0;
    ans = 0;
    if ((strlen(p1_s)>0)&&(strspn( p1_s ,"0123456789ABCDEFabcdef \t") == strlen(p1_s))) {
        sscanf(p1_s,"%x",&p1); // multi cam mode 有効ch指定ビット 1: 有効 0: 無効
        p1 = p1 & 0x000f;
    }
    else {
        ans = 4;
    }
    if (ans == 0) {
        pSetConfigParam[userArg].multi_cam_mode = p1;
    }
    return ans;
}

/**
 * @brief  
 */
static int set_in_var( char* msg)
{
#if 1
    int ans = 0;

    ans = get_var_id( msg );
    return(ans);
#endif

    return (ans);
}


/**
 * @brief  
 */
int set_config(char* F_name)
{
    FILE   *stream;
    FILE   *fd;
    char   rd_msg[256];
    char   rd_msg_s[256];
    char   *res;
    int    ans;


    fd = fopen("set_config.log","w");	// open log file

    stream = fopen( F_name, "r" );
    if( stream == NULL )	// Open ERROR
    {
        fclose(fd);
        return -1;
    }

    res = (char*)-1;
    while ( res != NULL)
    {
        res = fgets(rd_msg,255,stream);
        if (res == NULL) {
            break;
        }
        str_cut_(rd_msg,"\n");
        str_cut_(rd_msg,"\r");
        str_cut_(rd_msg,"//");
        strcpy(rd_msg_s,rd_msg);	// save message

        if (strlen(rd_msg) > 0) {
            switch( rd_msg[0])
            {
                case '$':		// 内部変数 定義
                    ans = set_in_var( &rd_msg[1] );
                    break;
                case 0:
                    ans = -1;
                    break;
                default:
                    ans = -2;
                    break;
            }
            if (ans) {
                fprintf(fd, "[%s] = %d\n", rd_msg_s, ans);
            }
        }
    }
    fclose(stream);	// V 1.01
    fclose(fd); 	// close Log file
    return 0;
}

/**
 * @brief messge out Ctrl parameter
 * @fn    void msgOut_ctrl_param(char* msg, data_param_t* pParam)
 * @param msg    : message buffer pointer
 * @param pParam : parameter pointer
 *
 */
char* msgOut_ctrl_param(char* msg, data_param_t* pParam)
{
	sprintf(msg, "bit[%04X] data = %04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X,%04X"
		, pParam->bit
		, pParam->dt[0]
		, pParam->dt[1]
		, pParam->dt[2]
		, pParam->dt[3]
		, pParam->dt[4]
		, pParam->dt[5]
		, pParam->dt[6]
		, pParam->dt[7]
		, pParam->dt[8]
		, pParam->dt[9]
		, pParam->dt[10]
		, pParam->dt[11]
		, pParam->dt[12]
		, pParam->dt[13]
		, pParam->dt[14]
		, pParam->dt[15]
	);
	return msg;
}

/**
 * @brief dump Config Parameter
 * @fn    void dump_ConfigParam(int bno)
 * @param bno : board Number <0 - 3>
 *
 */
void dump_ConfigParam(int bno)
{
	char msg[256];
	SET_CONFIG_PARAM *pParam;

	if(!pSetConfigParam) return;

    pParam = &pSetConfigParam[bno];
	printf("iopara   = %s\n", msgOut_ctrl_param(msg, &pParam->iopara));
	printf("ctrlpara = %s\n", msgOut_ctrl_param(msg, &pParam->ctrlpara));
	printf("can      = %s\n", msgOut_ctrl_param(msg, &pParam->can));
	printf("rs232    = %s\n", msgOut_ctrl_param(msg, &pParam->rs232));
	printf("i2c      = %s\n", msgOut_ctrl_param(msg, &pParam->i2c));
	printf("spi      = %s\n", msgOut_ctrl_param(msg, &pParam->spi));
	
	printf("set_iopara_sf  = %04X\n", pParam->set_iopara_sf);
	printf("set_ctrlpara_rf= %04X\n", pParam->set_ctrlpara_rf);
	printf("set_ctrlpara_sd= %04X\n", pParam->set_ctrlpara_sd);
	
	printf("set_can_sf     = %04X\n", pParam->set_can_sf);
	printf("set_can_rf     = %04X\n", pParam->set_can_rf);
	printf("set_can_sd     = %04X\n", pParam->set_can_sd);

	printf("set_rs232_sf   = %04X\n", pParam->set_rs232_sf);
	printf("set_rs232_rf   = %04X\n", pParam->set_rs232_rf);
	printf("set_rs232_sd   = %04X\n", pParam->set_rs232_sd);
	printf("rSIO_port_no   = %04X\n", pParam->rSIO_port_no);
	printf("rSIO_bps       = %04X\n", pParam->rSIO_bps);

	printf("set_i2c_sf     = %04X\n", pParam->set_i2c_sf);
	printf("set_i2c_rf     = %04X\n", pParam->set_i2c_rf);
	printf("set_i2c_sd     = %04X\n", pParam->set_i2c_sd);

	printf("set_spi_sf     = %04X\n", pParam->set_spi_sf);
	printf("set_spi_rf     = %04X\n", pParam->set_spi_rf);
	printf("set_spi_sd     = %04X\n", pParam->set_spi_sd);
	printf("SPI_port_no    = %04X\n", pParam->SPI_port_no);
	printf("SPI_bps        = %04X\n", pParam->SPI_bps);
}
