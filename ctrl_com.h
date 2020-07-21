/**
 * Control Communication Header file
 * @file   ctrl_com.h
 * @brief  Control Communication Header file
 * @author Katayama
 */
#ifndef ___CTRL_COM_H___
#define ___CTRL_COM_H___

#define CTRL_BOARD_MAX                (4)
#define CTRL_BOARD_SIO                (0)

extern int  Ctrl_com_stat;

extern int init_ctrl_com( void );
//extern int  init_ctrl_com_rq( int flg );
//extern void init_ctrl_com_ex();
extern int  del_ctrl_com();
//extern int  del_ctrl_com_rq( int flg );
//extern void del_ctrl_com_ex();
extern void reset_mxmi_time();
extern int  get_ver_fw(int bno, char* ver);

extern void req_send_com(void);

extern int  get_9512FW_stat(void);


#endif /* ___CTRL_COM_H___ */
