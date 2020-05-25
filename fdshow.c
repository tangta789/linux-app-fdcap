#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <errno.h>

//14byte文件头
typedef struct
{
	char cfType[2];  //文件类型，"BM"(0x4D42)
	int  cfSize;     //文件大小（字节）
	int  cfReserved; //保留，值为0
	int  cfoffBits;  //数据区相对于文件头的偏移量（字节）
}__attribute__((packed)) BITMAPFILEHEADER;
//__attribute__((packed))的作用是告诉编译器取消结构在编译过程中的优化对齐

//40byte信息头
typedef struct
{
	char ciSize[4];          //BITMAPFILEHEADER所占的字节数
	int  ciWidth;            //宽度
	int  ciHeight;           //高度
	char ciPlanes[2];        //目标设备的位平面数，值为1
	int  ciBitCount;         //每个像素的位数
	char ciCompress[4];      //压缩说明
	char ciSizeImage[4];     //用字节表示的图像大小，该数据必须是4的倍数
	char ciXPelsPerMeter[4]; //目标设备的水平像素数/米
	char ciYPelsPerMeter[4]; //目标设备的垂直像素数/米
	char ciClrUsed[4];       //位图使用调色板的颜色数
	char ciClrImportant[4];  //指定重要的颜色数，当该域的值等于颜色数时（或者等于0时），表示所有颜色都一样重要
}__attribute__((packed)) BITMAPINFOHEADER;

typedef struct
{
	unsigned char blue;
	unsigned char green;
	unsigned char red;
	unsigned char reserved;
}__attribute__((packed)) PIXEL; //颜色模式RGB

typedef struct
{
	int          fbfd;
	char         *fbp;
	unsigned int xres;
	unsigned int yres;
	unsigned int xres_virtual;
	unsigned int yres_virtual;
	unsigned int xoffset;
	unsigned int yoffset;
	unsigned int bpp;
	unsigned long line_length;
	unsigned long size;

	struct fb_bitfield red;
	struct fb_bitfield green;
	struct fb_bitfield blue;
} FB_INFO;

typedef struct
{
	unsigned int width;
	unsigned int height;
	unsigned int bpp;
	unsigned long size;
	unsigned int data_offset;
} IMG_INFO;

