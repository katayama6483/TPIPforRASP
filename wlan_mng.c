/**
 * @file   wlan_mng.c
 * @brief  WLAN 管理　モジュール
 * @author Katayama
 * @version 1.00  2015/10/06 katayama
 * @version 1.10  2019/07/10 katayama(Raspberry Pi 対応)
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

static char      w_lnk_msg[32];
static char      ap_mac_msg[32];
static char      *non_ap_mac_00 ="00:00:00:00:00:00";
static char      *non_ap_mac_ff ="FF:FF:FF:FF:FF:FF";
static int       w_lnk;

static int       f_wlan_mng;
static pthread_t wlan_mng_info;


char *str_cut(char* msg, char* st_char, char* ed_char, char* get_msg)
{
    char *st, *ed, *ans;
    int n;

    ans = NULL;

    if((st = strstr(msg,st_char))!=NULL)
    {
        if ((ed = strstr(st,ed_char))!=NULL)
        {
            st = st + strlen(st_char);
            n = ed - st;
            if ( n > 0)
            {
                strncpy( get_msg,st,n);
                get_msg[n]=0;
                ans = st;
            }
        }
    }

    return (ans);
}

/**
 * @brief WLAN情報取出し　関数
 */

int load_wlan_info(void)
{
    FILE *fd;
    char msg[256];
    int  n;
    int  ans = -1;
	int  wlnk = 0;
	
    msg[0]=0;
    system("iwconfig wlan0 > /tmp/w_stat.txt");
    fd  = fopen("/tmp/w_stat.txt","r");
    n = 0;
    while (fgets(msg,255,fd))
    {
        if((n == 0)&&(str_cut(msg,"Access Point: ","  ",ap_mac_msg)!=NULL) )
        {
            n = 1;
        }
        if((n == 1)&&(str_cut(msg,"Link Quality=","/",w_lnk_msg)!=NULL))
        {
            sscanf(w_lnk_msg,"%d",&wlnk);
            if ((strcmp(ap_mac_msg, non_ap_mac_00)==0)
                    ||(strcmp(ap_mac_msg, non_ap_mac_ff)==0)) {
                wlnk = 0;
            }
            n = 2;
            break;
        }
        if((n==0) &&(str_cut(msg,"Link Quality=","/",w_lnk_msg)!=NULL))
        {
            n=1;
        }
    }
    fclose( fd );
    if ( n > 0) {
        w_lnk = wlnk;
        ans = 0;
    }
    if ( n <= 0) {
        w_lnk = 0;
    }
    return (ans);
}

/**
 * @brief WLAN管理　タスク
 */
void  t_wlan_mng( unsigned long arg )
{
    int i;
    int ans;

    f_wlan_mng = 1;
    while (f_wlan_mng)
    {
        ans = load_wlan_info();
        if (ans == -1) {
            f_wlan_mng= -1;
        }
        for ( i = 0 ; i < 10 ; i++ )
        {
            usleep(100*1000);
            if (f_wlan_mng < 0) {
                break ;
            }
        }
        if (f_wlan_mng < 0) {
            f_wlan_mng = 0;
        }
    }
}


/**
 * @brief WLAN管理　初期化関数
 */
int init_wlan_mng(void)
{
    w_lnk = 0;
    f_wlan_mng = 0 ;
    pthread_create( &wlan_mng_info, NULL, (void(*))t_wlan_mng, (void *)NULL );
    return(0);
}

/**
 * @brief WLAN管理　終了関数
 */
int del_wlan_mng(void)
{

    if (f_wlan_mng > 0)
    {
        f_wlan_mng = -1;
        while (f_wlan_mng != 0) {
            usleep(1*1000);
        }
    }
    if (f_wlan_mng == 0) {
        pthread_join( wlan_mng_info, NULL );
    }
    else {
        return(1);
    }

    return(0);
}

/**
 * @brief WLAN情報取出し　関数
 */
int get_wlan_info(void)
{
    return (w_lnk);
}

