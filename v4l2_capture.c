/** 
 * @file v4l2_capture.c
* @brief V4L2 video capture program
 *
 * @author  Katayama
 * @date    2018-10-11
 * @version 1.00 2018/10/11
 *
 * Copyright (C) 2018 TPIP User Community All rights reserved.
 * このファイルの著作権は、TPIPユーザーコミュニティの規約に従い
 * 使用許諾をします。
 */

#include <stdio.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <string.h>

#include <time.h>


static int    vcap_error = -1;
static struct v4l2_capability vcap;	// information of video device capabilities

#define VINPUT_COUNT 10
static int    vinput_error = -1;
static struct v4l2_input vinput[VINPUT_COUNT];	// information of video device formats list

static struct v4l2_format  fmt;

#define VFMT_LIST_MAX 10
static int fmt_d_count = 0;
static struct v4l2_fmtdesc    fmt_d[VFMT_LIST_MAX];

static struct v4l2_control vc;
struct v4l2_buffer  buf;
static long max_buf_sz = 300*1024;	// mmap buffer size 300Kbyte

//#define BUFFER_CNT 4
#define BUFFER_CNT 2
unsigned char *buffer[BUFFER_CNT];
int       buffer_size[BUFFER_CNT];
unsigned long bytesused[BUFFER_CNT];
int       cam_number[BUFFER_CNT];


/** execute ioctrl
 *  @fn     int xioctl(int fd, int request, void *arg)
 *  @param  fd     : File descriptor of video device
 *  @param  request: request ID
 *  @param  arg    : argument vector pointer
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
static int xioctl(int fd, int request, void *arg){
	int	ret;

	for(; ; ){
		ret = ioctl(fd, request, arg);
		if(ret < 0){
			if(errno == EINTR)
				continue;
			return -errno;
		}
		break;
	}
	return 0;
}

/** get information of video device capabilities.
 *  @fn     int get_v4l2_capability(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int get_v4l2_capability(int fd)
{
	int err;

	err = xioctl(fd, VIDIOC_QUERYCAP, &vcap);
	if (err) printf("xioclt(VIDIOC_QUERYCAP) = %d[%s]\n", err, strerror(-err));
	if (err != 0) err = -err;
	
	vcap_error = err;
	return err;
}

/** print information of video device capabilities.
 *  @fn     int PRT_v4l2_capability(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int PRT_v4l2_capability(void)
{
	if (vcap_error == 0) {
		int ver = vcap.version;
		printf("---- V4L2 information of video device capabilities. ----\n");
		printf("   driver      :[%s]\n", (char*)&vcap.driver);
		printf("   card        :[%s]\n", (char*)&vcap.card);
		printf("   bus_info    :[%s]\n", (char*)&vcap.bus_info);
		printf("   version     :[%u.%u.%u]\n", (ver >> 16) & 0xFF, (ver >> 8) & 0xFF, ver & 0xFF);
		printf("   capabilities:[%08X]\n", vcap.capabilities);
		printf("   device_caps :[%08X]\n", vcap.device_caps);
		printf("--------------------------------------------------------\n");
	}
	return vcap_error;
}

/** get image formats list
 *  @fn     int get_v4l2_input(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int get_v4l2_input(int fd)
{
	int err;
	int ans = -1;
	int i;

	for (i = 0; i < 10; i++) {
		memset(&vinput[i], 0, sizeof(struct v4l2_input));
		vinput[i].index = i;
		err = xioctl(fd, VIDIOC_ENUMINPUT, &vinput[i]);
		if (err < 0) {
			if (err == -22) {
				err = 0;
				break;
			}
			printf("xioclt(VIDIOC_ENUMINPUT) = %d[%s]\n", err, strerror(-err));
		}
	}
	ans = err;
	vinput_error = ans;
	return ans;
}

/** print image formats list
 *  @fn     int PRT_v4l2_input(void)
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int PRT_v4l2_input(void)
{
	int i;
	
	if (vinput_error == 0) {
		printf("---- V4L2 information of video input applications. ----\n");
		for ( i = 0; i < 10; i++) {
			if (vinput[i].type == 0) break;
			if (i > 0) printf("\n");
			printf("    index       :[%d]\n",   vinput[i].index);
			printf("    name        :[%s]\n",   (char*)&vinput[i].name);
			printf("    type        :[%d]\n",   vinput[i].type);
			printf("    audioset    :[%d]\n",   vinput[i].audioset);
			printf("    tuner       :[%d]\n",   vinput[i].tuner);
			printf("    std         :[%08X]\n", (int)vinput[i].std);
			printf("    status      :[%08X]\n", vinput[i].status);
			printf("    capabilities:[%08X]\n", vinput[i].capabilities);
		}
		printf("-------------------------------------------------------\n");
	}
	return vinput_error;
}

/** set Video Bitrate
 *  @fn     int set_video_rate(int fd, int bps)
 *  @param  fd   : File descriptor of video device
 *  @param  bps  : Bit rate [bps]
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int set_video_rate(int fd, int bps)
{
	int err;
	struct v4l2_control vc;
// --- set MJPEG video bit rate ---
	vc.id = V4L2_CID_MPEG_VIDEO_BITRATE;
	vc.value = bps;
	err = xioctl(fd, VIDIOC_S_CTRL, &vc);
	return err;
}

/** get Video Bitrate
 *  @fn     int get_video_rate(int fd)
 *  @param  fd   : File descriptor of video device
 *  @param  bps  : Bit rate [bps]
 *  @retval >= 0 : OK(bit rate[bps])
 *  @retval <  0 : error(-errno)
 *
 */
