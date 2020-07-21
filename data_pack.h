/** 
 * Data Pack / Unpack Function Header file  
 * @file data_pack.h
 * @brief 制御通信データ　パック/アンパック関数　ヘッダーファイル
 *
 * @author Katayama
 * @date 2008-09-05
 * @version $Id: data_pack ,v 1.0 2008/09/05 00:00:00 katayama $
 *
 * Copyright (C) 2008 Sanritz Automation Co., Ltd. All rights reserved.
 */
#ifndef  ___DATA_PACK_H___
#define  ___DATA_PACK_H___

#include "def_mctrl_com16.h"
/*
#pragma pack(1)
typedef struct _DATA_F_STR {
	unsigned short bit;
	unsigned short dt[16];
} data16_f_t;
#pragma pack(8)
*/

extern int     data_pack(data_16_t* unp_dt, data_16_t* p_dt);
extern void    data_unpack(data_16_t* p_dt, data_16_t* unp_dt);
extern void    data_unpack_arb(data_param_t* p_dt, data_16_t* unp_dt, int arb_bit);
extern void    data_unpack2unpack_arb(data_16_t* d_p_dt, data_16_t* s_p_dt, int arb_bit);
extern void    data_unpack2unpack_arb(data_16_t* d_p_dt, data_16_t* s_p_dt, int arb_bit);
extern int     data_def_unpack2pack(mctrl_dt16_t* src, mctrl_dt16_t* des, mctrl_dt16_t* pre);

#endif  // ___DATA_PACK_H___
