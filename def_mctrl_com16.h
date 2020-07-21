/**
 * @file  def_mctrl_com16.h
 * @brief モーター制御ボード間通信フォーマット定義 (16bit版) のHeader file
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2008-09-21
 * @version v 1.0  2008/09/21 katayama
 * @version v 1.1d 2009/01/05 katayama
 * @version V 1.20 2009/03/17 katayama <windows => Linux>
 *
 * Copyright (C) 2008 Sanritz Automation Co.,Ltd. All rights reserved.
 */
//  Ver 1.1d body code 復活（過去versionとの整合性から）

#ifndef ___DEF_MCTRL_COM_H___
#define ___DEF_MCTRL_COM_H___

/**==========================================================================================
 *   #define
 **==========================================================================================*/
#define    CMD_VERSION              (0x00)
#define    CMD_PARAMETER            (0x01)
#define    CMD_CAN_SET              (0x02)
#define    CMD_RS232_SET            (0x03)
#define    CMD_CTRL_SET             (0x04)
#define    CMD_OFF_SET              (0x05)
#define    CMD_SPI_SET              (0x06)
#define    CMD_I2C_SET              (0x07)

#define    CMD_CTRL_DATA            (0x10)
#define    CMD_SBUS_DATA            (0x12)
#define    CMD_CAN_SEND             (0x20)
#define    CMD_RS232_SEND           (0x30)
#define    CMD_GPS_SET              (0x32)
#define    CMD_SPI_SEND             (0x40)
#define    CMD_I2C_SEND             (0x50)
#define    CMD_I2C_RECV_REQ         (0x52)
#define    CMD_PI_REQ               (0x80)
#define    CMD_ECHO                 (0xFF)

// define response ID
#define    RESP_VERSION             (0x00)
#define    RESP_PARAMETER           (0x01)
#define    RESP_CAN_SET             (0x02)
#define    RESP_RS232_SET           (0x03)
#define    RESP_CTRL_SET            (0x04)
#define    RESP_OFF_SET             (0x05)
#define    RESP_SENS_DATA           (0x11)
#define    RESP_CAN_RECV            (0x21)
#define    RESP_RS232_RECV          (0x31)
#define    RESP_SPI_RECV            (0x41)
#define    RESP_I2C_RECV            (0x51)
#define    RESP_I2C_RECV_REQ        (0x53)
#define    RESP_GPS_DATA            (0x33)
#define    RESP_PI_DATA             (0x81)
#define    RESP_ECHO                (0xFF)

// define HOST communication status
#define    STAT_PARAM_SET           (0x01)  // b0: set parameter
#define    STAT_I2C_RECV            (0x02)  // b1: receive I2C data
#define    STAT_GSP_RECV            (0x04)  // b2: receive GSP data
#define    STAT_RS232_RECV          (0x08)  // b3: receive RS232 data
#define    STAT_CAN_SNDERR          (0x10)  // b4: send error CAN
#define    STAT_CAN_RECV            (0x20)  // b5: receive CAN data
#define    STAT_HOST_COM            (0x40)  // b6: ready HOST communication
#define    STAT_FW_READY            (0x80)  // b7: ready SEB9519 Firmware

// control data b_ptn
#define    CTRL_DATA_BPTN_DOCTRL    (1<<0)
#define    CTRL_DATA_BPTN_NOUSE     (1<<1)
#define    CTRL_DATA_BPTN_PWM0      (1<<2)
#define    CTRL_DATA_BPTN_PWM1      (1<<3)
#define    CTRL_DATA_BPTN_PWM2      (1<<4)
#define    CTRL_DATA_BPTN_PWM3      (1<<5)

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
#pragma pack(1)

typedef struct mctrl_hd_str{
    unsigned char  ver;         // Header Version & F/W version
    unsigned char  msg_no;      // Message No
    unsigned char  d_id;        // Data ID
    unsigned char  info;        // d_id:0x20,0x21,0x30,0x31 - 制御ボード番号(0-3)
                                // d_id:0x11                - F/W status
                                // d_id:上記以外            - 未使用（常時0）
} mctrl_hd_t ;

// 制御・センサー データ
typedef struct _dt16_str {
	unsigned short bit;
	unsigned short dt[16];
} data_16_t;

// パラメータ データ
//typedef struct para_str {
//	unsigned short bit;
//	unsigned short dt[16];
//} data_param_t;
typedef data_16_t data_param_t;

// CAN データ
typedef struct mctrl_can_str{
    unsigned char  flg;         // send/receive flag
    unsigned char  RTR;         // RTR
    unsigned char  sz;          // data size
    unsigned char  stat;        // status
    unsigned short STD_ID;      // standard ID
    unsigned char  data[8];     // data
} mctrl_can_t ;

// GPS データ
typedef struct {
    unsigned char  YY;          // UTC 日付(YY)
    unsigned char  MM;          // UTC 日付(MM)
    unsigned char  DD;          // UTC 日付(DD)
    unsigned char  reserv1;
    unsigned char  hh;          // UTC 時刻(hh)
    unsigned char  mm;          // UTC 時刻(mm)
    unsigned char  ss;          // UTC 時刻(ss)
    unsigned char  reserv2;
    int            la_deg;      // 緯度(latitude) [度] x1000000
    int            lo_deg;      // 経度(longitude)[度] x1000000
    unsigned char  GPS_qlty;    // GPS Quality <0:非測位 1:GSP測位 2:DGPS測位>
    unsigned char  sat_cnt;     // Satellite cont 衛星数
    unsigned short HDOP;        // HDOP(horizontal dilution of precision) x100
    unsigned short speed;       // 速度 x10
    unsigned short course;      // 進行方位 x10
} mctrl_gps_t;


// General データフレーム定義
typedef struct {
    mctrl_hd_t  hd;
    char        dt[1020];       //PWM増設対応 2014/1/15
} mctrl_dt_t;

// 制御・センサー・パラメータ データフレーム定義
typedef struct {
    mctrl_hd_t     hd;
    unsigned short b_ptn;
    unsigned short dt[16];
} mctrl_dt16_t;


// 制御・センサー・パラメータ データフレーム定義(PWM増設版)
typedef struct {
    mctrl_hd_t  hd;
    unsigned short b_ptn;
    unsigned short dt[256];
} mctrl_dt256_t;

// CAN データフレーム定義
typedef struct {
    mctrl_hd_t  hd;
    mctrl_can_t CAN_data;
} mctrl_dtcan_t;

// RS232 データフレーム定義
typedef struct {
    mctrl_hd_t    hd;
    unsigned char sz;
    unsigned char reserv1;
    unsigned char dt[32];
} mctrl_dtsio_t;

// SPI データフレーム定義
typedef struct {
    mctrl_hd_t    hd;
    unsigned char sz;
    unsigned char reserv1;
    unsigned char dt[32];
} mctrl_dtspi_t;

#pragma pack(8)
#define VER_E2S_MSG (6)

#endif
