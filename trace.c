/**
 * @file   trace.c
 * @brief  プログラム　トレース　モジュール
 * @author Katayama
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

#include "time_sub.h"

#define TRACE_MAX (256)

static struct {
    short           id;
    short           v;
    struct timeval  tm;
}trace_buf[TRACE_MAX];

static int idx;
static int trace_on = 0;

/**
 * @brief  Set Check Point
 * @param[in] id  : 記録するチェックポイントのID
 * @param[in] vol : 記録するチェックポイントの値
 */
void chk_point(short id, short vol)
{
    trace_buf[idx].id = id;
    trace_buf[idx].v  = vol;
    gettimeofday(&trace_buf[idx].tm,NULL);
    idx = (idx + 1)% TRACE_MAX;
}

/**
 * @brief  Initialize trace buffer
 */
void init_trace(void)
{
    int i;
    for ( i = 0 ; i < TRACE_MAX ; i++ )
    {
        trace_buf[i].id = 0;
    }
    idx = 0;
}

/**
 * @brief  Trigger get trace data
 * @param[in] s : データ収集時間
 */
void trigger(int s)
{
    struct   timeval tv;
    char             msg[256];
    FILE             *fd;
    int              i,ix ;

    if (!trace_on) {
        return;
    }

    fd = fopen("/tmp/trace.txt","a");
    if (fd == NULL) {
        return;
    }

    gettimeofday(&tv , NULL);
    ix = idx;

    fputs("---start------------------\n", fd);
    for ( i = 0 ; i < TRACE_MAX ; i++ )
    {
        if ( trace_buf[ix].id != 0 )
        {
            sprintf(msg,"[%3d](%3d) --> (%4d)\n",trace_buf[ix].id, trace_buf[ix].v,(int)Def_time( &tv, &trace_buf[ix].tm));
            fputs( msg, fd );
        }
        ix = (ix + 1)% TRACE_MAX;
    }
    sprintf(msg,"---end  ------------[%d ms]--\n",s);
    fputs(msg, fd);
    fclose(fd);
}


/**
 * @brief Trace off
 */
void tr_off(void)
{
    trace_on = 0;
}

/**
 * @brief Trace Enable 処理
 * @retval  0  : diseble
 * @retval  0< : enable
 */
void trace_enable(int on)
{
    trace_on = on;
}

/**
 * @brief   Get Trace Enable flag
 * @retval  0  : diseble
 * @retval  0< : enable
 */
int Get_TraceON(void)
{
    return trace_on;
}

