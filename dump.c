/**
 * @file  dump.c
 * @brief 通信data dump
 *
 * @author Katayama (Sanritz Automation Co., Ltd.)
 * @date 2008-09-09
 * @version v 1.00 2008/09/09 katayama
 * @version V 1.01 2011/02/04 katayama
 *
 * Copyright (C) 2007 Sanritz Automation Co.,Ltd. All rights reserved.
 */

// Ver 1.01 Limit check dump file size

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>

static FILE  *fd;
static int   dump_flg  = 0;
static int   dump_cnt  = 0;
static int   dump_mode = 0;

static char  msg[256];
static char  msgbuf[1024*8];

/**
 * @brief  DUMP Enable 処理
 * @param  on   : 0== diseble, 0< enable + dump count
 * @param  mode : dump mode
 */
void dump_enable(int on, int mode)
{
    dump_flg  = on;
    dump_cnt  = on;
    dump_mode = mode;
}

/**
 * @brief  DUMP 初期化処理
 */
void dump_init(void)
{
    msgbuf[0] = 0;
}

/**
 * @brief  DUMP 終了処理
 */
void dump_end(void)
{
    int file_sz;

    if (dump_flg) {
        fd = fopen("/tmp/dump.txt","a");
        file_sz = ftell(fd);
        if (file_sz < 40*1024) {	// ver 1.01
            fputs("-start-   V  M  D  S  - 1 -- 2 -- 3 -- 4 -- 5 -- 6 -- 7 -- 8 -- 9 -\n", fd);
            fputs(msgbuf,fd);
            fputs("---end  ------------------\n", fd);
        }
        fclose(fd);
    }
}

/**
 * @brief  DUMP 出力処理＜送信＞
 * @param  p  : send data pointer
 * @param  sz : data size
 */
void dump_out(unsigned short* p, int sz)
{
    int i;
    int sz_;

    if (!dump_flg) {
        return;
    }
    sz_ = (sz + 1)/2;
    if (dump_cnt) {
        dump_cnt--;
        sprintf(msg,"[%02d:%2d]s<%02X %02X %02X %02X> ", dump_cnt, sz
                , (unsigned char)(p[0]& 0x00ff), (unsigned char)((p[0] >> 8) & 0x00ff)
                , (unsigned char)(p[1]& 0x00ff), (unsigned char)((p[1] >> 8) & 0x00ff));
        strcat(msgbuf,msg);
        for ( i=2 ; i< sz_ ; i++)
        {
            sprintf(msg," %04X", p[i]);
            strcat(msgbuf,msg);
        }
        strcat(msgbuf,"\n");
    }
}

/**
 * @brief  DUMP 出力処理＜受信＞
 * @param  p  : receive data pointer
 * @param  sz : data size
 */
void dump_out_R(unsigned short* p, int sz)
{
    int i;
    int sz_;

    if (!dump_flg) {
        return;
    }
    sz_ = (sz + 1)/2;
    if (dump_cnt) {
        dump_cnt--;
        sprintf(msg,"[%02d:%2d]r<%02X %02X %02X %02X> ", dump_cnt, sz
                , (unsigned char)(p[0]& 0x00ff), (unsigned char)((p[0] >> 8) & 0x00ff)
                , (unsigned char)(p[1]& 0x00ff), (unsigned char)((p[1] >> 8) & 0x00ff));
        strcat(msgbuf,msg);
        for ( i=2 ; i< sz_ ; i++)
        {
            sprintf(msg," %04X",(int)((unsigned short)p[i]));
            strcat(msgbuf,msg);
        }
        strcat(msgbuf,"\n");
    }
}

/**
 * @brief DUMP イベント・トレース出力処理
 * @param  sz : data size
 * @param  buf: send data pointer
 */
void dump_event(int sz, char* buf)
{
    int i;

    if (!dump_flg) {
        return;
    }
    if (dump_cnt) {
        dump_cnt--;
        sprintf(msg,"[%02d:%2d]<Event>", dump_cnt, sz);
        strcat(msgbuf,msg);
        for ( i=0 ; i< sz ; i++)
        {
            sprintf(msg," %02X",(int)((unsigned char)buf[i]));
            strcat(msgbuf,msg);
        }
        strcat(msgbuf,"\n");
    }
}

/**
 * @brief Get Dump ON flag
 * @fn     int Get_dumpON(void)
 * @retval : 0== diseble, 0< enable + dump count
 */
int Get_dumpON(void)
{
    return dump_flg;
}

/**
 * @brief  Get Dump mode
 * @fn     int Get_dump_mode(void)
 * @retval : dump mode
 */
int Get_dump_mode(void)
{
    return dump_mode;
}

