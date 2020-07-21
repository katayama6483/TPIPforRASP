/**
 * @file que_buf.h
 * @brief Queue(待ち行列)buffer管理関数 Header file
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2011-10-12
 * @version Ver 1.00 2011/10/12 katayama
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */

#ifndef ___QUE_BUF_H___
#define ___QUE_BUF_H___

#include "def_mctrl_com16.h"


#define MAX_ARRAY  (50)

typedef struct {
    mctrl_dt_t data_buf[MAX_ARRAY]; // queue buffer
    int data_sz[MAX_ARRAY];         // data size
    int get_pos;                    // get position
    int put_pos;                    // put position
    int buf_cnt;                    // queue count
} def_que_t;

extern int Init_que(def_que_t* p);
extern int Is_get(def_que_t* p);
extern int Get_que(def_que_t* p, mctrl_dt_t* buf, int buf_sz);
extern int Put_que(def_que_t* p, mctrl_dt_t* buf, int sz);

#endif  // #ifndef ___QUE_BUF_H___