int get_video_rate(int fd)
{
	int err;
	int bps;

	struct v4l2_control vc;
// --- get MJPEG video bit rate ---
	vc.id = V4L2_CID_MPEG_VIDEO_BITRATE;
	vc.value = 0;
	err = xioctl(fd, VIDIOC_G_CTRL, &vc);
	bps = vc.value;
	if (err < 0) {
//		printf("xioctl(VIDIOC_G_CTRL) = %d[%s] \n", err, strerror(-err));
		bps = err;
	}
	return bps;
}

/** set Video format
 *  @fn     int set_video_format(int fd, int width, int height, int pix_fmt)
 *  @param  fd     : File descriptor of video device
 *  @param  width  : VIDEO width
 *  @param  height : VIDEO height
 *  @param  pix_fmt: pixel format(0:MJPEG, 1:YUYV)
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int set_video_format(int fd, int width, int height, int pix_fmt)
{
	int err;
	
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width  = width;
	fmt.fmt.pix.height = height;
	if (!pix_fmt) fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	else fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	fmt.fmt.pix.field  = V4L2_FIELD_NONE;
	err = xioctl(fd, VIDIOC_S_FMT, &fmt);
	if (err) printf("xioctl(VIDIOC_S_FMT) = %d[%s] \n", err, strerror(-err));
	return err;
}

/** get Video format
 *  @fn     int get_video_format(int fd)
 *  @param  fd    : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int get_video_format(int fd)
{
	int err;
	int pix_f;
	
	memset(&fmt, 0, sizeof(fmt));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err = xioctl(fd, VIDIOC_G_FMT, &fmt);
	if (err == 0) {
		printf("    type        :[%d]\n",   fmt.type);
		printf("    width       :[%d]\n",   fmt.fmt.pix.width);
		printf("    height      :[%d]\n",   fmt.fmt.pix.height);
		pix_f = fmt.fmt.pix.pixelformat;
		printf("    pixel format:[%c%c%c%c]\n",
			pix_f >> 0, pix_f >> 8, pix_f >> 16, pix_f >> 24);
	}
	else printf("xioclt(VIDIOC_G_FMT) = %d[%s] \n", err, strerror(-err));
	
	printf("get_video_format() = %d\n", err);
	return err;
}

/** get Video capture image formats list
 *  @fn     int get_v4l2_fmtList(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int get_v4l2_fmtList(int fd)
{
	int i;
	int err;
	
	fmt_d_count = 0;
	for(i = 0; i < VFMT_LIST_MAX; i++) {
		memset(&fmt_d[i], 0, sizeof(struct v4l2_fmtdesc));
		fmt_d[i].index = i;
		fmt_d[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		err = xioctl(fd, VIDIOC_ENUM_FMT, &fmt_d[i]);
		if (err == -22) {
			err = 0;
			break;	// list end
		}
		if (err != 0)   break;	// error
		fmt_d_count = i;
	}
	return err;
}

/** print Video capture image formats list
 *  @fn     void PRT_v4l2_fmtList(void)
 *  @param  fd   : File descriptor of video device
 *
 */
