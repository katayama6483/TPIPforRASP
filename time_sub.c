/**
 * @brief  時間計測サブルーチン
 * @file   time_sub.c
 * @author Katayama
 */

#include <stdio.h>
#include <sys/time.h>

extern int abs(int j);

typedef unsigned long u_long;

static u_long         net_TM  = 0;         // network time( <-PC)
static struct timeval set_TM;              // set time of net_TM


typedef struct {
    u_long          rcv_TM; // recieve TMstmp
    u_long          delay;  // Delay time
    struct timeval  r_TM;   // recieve TMstmp time
} TMinf_t;

#define MAX_BLOCK (4096)
static int     TstmpGetPos;
static int     TstmpPutPos;
static int     TstmpBufCnt;
static TMinf_t TstmpBuf[MAX_BLOCK];

/**
 * @brief Initialize Time stamp BufferBlock
 */
void init_TstmpBuf(void)
{
    TstmpGetPos = 0;
    TstmpPutPos = 0;
    TstmpBufCnt = 0;
}

/**
 * @brief Get TimeStamp Buffer
 * @return ?
 */

TMinf_t* get_TstmpBuf(void)
{
    TMinf_t *ans;

    if (TstmpBufCnt >= MAX_BLOCK) {
        TstmpGetPos = (TstmpPutPos + 1)% MAX_BLOCK;
        TstmpBufCnt = MAX_BLOCK -1;
    }
    ans = &TstmpBuf[TstmpPutPos];
    TstmpBufCnt++;
    TstmpPutPos = (TstmpPutPos + 1)% MAX_BLOCK;
    return (ans);
}

/**
 * @brief Search Recieve TMstamp
 * @param[in] TMstmp : ?
 * @return ?
 */

TMinf_t* str_TstmpBuf(u_long TMstmp)
{
    TMinf_t *ans;
    int     i;
    int     getpos;

    ans = NULL;
    getpos = TstmpGetPos;
    for ( i = 0 ; i < TstmpBufCnt; i++ )
    {
        if ( TstmpBuf[getpos].rcv_TM > TMstmp ) {
            TstmpBufCnt = TstmpBufCnt - i ;
            TstmpGetPos = getpos;
            break;
        }
        if ( TstmpBuf[getpos].rcv_TM == TMstmp ) {
            ans = &TstmpBuf[getpos];
            TstmpBufCnt = TstmpBufCnt-(i+1);
            TstmpGetPos = (getpos + 1)% MAX_BLOCK;
            break;
        }
        getpos = (getpos + 1)% MAX_BLOCK;
    }
    return(ans);
}


/**
 * @brief 時間差の計算１　(t1 - t2)=def_time[ms]
 * @param *t1 : ?
 * @param *t2 : ?
 * @return ?
 */

long Def_time(struct timeval* t1, struct timeval* t2)
{
    long _usec;
    long _sec;

    _usec  = t1->tv_usec - t2->tv_usec;
    _sec   = t1->tv_sec  - t2->tv_sec;
    if (_usec < 0) {
        _usec = _usec + 1000000;
        _sec--;
    }
    return ((_sec*1000) + _usec/1000);
}

/**
 * @brief  時間差の計算２　(t1 - t2)=def_time[us]
 * @param *t1 : ?
 * @param *t2 : ?
 * @return ?
 */

long Def_time_u(struct timeval* t1, struct timeval* t2)
{
    long _usec;
    long _sec;

    _usec  = t1->tv_usec - t2->tv_usec;
    _sec   = t1->tv_sec  - t2->tv_sec;
    if (_usec < 0) {
        _usec = _usec + 1000000;
        _sec--;
    }
    return ((_sec*1000000) + _usec);
}

/**
 * @brief  Get Tick Count(Tick Count= ms)
 * @return ?
 */
unsigned long GetTickCount(void)
{
    struct   timeval tv;
    unsigned long    usec;
    unsigned long    sec;

    gettimeofday(&tv,NULL);
    usec = tv.tv_usec ;
    sec  = tv.tv_sec;
    return (sec*1000 + usec/1000);
}

