##
# @file    Makefile(for Raspberry Pi)
# @brief   TPIP for Raspberry Pi program Makefile
# 
# @author  katayama
# @date    2018/10/30
# @version 1.0.0 (2018/10/30)
# @version 1.0.1 (2020/05/27)
# 
# Copyright (C) 2018 TPIP User Community All rights reserved.
# このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
# 使用許諾をします。
#
##


CROSS_COMPILE?=
CC=$(CROSS_COMPILE)gcc
LINK=$(CROSS_COMPILE)gcc


##
# include path
##
INC=


##
# additional library
##

LIB=-lpthread -lwiringPi


##
# input source name
##

OBJ = main.o \
      jtcp_com_sv.o \
      jtp_com_sv.o \
      ctrl_mng.o \
      ctrl_com.o \
      sioDrv.o \
      JPEG_read.o \
      v4l2_capture.o \
      time_sub.o \
      data_pack.o \
      que_buf.o \
      set_config.o \
      kbhit.o \
      lnx_UDP_pl.o \
      camera_chg.o \
      wlan_mng.o \
      shared_msg.o \
      MEM_mng.o \
      dump.o \
      trace.o \
      version.o


##
# output binary name
##

TARGET = TPIPforRaspi


all: $(TARGET)

$(TARGET): $(OBJ)
	$(LINK) -o $(TARGET) $(OBJ) $(LIB) -lm

%.o: %.c
	$(CC) $(INC) -O3 -c $^ -o $@

.PHONY: clean
clean:
	rm -f $(TARGET) $(OBJ)


