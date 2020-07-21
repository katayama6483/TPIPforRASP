/**
 * @file que_buf.c
 * @brief Queue(待ち行列)buffer管理関数
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2011-10-12
 * @version Ver 1.00 2011/10/12 katayama
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */
#include <stdio.h>
#include <string.h>
#include "que_buf.h"


/**
 * @brief  Initialize Queue buffer
 * @fn     int Init_que(def_que_t* p)
 * @param  p    : queue buffer management infomation pointer
 * @retval == 0 : NG
 * @retval != 0 : OK
 */
int Init_que(def_que_t* p)
{
    int i;

    for ( i = 0 ; i < MAX_ARRAY ; i++) {
        p->data_sz[i] = 0;
        memset(&p->data_buf[i], 0, sizeof(mctrl_dt_t));
    }
    p->get_pos = 0;
    p->put_pos = 0;
    p->buf_cnt = 0;
    return (1);
}

/**
 * @brief  Is get data
 * @fn     int Is_get(def_que_t* p)
 * @param  p   : queue buffer management infomation pointer
 * @retval ==0 : no data
 * @retval !=0 : get data size
 */
int Is_get(def_que_t* p)
{
    return p->buf_cnt;
}


/**
 * @brief  Get Queue data
 * @fn     int Get_que(def_que_t* p, mctrl_dt_t* buf, int buf_sz)
 * @param  p      : queue buffer management infomation pointer
 * @param  buf    : data buffer pointer
 * @param  buf_sz : size of buffer
 * @retval ==0    : no data
 * @retval !=0    : get data size
 */
int Get_que(def_que_t* p, mctrl_dt_t* buf, int buf_sz)
{
    int ans = 0;

    if (p->buf_cnt) {
        ans = p->data_sz[p->get_pos];
        if ((ans == 0)||(ans > buf_sz)) {
            ans = 0;
        }
        if (ans > 0) {
            memcpy(buf, &p->data_buf[p->get_pos], ans);
        }
        p->data_sz[p->get_pos] = 0;
        p->get_pos             = (p->get_pos + 1) % MAX_ARRAY;
        p->buf_cnt--;
    }
    return ans;
}

/**
 * @brief  Put Queue data
 * @fn     int Put_que(def_que_t* p, mctrl_dt_t* buf, int sz)
 * @param  p   : queue buffer management infomation pointer
 * @param  buf : data buffer pointer
 * @param  sz  : data size
 * @retval ==0 : put error
 * @retval !=0 : put OK
 */
int Put_que(def_que_t* p, mctrl_dt_t* buf, int sz)
{
    int ans = 0;

    if (sz > sizeof(mctrl_dt_t)) {
        sz = sizeof(mctrl_dt_t);
    }
    if (p->buf_cnt < MAX_ARRAY) {
        if (sz > 0) {
            memcpy(&p->data_buf[p->put_pos], buf, sz);
        }
        p->data_sz[p->put_pos] = sz;
        p->put_pos = (p->put_pos + 1)% MAX_ARRAY;
        p->buf_cnt++;
        ans = sz;
    }
    return ans;
}

