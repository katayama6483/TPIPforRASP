/**
 * @brief  TRACE Module Header file
 * @file   trace.h
 * @author Katayama
 */
#ifndef ___TRACE_H___
#define ___TRACE_H___

extern void chk_point(short id, short vol);
extern void init_trace(void);
extern void trigger(int s);
extern void tr_off(void);
extern void trace_enable(int on);
extern int  Get_TraceON(void);

#ifdef ___TRACE_INIT___
#define INIT_TRACE()         init_trace()
#define TRIGGER(s)           trigger(s)
#else
#define INIT_TRACE()
#define TRIGGER(s)
#endif // #ifdef ___TRACE_INIT___

#ifdef ___TRACE_ON___
#define CHECK_POINT(id, vol) chk_point( id, vol)
#else
#define CHECK_POINT(id, vol)
#endif // #ifdef ___TRACE_ON___
#endif

