/** 
 * @file kbhit.c
 * @brief kbhit function program
 *
 * @author  Katayama
 * @date    2014-09-10
 * @version 1.00 2014/09/10
 *
 * Copyright (C) 2014 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

/** Function : getch
 * @fn     int getch( void )
 * @retval   : key code
 *
 */
int getch( void )
{
	struct termios oldt, newt;
	int            ch;

	tcgetattr( STDIN_FILENO, &oldt );
	newt = oldt;
	newt.c_lflag &= ~( ICANON | ECHO );
	tcsetattr( STDIN_FILENO, TCSANOW, &newt );
	ch = getchar();
	tcsetattr( STDIN_FILENO, TCSANOW, &oldt );

	return ch;
}

/** Function : kbhit
 * @fn     int kbhit(void)
 * @retval == 1 : key in
 * @retval == 0 : non key data
 *
 */
int kbhit(void)
{
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if (ch != EOF) {
		ungetc(ch, stdin);
		return 1;
	}

	return 0;
}

