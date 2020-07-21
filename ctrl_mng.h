/**
 * @file  ctrl_mng.h
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

#ifndef ___CTRL_MNG_H___
#define ___CTRL_MNG_H___



/**==========================================================================================
 *   #define
 **==========================================================================================*/

/**==========================================================================================
 *   構造体定義
 **==========================================================================================*/
//extern struct ctrl_data_STR ctrl_data;

extern int init_ctrl_mng();
extern int del_ctrl_mng();

extern int set_ctrl_eu2sdata(char *buf, int size);
extern int get_ctrl_eu2sdata( int bno, char *buf, int buf_sz, int flg);
extern int set_ctrl_s2eudata( int bno, char *buf, int size, int flg);
extern int get_ctrl_s2eudata(char *buf, int buf_sz);

extern void set_owner_mask( int bno, unsigned short do_mask, unsigned short dt_mask, unsigned short add_mask );


#endif	// #ifndef ___CTRL_MNG_H___

