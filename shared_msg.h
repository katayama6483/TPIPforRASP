/** 
 * @file shared_msg.h
 * @brief shared message header file
 *
 * @author Katayama
 * @date 2019-07-24
 * @version 1.00  2019/07/24 katayama
 *
 */
#ifndef __SHARED_MSG_H__
#define __SHARED_MSG_H__


extern int split_MSG(char* msg, char* delimit, char *block[], int max);

extern int init_share_MSG(char* fl_name, int fl_size, int mode);
extern int close_share_MSG(void);
extern int set_share_MSG(char* msg, int max_);
extern int clear_share_MSG(void);
extern int get_share_MSG(char* msg, int bf_size);

#endif  // #ifndef __SHARED_MSG_H__

