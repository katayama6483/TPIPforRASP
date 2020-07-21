/** 
 * @file MEM_mng.c
 * @brief Memory pool manager program module
 *
 * @author  Katayama
 * @date    2020-05-27
 * @version 1.00 2020/05/27
 *
 * Copyright (C) 2020 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include "MEM_mng.h"

#define BASE_SIZE     (32*1024)
#define BLOCK_SIZE    (16*1024)


static void* mem_ptr;
static unsigned long mem_size;

/** Initialize Memory pool manager
 *  @fn     int MEM_init(unsigned long size_init)
 *  @param  size_init : initalize memory size
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int MEM_init(unsigned long size_init)
{
	int err = -1;
	
	mem_ptr  = NULL;
	mem_size = 0;
	
	if (size_init > 0) {
		mem_ptr  = malloc(size_init);
		if (mem_ptr) {
			mem_size = size_init;
			err = 0;
		}
	}
	
	return err;
}

/** Endding Memory pool manager
 *  @fn     int MEM_end(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error
 *
 */
int MEM_end(void)
{
	int err = -1;
	
	if (mem_ptr) {
		if (mem_size) {
			free(mem_ptr);
			mem_size = 0;
			err = 0;
		}
		mem_ptr = NULL;
	}
	
	return err;
}

/** Get Memory block
 *  @fn     void* MEM_get(unsigned log req_size)
 *  @param  req_size : request memory size
 *  @retval != 0 : OK memory pointer
 *  @retval == 0 : error
 *
 */
void* MEM_get(unsigned long req_size)
{
	void* mem_p = mem_ptr;
	unsigned long _rq_size = ((req_size / BLOCK_SIZE)+ 1)* BLOCK_SIZE;
	
	if ((mem_size < req_size)||(mem_size > (_rq_size * 2))) {
		if (mem_ptr) {
			free (mem_ptr);
			mem_ptr  = NULL ;
			mem_p    = NULL;
		}
		mem_size = 0;
		mem_p  = malloc (_rq_size);
		if (mem_p) {
			mem_ptr = mem_p;
			mem_size = _rq_size;
//			printf("[%s:%s] mem_p=%p [mem_ptr=%p] mem_size=%d [req_size =%d]\n", __FILE__,__func__, mem_p, mem_ptr, mem_size, req_size);
		}
	}
//	printf("[%s:%s] mem_p=%p [mem_ptr=%p] mem_size=%d [req_size =%d]\n", __FILE__,__func__, mem_p, mem_ptr, mem_size, req_size);
	return mem_p;
}

/** Relese Memory block
 *  @fn     int MEM_relese(void)
 *  @retval == 0 : OK memory pointer
 *  @retval <  0 : error
 *
 */
int MEM_relese(void)
{
	int err = 0;
	
	return err;
}
