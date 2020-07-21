/**  
 * @file set_config.h
 * @brief システム構成用の内部変数を設定する関数のヘッダーファイル
 *
 * @author Katayama
 * @date 2007-08-06
 * @version v 1.0 2007/08/06 katayama
 *
 * @version v 1.10 2018/10/31 katayama (for Raspberry Pi)
 * @version v 1.11 2020/01/22 katayama (Multi CAM 対応)
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#ifndef _SET_CONFIG_H
#define _SET_CONFIG_H

#include "def_mctrl_com16.h"

/**==========================================================================================
 *   #define
 **==========================================================================================*/
//SETPARAM1
#define SET_PARA1_PI1_ENCMODE     (0)  // PI_1 encoder mode (0:Z-rpm 1:AB-rpm　2:Z-absolute value 3:AB- absolute value)
#define SET_PARA1_PI1_NUM_OF_PLS  (1)  // PI_1 Number of pulses per revolution
#define SET_PARA1_PI1_GEAR_RATIO  (2)  // PI_1 Gear ratio a/b (a = upper 8bit,b = lower 8bit)
#define SET_PARA1_PI2_ENCMODE     (10) // PI_2 encoder mode (0:Z-rpm 1:AB-rpm　2:Z-absolute value 3:AB- absolute value)
#define SET_PARA1_PI2_NUM_OF_PLS  (11) // PI_2 Number of pulses per revolution
#define SET_PARA1_PI2_GEAR_RATIO  (12) // PI_2 Gear ratio a/b (a = upper 8bit,b = lower 8bit)
#define SET_PARA1_SENS_E_BIT      (14) // sensor Enable bit(0:disable 1:enable)
#define SET_PARA1_PWM_SEL_BIT     (15) // PWM select bit [(0:RC PWM 1:full PWM)


//SETPARAM2
#define SET_PARA2_PWM_CTL_HOLD    (1)  // PWM control hold with communication down(0:free 1:hold)
#define SET_PARA2_PWM_ADD_NUM     (3)  // PWM 増設設定

//CAN設定
#define SET_CAN_PARA_COM_SPEED    (0)  // CAN通信速度設定
#define SET_CAN_PARA_RECV_ID      (1)  // CAN受信ID
#define SET_CAN_PARA_ID_MASK      (2)  // CAN IDマスクビット 

//I2C設定
#define SET_I2C_PARA_COM_SPEED    (0)  // I2C通信速度設定

//SPI設定
#define SET_SPI_PARA_COM_SPEED    (0)  // SPI通信速度設定
#define SET_SPI_PARA_DATA_BIT_NUM (1)  // SPIデータビット数
#define SET_SPI_PARA_MODE         (2)  // SPI動作モード

// 制御ボード構成
#define SET_COMTYPE_RS232C        (0)  // RS232C接続
#define SET_COMTYPE_I2C           (1)  // I2C接続

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
typedef struct {
    data_param_t  iopara;              // Ctrl parameter
    data_param_t  ctrlpara;            // Ctrl parameter 2
    data_param_t  can;                 // CAN communication
    data_param_t  rs232;               // RS232 communication
    data_param_t  i2c;                 // I2C communication
    data_param_t  spi;                 // SPI communication
    int           set_iopara_sf;       // set flag (parameter)
    int           set_iopara_rf;       // renewal flag (parameter)
    int           set_iopara_sd;       // sending flag (parameter)

// Ctrl parameter 2
    int           set_ctrlpara_sf;     // set flag (parameter 2)
    int           set_ctrlpara_rf;     // renewal flag (parameter 2)
    int           set_ctrlpara_sd;     // sending flag (parameter 2)

// CAN communication
    int           set_can_sf;          // set flag (CAN param)
    int           set_can_rf;          // renewal flag (CAN param)
    int           set_can_sd;          // sending flag (CAN param)

// RS232 communication
    int           set_rs232_sf;        // set flag (RS232 param)
    int           set_rs232_rf;        // renewal flag (RS232 param)
    int           set_rs232_sd;        // sending flag (RS232 param)
    int           rSIO_port_no;        // remoteSIO UDPport no
    int           rSIO_bps;            // remoteSIO speed

// I2C communication
    int           set_i2c_sf;          // set flag (I2C param)
    int           set_i2c_rf;          // renewal flag (I2C param)
    int           set_i2c_sd;          // sending flag (I2C param)

// SPI communication
    int           set_spi_sf;          // set flag (SPI param)
    int           set_spi_rf;          // renewal flag (SPI param)
    int           set_spi_sd;          // sending flag (SPI param)
    int           SPI_port_no;         // SPI port no
    int           SPI_bps;             // SPI speed

// 制御ボード構成
    int          connectType;
    int          boardNum;
	int          com_speed;           // 制御ボード間通信(RS232 speed)[0=38400<default>,1=115200<high speed> ver 1.10

// マルチ・カメラ
    int          multi_cam_mode;      // multi camera mode 有効ch指定(b0:chA, b1:chB, b2:chC, b3:chD)

} SET_CONFIG_PARAM;


/**==========================================================================================
 *   外部関数参照
 **==========================================================================================*/
extern int    set_config(char* F_name);

extern void   init_ctrl_para( SET_CONFIG_PARAM *pParam );

extern int    get_rSIO_udpport(int bno);
extern int    get_rSIO_bps(int bno);

extern int    get_SPI_udpport(int bno);
extern int    get_SPI_bps(int bno);

extern int    get_io_para_bit(int bno);
extern void   reset_io_para_bit(int bno);

extern int    get_ctrl_para_bit(int bno);
extern void   reset_ctrl_para_bit(int bno);

extern int    get_can_para_bit(int bno);
extern void   reset_can_para_bit(int bno);

extern int    get_rs232_para_bit(int bno);
extern void   reset_rs232_para_bit(int bno);

extern int    get_i2c_para_bit(int bno);
extern void   reset_i2c_para_bit(int bno);

extern int    get_spi_para_bit(int bno);
extern void   reset_spi_para_bit(int bno);

extern int    get_ctrl_connectType(void);
extern int    get_ctrl_boardNum(void);
extern int    get_com_speed(void);	// ver 1.10

extern int    get_multi_cam(void);	// ver 1.11

extern void   dump_ConfigParam(int bno);

#endif