void PRT_v4l2_fmtList(void)
{
	int pix_f;
	int err = 0;
	int i = 0;
	printf(" --- Video capture image formats list --- \n");
	for (i = 0; i < fmt_d_count; i++) {
		pix_f = (int)fmt_d[i].pixelformat;
		if (pix_f) {
			printf("    [%i] --> <%08X>  %c%c%c%c (%s)\n",
				fmt_d[i].index, fmt_d[i].flags,
				pix_f >> 0, pix_f >> 8, pix_f >> 16, pix_f >> 24,
				(char*)&fmt_d[i].description);
		}
	}
}

/** VIDIOC_QBUF
 *  @fn     int QBUF(int fd, int index, int cam_no)
 *  @param  fd    : File descriptor of video device
 *  @param  index : index <0,1, >
 *  @param  cam_no: camera number
 *  @retval >= 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int QBUF(int fd, int index, int cam_no)
{
	int err;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index  = index;
	err = xioctl(fd, VIDIOC_QBUF, &buf);
	if (err >= 0) cam_number[index] = cam_no;
	if (err < 0) printf("xioclt(VIDIOC_QBUF) = %d[%s] index = %d\n", err, strerror(-err), index);
	return err;
}

/** VIDIOC_DQBUF
 *  @fn     int DQBUF(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK (index)
 *  @retval <  0 : error(-errno)
 *
 */
int DQBUF(int fd)
{
	int err;

	memset(&buf, 0, sizeof(buf));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index  = 0;
	err = xioctl(fd, VIDIOC_DQBUF, &buf);
	if (err < 0) printf("v_capture() ERROR: xioclt(VIDIOC_DQBUF) = %d[%s] \n", err, strerror(-err));
	if (!err) err = buf.index;
	
	return err;
}

/** Query the status of a buffer
 *  @fn     int query_buf(int fd, int index)
 *  @param  fd    : File descriptor of video device
 *  @retval >= 0  : OK
 *  @retval <  0  : error(-errno)
 *
 */
int query_buf(int fd, int index)
{
	int err;

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index  = index;
	err = xioctl(fd, VIDIOC_QUERYBUF, &buf);
	if (err < 0) printf("xioclt(VIDIOC_QUERYBUF) = %d\n", err);
	if (err < 0) return err;
	return err;
}


/** initialize mmap
 *  @fn     int init_mmap(int fd)
 *  @param  fd     : File descriptor of video device
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int init_mmap(int fd, int cam_no)
{
	int err;
	int index = 0;

	struct v4l2_requestbuffers req;
	memset(&req, 0, sizeof(req));
	req.count  = BUFFER_CNT;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;
	err = xioctl(fd, VIDIOC_REQBUFS, &req);
	if (err < 0) printf("xioclt(VIDIOC_REQBUFS) = %d\n", err);
	if (err < 0) {
		return err;
	}

	for (index = 0; index < BUFFER_CNT; index++){
		err = query_buf(fd, index);
		if (err < 0) return err;

		buffer[index]      = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		buffer_size[index] = buf.length;
//		printf("(index=%d) Length: %d  <offset:%p>Address: %p\n", index, buffer_size[index], buf.m.offset, buffer[index]);
//		printf("   Image Length: %d\n", buf.bytesused);

		err = QBUF(fd, index, cam_no);
//		printf("QBUF(%d, %d) = %d\n", fd, index, err);
		if (err < 0) return err;
	}
//	printf("--- exit init_mmap() ---\n");
	printf("init_mmap() = %d\n", err);

	return 0;
}

/** release mmap
 *  @fn     int release_mmap(int fd)
 *  @param  fd     : File descriptor of video device
 *  @retval == 0 : OK
 *  @retval <  0 : error(-errno)
 *
 */
