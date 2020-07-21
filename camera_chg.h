/** 
 * @file camera_chg.h
 * @brief camera change control([Raspberry Pi]Multi Camera Adapter Module)Header file
 *
 * @author Katayama
 * @date 2019-04-19
 * @version 1.00  2019/04/19 katayama
 * @version 1.10  2020/01/21 katayama SANRITZ 4ch camera prototype board
 *
 * Copyright (C) 2019 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#ifndef __CAMERA_CHG_H___
#define __CAMERA_CHG_H___

extern int init_cam_chg(int cam_num);
extern int close_cam_chg(void);
extern int chg_cam(int cam_num);

extern int set_multi_cam_mode(int conect_CAM);
extern int get_multi_cam_mode(void);

#endif // #ifndef __CAMERA_CHG_H___
