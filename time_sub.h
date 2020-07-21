/**
 * @brief  Time subroutine Header file
 * @file   time_sub.h
 * @author Katayama
 */

#ifndef ___TIME_SUB_H___
#define ___TIME_SUB_H___

#include <sys/time.h>

extern long Def_time(struct timeval* t1, struct timeval* t2);
extern long Def_time_u(struct timeval* t1, struct timeval* t2);
extern unsigned long GetTickCount(void);
extern void ent_net_TM(unsigned long TM,struct timeval* tm);
extern void ent_net_TM_now(unsigned long TM);
extern void apnd_net_time(u_long rcv_TMstmp, u_long delay_tm, u_long rcv_stayTM);
extern unsigned long GetTimeSTMP(void);
extern void resetTimeSTMP(void);

/* @@@ */
extern void timer_start( struct timeval *seed_time );
extern int  timer_dif_time_get( struct timeval *seed_time );

#endif