int release_mmap(int fd)
{
	int err   = -1;
	int index = 0;
	int i;

	for (index = 0; index < BUFFER_CNT; index++){
		err = munmap(buffer[index], buffer_size[index]);
		if (err < 0) break;
	}
	printf ("release_mmap() = %d \n", err);
	return err;
}

/** video capture
 *  @fn     int v_capture(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int v_capture(int fd)
{
	int index;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
//	tv.tv_sec  = 2;
//	tv.tv_usec = 50*1000;
	tv.tv_usec = 200*1000;
    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(r <= 0)
    {
        printf("[%s:%s]Waiting for Frame\n", __FILE__,__func__);
        return -1;	// Out of frame waiting time
    }

	index = DQBUF(fd);
	if ((index >= 0)&&(index < BUFFER_CNT)) {
		bytesused[index] = buf.bytesused;
	}
	else {
//		printf("v_capture() ERROR:index over(index = %d)\n", index);
		return (-2);
	}
    return index;
}

/** video capture dummy
 *  @fn     int v_capture_dummy(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int v_capture_dummy(int fd)
{
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = {0};
    tv.tv_sec = 2;
    int r = select(fd+1, &fds, NULL, NULL, &tv);
    if(r <= 0)
    {
        printf("Waiting for Frame\n");
        return -1;
    }
    return 0;
}


/** get size of video capture
 *  @fn     int get_size_v_capture(int index)
 *  @param  index  : index of capture
 *  @retval >= 0 : OK(capture size)
 *  @retval == -1: error <index over>
 *  @retval == -2: error <capture size>
 *
 */
int get_size_v_capture(int index)
{
	int ans = -1;
	
	if ((index >= 0)&&(index < BUFFER_CNT)) ans = bytesused[index];
	else printf("get_size_v_capture() ERROR: index = %d)\n", index);
	
	if (ans < 0) {
		printf("get_size_v_capture() ERROR: capture_size = %d \n", ans);
		ans = -2;
	}
	
	return ans;
}

/** get buffer pointer of video capture
 *  @fn     unsigned char* get_buf_p_v_capture(int index)
 *  @param  index  : index of capture
 *  @retval >= 0 : OK(buffer pointer)
 *  @retval <  0 : error(-errno)
 *
 */
unsigned char* get_buf_p_v_capture(int index)
{
	return buffer[index];
}

/** get camera nuber of video capture
 *  @fn     int get_cam_no_v_capture(int index)
 *  @param  index  : index of capture
 *  @retval >= 0 : OK(camera number)
 *  @retval <  0 : error(-errno)
 *
 */
int get_cam_no_v_capture(int index)
{
	return cam_number[index];
}

/** video stream on
 *  @fn     int STREAM_on(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int STREAM_on(int fd)
{
	int err;
//	enum v4l2_buf_type type;
	int  type;
	
	printf("STREAM_on() : start [");
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err = xioctl(fd, VIDIOC_STREAMON, &type);
	printf("] --> err = %d ", err);
	if (!err) printf ("[--- Stream ON ---]\n");
	else printf ("[--- ERROR ---]\n");

	return err;
}

/** video stream off
 *  @fn     int STREAM_off(int fd)
 *  @param  fd   : File descriptor of video device
 *  @retval >= 0 : OK(index)
 *  @retval <  0 : error(-errno)
 *
 */
int STREAM_off(int fd)
{
	int err;
	int type;
	
	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	err  = xioctl(fd, VIDIOC_STREAMOFF, &type);
	return err;
}
