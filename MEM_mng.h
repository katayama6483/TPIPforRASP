/** 
 * @file MEM_mng.h
 * @brief Memory pool manager program module
 *
 * @author  Katayama
 * @date    2020-05-27
 * @version 1.00 2020/05/27
 *
 * Copyright (C) 2020 TPIP User Community All rights reserved.
 * ���̃t�@�C���̒��쌠�́ATPIP���[�U�[�R�~���j�e�B�̋K��ɏ]��
 * �g�p���������܂��B
 */

#ifndef __MEM_MNG_H__
#define __MEM_MNG_H__

extern int MEM_init(unsigned long size_init);
extern int MEM_end(void);
extern void* MEM_get(unsigned long req_size);
extern int MEM_relese(void);


#endif // #ifndef __MEM_MNG_H__
