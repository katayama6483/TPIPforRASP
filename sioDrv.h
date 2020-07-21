/**
 * @file   sioDrv.h
 * @brief  RS232 SIO Access関数群　ヘッダ
 * @author Katayama
 */
extern int  sioInit( char* ComNM, int BaudRate, int ByteSize, int Parity, int StopBits, int TimeOut );
extern int  sioClose( int fd );
extern int  sioTimedRecvByte( int fd, char *dt, int timeout );
extern int  sioTimedRecvBuffer( int fd, void *buffer, int Size, int *ActualSize, int timeout );
extern int  sioSendTransparent( int fd, char *buffer, int sndSz );
extern int  sioRecvTransparent( int fd, void *buffer, int Size, int *ActualSize, int timeout );
extern int  sioSendBuffer( int fd, void *buffer, int Size, int *ActualSize );


#define NOPARITY            (0)
#define ODDPARITY           (1)
#define EVENPARITY          (2)

#define ONESTOPBIT          (0)
#define ONE5STOPBITS        (1)
#define TWOSTOPBITS         (2)

#define SETRTS              (0x02)
#define CLRRTS              (0x00)