static FB_INFO fb_info;
static IMG_INFO img_info;
static int m_debug = 0;
static int Usage(int ret);
static int ShowBmp(char *img_name);
static int ShowPicture(char *img_name);
static int BitmapFormatConvert(char *dst,char *src, unsigned long img_len_one_line);
int main(int argc, char **argv) 
{
	int i;
	int ret;
	char *dev = NULL;
	char *img_name = NULL;

	for (i = 1;i < argc;i++) {
		if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"--help") == 0) {
			return Usage(0);
		} else if (strcmp(argv[i],"-d") == 0) {
			if (i + 1 < argc && strlen(argv[++i]) > 0 && dev == NULL) {
				dev = argv[i];
			} else {
				return Usage(-1);
			}
		} else if (strlen(argv[i]) > 0 && img_name == NULL) {
			img_name = argv[i];
		} else if (strcmp(argv[i],"-v") == 0) {
			m_debug = 1;
		} else {
			return Usage(-2);
		}
	}

	if (dev == NULL) dev = "/dev/fb0";
	if (img_name == NULL) img_name = "screen.bmp";

	fb_info.fbfd = open(dev, O_RDWR);
	if (!fb_info.fbfd) {
		printf("Error: cannot open framebuffer device(%s).\n",dev);
		return -1;
	}
	ret = ShowPicture(img_name);
	close(fb_info.fbfd);
	return ret;
}
static int Usage(int ret)
{
	FILE *file = stdout;
	if (ret < 0) file = stderr;
	fprintf(file,"Usage: fdshow [-d DEV] [FILE] [-v]\n");
	return ret;
}
static int ShowPicture(char *img_name)
{
	struct fb_var_screeninfo vinfo;
	struct fb_fix_screeninfo finfo;

	if (fb_info.fbfd <= -1) {
		fprintf(stderr,"fb open fialed, case '%s'\n",strerror(errno));
		return -1;
	}

	if (ioctl(fb_info.fbfd, FBIOGET_FSCREENINFO, &finfo)) {
		fprintf(stderr,"fb ioctl FBIOGET_FSCREENINFO fialed, case '%s'\n",strerror(errno));
		return -1;
	}

	if (ioctl(fb_info.fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
		fprintf(stderr,"fb ioctl FBIOGET_VSCREENINFO fialed, case '%s'\n",strerror(errno));
		return -1;
	}

	fb_info.xres = vinfo.xres;
	fb_info.yres = vinfo.yres;
	fb_info.xres_virtual = vinfo.xres_virtual;
	fb_info.yres_virtual = vinfo.yres_virtual;
	fb_info.xoffset = vinfo.xoffset;
	fb_info.yoffset = vinfo.yoffset;
	fb_info.bpp  = vinfo.bits_per_pixel;
	fb_info.line_length = finfo.line_length;
	fb_info.size = finfo.smem_len;

	memcpy(&fb_info.red, &vinfo.red, sizeof(struct fb_bitfield));
	memcpy(&fb_info.green, &vinfo.green, sizeof(struct fb_bitfield));
	memcpy(&fb_info.blue, &vinfo.blue, sizeof(struct fb_bitfield));

	if (m_debug) fprintf(stdout,"fb info x[%d] y[%d] x_v[%d] y_v[%d] xoffset[%d] yoffset[%d] bpp[%d] line_length[%ld] size[%ld]\n", fb_info.xres, fb_info.yres, fb_info.xres_virtual, fb_info.yres_virtual, fb_info.xoffset, fb_info.yoffset, fb_info.bpp, fb_info.line_length, fb_info.size);

	if (m_debug) fprintf(stdout,"fb info red off[%d] len[%d] msb[%d]\n", fb_info.red.offset, fb_info.red.length, fb_info.red.msb_right);
	if (m_debug) fprintf(stdout,"fb info green off[%d] len[%d] msb[%d]\n", fb_info.green.offset, fb_info.green.length, fb_info.green.msb_right);
	if (m_debug) fprintf(stdout,"fb info blue off[%d] len[%d] msb[%d]\n", fb_info.blue.offset, fb_info.blue.length, fb_info.blue.msb_right);

	if (fb_info.bpp != 16 && fb_info.bpp != 24 && fb_info.bpp != 32) {
		fprintf(stderr,"fb bpp is not 16,24 or 32\n");
		return -1;
	}
	if (fb_info.red.length > 8 || fb_info.green.length > 8 || fb_info.blue.length > 8) {
		fprintf(stderr,"fb red|green|blue length is invalid\n");
		return -1;
	}
	// 内存映射
	fb_info.fbp = (char *)mmap(0, fb_info.size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_info.fbfd, 0);
	if (fb_info.fbp == (char *)-1) {
		fprintf(stderr,"mmap fialed, case '%s'\n",strerror(errno));
		return -1;
	}
	ShowBmp(img_name);
	//删除映射
	munmap(fb_info.fbp, fb_info.size);
	return 0;
}
static int ShowBmp(char *img_name)
{

	FILE *fp;
	int ret = 0;
	BITMAPFILEHEADER FileHead;
	BITMAPINFOHEADER InfoHead;

	if(img_name == NULL) {
		fprintf(stderr,"img_name is null\n");
		return -1;
	}

	fp = fopen( img_name, "rb" );
	if(fp == NULL) {
		fprintf(stderr,"img[%s] open failed,case '%s'\n", img_name,strerror(errno));
		ret = -1;
		goto err_showbmp;
	}

	/* 移位到文件头部 */
	fseek(fp, 0, SEEK_SET);

	ret = fread(&FileHead, sizeof(BITMAPFILEHEADER), 1, fp);
	if ( ret != 1) {
		fprintf(stderr,"img[%s] read failed,case '%s'\n", img_name,strerror(errno));
		ret = -1;
		goto err_showbmp;
	}

	//检测是否是bmp图像
	if (memcmp(FileHead.cfType, "BM", 2) != 0) {
		fprintf(stderr,"it's not a BMP file[%c%c]\n", FileHead.cfType[0], FileHead.cfType[1]);
		ret = -1;
		goto err_showbmp;
	}

	ret = fread( (char *)&InfoHead, sizeof(BITMAPINFOHEADER),1, fp );
	if ( ret != 1) {
		fprintf(stderr,"read infoheader error!\n");
		ret = -1;
		goto err_showbmp;
	}

	img_info.width       = InfoHead.ciWidth;
	img_info.height      = InfoHead.ciHeight;
	img_info.bpp         = InfoHead.ciBitCount;
	img_info.size        = FileHead.cfSize;
	img_info.data_offset = FileHead.cfoffBits;

	if (m_debug) fprintf(stdout,"img info w[%d] h[%d] bpp[%d] size[%ld] offset[%d]\n", img_info.width, img_info.height, img_info.bpp, img_info.size, img_info.data_offset);

	if (img_info.bpp != 24 && img_info.bpp != 32) {
		fprintf(stderr,"img bpp is not 24 or 32\n");
		ret = -1;
		goto err_showbmp;
	}


	/*
	 *一行行处理
	 */
	char *buf_img_one_line;
	char *buf_fb_one_line;
	char *p;
	int fb_height;

	long img_len_one_line = img_info.width * (img_info.bpp / 8);
	long fb_len_one_line = fb_info.line_length;

	if (m_debug) fprintf(stdout,"img_len_one_line = %ld\n", img_len_one_line);
	if (m_debug) fprintf(stdout,"fb_len_one_line = %ld\n", fb_info.line_length);

	buf_img_one_line = (char *)calloc(1, img_len_one_line + 256);
	if(buf_img_one_line == NULL) {
		fprintf(stderr,"alloc failed\n");
		ret = -1;
		goto err_showbmp;
	}

	buf_fb_one_line = (char *)calloc(1, fb_len_one_line + 256);
	if(buf_fb_one_line == NULL) {
		fprintf(stderr,"alloc failed\n");
		ret = -1;
		goto err_showbmp;
	}


	fseek(fp, img_info.data_offset, SEEK_SET);

	p = fb_info.fbp + fb_info.yoffset * fb_info.line_length; /*进行y轴的偏移*/
	fb_height = fb_info.yres;
	while (1) {
		memset(buf_img_one_line, 0, img_len_one_line);
		memset(buf_fb_one_line, 0, fb_len_one_line);
		ret = fread(buf_img_one_line, 1, img_len_one_line, fp);
		if (ret < img_len_one_line) {
			/*图片读取完成，则图片显示完成*/
			if (m_debug) fprintf(stdout,"read to end of img file\n");
			BitmapFormatConvert(buf_fb_one_line, buf_img_one_line, img_len_one_line); /*数据转换*/
			memcpy(fb_info.fbp, buf_fb_one_line, fb_len_one_line);
			break;
		}

		BitmapFormatConvert(buf_fb_one_line, buf_img_one_line, img_len_one_line); /*数据转换*/
		memcpy(p, buf_fb_one_line, fb_len_one_line); /*显示一行*/
		p += fb_len_one_line;

		/*超过显示屏宽度认为图片显示完成*/
		fb_height--;
		if (fb_height <= 0)
			break;
	}

	free(buf_img_one_line);
	free(buf_fb_one_line);

	fclose(fp);
	return ret;
err_showbmp:
	if (fp)
		fclose(fp);
	return ret;
}
static int BitmapFormatConvert(char *dst,char *src, unsigned long img_len_one_line)
{
	int img_len ,fb_len ;
	char *p;
	__u32 val;
	PIXEL pix;

	p = (char *)&val;

	img_len = img_info.width; /*一行图片的长度*/
	fb_len = fb_info.xres; /*一行显示屏的长度*/

	/*进行x轴的偏移*/
	dst += fb_info.xoffset * (fb_info.bpp / 8);
	fb_len -= fb_info.xoffset;

	/*bmp 数据是上下左右颠倒的，这里只进行左右的处理*/
	/*先定位到图片的最后一个像素的地址，然后往第一个像素的方向处理，进行左右颠倒的处理*/
	src += img_len_one_line - 1;

	/*处理一行要显示的数据*/
	while(1) {
		if (img_info.bpp == 32)
			pix.reserved = *(src--);
		pix.red   = *(src--);
		pix.green = *(src--);
		pix.blue  = *(src--);

		val = 0x00;
		val |= (pix.red >> (8 - fb_info.red.length)) << fb_info.red.offset;
		val |= (pix.green >> (8 - fb_info.green.length)) << fb_info.green.offset;
		val |= (pix.blue >> (8 - fb_info.blue.length)) << fb_info.blue.offset;


		if (fb_info.bpp == 16) {
			*(dst++) = *(p + 0);
			*(dst++) = *(p + 1);
		}
		else if (fb_info.bpp == 24) {
			*(dst++) = *(p + 0);
			*(dst++) = *(p + 1);
			*(dst++) = *(p + 2);
		}
		else if (fb_info.bpp == 32) {
			*(dst++) = *(p + 0);
			*(dst++) = *(p + 1);
			*(dst++) = *(p + 2);
			*(dst++) = *(p + 3);
		}

		/*超过图片长度或显示屏长度认为一行处理完了*/
		img_len--;
		fb_len--;
		if (img_len <= 0 || fb_len <= 0)
			break;
	}
#if 0
	printf("r = %d\n", pix.red);
	printf("g = %d\n", pix.green);
	printf("b = %d\n", pix.blue);
#endif
	return 0;
}
