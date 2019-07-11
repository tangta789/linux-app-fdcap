#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

// 保存BMP图像所需的Header结构
#pragma pack(push,2)
typedef struct BitmapFileHeader_s
{
	uint16_t    bfType;
	uint32_t    bfSize;
	uint16_t    bfReserved1;
	uint16_t   bfReserved2;
	uint32_t   bfOffBits;
} BitmapFileHeader;
#pragma pack(pop)

typedef struct BitmapInfoHeader_s{
	uint32_t     biSize;
	uint32_t      biWidth;
	uint32_t      biHeight;
	uint16_t      biPlanes;
	uint16_t      biBitCount;
	uint32_t     biCompression;
	uint32_t     biSizeImage;
	uint32_t      biXPelsPerMeter;
	uint32_t      biYPelsPerMeter;
	uint32_t     biClrUsed;
	uint32_t     biClrImportant;
} BitmapInfoHeader;
static int ScreensHot(const char *dev, const char *output) ;
static int Usage(int ret);

int main(int argc, char **argv) 
{
	int i;
	char *dev = NULL;
	char *output = NULL;

	for (i = 1;i < argc;i++) {
		if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"--help") == 0) {
			return Usage(0);
		} else if (strcmp(argv[i],"-d") == 0) {
			if (i + 1 < argc && strlen(argv[++i]) > 0 && dev == NULL) {
				dev = argv[i];
			} else {
				return Usage(-1);
			}
		} else if (strlen(argv[i]) > 0 && output == NULL) {
			output = argv[i];
		} else {
			return Usage(-2);
		}
	}

	if (dev == NULL) dev = "/dev/fb0";
	if (output == NULL) output = "screen.bmp";

	return ScreensHot(dev,output);
}

static int ScreensHot(const char *dev, const char *output) 
{
	int i = 0;
	int fbfd = 0;
	int fd = 0;
	char* fbp = NULL;
	char* fp = NULL;
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;
	long int linesize = 0;
	long int screensize = 0;
	long int fsize = 0;
	BitmapFileHeader* fileHeader = NULL;
	BitmapInfoHeader* infoHeader = NULL;
	/*打开设备文件*/
	fbfd = open(dev, O_RDONLY);
	if (fbfd < 0) {
		fprintf(stderr,"open dev '%s' failed, case '%s'\n",dev,strerror(errno));
		return -11;
	}
	/*取得屏幕相关参数*/
	if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) < 0) {
		fprintf(stderr,"ioctl dev '%s' FBIOGET_FSCREENINFO failed, case '%s'\n",dev,strerror(errno));
		close(fbfd);
		return -12;
	}
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) < 0) {
		fprintf(stderr,"ioctl dev '%s' FBIOGET_VSCREENINFO failed, case '%s'\n",dev,strerror(errno));
		close(fbfd);
		return -13;
	}
	/*计算屏幕缓冲区大小*/
	linesize = vinfo.xres * vinfo.bits_per_pixel / 8;
	screensize = linesize * vinfo.yres;
	/*映射屏幕缓冲区到用户地址空间*/
	fbp=(char*)mmap(0,screensize,PROT_READ,MAP_SHARED, fbfd, 0);
	if (fbp == NULL) {
		close(fbfd);
		fprintf(stderr,"mmap dev '%s',failed, case '%s'\n",dev,strerror(errno));
		return -14;
	}

	fd = open(output, O_CREAT|O_RDWR,0664);
	if (fd < 0) {
		close(fbfd);
		munmap(fbp, screensize);
		fprintf(stderr,"open save file '%s' failed, case '%s'\n",output,strerror(errno));
		return -15;
	}
	/*计算BMP文件大小*/
	fsize = screensize + sizeof(BitmapFileHeader) + sizeof(BitmapInfoHeader);
	/*调整BMP文件大小，避免mmap之后写内存时出现SIGBUS*/
	if (ftruncate(fd, fsize) < 0) {
		close(fbfd);
		munmap(fbp, screensize);
		close(fd);
		fprintf(stderr,"ftruncate save file '%s',failed, case '%s'\n",output,strerror(errno));
		return -16;
	}
	fp = (char*)mmap(0,fsize,PROT_READ|PROT_WRITE,MAP_SHARED, fd, 0);
	if (fp == NULL) {
		close(fbfd);
		munmap(fbp, screensize);
		close(fd);
		fprintf(stderr,"mmap save file '%s',failed, case '%s'\n",output,strerror(errno));
		return -17;
	}

	/*设置BMP图像信息*/
	fileHeader = (BitmapFileHeader*)fp;
	infoHeader = (BitmapInfoHeader*)(fp + sizeof(BitmapFileHeader));
	fileHeader->bfOffBits = fsize-screensize;
	fileHeader->bfSize = fsize;
	fileHeader->bfType = 0x4d42;
	infoHeader->biHeight = vinfo.yres;
	infoHeader->biWidth = vinfo.xres;
	infoHeader->biPlanes = 1;
	infoHeader->biBitCount = vinfo.bits_per_pixel;
	infoHeader->biSizeImage = screensize;
	infoHeader->biSize = sizeof(BitmapInfoHeader);

	/*逐行拷贝图像，反转行顺序*/
	for (i = 0; i < vinfo.yres; ++i) {
		memcpy(fp + (fsize-screensize) + i * linesize, fbp + (vinfo.yres - i - 1) * linesize, linesize);
	}
	munmap(fp, fsize);
	close(fd);

	/*释放缓冲区，关闭设备*/
	munmap(fbp, screensize);
	close(fbfd);
	return 0;
}
static int Usage(int ret)
{
	FILE *file = stdout;
	if (ret < 0) file = stderr;
	fprintf(file,"Usage: fdcap [-d DEV] [FILE]\n");
	return ret;
}
