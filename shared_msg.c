/** 
 * @file shared_msg.c
 * @brief shared message program
 *
 * @author Katayama
 * @date 2019-07-23
 * @version 1.00  2019/07/23 katayama
 *
 */
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>

#include "shared_msg.h"


static int fd = -1;
static char *map = MAP_FAILED;
static long map_size = 0;


/** split of message
 * @fn     int split_MSG(char* msg, char* delimit, char *block[], int max)
 * @param  msg      : message top address
 * @param  delimit  : delimit string(ex:",")
 * @param  block    : Separated section
 * @param  max      : Maximum sections
 * @retval          : Number of separations
 *
 */
int split_MSG(char* msg, char* delimit, char *block[], int max)
{
	int n = 0;
	char* ptr;
	
	ptr = strtok(msg, delimit);
	while (ptr) {
		block[n] = ptr;
		ptr = strtok(NULL, delimit);
		n++;
		if (n >= max) break;
	}
	return n;
}


/** Initialize shared message
 * @fn     int init_share_MSG(char* fl_name, int fl_size, int mode)
 * @param  fl_name :
 * @param  fl_size : 
 * @param  mode    : <0: non create, 1: create>
 * @retval ==  0   : OK
 * @retval == -1   : ERROR file open
 * @retval == -2   : ERROR ftruncate()
 * @retval == -3   : ERROR mmap()
 *
 */
int init_share_MSG(char* fl_name, int fl_size, int mode)
{
	long page_size = getpagesize();
	int err;
	
	map_size  = (fl_size / page_size + 1) * page_size;	// ページサイズからマッピング時のサイズを計算
//	printf(" FILE_SIZE = %d\n page_size = %d\n map_size = %d\n", fl_size, page_size, map_size);
	
	if (mode == 1) { // create mode
		fd = open(fl_name, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH | S_IRGRP | S_IWGRP);
		if (fd < 0) return -1;
		
		err = ftruncate(fd, map_size);	// fileをmap_size分zeroで埋めてfile sizeを確保。
		if (err) {
			close(fd);
			fd = -1;
			return -2;
		}
	}
	else { // non create mode
		fd = open(fl_name, O_RDWR);
		if (fd < 0) return -1;
	}
	
	map = (char*)mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		close(fd);
		fd = -1;
		munmap(map, map_size);
		map      = MAP_FAILED;
		map_size = 0;
		return -3;
	}
	return 0;
}

/** Close shared message
 * @fn     int close_share_MSG(void)
 * @retval ==  0   : OK
 * @retval == -1   : ERROR Already close
 *
 */
int close_share_MSG(void)
{
	if (fd < 0) return -1;
	
	close(fd);
	fd = -1;
	munmap(map, map_size);
	map      = MAP_FAILED;
	map_size = 0;
	return 0;
}

/** Set shared message
 * @fn     int set_share_MSG(char* msg, int max_)
 * @param  msg     : message top address
 * @param  max_    : message max size
 * @retval >   0   : OK <size of message>
 * @retval ==  0   : ERROR Message size is zero or Invalid
 * @retval == -1   : ERROR Message size is over
 * @retval == -2   : ERROR not initialize MMAP
 *
 */
int set_share_MSG(char* msg, int max_)
{
	int size = strnlen(msg, max_);
	
	if (map == MAP_FAILED) return -2;
	if (size <= 0)         return  0;
	if (size > 256-1)      return -1;
	if (size >= map_size -1)  return -1;
	
	strncpy(&map[1], msg, map_size-1);
	msync(map, map_size, MS_SYNC);	// Synchronize memory and files
	map[0] = size & 0xff;
	msync(map, map_size, 0);	// Synchronize memory and files
	return size;
}

/** Clear shared message
 * @fn     int clear_share_MSG(void)
 * @retval ==  0   : OK
 * @retval == -2   : ERROR not initialize MMAP
 *
 */
int clear_share_MSG(void)
{
	if (map == MAP_FAILED) return -2;
	
	map[1] = 0;
	msync(map, map_size, MS_SYNC);	// Synchronize memory and files
	map[0] = 0;
	msync(map, map_size, 0);	// Synchronize memory and files
	return 0;
}

/** Get shared message
 * @fn     int get_share_MSG(char* msg, int bf_size)
 * @param  msg     : save buffer address
 * @param  bf_size : buffer size
 * @retval >   0   : OK <size of message>
 * @retval ==  0   : ERROR Message size is zero or Invalid
 * @retval == -1   : ERROR Message size is over
 * @retval == -2   : ERROR not initialize MMAP
 *
 */
int get_share_MSG(char* msg, int bf_size)
{
	int size1;
	int size2;
	
	if (map == MAP_FAILED) return -2;

	size1 = (unsigned char)map[0];
	if (size1 == 0)  return 0;
	size2 = strnlen(&map[1], map_size-2);
	if (size1 != size2) {
//		printf("\n #### size1=%d size2=%d ###\n", size1, size2);
		return  -3;	// test
	}
	if (size1 >= map_size)  return  0;
	if (size1 > 0) {
		strncpy(msg, &map[1], bf_size -1);
		if (size1 >= bf_size -1) {
			size1 = bf_size -1;
			msg[size1+1] = 0;
		}
	}
	return size1;
}


