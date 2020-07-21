/**
 * Data Pack / Unpack Function
 * @file data_pack.c
 * @brief 制御通信データ　パック/アンパック関数
 *
 * @author Katayama
 * @date 2008-09-05
 * @version 1.00 2008/09/05 katayama
 * @version 1.10 2009/11/25 katayama
 * @version 1.11 2011/02/17 katayama
 *
 * Copyright (C) 2008 Sanritz Automation Co., Ltd. All rights reserved.
 */

// ver 1.10 : プロセス間通信対応
// ver 1.11 : 差分転送対応

#include <stdio.h>
#include <string.h>
#include "data_pack.h"
//#include "def_jtp.h"

/**
 * @brief  データパック関数
 * @param  unp_dt  : Unpack data pointer (input)
 * @param  p_dt    : Pack   data pointer (output)
 * @retval         : pack data size
 */
int data_pack(data_16_t* unp_dt, data_16_t* p_dt)
{
    int i, p;
    int p_bit, b_ptn;

    p_bit = unp_dt->bit;
    b_ptn = 0x01;
    p     = 0;
    for ( i = 0 ; i < 16 ; i++ )
    {
        if (p_bit & b_ptn) {
            p_dt->dt[p++] = unp_dt->dt[i];
        }
        b_ptn = b_ptn << 1;
    }
    p_dt->bit = p_bit;
    return ((p + 1)*2);
}

/**
 * @brief  データ　アン・パック関数
 * @param  p_dt    : Pack   data pointer (input)
 * @param  unp_dt  : Unpack data pointer (output)
 */
void data_unpack(data_16_t* p_dt, data_16_t* unp_dt)
{
    int i, p;
    int p_bit, b_ptn;

    p_bit = p_dt->bit;
    b_ptn = 0x01;
    p     = 0;
    for ( i = 0 ; i < 16 ; i++ )
    {
        if (p_bit & b_ptn) {
            unp_dt->dt[i] = p_dt->dt[p++];
        }
        b_ptn = b_ptn << 1;
    }
    unp_dt->bit |= p_bit;
}

/**
 * @brief  データ　アン・パック(Arbitratin bit機能付)関数
 * @param  p_dt    : Pack   data pointer (input)
 * @param  unp_dt  : Unpack data pointer (output)
 * @param  arb_bit : Arbitration bit ( bit '1': Access permission )
 */
void data_unpack_arb(data_param_t* p_dt, data_16_t* unp_dt, int arb_bit)
{
    int i, p;
    int p_bit, b_ptn;

    p_bit = p_dt->bit;
    b_ptn = 0x01;
    p     = 0;
    for ( i = 0 ; i < 16 ; i++ )
    {
        if (p_bit & b_ptn) {
            if(arb_bit & b_ptn) {
                unp_dt->dt[i] = p_dt->dt[p++];
            }
            else {
                p++;
            }
        }
        b_ptn = b_ptn << 1;
    }
    unp_dt->bit |= p_bit;
}

/**
 * @brief  データ unpack -> unpack (Arbitratin bit機能付)関数
 * @param  unp_dt  : Unpack data pointer (input)
 * @param  p_dt    : Pack   data pointer (output)
 * @param  arb_bit : Arbitration bit ( bit '1': Access permission )
 */
void data_unpack2unpack_arb(data_16_t* d_p_dt, data_16_t* s_p_dt, int arb_bit)
{
    int i;
    int p_bit, b_ptn;

    p_bit = s_p_dt->bit;
    b_ptn = 0x01;
    for ( i = 0 ; i < 16 ; i++ )
    {
        if (p_bit & b_ptn) {
            if(arb_bit & b_ptn) {
                d_p_dt->dt[i] = s_p_dt->dt[i];
            }
        }
        b_ptn = b_ptn << 1;
    }
    d_p_dt->bit |= p_bit;
}

/**
 * @brief  データ unpack -> pack 差分コピー関数（前回と異なるデータのみ転送）
 * @param  src : unpack source data    (input)
 * @param  des : pack destination data (output)
 * @param  pre : unpack previous data  (renew)
 * @retval     : data size(byte)
 */
int data_def_unpack2pack(mctrl_dt16_t* src, mctrl_dt16_t* des, mctrl_dt16_t* pre)
{
    int   sz = sizeof(mctrl_hd_t);    // set header size
    short mask;
    short b_ptn = 0;
    short bit   = 1;
    int   i, j;

    mask       = src->b_ptn;
    pre->b_ptn = src->b_ptn;
    pre->hd    = src->hd ;              // copy header
    des->hd    = src->hd ;              // copy header
//    sz         = sizeof(mctrl_hd_t);    // set header size
    j = 0;
    for ( i = 0 ; i < 16 ; i++ ) {
        if ( mask & bit) {              // set data ?
            if ( src->dt[i] != pre->dt[i] ) {	// different ?
                pre->dt[i] = src->dt[i];
                des->dt[j] = src->dt[i];
                b_ptn |= bit;           // set bit pattern
                sz += 2;                // add size
                j++;                    // next position
            }
        }
        bit <<= 1;                      // shift bit
    }
    des->b_ptn = b_ptn;                 // set bit pattrn
    sz += 2;
    return sz;
}

