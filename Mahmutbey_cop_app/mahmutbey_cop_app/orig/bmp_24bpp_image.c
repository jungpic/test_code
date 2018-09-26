/*
 *
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "bmp_image.h"

#define NOT_REVERSE_RGB565  // for SDK's TOOLS

#define FBIO_GET_BACKBUFFER    0xADCFB000
#define FBIO_FLIP              0xADCFB001

static unsigned char __attribute__((__aligned__(4))) ImageBuffer[320*240*3];
static unsigned char __attribute__((__aligned__(4))) PaletteBuffer[256*4];

int ComputeImagePitch(char bpp, unsigned short width, int *pitch, char *bytesperpixel);

//int load_to_fb_from_bmp_24bpp(int fb_dev, unsigned char *img_name)
int load_to_fb_from_bmp_24bpp(unsigned int fb_buffer, char *img_name)
{
    FILE *fpi;
    BmpFileHead bmpf;
    BmpInfoHead bmpi;
    unsigned char *bibuf, tmp;
    unsigned short *wibuf;
    short h, w;
    short align_h, align_w;
    unsigned int align_pitch;
    int i, k;
    int remain_pitch;
    unsigned int bgr24;
    PalEntry *palette;
    unsigned int *pixels = (unsigned int *)ImageBuffer;
    int bpp, palsize;
    int pitch;
    char bytesperpixel;
    unsigned int fb_back_buffer = fb_buffer;

    if ((fpi = fopen(img_name, "rb")) == NULL) {
        printf("%s(), Cannot open %s\n", __FUNCTION__, img_name);
        return -1;
    }
printf("\n%s(), open %s\n", __FUNCTION__, img_name);

    fread(&bmpf.bfType[0], sizeof(unsigned char), 2, fpi);
    fread(&bmpf.bfSize, sizeof(unsigned char), FILEHEADSIZE-2, fpi);

#if 1
    printf("bmpf.bfSize: %d\n", bmpf.bfSize);
    printf("bmpf.bfOffBits: %d\n", bmpf.bfOffBits);
#endif

    if (*(unsigned short*)&bmpf.bfType[0] != (unsigned short)(0x4D42)) { /* 'BM' */
        printf("Not bmp image\n");
        fclose(fpi);
		return -1;	/* not bmp image*/
    }

    fread(&bmpi, sizeof(unsigned char), INFOHEADSIZE, fpi);

#if 0
    if (bmpi.BiSize != INFOHEADSIZE) {
        printf("BMP File is not windows type\n");
        return -1;
    }
    printf("bmpi.BiSize: %d\n", bmpi.BiSize);
#endif

    if(bmpi.BiBitCount > 8 && bmpi.BiBitCount != 24) {
        printf("Image bpp is not 24bit\n");
        fclose(fpi);
        return -1;	/* image loading error*/
    }

    palette = (PalEntry *)&PaletteBuffer[0];

    w = bmpi.BiWidth;
    bpp = bmpi.BiBitCount;
    palsize = bmpi.BiClrUsed;
    if (palsize > 256)
        palsize = 0;
    else if(palsize == 0 && bpp <= 8)
        palsize = 1 << bpp;

	/* get colormap*/
    for(i=0; i < palsize; i++) {
        palette[i].b = fgetc(fpi);
        palette[i].g = fgetc(fpi);
        palette[i].r = fgetc(fpi);
        tmp = fgetc(fpi);
    }

    remain_pitch = ComputeImagePitch(bpp, w, &pitch, &bytesperpixel);

    for (i = 8, k = 0; i <= 1024; k++) {
        if (i >= w)
            break;
        i <<= 2;
    }
    align_w = i;
    align_pitch = align_w * bytesperpixel;
//printf("%s(), align_pitch: %d\n", __FUNCTION__, align_pitch);

    h = bmpi.BiHeight;
    for (i = 8, k = 0; i <= 1024; k++) {
        if (i >= h)
            break;
        i <<= 1;
    }
    align_h = i;
    //psurface->TileYSize = k;

#if 1
    printf("w: %d\n", w);
    printf("align width: %d\n", align_w);
    printf("h: %d\n", h);
    printf("align_h: %d\n", align_h);
    printf("bpp: %d\n", bpp);
    printf("palsize: %d\n", palsize);
    printf("pitch: %d\n", pitch);
    printf("bytesperpixel: %d\n", bytesperpixel);
    printf("remain_pitch: %d\n", remain_pitch);
#endif

    fseek(fpi, bmpf.bfOffBits, SEEK_SET);

#if 0
    if (ioctl(fb_dev, FBIO_GET_BACKBUFFER, &fb_back_buffer)) {
        printf("Can't get Frame BACK Buffer\n");
        fclose(fpi);
        return -1;
    }
    //printf(" BACK BUFFER: 0x%X\n", fb_back_buffer);
#endif

    if (bmpi.BiBitCount == 24) {
        unsigned char *buf;
        fread(pixels, pitch, h, fpi);

        k = 0;
        while (--h >= 0) {
            /* turn image rightside up*/
            //wibuf = (unsigned short *)((int)fb_back_buffer + h*align_pitch);
            wibuf = (unsigned short *)((int)fb_back_buffer + h*320*2);
//printf("%s(), h: %d, wibuf: 0x%X\n", __FUNCTION__, h, (int)wibuf);

            buf = (unsigned char *)pixels;
            for (i = 0; i < w; i++) {
                //bgr24 = (unsigned int)buf[k] + ((unsigned int)buf[k+1] << 8)
                //    + ((unsigned int)buf[k+2] << 16);
                //bgr24 = ((unsigned int)buf[k] >> 3) + (((unsigned int)buf[k+1] >> 2) <<5)
                //    + (((unsigned int)buf[k+2] >> 3)<<11);
                bgr24 = (((unsigned int)buf[k] >> 3)<<11) + (((unsigned int)buf[k+1] >> 2) <<5)
                    + (((unsigned int)buf[k+2] >> 3));
                wibuf[i] = (unsigned short)bgr24;
                k += 3;
            }
        }
    }

    //ioctl(fb_dev, FBIO_FLIP, &fb_back_buffer);

    fclose(fpi);

    return 0;
}

#define PIX2BYTES(n)	(((n)+7) >> 3)
/*
 * compute image line size and bytes per pixel
 * from bits per pixel and width
 */
int ComputeImagePitch(char bpp, unsigned short width, int *pitch, char *bytesperpixel)
{
	int	linesize;
	int	align_linesize;
	int	bytespp = 2;

	if(bpp == 1) {
	    bytespp = 1;
		linesize = PIX2BYTES(width);
    }
	else if(bpp <= 4) {
	    bytespp = 1;
		linesize = PIX2BYTES(width<<2);
    }
	else if(bpp <= 8) {
	    bytespp = 1;
		linesize = width;
    }
	else if(bpp <= 16) {
		linesize = width << 1; //width * 2;
	} else if(bpp <= 24) {
		linesize = width * 3;
	} else {
		linesize = width << 2; //width * 4;
		bytespp = 4;
	}

	/* rows are DWORD right aligned*/
	align_linesize = (linesize + 3) & ~3;

//printf("%s(), align_linesize: %d\n", __FUNCTION__, align_linesize);
//printf("%s(), linesize: %d\n", __FUNCTION__, linesize);

	*pitch = align_linesize;
	*bytesperpixel = bytespp;

    return align_linesize - linesize;
}

