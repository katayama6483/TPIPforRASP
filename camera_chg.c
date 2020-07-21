/** 
 * @file camera_chg.c
 * @brief camera change control([Raspberry Pi]Multi Camera Adapter Module) program
 *
 * @author Katayama
 * @date 2019-04-19
 * @version 1.00  2019/04/19 katayama
 * @version 1.10  2020/01/14 katayama SANRITZ 4ch camera prototype board
 *
 * Copyright (C) 2019 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#include <stdio.h>
#include <wiringPi.h>
#include "camera_chg.h"

//#define _MULTI_CAM_

#define PIN_29  5	// GPIO_05
#define PIN_31  6	// GPIO_06

static int mult_cam_on = 0;		// Connected multi camera CH (b0:chA, b1:chB, b2:chC, b3:chD)


/** Initialize camera_chage_control Program
 * @fn     int init_cam_chg(int cam_num)
 * @param  cam_num : Camera Number(0: camera A, 1: camera B, 2: camera C, 3: camera D)
 * @retval >= 0    : OK (CAM Num [0 - 3])
 * @retval == -1   : error
 */
int init_cam_chg(int cam_num)
{
	int err = 0;
	int _cam_num = cam_num & 0x03;
	int i;
	
	if (mult_cam_on) {
		if(wiringPiSetupGpio() == -1) return -1;
	
		pinMode(PIN_29, OUTPUT);
		pinMode(PIN_31, OUTPUT);
	
		int _bit_ptn;
		for ( i = 0; i < 4; i++) {
			_bit_ptn = (1 << _cam_num) & 0x0F;
			if (mult_cam_on & _bit_ptn) {
				break;
			}
			_cam_num = (_cam_num + 1)  & 0x03;
		}

		if ( i < 4) {
			err = _cam_num;
			chg_cam(_cam_num);
		}
		else err = -1;
	}

	return(err);
}

/** Close camera_chage_control Program
 * @fn     int closecam_chg(void)
 * @retval == 0  : OK
 */
int close_cam_chg(void)
{
	int err = 0;

	if (mult_cam_on) {
		pinMode(PIN_29, INPUT);
		pinMode(PIN_31, INPUT);
	}
	return(err);
}

/** int change_camera Program
 * @fn     int chg_cam(int cam_no)
 * @param  cam_num : Camera Number(0: camera A, 1: camera B, 2: camera C, 3: camera D)
 * @retval == 0    : OK
 * @retval == -1   : error
 */
int chg_cam(int cam_num)
{
	int err = 0;

	if (mult_cam_on) {
		int _bit_ptn = 0x01;

		if ((cam_num > -1)&&(cam_num < 4)) {
			_bit_ptn = _bit_ptn << cam_num;
			if ((mult_cam_on & _bit_ptn & 0x0F)==0)  cam_num = -1;
		
			switch(cam_num) {
			case 0:		// camera A
				printf("[camera A]\n");
				digitalWrite(PIN_29, FALSE);
				digitalWrite(PIN_31, FALSE);
				break;
			
			case 1:		// camera B
				printf("[camera B]\n");
				digitalWrite(PIN_29, FALSE);
				digitalWrite(PIN_31, TRUE);
				break;
		
			case 2:		// camera C
				printf("[camera C]\n");
				digitalWrite(PIN_29, TRUE);
				digitalWrite(PIN_31, FALSE);
				break;
		
			case 3:		// camera D
				printf("[camera D]\n");
				digitalWrite(PIN_29, TRUE);
				digitalWrite(PIN_31, TRUE);
				break;
		
			default:
				printf("[camera Num error(%d)]\n", cam_num);
				err = -1;
			}
		}
	}

	return(err);
}

/** Set Multi camera mode
 * @fn     int set_multi_cam_mode(int conect_CAM)
 * @param  connect_CAM : Connected multi camera CH (b0:chA, b1:chB, b2:chC, b3:chD)
 * @retval == 0  : OK
 */
int set_multi_cam_mode(int conect_CAM)
{
	int err = 0;
	
	mult_cam_on = conect_CAM & 0x0F;
	return (err);
}

/** Get Multi camera mode
 * @fn     int get_multi_cam_mode(void)
 * @retval mult_cam_on : Connected multi camera CH (b0:chA, b1:chB, b2:chC, b3:chD)
 */
int get_multi_cam_mode(void)
{
	return (mult_cam_on);
}
