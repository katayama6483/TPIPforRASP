//
//    JTP(Jpeg-data Transport Protocol) Header file
//

#ifndef __DEF_JTP_H__
#define __DEF_JTP_H__

#include <sys/types.h>
#include "def_mctrl_com16.h"

/**==========================================================================================
 *   #define
 **==========================================================================================*/

#define UDP_PORT_JPEG     (9876)
#define UDP_PORT_CTRL     (7070)
#define UDP_PORT_USERPROC (9120)

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/

#pragma pack(1)

// JTCP(Jpeg-data Transport control Protocl) Header
typedef struct jtcp_hd_str{
  u_char  mark;		// Header Marker
  u_char  ver;		// Header Version
  u_short size;		// Body size
  u_char  rq_st;	// Rq/Stat
  u_char  seq;		// Squence Number
  u_char  cont_ID;	// Conetct ID
  u_char  w_lnk;	// WLAN Link Quality
  u_short speed;	// Communication speed [bps]
  u_short pk_loss;	// Packet loss[*256]
  u_long  delay;	// Delay time [ms]
  u_long  TMstmp;	// Time stamp [ms]
  u_long  stayTM;	// Stay time  [ms]
} jtcp_hd_t ;

// Video control infomation
typedef struct v_ctrl_str{
  u_short v_type;	// Video type 0=QVGA 1=VGA
  u_short q_fact;	// Q-factor
  u_char  cam_no;	// Camera No.
  u_char  v_fps;    // fps 0:30fps 1:15fps 2:10fps 3:5fps (未使用)
  u_short reserv2;	// Reserved
} v_ctrl_t;

// JTCP Frame format
#define JTCP_H_SZ  (sizeof(jtcp_hd_t) + sizeof(v_ctrl_t))
#define JTCP_BODY_SZ (1024)  //PWM増設対応 2014/1/15
typedef struct jtcp_str{
  jtcp_hd_t jtcp_h;
  v_ctrl_t  v_ctrl;
  u_char    body[JTCP_BODY_SZ];	// rev 2008.09.21
} jtcp_t ;

//制御出力データ
struct OUT_DT_STR {
    unsigned short d_out;    // [ver 2.50] DO bit3 - 0
    short          nouse1;   // [ver 2.50]
    short          PWM[4];   // [ver 2.50]
    short          PWM2[16]; // [ver 2.50]
} ;

//制御入力データ
struct INP_DT_STR {
    unsigned short b_ptn;   // [ver 2.20]
    unsigned short AI[8];   // [ver 2.50]
    short          PI[4];   // [ver 2.50]
    unsigned short batt;    // [ver 2.20]
    short          PI1;     // PI[0]と同じ [ver 2.20]
    short          PI2;     // PI[1]と同じ [ver 2.20]
    unsigned short DI;      // [ver 2.50] bit3 - 0
} ;

// V-C基板間 通信電文(制御入力データ C基板一枚分)
struct jtcp_inp_mes {
    mctrl_hd_t            hd;
    struct INP_DT_STR     dt;
} jtcp_inp_mes_t;

// V-C基板間 通信電文(制御入力データ)
typedef struct jtcp_inp_str {
    struct jtcp_inp_mes   mes[4];
} jtcp_inp_t;

// V-C基板間 通信電文(制御出力データ)
typedef struct jtcp_out_str {
    mctrl_hd_t            hd;
    unsigned short        b_ptn;
    struct OUT_DT_STR     dt[4];
} jtcp_out_t;


// JTP(Jpeg-data Transport Protocl) Header
typedef struct jtp_hd_str{
  u_char   JTP_flag;	// Comm flag bit0:start bit1:end
  u_char   seq;		// Squence Number
  u_char   vga;		// Display resolution [0:QVGA, 1:VGA, 2:SVGA, 3:XGA, 4:HD, 5:SXGA, 6:UXGA, 7:FHD]
  u_char   frm_no;	// Frame Number
  u_short  size;	// Body size
  u_short  nouse2;	// 
  u_long   TMstmp;	// Time stamp
  u_long   j_size;	// JPEG size
} jtp_hd_t ;

// JTP Frame format
typedef struct jtp_str{
    jtp_hd_t jtp_h;
//	char     body[1024-sizeof(jtp_hd_t)];
	char     body[1400-sizeof(jtp_hd_t)];
} jtp_t ;

#define JTP_H_SZ  (sizeof(jtp_hd_t))

#pragma pack(8)
#endif /* ifndef __DEF_JTP_H__ */