/**
 * @brief  Entry recieve network time（未使用）
 * @param[in] TM  : ?
 * @param[in] *tm : ?
 */
void ent_net_TM( unsigned long TM, struct timeval* tm )
{
    TMinf_t* _tm;

    _tm               = get_TstmpBuf();
    _tm->rcv_TM       = TM;
    _tm->r_TM.tv_usec = tm->tv_usec;
    _tm->r_TM.tv_sec  = tm->tv_sec;
}

/**
 * @brief  Entry recieve network time（使用）
 * @param[in] TM : 受信したネットワーク時間
 */
void ent_net_TM_now(unsigned long TM)
{
    TMinf_t* tm;

    tm         = get_TstmpBuf();
    tm->rcv_TM = TM;
    gettimeofday(&tm->r_TM,NULL);
}
/**
 * @brief  Get time stamp
 * @return ?
 */
unsigned long GetTimeSTMP(void)
{
    struct timeval now_TM;
    unsigned long  ans;

    if (net_TM == 0) {
        return (0);
    }
    gettimeofday(&now_TM,NULL);
    ans = net_TM + Def_time(&now_TM,&set_TM);
    return (ans);
}

/**
 * @brief  Reset time stamp
 */
void resetTimeSTMP(void)
{
    net_TM = 0;
    init_TstmpBuf();
}

/**
 * @brief  Append time stamp information
 * @param[in] rcv_TMstmp : 受信したタイムスタンプ
 * @param[in] delay_tm   : 受信した通信遅延時間
 * @param[in] rcv_stayTM : 受信した通信滞留時間
 */
void apnd_net_time(u_long rcv_TMstmp, u_long delay_tm, u_long rcv_stayTM)
{
    u_long         p_TMstmp;
    u_long         n_TMstmp;
    u_long         r_TMstmp;
    int            d_TM;
    TMinf_t        *tm;
    struct timeval now_TM;

    /*  */
    p_TMstmp = rcv_TMstmp - delay_tm - rcv_stayTM;
    if ((tm=str_TstmpBuf(p_TMstmp)) != NULL)
    {
        if (net_TM == 0) {

            /* 初回 */
            net_TM = tm->rcv_TM + (delay_tm / 2);
            set_TM.tv_usec = tm->r_TM.tv_usec;
            set_TM.tv_sec  = tm->r_TM.tv_sec;
        }
        else {

            /* ２回目以降 */
            gettimeofday(&now_TM,NULL);
            n_TMstmp = net_TM + Def_time(&now_TM,&set_TM);
            r_TMstmp = tm->rcv_TM + (delay_tm / 2) + Def_time(&now_TM,&tm->r_TM);
            d_TM     = abs(n_TMstmp - r_TMstmp);
            if (( d_TM > 100 )&&( d_TM < 300 )) {
                if ( n_TMstmp > r_TMstmp ) {
                    net_TM--;
                }
                if ( n_TMstmp < r_TMstmp ) {
                    net_TM++;
                }
            }
            if ( d_TM >= 300 ) {
                net_TM = tm->rcv_TM + (delay_tm / 2);
                set_TM.tv_usec = tm->r_TM.tv_usec;
                set_TM.tv_sec  = tm->r_TM.tv_sec;
            }
        }

    }
}

/* @@@ */
void timer_start( struct timeval *seed_time )
{
    if( seed_time == NULL )
    {
        return;
    }

    gettimeofday( seed_time, NULL );
}

int timer_dif_time_get( struct timeval *seed_time )
{
    struct timeval now_time;
    
    if( seed_time == NULL )
    {
        return( -1);
    }
    
    gettimeofday( &now_time, NULL );
    return( ((now_time.tv_sec - seed_time->tv_sec) * 1000) + ((now_time.tv_usec - seed_time->tv_usec) / 1000) );
}

