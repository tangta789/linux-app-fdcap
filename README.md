# fdcap - Linux 截屏工具  
[![Travis CI Build Status](https://www.travis-ci.org/tangta789/linux-app-fdcap.svg)](https://www.travis-ci.org/tangta789/linux-app-fdcap)
>该工具仅做键Linux 截屏功能，直接将显示设备 '/dev/fb0' 中的数据保存成bmp格式。  
## Usage  
>  fdcap [-d DEV] [FILE_NAME]  
>    -d DEV     device path, default '/dev/fb0'.  
>    FILE_NAME  save image file name, image formt is bmp.  
>    -h         for this.  
## 其它截屏方法  
>```
>  cat /dev/fb0 > /tmp/frame.raw  
>  ffmpeg -vcodec rawvideo -f rawvideo -pix_fmt rgb32  -s 1280X720 -i frame.raw -f image2 -vcodec png frame-%d.png  
>```
