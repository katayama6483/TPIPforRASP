/**
 * @file version.c
 * @brief Set version infomation  Module
 *
 * @author Katayama
 * @date 2009-11-25
 * @version 1.00 : 2009/11/25 katayama
 * @version 1.10 : 2020/01/14 katayama SANRITZ 4ch camera prototype board
 * @version 1.12 : 2020/01/22 katayama SANRITZ 4ch camera prototype board config.txt対応
 * @version 1.13 : 2020/02/12 katayama ch4 camera 未接続時の対応
 * @version 1.20 : 2020/06/29 katayama Full HD 対応
 *
 * Copyright (C) 2009 Sanritz Automation All rights reserved.
 */

// ver 1.00 プロセス間通信対応 jpeg_sv

#include <stdio.h>
#include <unistd.h>
#include <string.h>

char  msg[]    = "TPIPforR  ver:B.20\n";    //jpeg_svバージョン
char  msg_fm[] = "9519_FW   ver:";          //SEB9519バージョン


/**
 * @brief  set version infomation SEB9516
 * @fn     int set_ver(void)
 * @retval == 0 : OK
 * @retval ==-2 : file open error
 */
int set_ver(void)
{
    FILE* fd;

    fd = fopen("/tmp/version.txt","a");
    if (fd == NULL) {
        return(-2);
    }
    fputs( msg, fd );
    fclose(fd);
    return(0);
}

/**
 * @brief  set version infomation SEB9512 FW
 * @fn     int set_ver_fw(char* ver)
 * @param  ver  : version number message
 * @retval == 0 : OK
 * @retval ==-2 : file open error
 */
int set_ver_fw(char* ver)
{
    FILE* fd;

    fd = fopen("/tmp/version.txt","a");
    if (fd == NULL) {
        return(-2);
    }
    fputs( msg_fm, fd );
    fputs( ver, fd );
    fputs( "\n", fd);
    fclose(fd);
    return(0);
}

