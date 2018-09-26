/*
 * fbutils.c
 *
 * Utility routines for framebuffer interaction
 *
 * Copyright 2002 Russell King and Doug Lowder
 *
 * This file is placed under the GPL.  Please see the
 * file COPYING for details.
 *
 */

//#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <linux/vt.h>
#include <linux/kd.h>
#include <linux/fb.h>

#include "font.h"
//#include "fbutils.h"

//#include "write_font.h"
#include "gpio_key_dev.h"
#include "cob_process.h"
#include "lcd.h"
#include "crc16.h"

#define DEBUG_LINE                      1

#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
	        do {                    \
				                fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
				        } while(0)
#else
#define DBG_LOG(fmt, args...)
#endif


extern int selected_language;
extern int menu_force_redraw;

#define XORMODE	0x80000000

union multiptr {
	unsigned char *p8;
	unsigned short *p16;
	unsigned long *p32;
};

static int fb_fd;

static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;

static unsigned char *fbuffer;

static unsigned char **line_addr;
static int bytes_per_pixel;
__u32 xres, yres;

static unsigned colormap [256];
#define COLOR_black	0x00000000
#define COLOR_black_idx	0

#define COLOR_gray	0x00181818
#define COLOR_gray_idx	1

#define COLOR_white	0x00ffffff
#define COLOR_white_idx	2

#define COLOR_green	0x0000ff00
#define COLOR_green_idx	3

#define COLOR_blue	0x00ff0000
#define COLOR_blue_idx	4

#define COLOR_blue2	0x00ff6464
#define COLOR_blue2_idx	5

#define COLOR_red	0x000000FF
#define COLOR_red_idx	6

#define COLOR_orange	0x0064B4FF
#define COLOR_orange_idx	7

#define COLOR_line_1	0x00C07000
#define COLOR_line_1_idx 8

/*#define COLOR_BG_1	0x0079CCAD*/
#define COLOR_BG_1	0x005BC19B
#define COLOR_BG_1_idx	9

#define COLOR_BG_2	0x00303030
#define COLOR_BG_2_idx	10

#define COLOR_BG_3	0x00C6EADC
#define COLOR_BG_3_idx  11	

#define COLOR_FG_1	0x00000000
#define COLOR_FG_1_idx	12

#define COLOR_FG_2	0x00101010
#define COLOR_FG_2_idx	13

//#define COLOR_HEAD_BG_1	0x00C8C8C8
#define COLOR_HEAD_BG_1	0x00008000
#define COLOR_HEAD_BG_1_idx 14

#define COLOR_HEAD_BG_2	0x00969696
#define COLOR_HEAD_BG_2_idx 15

#define COLOR_HEAD_BG_3	0x00008080
#define COLOR_HEAD_BG_3_idx 16

#define COLOR_HEAD_BG_4	0x00FFFF80
#define COLOR_HEAD_BG_4_idx 17

#define COLOR_HEAD_BG_5	0x005A805A
#define COLOR_HEAD_BG_5_idx 18

#define COLOR_HEAD_BG_6	0x00646464
#define COLOR_HEAD_BG_6_idx 19

#define COLOR_HEAD_BG_7	0x00C8C8C8
#define COLOR_HEAD_BG_7_idx 20

#define COLOR_HEAD_BG_8	0x00FF8000
#define COLOR_HEAD_BG_8_idx 21

static int palette [] = {
    COLOR_black, COLOR_gray, COLOR_white,  COLOR_green,  COLOR_blue,
    COLOR_blue2, COLOR_red,  COLOR_orange, COLOR_line_1, COLOR_BG_1,
    COLOR_BG_2,  COLOR_BG_3, COLOR_FG_1,   COLOR_FG_2,   COLOR_HEAD_BG_1,
    COLOR_HEAD_BG_2, COLOR_HEAD_BG_3, COLOR_HEAD_BG_4, COLOR_HEAD_BG_5, COLOR_HEAD_BG_6,
    COLOR_HEAD_BG_7, COLOR_HEAD_BG_8,
};

#define FB_DEVICE_NAME	"/dev/fb0"

extern int load_to_fb_from_bmp_24bpp(unsigned int fb_buffer, unsigned char *img_name);

static int open_framebuffer(void);

#define NR_COLORS (sizeof (palette) / sizeof (palette [0]))

extern int mic_volume_enable;
extern int spk_volume_enable;

static void LCD_InitPalette()
{
    int i =0;
    for (i = 0; i < NR_COLORS; i++)
        setcolor (i, palette [i]);
}

int lcd_init(void)
{
    int ret;
    //char buf[16];

    selected_language = 0;

#if 0
    ret = open_framebuffer();

    LCD_InitPalette();
#endif
#if 0
//load_to_fb_from_bmp_24bpp((unsigned int)fbuffer, "/mnt/data/logo_cop_320_240.bmp");
lcd_fillrect (0,0, 320, 4, 2);
lcd_fillrect (0,(240/2-2), 320, 240/2+2, 2);
lcd_fillrect (0,(240-4), 320, 240, 2);

lcd_fillrect (0,0, 4, 240, 2);
lcd_fillrect (320-4,0, 320, 240, 2);
#endif

    return ret;
}

void set_language_type(int set)
{
    selected_language = set;
}

/*void clear_PA_area(void)
{
    lcd_fillrect (30,30, 320-60, 240/2-30, 0);
}

void clear_CALL_area(void)
{
    lcd_fillrect (30,240/2+30, 320-60, 240-30, 0);
}
*/

static int open_framebuffer(void)
{
    unsigned y, addr;

    fb_fd = open(FB_DEVICE_NAME, O_RDWR);

    if (fb_fd == -1) {
        perror("open fbdevice");
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fix) < 0) {
        perror("ioctl FBIOGET_FSCREENINFO");
        close(fb_fd);
        return -1;
    }

    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &var) < 0) {
        perror("ioctl FBIOGET_VSCREENINFO");
        close(fb_fd);
        return -1;
    }
    xres = var.xres;
    yres = var.yres;

    fbuffer = mmap(NULL, fix.smem_len, PROT_READ | PROT_WRITE, MAP_FILE | MAP_SHARED, fb_fd, 0);
    if (fbuffer == (unsigned char *)-1) {
        perror("mmap framebuffer");
        close(fb_fd);
        return -1;
    }

    memset(fbuffer,0,fix.smem_len);

    bytes_per_pixel = (var.bits_per_pixel + 7) / 8; // --> 2
    line_addr = malloc (sizeof (__u32) * var.yres_virtual); // --> 240
    addr = 0;

    /* fix.line_length: 320 x 2 = 640 */
    for (y = 0; y < var.yres_virtual; y++, addr += fix.line_length)
        line_addr [y] = fbuffer + addr;


    return 0;
}

void close_framebuffer(void)
{
    munmap(fbuffer, fix.smem_len);
    close(fb_fd);
    free (line_addr);
    fb_fd = -1;
}

void lcd_put_cross(int x, int y, unsigned colidx)
{
	return;

    if (fb_fd < 0)
        return;
//printf("==lcd_put_cross(), x:%d, y:%d, colidx: %d\n", x, y, colidx);
#if 0
	line (x - 10, y, x - 2, y, colidx);
	line (x + 2, y, x + 10, y, colidx);
	line (x, y - 10, x, y - 2, colidx);
	line (x, y + 2, x, y + 10, colidx);

#if 1
	line (x - 6, y - 9, x - 9, y - 9, colidx + 1);
	line (x - 9, y - 8, x - 9, y - 6, colidx + 1);
	line (x - 9, y + 6, x - 9, y + 9, colidx + 1);
	line (x - 8, y + 9, x - 6, y + 9, colidx + 1);
	line (x + 6, y + 9, x + 9, y + 9, colidx + 1);
	line (x + 9, y + 8, x + 9, y + 6, colidx + 1);
	line (x + 9, y - 6, x + 9, y - 9, colidx + 1);
	line (x + 8, y - 9, x + 6, y - 9, colidx + 1);
#else
	line (x - 7, y - 7, x - 4, y - 4, colidx + 1);
	line (x - 7, y + 7, x - 4, y + 4, colidx + 1);
	line (x + 4, y - 4, x + 7, y - 7, colidx + 1);
	line (x + 4, y + 4, x + 7, y + 7, colidx + 1);
#endif
#endif
}

static void lcd_put_char(int x, int y, int c, int colidx)
{
	return; 

	int i,j,bits;

    if (fb_fd < 0)
        return;
	for (i = 0; i < font_vga_8x8.height; i++) {
		bits = font_vga_8x8.data [font_vga_8x8.height * c + i];
		for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
			if (bits & 0x80)
				pixel (x + j, y + i, colidx);
	}
}

void lcd_put_string(int x, int y, char *s, unsigned int colidx)
{
	return;

//printf("==lcd_put_string, x:%d, y:%d, str: %s, colidx: %x\n", x,y,s,colidx);
#if 1
	int i;

    if (fb_fd < 0)
        return;

	for (i = 0; *s; i++, x += font_vga_8x8.width, s++)
		lcd_put_char (x, y, *s, colidx);
#endif
}

#if 0
void lcd_put_string_center(int x, int y, char *s, unsigned colidx)
{
//printf("==lcd_put_string_center, x:%d, y:%d, str: %s, colidx: %x\n", x,y,s,colidx);
#if 1
	size_t sl = strlen (s);

    if (fb_fd < 0)
        return;

        lcd_put_string (x - (sl / 2) * font_vga_8x8.width,
                    y - font_vga_8x8.height / 2, s, colidx);
#endif
}
#endif

void setcolor(unsigned int colidx, unsigned int value)
{
	unsigned res;
	unsigned short red, green, blue;
	struct fb_cmap cmap;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color index = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

    if (fb_fd < 0)
        return;

	switch (bytes_per_pixel) {
	default:
	case 1:
		res = colidx;
		//red = (value >> 8) & 0xff00;
		red = (value << 8) & 0xff00;
		green = value & 0xff00;
		blue = (value >> 8) & 0xff00;
		//blue = (value << 8) & 0xff00;
		cmap.start = colidx;
		cmap.len = 1;
		cmap.red = &red;
		cmap.green = &green;
		cmap.blue = &blue;
		cmap.transp = NULL;

        	if (ioctl (fb_fd, FBIOPUTCMAP, &cmap) < 0)
        	        perror("ioctl FBIOPUTCMAP");
		break;
	case 2:
	case 3:
	case 4:
		//blue = (value >> 16) & 0xff;
		//green = (value >> 8) & 0xff;
		//red = value & 0xff;
		red = (value >> 16) & 0xff;
		green = (value >> 8) & 0xff;
		blue = value & 0xff;

//printf("\nvar.blue.length:%d\n", var.blue.length);
//printf("var.blue.offset:%d\n", var.blue.offset);

//printf("var.green.length:%d\n", var.green.length);
//printf("var.green.offset:%d\n", var.green.offset);

//printf("var.red.length:%d\n", var.red.length);
//printf("var.red.offset:%d\n", var.red.offset);

		res = ((blue >> (8 - var.blue.length)) << var.blue.offset) |
                      ((green >> (8 - var.green.length)) << var.green.offset) |
                      ((red >> (8 - var.red.length)) << var.red.offset);
	}
        colormap [colidx] = res;
}

static inline void __setpixel (union multiptr loc, unsigned xormode, unsigned color)
{
	return;
#if 0
	switch(bytes_per_pixel) {
	case 1:
	default:
		if (xormode)
			*loc.p8 ^= color;
		else
			*loc.p8 = color;
		break;
	case 2:
		//if (xormode)
		//	*loc.p16 ^= color;
		//else
			*loc.p16 = color;
#endif
#if 0
		break;
	case 3:
		if (xormode) {
			*loc.p8++ ^= (color >> 16) & 0xff;
			*loc.p8++ ^= (color >> 8) & 0xff;
			*loc.p8 ^= color & 0xff;
		} else
#endif
                {
			*loc.p8++ = (color >> 16) & 0xff;
			*loc.p8++ = (color >> 8) & 0xff;
			*loc.p8 = color & 0xff;
		}
#if 0
		break;
	case 4:
		if (xormode)
			*loc.p32 ^= color;
		else
			*loc.p32 = color;
		break;
	}
#endif
}

void pixel (int x, int y, unsigned int colidx)
{
	return;

	unsigned xormode;
	union multiptr loc;

    if (fb_fd < 0)
        return;

	if ((x < 0) || ((__u32)x >= var.xres_virtual) ||
	    (y < 0) || ((__u32)y >= var.yres_virtual))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	//loc.p8 = line_addr [y] + x * bytes_per_pixel;
	//loc.p8 = line_addr [y] + (x << 1); // 2bytes
	loc.p8 = line_addr [y] + (x << 1) + x; // 3bytes
	__setpixel (loc, xormode, colormap [colidx]);
}

void line (int x1, int y1, int x2, int y2, unsigned colidx)
{
	return;

//printf("==line, x1:%d, y1:%d, x2: %d, y2: %d, colidx: %x\n", x1,y1,x2,y2,colidx);
#if 1
	int tmp;
	int dx = x2 - x1;
	int dy = y2 - y1;

    if (fb_fd < 0)
        return;

	if (abs (dx) < abs (dy)) {
		if (y1 > y2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		x1 <<= 16;
		/* dy is apriori >0 */
		dx = (dx << 16) / dy;
		while (y1 <= y2) {
			pixel (x1 >> 16, y1, colidx);
			x1 += dx;
			y1++;
		}
	} else {
		if (x1 > x2) {
			tmp = x1; x1 = x2; x2 = tmp;
			tmp = y1; y1 = y2; y2 = tmp;
			dx = -dx; dy = -dy;
		}
		y1 <<= 16;
		dy = dx ? (dy << 16) / dx : 0;
		while (x1 <= x2) {
			pixel (x1, y1 >> 16, colidx);
			y1 += dy;
			x1++;
		}
	}
#endif
}

void rect (int x1, int y1, int x2, int y2, unsigned colidx)
{
	return;

//printf("==rect, x1:%d, y1:%d, x2: %d, y2: %d, colidx: %x\n", x1,y1,x2,y2,colidx);
#if 1
    if (fb_fd < 0)
        return;

	line (x1, y1, x2, y1, colidx);
	line (x2, y1, x2, y2, colidx);
	line (x2, y2, x1, y2, colidx);
	line (x1, y2, x1, y1, colidx);
#endif
}

#if 0
void fillrect (int x1, int y1, int x2, int y2, unsigned colidx)
{
//printf("==fillrect, x1:%d, y1:%d, x2: %d, y2: %d, colidx: %x\n", x1,y1,x2,y2,colidx);
#if 1
	int tmp;
	unsigned xormode;
	union multiptr loc;

    if (fb_fd < 0)
        return;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 < 0) x1 = 0; if ((__u32)x1 >= xres) x1 = xres - 1;
	if (x2 < 0) x2 = 0; if ((__u32)x2 >= xres) x2 = xres - 1;
	if (y1 < 0) y1 = 0; if ((__u32)y1 >= yres) y1 = yres - 1;
	if (y2 < 0) y2 = 0; if ((__u32)y2 >= yres) y2 = yres - 1;

	if ((x1 > x2) || (y1 > y2))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	colidx = colormap [colidx];

	for (; y1 <= y2; y1++) {
		loc.p8 = line_addr [y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			__setpixel (loc, xormode, colidx);
			loc.p8 += bytes_per_pixel;
		}
	}
#endif
}
#endif

void lcd_fillrect (int x1, int y1, int x2, int y2, unsigned int colidx)
{
	return;

	int tmp;
	unsigned xormode;
	union multiptr loc;

//static char unicode_pa_occ_area[] = {247, 5, 257, 5+36};
    if (fb_fd < 0)
        return;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 < 0) x1 = 0; if ((__u32)x1 >= xres) x1 = xres - 1;
	if (x2 < 0) x2 = 0; if ((__u32)x2 >= xres) x2 = xres - 1;
	if (y1 < 0) y1 = 0; if ((__u32)y1 >= yres) y1 = yres - 1;
	if (y2 < 0) y2 = 0; if ((__u32)y2 >= yres) y2 = yres - 1;

	if ((x1 > x2) || (y1 > y2))
		return;

	xormode = colidx & XORMODE;
	colidx &= ~XORMODE;

#ifdef DEBUG
	if (colidx > 255) {
		fprintf (stderr, "WARNING: color value = %u, must be <256\n",
			 colidx);
		return;
	}
#endif

	colidx = colormap [colidx];

	for (; y1 <= y2; y1++) {
		loc.p8 = line_addr [y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			__setpixel (loc, xormode, colidx);
			loc.p8 += bytes_per_pixel;
		}
	}
}

#define __HANGLE__

#ifdef __HANGLE__
#define HANGLE_FONT_START           0xB0A1
#define HANGLE_FONT_END             0xC8FE
#define HANGLE_FONT_GRPNUM          94

#include "Humetro_256.h"
#include "HUMETRO_logo.h"
#include "Fire_119.h"
#include "Call_119.h"

#define RGB24BIT888(r,g,b) ( (r<<16) | (g<<8) | b )

void lcd_draw_image (int x1, int y1, int x2, int y2, int select)
{
	return;

	int tmp, cnt=0;
	union multiptr loc; 
	unsigned short bmp, red, blue, green;
	unsigned color;

	if (fb_fd < 0) 
	return;

	/* Clipping and sanity checking */
	if (x1 > x2) { tmp = x1; x1 = x2; x2 = tmp; }
	if (y1 > y2) { tmp = y1; y1 = y2; y2 = tmp; }
	if (x1 < 0) x1 = 0; if ((__u32)x1 >= xres) x1 = xres - 1;
	if (x2 < 0) x2 = 0; if ((__u32)x2 >= xres) x2 = xres - 1;
	if (y1 < 0) y1 = 0; if ((__u32)y1 >= yres) y1 = yres - 1;
	if (y2 < 0) y2 = 0; if ((__u32)y2 >= yres) y2 = yres - 1;
		
	if ((x1 > x2) || (y1 > y2))
		return;

	//printf("\nvar.blue.length:%d\n", var.blue.length);
	//printf("var.blue.offset:%d\n", var.blue.offset);
 
	//printf("var.green.length:%d\n", var.green.length);
	//printf("var.green.offset:%d\n", var.green.offset);
 
	//printf("var.red.length:%d\n", var.red.length);
	//printf("var.red.offset:%d\n", var.red.offset);

	//printf("bytes_per_pixel = %d\n", bytes_per_pixel);

	for (; y1 <= y2; y1++) {
		loc.p8  = line_addr [y1] + x1 * bytes_per_pixel;
		for (tmp = x1; tmp <= x2; tmp++) {
			switch(select) {
				case 0:
					bmp = IMG_Humetro_24bit2[cnt++];
					break;
				case 1:
					bmp = IMG_HUMETRO_logo2[cnt++];
					break;
				case 2:
					bmp = IMG_Fire_119_2[cnt++];
					break;
				case 3:
					bmp = IMG_Call_119_2[cnt++];
					break;
				default:
					return;
			}    
			red   = ((bmp & 0xF800) >> 11)<<3;
			green = ((bmp & 0x07E0) >> 5)<<2;
			blue  = ((bmp & 0x001F) >> 0)<<3;
		
			color = RGB24BIT888(red, green, blue);
	
			__setpixel (loc, 0, color);
			loc.p8 += bytes_per_pixel;
		}    
	} 
	//printf("red(0x%04X), green(0x%04X), blue(0x%04X)\n", red, green, blue);
}
#endif

#ifdef __HANGLE__
#define MAXIMUM_FONT_COUNT		6
#else
#define MAXIMUM_FONT_COUNT		4
#endif

static char *unicode_fontdata_list[] = {
    "/mnt/data/fonts/arial_2th_level_unicode_fnt.bin",
    "/mnt/data/fonts/arial_3th_level_unicode_fnt.bin",
    "/mnt/data/fonts/arial_4th_level_unicode_fnt.bin",
    "/mnt/data/fonts/arial_5th_level_unicode_fnt.bin",
#ifdef __HANGLE__
	"/mnt/data/fonts/gulim_2th_level_cob_hangle_fnt.bin",
    "/mnt/data/fonts/arimo_1th_level_unicode_fnt.bin",
#endif
};

static char *unicode_fontwidth_list[] = {
    "/mnt/data/fonts/arial_2th_level_unicode_width.bin",
    "/mnt/data/fonts/arial_3th_level_unicode_width.bin",
    "/mnt/data/fonts/arial_4th_level_unicode_width.bin",
    "/mnt/data/fonts/arial_5th_level_unicode_width.bin",
#ifdef __HANGLE__
	"/mnt/data/fonts/gulim_2th_level_cob_hangle_width.bin",
    "/mnt/data/fonts/arimo_1th_level_unicode_width.bin",
#endif
};

struct Font_Info _fontinfo_list[MAXIMUM_FONT_COUNT];

static int  check_font_data_crc16(unsigned char *buf, int size)
{
	return 0;

    unsigned short gen_crc16, read_crc16;
    int ret = 0;

    gen_crc16 = make_crc16(buf, size - 2);

    read_crc16 = buf[size-2];
    read_crc16 += buf[size-1] << 8;

    if (read_crc16 != gen_crc16) {
        ret = -1;
        printf("<ERROR> FONT CRC, READ: 0x%04X, CAL: 0x%04X\n", read_crc16, gen_crc16);
    }

    return ret;
}

static int read_font_image(struct Font_Info *finfo, char *font_name, char *width_name)
{
	return 0;

    int fd_font, fd_width;
    int i, ret = -1;
    struct stat st_font, st_width;
    int size_font, size_width;
    char *buf;

    //printf("%s, font_name: %s, width_name: %s\n", __func__, font_name, width_name);

    memset(finfo, 0, sizeof(struct Font_Info));

    fd_font = open(font_name, O_RDONLY);
    if (fd_font < 0) {
        printf("Cannot open %s\n", font_name);
        return -1;
    }
    ret = fstat(fd_font, &st_font);

    fd_width = open(width_name, O_RDONLY);
    if (fd_width < 0) {
        printf("Cannot open %s\n", width_name);
        return -1;
    }
    ret = fstat(fd_width, &st_width);

    size_font = st_font.st_size;
    size_width = st_width.st_size;

    size_font += 3;
    size_font &= ~3;
    size_width += 3;
    size_width &= ~3;

    buf = malloc(size_font + size_width);
    if (buf == NULL) {
        printf("Cannnot alloc memory for font.\n");
        return -1;
    }

    finfo->font_buf = buf;
    finfo->font_width = buf + size_font;

    i = 0;
    buf = finfo->font_buf;
    while (1) {
        ret = read(fd_font, &buf[i], 2048);
        if (ret < 2048) {
            break;
        }
        i += ret;
    }

    close(fd_font);

    ret = check_font_data_crc16((unsigned char *)buf, st_font.st_size);
    if (ret != 0) {
        printf("<ERROR> FONT ERROR: %s\n", font_name); // Ver 0.77, 2014/08/06, at BUSAN
        return -1;
    }

    i = 0;
    buf = finfo->font_width;
    while (1) {
        ret = read(fd_width, &buf[i], 2048);
        if (ret < 2048) {
            break;
        }
        i += ret;
    }

    close(fd_width);

    //printf("%s, Done.\n", __func__);

    return 0;
}

int font_init(void)
{
	return 0;

    struct Font_Info *finfo;
    int i;

#ifdef __HANGLE__
	for (i = 0; i < MAXIMUM_FONT_COUNT; i++) {
#else
	for (i = 0; i < 4; i++) {
#endif
        finfo = &_fontinfo_list[i];
//printf("%s, finfo: 0x%x\n", __func__, (int)finfo);
        read_font_image(finfo, unicode_fontdata_list[i], unicode_fontwidth_list[i]);
        switch (i) {
            case 0:
                finfo->width = 26;
                finfo->height = 32;
                finfo->pixel_interval = 2;
                break;
            case 1:
                finfo->width = 23;
                finfo->height = 32;
                finfo->pixel_interval = 1;
                break;
            case 2:
                finfo->width = 22;
                finfo->height = 32;
                finfo->pixel_interval = 1;
                break;
            case 3:
                finfo->width = 16;
                finfo->height = 16;
                finfo->pixel_interval = 0;
                break;
#ifdef __HANGLE__
			case 4:
                finfo->width = 32;
                finfo->height = 32;
                finfo->pixel_interval = 2;
                break;
			case 5:
                finfo->width = 64;
                finfo->height = 64;
                finfo->pixel_interval = 2;
                break;

#endif
        }
    }

    return 0;
}

static int get_unicode_strlen(char *str)
{
	return 0;
    int len = 0, idx;
    unsigned short ch = 0;

//printf("%s, CHK1\n", __func__);
    idx = 0;
    while (1) {
        ch = str[idx] << 8;
//printf("%s, ch1: 0x%x\n", __func__, ch);
        ch |= str[idx+1];
//printf("%s, ch2: 0x%x\n", __func__, ch);
        if (ch == 0)
            break;
        len += 2;
        idx += 2;
    }

//printf("%s, len: %d\n", __func__, len);
    return len;
}

#if 0
void __lcd_put_char(int x, int y, int c, int colidx, struct Font_Info *finfo)
{
    int i, j, bits;

    for (i = 0; i < font_vga_8x8.height; i++) {
        bits = font_vga_8x8.data [font_vga_8x8.height * c + i];
        for (j = 0; j < font_vga_8x8.width; j++, bits <<= 1)
            if (bits & 0x80)
                pixel (x + j, y + i, colidx);
    }
}
#endif

void __lcd_put_string(int x, int y, char *s, unsigned int colidx, int font_level)
{
	return;

    int i, j, k, l, m;
    char *font_buf, *width_buf, *one_font_buf;
    struct Font_Info *finfo;
    int rn, yn, font_char_size, font_off;
    int slen, idx, w;
    unsigned int start_mask, mask, mask2;
    int data_bits, data_bits2;

    if (s == NULL)
       return;

    finfo = &_fontinfo_list[font_level];
    font_buf = finfo->font_buf;
    width_buf = finfo->font_width;

    rn = finfo->width;
    rn += (8-1);
    rn &= ~(8-1);
    rn >>= 3;
    yn = finfo->height;
    font_char_size = rn * yn;


	if(font_level == 5) {
		start_mask = 0x80000000;
	} else {
   		start_mask = 1 << ((8 * rn)-1);
	}

    slen = get_unicode_strlen(s);

    for (i = 0; i < slen; i += 2) {
        idx = s[i] << 8;
        idx |= s[i+1];
#ifdef __HANGLE__
		if(idx >= 0x80) {
			if ( idx >= HANGLE_FONT_START && idx <= HANGLE_FONT_END )
				idx -= (unsigned int)HANGLE_FONT_START;
			else
				continue;

			idx = ( ( idx >> 8 ) * HANGLE_FONT_GRPNUM ) + ( idx & 0xff );
		}
#endif
        font_off = font_char_size * idx;
        w = width_buf[idx];

#ifdef __HANGLE__	/* For using the Space */
		if(w == 00)
			w = 0x1C;
#endif
	
        one_font_buf = &font_buf[font_off];

        /* write one character */
        k = 0;
        for (j = 0; j < yn; j++) {
			if(font_level == 5)
				data_bits2 = 0;

            data_bits = 0;

            for (l = 0; l < rn; l++) {
				if(l > 3) {
					data_bits2 <<= 8;
					data_bits2 |= one_font_buf[k+l];
				} else {
                	data_bits <<= 8;
                	data_bits |= one_font_buf[k+l];
				}
            }

            k += rn;
            mask = start_mask;
			mask2 = start_mask;

            for (m = 0; m < w; m++) {
				if(m < 32) {
                	if (data_bits & (mask))
                    	pixel (x + m, y + j, colidx);
                	mask >>= 1;
				} else {
                	if (data_bits2 & (mask2))
                    	pixel (x + m, y + j, colidx);
					mask2 >>= 1;
				}
            }
        }
        x += w + finfo->pixel_interval;
    }
}

static int unicode_cal_font_width(char *s, int width, int font_start, int font_end)
{
	return 0;

    int i, f;
    char *width_buf;
    struct Font_Info *finfo;
    int slen, idx, w, fw = 0;

    slen = get_unicode_strlen(s);

	for (f = font_start; f <= font_end; ) {
		finfo = &_fontinfo_list[f];
		width_buf = finfo->font_width;

		fw = 0;
		for (i = 0; i < slen; i += 2) {
			idx = s[i] << 8;
			idx |= s[i+1];

			w = width_buf[idx];
			w += finfo->pixel_interval;

			fw += w;
		}
		if (fw <= width)
			break;

		if ((f+1) <= font_end)
			f++;
		else
			break;
	}

	//DBG_LOG("f:%d, fw: %d\n", f, fw);
	return ((fw<<16) | f);
}

static int get_font_height(int font_level)
{
    struct Font_Info *finfo;
    finfo = &_fontinfo_list[font_level];

    return finfo->height;
}

void ascii_to_unicode_lcd_put_string_center(int x, int y, int width, int height, char *a, unsigned int colidx,
                int font_start, int font_end)
{
	return;

    int x1, y1;
    int fonts_w;
    int i, n, l, h;
    char u[128];
    int font_level;

    l = strlen(a);
    if (l > (64-2))
        return;

    n = 0;
    for (i = 0; i < l; i++) {
        u[n++] = 0;
        u[n++] = a[i];
    }
    u[n++] = 0;
    u[n++] = 0;

    fonts_w = unicode_cal_font_width(u, width, font_start, font_end);
    font_level = fonts_w & 0xff;
    fonts_w >>= 16;

    x1 = x;
    if (fonts_w < width) {
        x1 += (width - fonts_w) >> 1;
    }

    h = get_font_height(font_level);
    y1 = y;
    if (height > h) {
        y1 += (height - h) >> 1;
    }

    __lcd_put_string(x1, y1, u, colidx, font_level);
}

void __lcd_put_string_center(int x, int y, int width, int height, char *s, unsigned int colidx,
                int font_start, int font_end)
{
	return;

    int x1, y1;
    int fonts_w;
    int h;
    int font_level;

    fonts_w = unicode_cal_font_width(s, width, font_start, font_end);
    font_level = fonts_w & 0xff;
    fonts_w >>= 16 ;
    x1 = x;
    if (fonts_w < width) {
        x1 += (width - fonts_w) >> 1;
    }

    h = get_font_height(font_level);
    y1 = y;
    if (height > h) {
        y1 += (height - h) >> 1;
    }

    __lcd_put_string(x1, y1, s, colidx, font_level);
}

#ifdef __HANGLE__
static int hangle_cal_font_width(char *s, int width, int font_start, int font_end)
{
	return 0;

	int i, j;
	struct Font_Info *finfo;
	int slen, idx, w, fw = 0;
	char *width_buf;
	unsigned char ch;
		
	slen = get_unicode_strlen(s);

	finfo = &_fontinfo_list[font_start];
	width_buf = finfo->font_width;

	//printf("%s-%s[%d] : slen = %d \n", __FILE__, __FUNCTION__, __LINE__, slen);
	for(j=0; j<slen; j++) {
		ch = s[j];
		//printf("%s-%s[%d] : ch = 0x%02X\n", __FILE__, __FUNCTION__, __LINE__, ch);
		if(ch >= 0x80) {
			j++;
			idx = (ch<<8);
			idx += (unsigned char)s[j];
			//printf("%s-%s[%d] : S[%d]:[%d] = 0x%02X, 0x%02X\n", __FILE__, __FUNCTION__, __LINE__, j-1, j, s[j-1], s[j]);
	
			if ( idx >= HANGLE_FONT_START && idx <= HANGLE_FONT_END )
				idx -= (unsigned int)HANGLE_FONT_START;
			else
				continue;

			//printf("%s-%s[%d] : %c%c : idx[%d]\n", __FILE__, __FUNCTION__, __LINE__, s[j-1], s[j], idx);
		
			idx = ( ( idx >> 8 ) * HANGLE_FONT_GRPNUM ) + ( idx & 0xff );
			//printf("%s-%s[%d] : %c%c : final_idx[%d]\n", __FILE__, __FUNCTION__, __LINE__, s[j-1], s[j], idx);
		
			w = width_buf[idx];
			if(w == 0x00)
				w = 0x1C;	/* For the Space */

			//printf("%s-%s[%d] : %c%c : w[%d]\n", __FILE__, __FUNCTION__, __LINE__, s[j-1], s[j], w);
			if ( j < (slen-1) &&  finfo->pixel_interval ) {
				w += finfo->pixel_interval;
			}
			fw += w;
		}
	}
	//printf("%s-%s[%d] : font_width = %d\n", __FILE__, __FUNCTION__, __LINE__, fw);
	return ((fw<<16)|font_start);
}

void __lcd_hangle_put_string_center(int x, int y, int width, int height, char *s, unsigned int colidx,
											int font_start, int font_end)
{
	return;

	int x1, y1;
	int fonts_w;
	int h;
	int font_level;
		
	fonts_w = hangle_cal_font_width(s, width, font_start, font_end);
	font_level = fonts_w & 0xff;
	fonts_w >>= 16 ;
	x1 = x;
		
	if (fonts_w < width) {
		x1 += (width - fonts_w) >> 1;
	}
	//printf("%s-%s[%d] : x1(%d)\n", __FILE__, __FUNCTION__, __LINE__, x1);
		
	h = get_font_height(font_level);
	y1 = y;
	if (height > h) {
		y1 += (height - h) >> 1;
	}
	
	//printf("%s-%s[%d] : x1(%d), y1(%d)\n", __FILE__, __FUNCTION__, __LINE__, x1, y1);
	__lcd_put_string(x1, y1, s, colidx, font_level);
}
#endif

/****** PA AREA ********************************************/
#if (BCAST_CALL_HEAD_NEW == 1)
static short bcast_head_area[] = {2, 2, 320-3, 2+40};
#else
static short bcast_head_area[] = {2, 2, 160-2, 2+40};
#endif

static char unicode_turkish_bcast_head_str[] = {0x00,0x59, 0x00,0x61, 0x00,0x79, 0x01,0x31, 0x00,0x6e, 0x00,0x00};
static char unicode_eng_bcast_head_str[] = {0, 'B', 0,'r', 0,'o', 0,'a', 0,'d', 0,'c', 0,'a', 0,'s', 0,'t', 0,0};
#ifdef __HANGLE__
static char unicode_hangul_bcast_head_str[] = {0xB9, 0xE6, 0xC8, 0xFD, 0xBC, 0xDB, 0x00, 0x00};	/* 諛⑹넚 */
static char unicode_hangul_func_bcast_head_str[] = {0xC8, 0xAB, 0xC8, 0xFD, 0xBA, 0xB8, 0xC8, 0xFD, 0xB9, 0xE6, 0xC8, 0xFD, 0xBC, 0xDB, 0x00, 0x00};	/* 諛⑹넚 */
static char unicode_hangul_start_bcast_head_str[] = {0xC3, 0xE2, 0xC8, 0xFD, 0xB9, 0xDF, 0xC8, 0xFD, 0xB9, 0xE6, 0xC8, 0xFD, 0xBC, 0xDB, 0x00, 0x00};	/* 諛⑹넚 */
static char unicode_hangul_station_bcast_head_str[] = {0xBF, 0xAA, 0xC8, 0xFD, 0xBE, 0xC8, 0xC8, 0xFD, 0xB3, 0xBB, 0xC8, 0xFD, 0xB9, 0xE6, 0xC8, 0xFD, 0xBC, 0xDB, 0x00, 0x00};	/* 諛⑹넚 */
#endif

static short unicode_pa_in_area[] =  {          20, 160, 20+120, 160+32};
static short unicode_pa_out_area[] = {   20+120+35, 160, 20+120+35+120, 160+32};


static char unicode_turkish_in_str[] =  {0x01,0x30, 0,0xC7, 0,0};
static char unicode_turkish_out_str[] = {0,0x44, 0,0x49, 1,0x5E, 0,0};

static char unicode_eng_in_str[] = {0,'I', 0,'N', 0,0};
static char unicode_eng_out_str[] = {0,'O', 0,'U', 0, 'T', 0,0};

#ifdef __HANGLE__
static char unicode_hangle_in_str[]  = {0xBD, 0xC7, 0xC8, 0xFD, 0xB3, 0xBB, 0, 0};
static char unicode_hangle_out_str[] = {0xBD, 0xC7, 0xC8, 0xFD, 0xBF, 0xDC, 0, 0};
#endif

/*
static char unicode_in_out_str[] = {
0,0x69, 0,0xe7, 0,0x69, 0,0x6e, 0,0x64, 0,0x65, 0,0x2F,
0,0x64, 1,0x31, 1,0x5F, 0,0x61, 0,0x72, 1,0x31,
0,0};
*/
static char unicode_pa_occ_str[] = {0, 0x4F, 0, 0x43, 0,0x43, 0x00, 0x00};
/************************************************************/

/********** LOG Messgae AREA ********************************/
static short log_msg_head_area[] = {2, 240-35, 320-3, 240-3};
/************************************************************/

/****** Function AREA ********************************************/
static short unicode_func_num_area[14][4] = {
       {0,   0, 0, 0},
/*01*/ {7,			56, 7+36, 56+40},
/*02*/ {7+36+9,			56, 7+36+36+9, 56+40},
/*03*/ {7+36+36+18,		56, 7+36+36+36+18, 56+40},
/*04*/ {7+36+36+36+27,		56, 7+36+36+36+36+26, 56+40},
/*05*/ {7+36+36+36+36+36,	56, 7+36+36+36+36+36+36, 56+40},
/*06*/ {7+36+36+36+36+36+45,	56, 7+36+36+36+36+36+36+45, 56+40},
/*07*/ {7+36+36+36+36+36+36+54,	56, 7+36+36+36+36+36+36+36+54, 56+40},

/*08*/ {7,			106, 7+36, 106+40},
/*09*/ {7+36+9,			106, 7+36+36+9, 106+40},
/*10*/ {7+36+36+18,		106, 7+36+36+36+18, 106+40},
/*11*/ {7+36+36+36+27,		106, 7+36+36+36+36+27, 106+40},
/*12*/ {7+36+36+36+36+36,	106, 7+36+36+36+36+36+36, 106+40},
/*13*/ {7+36+36+36+36+36+45,	106, 7+36+36+36+36+36+36+45, 106+40},

};
static char unicode_str_numbers[14][6] = {
    {0,0x30, 0,0},        {0,0x31, 0,0},        {0,0x32, 0,0},        {0,0x33, 0,0},       {0,0x34, 0,0},
    {0,0x35, 0,0},        {0,0x36, 0,0},        {0,0x37, 0,0},        {0,0x38, 0,0},       {0,0x39, 0,0},
    {0,0x31, 0,0x30,0,0}, {0,0x31, 0,0x31,0,0}, {0,0x31, 0,0x32,0,0}, {0,0x31, 0,0x33,0,0},
};
/************************************************************/

/****** CALL AREA ********************************************/
#if (BCAST_CALL_HEAD_NEW == 1)
static short call_head_area[] = {2, 2, 320-3, 2+40};
#else
static short call_head_area[] = {160+1, 2, 320-3, 2+40};
#endif

static char unicode_turkish_call_head_str[] = { 0, 'A', 0,'R', 0,'A', 0,0};
static char unicode_eng_call_head_str[] = { 0, 'C', 0,'A', 0,'L', 0,'L', 0,0};
#ifdef __HANGLE__
static char unicode_hangle_call_head_str[] = {0xC5, 0xEB, 0xC8, 0xFD, 0xC8, 0xAD, 0,0};
#endif

static char unicode_turkish_cab_str[] = { 0, 'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,0};

static short turkish_cab_cab_area[2][4] = {
    {20, 62, 20+150, 62+50},
    {20, 62+50+24, 20+150, 62+50+24+50}
};

static short eng_cab_cab_area[2][4] = {
    {20, 62, 20+114, 62+50},
    {20, 62+50+24, 20+114, 62+50+24+50}
};

static short turkish_pei_cab_area[2][4] = {
    {320-20-100, 62, 320-20, 62+50},
    {320-20-100, 62+50+24, 320-20, 62+50+24+50}
};

static short eng_pei_cab_area[2][4] = {
    {182, 62, 182+114, 62+50},
    {182, 62+50+24, 182+114, 62+50+24+50}
};

//static short pei_talk_cab_area1[] = {182-10, 62-10, 182+114+10, 62};
//static short pei_talk_cab_area2[] = {182-10, 62+50, 182+114+10, 62+50+10};
//static short pei_talk_cab_area3[] = {182-10, 62, 182+114, 62+50};
//static short pei_talk_cab_area4[] = {182+114, 62, 182+114+10, 62+50};
/************************************************************/

/****** Control AREA ********************************************/
static short vol_ctrl_head_area[] = {2, 2, 320-3, 2+40};
static short iovol_ctrl_head_area[] = {2, 2, 320-3, 2+40};
/* Volume Control */
static char volume_ctrl_head_turkish_str[] = { 0, 'S', 0,'e', 0,'s', 0,' ',
                                0,'K', 0,'o', 0,'n', 0,'t', 0,'r', 0,'o', 0,'l', 0,0xFc, 0,0};
static char volume_ctrl_head_eng_str[] = { 0, 'V', 0,'o', 0,'l', 0,'u', 0,'m', 0,'e', 0,' ',
                                0,'C', 0,'o', 0,'n', 0,'t', 0,'r', 0,'o', 0,'l', 0,0};
static char iovolume_ctrl_head_turkish_str[] = { 0, 'S', 0,'e', 0,'s', 0,' ',
                                0,'K', 0,'o', 0,'n', 0,'t', 0,'r', 0,'o', 0,'l', 0,0xFc, 0,0};
static char iovolume_ctrl_head_eng_str[] = { 0, 'V', 0,'o', 0,'l', 0,'u', 0,'m', 0,'e', 0,' ',
                                0,'C', 0,'o', 0,'n', 0,'t', 0,'r', 0,'o', 0,'l', 0,0};

#ifdef __HANGLE__
static char volume_ctrl_head_hangle_str[] = {0xBF, 0xEE, 0xC8, 0xFD, 0xC0, 0xFC, 0xC8, 0xFD, 0xBD, 0xC7, 0xC8, 0xFD, 0xBA, 0xBC, 0xC8, 0xFD, 0xB7, 0xFD, 0,0};
static char iovolume_ctrl_head_hangle_str[] = {0xB0, 0xB4, 0xC8, 0xFD, 0xC2, 0xF7, 0xC8, 0xFD, 0xBA, 0xBC, 0xC8, 0xFD, 0xB7, 0xFD, 0,0};
#endif

static char mic_turkish_str[] = {0,'M', 0,'i', 0,'k', 0,'r', 0,'o', 0,'f', 0,'o', 0,'n', 0,0};
static char mic_eng_str[] = {0,'M', 0,'I', 0,'C', 0,0};
#ifdef __HANGLE__
static char mic_hangle_str[] = {0xB8, 0xB6, 0xC0, 0xCC, 0xC5, 0xA9, 0,0};
#endif

static char inspk_turkish_str[] = {0,'K', 0,'a', 0,'p', 0,'a', 0,'l', 0x01, 0x31, 0,0};
static char inspk_eng_str[] = {0,'I', 0,'N', 0,0};
#ifdef __HANGLE__
static char inspk_hangle_str[] = {0xBD, 0xC7, 0xC8, 0xFD, 0xB3, 0xBB, 0,0};
#endif

static char spk_turkish_str[] = {0,'H', 0,'o', 0,'p', 0,'a', 0,'r', 0,'l', 0,0xF6, 0,'r', 0,0};
static char spk_eng_str[] = {0,'S', 0,'P', 0,'K', 0,0};
#ifdef __HANGLE__
static char spk_hangle_str[] = {0xBD, 0xBA, 0xC7, 0xC7, 0xC4, 0xBF, 0,0};
#endif

static char outspk_turkish_str[] = {0,'A', 0, 0xE7, 0x01, 0x31, 0,'k', 0,0};
static char outspk_eng_str[] = {0,'O', 0,'U', 0,'T', 0,0};
#ifdef __HANGLE__
static char outspk_hangle_str[] = {0xBD, 0xC7, 0xC8, 0xFD, 0xBF, 0xDC, 0,0};
#endif

static short turkish_vol_area[25][4] = {
    {115,   67, 115+6, 67+40},
    {121+4, 67, 125+6, 67+40},
    {131+4, 67, 135+6, 67+40},
    {141+4, 67, 145+6, 67+40},
    {151+4, 67, 155+6, 67+40},
    {161+4, 67, 165+6, 67+40},
    {171+4, 67, 175+6, 67+40},
    {181+4, 67, 185+6, 67+40},
    {191+4, 67, 195+6, 67+40},
    {201+4, 67, 205+6, 67+40},
    {211+4, 67, 215+6, 67+40},
    {221+4, 67, 225+6, 67+40},
    {231+4, 67, 235+6, 67+40},
    {241+4, 67, 245+6, 67+40},
    {251+4, 67, 255+6, 67+40},
    {261+4, 67, 265+6, 67+40},
    {271+4, 67, 275+6, 67+40},
    {281+4, 67, 285+6, 67+40},
    {291+4, 67, 295+6, 67+40},
    {301+4, 67, 305+6, 67+40},
    {0, 0, 0, 0},
};

#ifdef __HANGLE__
static short hangle_pamp_vol_area[25][4] = {
	{115,   67, 125+6, 67+40},
	{131+4, 67, 145+6, 67+40},
	{151+4, 67, 165+6, 67+40},
	{171+4, 67, 185+6, 67+40},
	{191+4, 67, 205+6, 67+40},
	{211+4, 67, 225+6, 67+40},
	{231+4, 67, 245+6, 67+40},
	{251+4, 67, 265+6, 67+40},
	{271+4, 67, 285+6, 67+40},
	{291+4, 67, 305+6, 67+40},
    {0, 0, 0, 0},
};


static short hangle_vol_area[25][4] = {
    {115,   67, 115+6, 67+40},
    {121+4, 67, 125+6, 67+40},
    {131+4, 67, 135+6, 67+40},
    {141+4, 67, 145+6, 67+40},
    {151+4, 67, 155+6, 67+40},
    {161+4, 67, 165+6, 67+40},
    {171+4, 67, 175+6, 67+40},
    {181+4, 67, 185+6, 67+40},
    {191+4, 67, 195+6, 67+40},
    {201+4, 67, 205+6, 67+40},
    {211+4, 67, 215+6, 67+40},
    {221+4, 67, 225+6, 67+40},
    {231+4, 67, 235+6, 67+40},
    {241+4, 67, 245+6, 67+40},
    {251+4, 67, 255+6, 67+40},
    {261+4, 67, 265+6, 67+40},
    {271+4, 67, 275+6, 67+40},
    {281+4, 67, 285+6, 67+40},
    {291+4, 67, 295+6, 67+40},
    {301+4, 67, 305+6, 67+40},
    {0, 0, 0, 0},
};
#endif

static short eng_vol_area[25][4] = {
    { 70,   67,  70+8, 67+40},
    { 78+4, 67,  82+8, 67+40},
    { 90+4, 67,  94+8, 67+40},
    {102+4, 67, 106+8, 67+40},
    {114+4, 67, 118+8, 67+40},
    {126+4, 67, 130+8, 67+40},
    {138+4, 67, 142+8, 67+40},
    {150+4, 67, 154+8, 67+40},
    {162+4, 67, 166+8, 67+40},
    {174+4, 67, 178+8, 67+40},
    {186+4, 67, 190+8, 67+40},
    {198+4, 67, 202+8, 67+40},
    {210+4, 67, 214+8, 67+40},
    {222+4, 67, 226+8, 67+40},
    {234+4, 67, 238+8, 67+40},
    {246+4, 67, 250+8, 67+40},
    {258+4, 67, 262+8, 67+40},
    {270+4, 67, 274+8, 67+40},
    {282+4, 67, 286+8, 67+40},
    {294+4, 67, 298+8, 67+40},
    {0, 0, 0, 0},
};

/************************************************************/

#if 0
/****** STATUS AREA ********************************************/
static short unicode_status_head_area[] = {2, 45+45+89+45, 320-3, 240-3};
static char unicode_status_head_str[] = {
0, 0x53, 0,0x74, 0,0x61, 0,0x74, 0,0x75, 0,0x73, 0,0x3A,
0, 0};
/************************************************************/
#endif

static void lcd_draw_bcast_head_area(int menu_active, int bcast_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    //int x11, y11, x22, y22;
    int bg_idx, fg_idx;//, bg_bold_line_idx;
    //static int old_menu_active = -1;

///printf("%s, menu_active:%d, bcast_active: %d\n", __func__, menu_active, bcast_active);
    if (menu_active) {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = bcast_head_area[0];
    y1 = bcast_head_area[1];
    x2 = bcast_head_area[2];
    y2 = bcast_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0: 	/* Engligh */
			msg = unicode_eng_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

		case 1: 	/* Turkish */
			msg = unicode_turkish_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

#ifdef __HANGLE__
		case 2: 	/* Hangul */
			msg = unicode_hangul_bcast_head_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_func_bcast_head_area(int menu_active, int bcast_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;//, bg_bold_line_idx;

    if (menu_active) {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = bcast_head_area[0];
    y1 = bcast_head_area[1];
    x2 = bcast_head_area[2];
    y2 = bcast_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0: 	/* Engligh */
			msg = unicode_eng_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

		case 1: 	/* Turkish */
			msg = unicode_turkish_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

#ifdef __HANGLE__
		case 2: 	/* Hangul */
			msg = unicode_hangul_func_bcast_head_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_start_bcast_head_area(int menu_active, int bcast_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;//, bg_bold_line_idx;

    if (menu_active) {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = bcast_head_area[0];
    y1 = bcast_head_area[1];
    x2 = bcast_head_area[2];
    y2 = bcast_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0: 	/* Engligh */
			msg = unicode_eng_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

		case 1: 	/* Turkish */
			msg = unicode_turkish_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

#ifdef __HANGLE__
		case 2: 	/* Hangul */
			msg = unicode_hangul_start_bcast_head_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_station_bcast_head_area(int menu_active, int bcast_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;//, bg_bold_line_idx;

    if (menu_active) {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (bcast_active) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = bcast_head_area[0];
    y1 = bcast_head_area[1];
    x2 = bcast_head_area[2];
    y2 = bcast_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0: 	/* Engligh */
			msg = unicode_eng_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

		case 1: 	/* Turkish */
			msg = unicode_turkish_bcast_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;

#ifdef __HANGLE__
		case 2: 	/* Hangul */
			msg = unicode_hangul_station_bcast_head_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_call_head_area(int menu_active, int call_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;

    if (menu_active) {
        if (call_active) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (call_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; //0x00AAAAAA
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = call_head_area[0];
    y1 = call_head_area[1];
    x2 = call_head_area[2];
    y2 = call_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0:
			msg = unicode_eng_call_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
		case 1:
			msg = unicode_turkish_call_head_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
#ifdef __HANGLE__
		case 2:
			msg = unicode_hangle_call_head_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_vol_ctrl_head_area(int menu_active, int ctrl_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;

    if (menu_active) {
        if (ctrl_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (ctrl_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; //0x00AAAAAA
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = vol_ctrl_head_area[0];
    y1 = vol_ctrl_head_area[1];
    x2 = vol_ctrl_head_area[2];
    y2 = vol_ctrl_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0:
			msg = volume_ctrl_head_eng_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
		case 1:
			msg = volume_ctrl_head_turkish_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
#ifdef __HANGLE__
		case 2:
			msg = volume_ctrl_head_hangle_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

static void lcd_draw_iovol_ctrl_head_area(int menu_active, int ctrl_active)
{
	return;

    char *msg;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;

    if (menu_active) {
        if (ctrl_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; // 0x00008000
            fg_idx = COLOR_HEAD_BG_2_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
        }
    }
    else {
        if (ctrl_active) {
            bg_idx = COLOR_HEAD_BG_1_idx; //0x00AAAAAA
            fg_idx = COLOR_black_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
    }

    x1 = iovol_ctrl_head_area[0];
    y1 = iovol_ctrl_head_area[1];
    x2 = iovol_ctrl_head_area[2];
    y2 = iovol_ctrl_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0:
			msg = iovolume_ctrl_head_eng_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
		case 1:
			msg = iovolume_ctrl_head_turkish_str;
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
			break;
#ifdef __HANGLE__
		case 2:
			msg = iovolume_ctrl_head_hangle_str;
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
			break;
#endif
	}
}

void write_font_pa_inout_area(int inout_stat, int force_active)
{
	return;

    int i, x1, y1, x2, y2;
    int bg_idx = 0, fg_idx = 0;
    char *msg;
    int active[2];// [0]: in, [1]: out
    static int old_active[2] = {-1, -1};

//printf("%s, inout_stat: %d\n", __func__, inout_stat);

    if (force_active) {
        old_active[0] = -1;
        old_active[1] = -1;
    }

    switch(inout_stat) {
        case INOUT_BUTTON_STATUS_ALLOFF:
            active[0] = 0;
            active[1] = 0;
            break;

        case INOUT_BUTTON_STATUS_INOUT_ON:
            active[0] = 1;
            active[1] = 1;
            break;

        case INOUT_BUTTON_STATUS_IN_ON:
            active[0] = 1;
            active[1] = 0;
            break;

        case INOUT_BUTTON_STATUS_OUT_ON:
            active[0] = 0;
            active[1] = 1;
            break;

        default: return;
    }

    if ((old_active[0] == active[0]) && (old_active[1] == active[1])) {
        return;
    }

    old_active[0] = active[0];
    old_active[1] = active[1];

    for (i = 0; i < 2; i++) {
        if (active[i]) {
            bg_idx = COLOR_BG_1_idx;
            fg_idx = COLOR_FG_1_idx;
        }
        else {
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
        }
        if (i == 0) {
			switch(selected_language) {
				default:
				case 0:
					msg = unicode_eng_in_str;
					break;
				case 1:
					msg = unicode_turkish_in_str;
					break;
#ifdef __HANGLE__
				case 2:
					msg = unicode_hangle_in_str;
					break;
#endif
			}
            x1 = unicode_pa_in_area[0];
            y1 = unicode_pa_in_area[1];
            x2 = unicode_pa_in_area[2];
            y2 = unicode_pa_in_area[3];
        }
        else {
			switch(selected_language) {
				default:
				case 0:
					msg = unicode_eng_out_str;
					break;
				case 1:
					msg = unicode_turkish_out_str;
					break;
#ifdef __HANGLE__
				case 2:
					msg = unicode_hangle_out_str;
					break;
#endif
			}
            x1 = unicode_pa_out_area[0];
            y1 = unicode_pa_out_area[1];
            x2 = unicode_pa_out_area[2];
            y2 = unicode_pa_out_area[3];
        }



        lcd_fillrect (x1, y1, x2, y2, bg_idx);
#ifdef __HANGLE__
		if(selected_language == 2)
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
		else
#endif
        	__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 0, 0);
    }
}

#if 0
void write_font_pa_occ_area(int occ_stat)
{
    int x1, y1, x2, y2;
    int bg_idx = 0, fg_idx = 0;

    switch(occ_stat) {
        case 0: // in active
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
           break;

        case 1: // select
           break;

        case 2: // active
           bg_idx = COLOR_BG_1_idx;
           fg_idx = COLOR_FG_1_idx;
           break;
    }

    x1 = unicode_pa_occ_area[0];
    y1 = unicode_pa_occ_area[1];
    x2 = unicode_pa_occ_area[2];
    y2 = unicode_pa_occ_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);
    __lcd_put_string_center(x1, y1, x2-x1, y2-y1, unicode_pa_occ_str, fg_idx, 1, 1);
}
#endif

static void write_font_func_area(int func_num, int force_active, int active, int func_start)
{
	return;

    int i;
    int x1, y1, x2, y2;
    int bg_idx, fg_idx;
    int color_bg_idx, color_fg_idx;
    int disp_func_num[2], num;
    static int old_func_num = 0;

//printf("\n%s, func_num: %d, force_active: %d, active:%d\n", __func__, func_num, force_active, active);

    bg_idx = COLOR_HEAD_BG_2_idx;
    fg_idx = COLOR_black_idx;

    if (force_active) {
        for (i = 1; i <= 13; i++) {
            x1 = unicode_func_num_area[i][0];
            y1 = unicode_func_num_area[i][1];
            x2 = unicode_func_num_area[i][2];
            y2 = unicode_func_num_area[i][3];
            lcd_fillrect (x1, y1, x2, y2, bg_idx);
            __lcd_put_string_center(x1, y1, x2-x1, y2-y1, &unicode_str_numbers[i][0], fg_idx, 0, 0);
        }
    }

    //if (!force_active && old_func_num == func_num && old_active == active)
    //    return;

    disp_func_num[0] = old_func_num;
    disp_func_num[1] = func_num;

    for (i = 0; i < 2; i++) {
        num = disp_func_num[i];

        if (num == func_num) {
            if (active) {
                if (!func_start)
                    color_bg_idx = COLOR_HEAD_BG_1_idx;
                else
                    color_bg_idx = COLOR_HEAD_BG_8_idx;
                color_fg_idx = COLOR_white_idx;
            }
            else {
                color_bg_idx = COLOR_HEAD_BG_5_idx;
                color_fg_idx = COLOR_gray_idx;
            }
        } else {
            color_bg_idx = bg_idx;
            color_fg_idx = fg_idx;
        }

        if (num >= 1 && num <= 13) {
            x1 = unicode_func_num_area[num][0];
            y1 = unicode_func_num_area[num][1];
            x2 = unicode_func_num_area[num][2];
            y2 = unicode_func_num_area[num][3];
            lcd_fillrect (x1, y1, x2, y2, color_bg_idx);
            __lcd_put_string_center(x1, y1, x2-x1, y2-y1, &unicode_str_numbers[num][0], color_fg_idx, 0, 0);
        }
    }

    old_func_num = func_num;
}

#define BLINK_DELAY	50

struct CAB_Call_Status {
	int stat;
	char car;
	char id;
	char train_num;
};

struct CAB_Call_Status	cab_call_status[2];

static void lcd_draw_cab_call_area(struct multi_call_status *cop_list, int re_new)
{
    int i, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int bg_idx = 0, fg_idx = 0;
    char msg[128] = {0, };
    char car, id, train_num;
    short status = 0;
    int cab_in_ctr;
    int __call_stat[2];
    char cab_dev_num[2][3] = {{0,}, {0,}};
    static int __old_call_stat[2] = {0, 0};
    static int blink_val[2] = {0,}, blink_delay[2] = {BLINK_DELAY, BLINK_DELAY};

    int old_stat, stat;

    if (re_new) {
        __old_call_stat[0] = 0;
        __old_call_stat[1] = 0;
    }

    cab_in_ctr = cop_list->ctr;
    if (cab_in_ctr == 1) {
        status = cop_list->info[cop_list->out_ptr].status;
        if (status == CALL_STATUS_WAKEUP) {
			//printf("%s, 1 -> CHK1, WAKEUP\n", __func__);
            __call_stat[0] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE) {
			//printf("%s, 1 -> CHK2, LIVE\n", __func__);
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_MON) {
			//printf("%s, 1 -> CHK2, LIVE\n", __func__);
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON_WAIT) {
			//printf("%s, 1 -> CHK2, MON WAITING\n", __func__);
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        }
        else
            __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;

        cab_dev_num[0][0] = cop_list->info[cop_list->out_ptr].car_id;
        cab_dev_num[0][1] = cop_list->info[cop_list->out_ptr].idx_id;
		cab_dev_num[0][2] = cop_list->info[cop_list->out_ptr].train_num;
		//printf("%s, 1 -> Dev: %d-%d\n", __func__, cab_dev_num[0][0], cab_dev_num[0][1]);

        __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;
    }
    else if (cab_in_ctr >= 2) {
        status = cop_list->info[cop_list->out_ptr].status;
        if (status == CALL_STATUS_WAKEUP) {
			//printf("%s, 2 -> CHK3, WAKEUP\n", __func__);
            __call_stat[0] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE) {
			//printf("%s, 2 -> CHK4, LIVE\n", __func__);
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_MON) {
			//printf("%s, 2 -> CHK4, LIVE\n", __func__);
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON_WAIT) {
			//printf("%s, 2 -> CHK4, LIVE\n", __func__);
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        }
        else
            __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;
 
        cab_dev_num[0][0] = cop_list->info[cop_list->out_ptr].car_id;
        cab_dev_num[0][1] = cop_list->info[cop_list->out_ptr].idx_id;
		cab_dev_num[0][2] = cop_list->info[cop_list->out_ptr].train_num;
		//printf("%s, 2-1 -> Dev: %d-%d\n", __func__, cab_dev_num[0][0], cab_dev_num[0][1]);

        status = cop_list->info[cop_list->out_ptr+1].status;
        if (status == CALL_STATUS_WAKEUP) {
			//printf("%s, 2 -> CHK5, WAKEUP\n", __func__);
            __call_stat[1] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE) {
			//printf("%s, 2 -> CHK6, LIVE\n", __func__);
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_MON) {
			//printf("%s, 2 -> CHK6, LIVE\n", __func__);
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON_WAIT) {
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        }
        else if (status == CALL_STATUS_LIVE) {
			//printf("%s, 2 -> CHK6, LIVE\n", __func__);
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else
            __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;

        cab_dev_num[1][0] = cop_list->info[cop_list->out_ptr+1].car_id;
        cab_dev_num[1][1] = cop_list->info[cop_list->out_ptr+1].idx_id;
		cab_dev_num[1][2] = cop_list->info[cop_list->out_ptr+1].train_num;
		//printf("%s, 2-2 -> Dev: %d-%d\n", __func__, cab_dev_num[1][0], cab_dev_num[1][1]);
    } else {
        __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;
        __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;
    }

    for (i = 0; i < 2; i++) {
        car = id = 0;
        stat = __call_stat[i];
        old_stat = __old_call_stat[i];

        if (   (old_stat == stat)
            && (stat != DISP_STAT_BLINK)
            && (stat != DISP_STAT_ACTIVE_SELECT_MON_WAIT)
           ) {
            continue;
		}

        __old_call_stat[i] = stat;

        switch (stat) {
        case DISP_STAT_NO_ACTIVE_NO_SELECT:	// no active
            bg_idx = COLOR_HEAD_BG_2_idx;
            fg_idx = COLOR_black_idx;
            break;
        case DISP_STAT_NO_ACTIVE_SELECT:	// no active select
            bg_idx = COLOR_BG_3_idx;
            fg_idx = COLOR_black_idx;
            break;
        case DISP_STAT_ACTIVE_SELECT:	// active, select
            bg_idx = COLOR_BG_3_idx;
            fg_idx = COLOR_white_idx;
            break;
        case DISP_STAT_ACTIVE_NO_SELECT:	// active, no select
            bg_idx = COLOR_HEAD_BG_1_idx;
            fg_idx = COLOR_white_idx;
            break;
        case DISP_STAT_ACTIVE_SELECT_MON:	// active, no select
            bg_idx = COLOR_blue2_idx;
            fg_idx = COLOR_white_idx;
            break;

        case DISP_STAT_ACTIVE_SELECT_MON_WAIT:	// active, no select, WAIT
            blink_delay[i]++;
            if (blink_delay[i] > BLINK_DELAY) {
    			//printf("CAB MON CALL 1, i: %d, MON blinking\n", i);
                blink_delay[i] = 0;
                if (blink_val[i] == 0) {
                    bg_idx = COLOR_HEAD_BG_2_idx;
                    fg_idx = COLOR_black_idx;
                } else {
                    bg_idx = COLOR_blue2_idx;
                    fg_idx = COLOR_white_idx;
                }
                blink_val[i] ^= 1;
            }
            break;

        case DISP_STAT_BLINK:
            blink_delay[i]++;
            if (blink_delay[i] > BLINK_DELAY) {
    			//printf("CAB CALL 1, i: %d, blinking\n", i);
                blink_delay[i] = 0;
                if (blink_val[i] == 0) {
                    bg_idx = COLOR_HEAD_BG_2_idx;
                    fg_idx = COLOR_black_idx;
                }
                else {
                    bg_idx = COLOR_HEAD_BG_1_idx;
                    fg_idx = COLOR_white_idx;
                }
                blink_val[i] ^= 1;
            }
            break;
        }

    	//printf("CAB CALL 2, i: %d, stat: %d\n", i, stat);
        if (  ((stat == DISP_STAT_BLINK) || (stat == DISP_STAT_ACTIVE_SELECT_MON_WAIT))
            && blink_delay[i] != 0)
            continue;

    	//printf("CAB CALL, CHK2\n");

        if (selected_language == 1) {
            x1 = turkish_cab_cab_area[i][0];
            y1 = turkish_cab_cab_area[i][1];
            x2 = turkish_cab_cab_area[i][2];
            y2 = turkish_cab_cab_area[i][3];
        } else {
            x1 = eng_cab_cab_area[i][0];
            y1 = eng_cab_cab_area[i][1];
            x2 = eng_cab_cab_area[i][2];
            y2 = eng_cab_cab_area[i][3];
        }

#if 0
        lcd_fillrect (x1, y1, x2, y2, bg_idx);

        if ((stat == DISP_STAT_NO_ACTIVE_SELECT) || (stat == DISP_STAT_ACTIVE_SELECT)) {
            bg_idx = COLOR_HEAD_BG_1_idx;
            lcd_fillrect (x1, y1, x2, y1+3, bg_idx);
            lcd_fillrect (x1, y2-3, x2, y2, bg_idx);
            lcd_fillrect (x1, y1, x1+3, y2, bg_idx);
            lcd_fillrect (x2-3, y1, x2, y2, bg_idx);
        }
#endif
        car = cab_dev_num[i][0];
        id = cab_dev_num[i][1];
		train_num = cab_dev_num[i][2];

        if (car < 0 || car > MAX_CAR_NUM) car = 0;
        if (id < 0 || id > MAX_CAB_ID_NUM) id = 0;
#if 0
        if (selected_language == 1) {
            i = get_unicode_strlen(unicode_turkish_cab_str);
            memcpy(msg, unicode_turkish_cab_str, i);

            if (car > 0 && car < 10 && id > 0 && id < 10) {
                msg[i++] = 0;
                msg[i++] = ' ';
                msg[i++] = 0,
                msg[i++] = car + '0';
                msg[i++] = 0;
                msg[i++] = '-';

                msg[i++] = 0,
                msg[i++] = id + '0';
            }
        } else {
            if (car > 0 && car < 10 && id > 0 && id < 10)
                sprintf(msg, "%s %d-%d", "CAB", car, id);
            else
                sprintf(msg, "%s", "CAB");
        }

        if (selected_language == 1)
            __lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
        else
            ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
#else
        if (car > 0 && car <= MAX_CAR_NUM && id > 0 && id < MAX_CAB_ID_NUM) {
			cab_call_status[i].stat = stat;
			cab_call_status[i].car = car;
			cab_call_status[i].id = id;
			cab_call_status[i].train_num = train_num;
		} else {
			cab_call_status[i].stat = stat;
			cab_call_status[i].car = 0;
			cab_call_status[i].id = 0;
			cab_call_status[i].train_num = 0;
		}

        //printf("[loop=%d, stat=%d] fg_idx = %d, %d-%d\n", i, stat, fg_idx, car, id);
#endif
    }
}

struct PEI_Call_Status {
	int stat;
	int emergency;
	char car;
	char id;
	char train_num;
};

struct PEI_Call_Status	pei_call_status[2];
static void lcd_draw_pei_call_area(struct multi_call_status *pei_list, int re_new, int is_pei_half, int is_talk)
{
    int i, x1 = 0, y1 = 0, x2 = 0, y2 = 0;
    int bg_idx = 0, fg_idx = 0;
    char msg[16] = {0, };
    char car, id, train_num;
    short status = 0, emergency[2] = {0, 0};
    int cab_in_ctr;
    int __call_stat[2];
    char pei_dev_num[2][3] = {{0,}, {0,}};
    static int __old_call_stat[2] = {0, 0};
    static int blink_val[2] = {0,}, blink_delay[2] = {BLINK_DELAY, BLINK_DELAY};

    int old_stat = 0, stat = 0;

    if (re_new) {
        __old_call_stat[0] = 0;
        __old_call_stat[1] = 0;
    }

    cab_in_ctr = pei_list->ctr;
    if (cab_in_ctr == 1) {
        status = pei_list->info[pei_list->out_ptr].status;
        emergency[0] = pei_list->info[pei_list->out_ptr].emergency;
        emergency[1] = 0;
//printf("%s, CHK0, emergency:%d\n", __func__, emergency);
        if (status == CALL_STATUS_WAKEUP)
        {
//printf("%s, 1 -> CHK1, WAKEUP\n", __func__);
            __call_stat[0] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE && !emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_LIVE && emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT_EMER;
        }
        else if (status == CALL_STATUS_MON && !emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON && emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_EMER;
        }
        else if (status == CALL_STATUS_MON_WAIT && !emergency[0])
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        else if (status == CALL_STATUS_MON_WAIT && emergency[0])
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT;
        else
            __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;

        pei_dev_num[0][0] = pei_list->info[pei_list->out_ptr].car_id;
        pei_dev_num[0][1] = pei_list->info[pei_list->out_ptr].idx_id;
		pei_dev_num[0][2] = pei_list->info[pei_list->out_ptr].train_num;
//printf("%s, 1 -> Dev: %d-%d\n", __func__, pei_dev_num[0][0], pei_dev_num[0][1]);

        __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;
    }
    else if (cab_in_ctr >= 2) {
        status = pei_list->info[pei_list->out_ptr].status;
        emergency[0] = pei_list->info[pei_list->out_ptr].emergency;
        if (status == CALL_STATUS_WAKEUP) {
//printf("%s, 2 -> CHK3, WAKEUP\n", __func__);
            __call_stat[0] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE && !emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_LIVE && emergency[0]) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_NO_SELECT_EMER;
        }
        else if (status == CALL_STATUS_MON) {
            blink_delay[0] = BLINK_DELAY;
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON_WAIT && !emergency[0])
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        else if (status == CALL_STATUS_MON_WAIT && emergency[0])
            __call_stat[0] = DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT;
        else
            __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;
 
        pei_dev_num[0][0] = pei_list->info[pei_list->out_ptr].car_id;
        pei_dev_num[0][1] = pei_list->info[pei_list->out_ptr].idx_id;
		pei_dev_num[0][2] = pei_list->info[pei_list->out_ptr].train_num;
//printf("%s, 2-1 -> Dev: %d-%d\n", __func__, pei_dev_num[0][0], pei_dev_num[0][1]);

        status = pei_list->info[pei_list->out_ptr+1].status;
        emergency[1] = pei_list->info[pei_list->out_ptr+1].emergency;
        if (status == CALL_STATUS_WAKEUP) {
//printf("%s, 2 -> CHK5, WAKEUP\n", __func__);
            __call_stat[1] = DISP_STAT_BLINK;
        }
        else if (status == CALL_STATUS_LIVE && !emergency[1]) {
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_NO_SELECT;
        }
        else if (status == CALL_STATUS_LIVE && emergency[1]) {
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_NO_SELECT_EMER;
        }
        else if (status == CALL_STATUS_MON) {
            blink_delay[1] = BLINK_DELAY;
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON;
        }
        else if (status == CALL_STATUS_MON_WAIT && !emergency[1])
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON_WAIT;
        else if (status == CALL_STATUS_MON_WAIT && emergency[1])
            __call_stat[1] = DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT;
        else
            __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;

        pei_dev_num[1][0] = pei_list->info[pei_list->out_ptr+1].car_id;
        pei_dev_num[1][1] = pei_list->info[pei_list->out_ptr+1].idx_id;
		pei_dev_num[1][2] = pei_list->info[pei_list->out_ptr+1].train_num;
//printf("%s, 2-2 -> Dev: %d-%d\n", __func__, pei_dev_num[1][0], pei_dev_num[1][1]);
    }
    else {
        __call_stat[0] = DISP_STAT_NO_ACTIVE_NO_SELECT;
        __call_stat[1] = DISP_STAT_NO_ACTIVE_NO_SELECT;
    }

for (i = 0; i < 2; i++) {
    car = id = 0;
	train_num = 0;
    stat = __call_stat[i];
    old_stat = __old_call_stat[i];

    if (   (old_stat == stat)
        && (stat != DISP_STAT_BLINK)
        && (stat != DISP_STAT_ACTIVE_SELECT_MON_WAIT)
        && (stat != DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT)
       )
        continue;

    __old_call_stat[i] = stat;

    switch (stat) {
    case DISP_STAT_NO_ACTIVE_NO_SELECT:	// no active
        bg_idx = COLOR_HEAD_BG_2_idx;
        fg_idx = COLOR_black_idx;
        break;
    case DISP_STAT_NO_ACTIVE_SELECT:	// no active select
        bg_idx = COLOR_BG_3_idx;
        fg_idx = COLOR_black_idx;
        break;
    case DISP_STAT_ACTIVE_SELECT:	// active, select
        //bg_idx = COLOR_HEAD_BG_1_idx;
        bg_idx = COLOR_BG_3_idx;
        //bg_idx = COLOR_BG_1_idx;
        fg_idx = COLOR_white_idx;
        break;
    case DISP_STAT_ACTIVE_NO_SELECT:	// active, no select
        bg_idx = COLOR_HEAD_BG_1_idx;
        fg_idx = COLOR_white_idx;
        break;
    case DISP_STAT_ACTIVE_SELECT_MON:	// active, no select
        bg_idx = COLOR_blue2_idx;
        fg_idx = COLOR_white_idx;
        break;
    case DISP_STAT_ACTIVE_SELECT_MON_EMER:	// active, no select
        //bg_idx = COLOR_orange_idx;
        bg_idx = COLOR_blue2_idx;
        fg_idx = COLOR_white_idx;
        break;
    case DISP_STAT_ACTIVE_NO_SELECT_EMER:	// active, no select
        bg_idx = COLOR_red_idx;
        fg_idx = COLOR_white_idx;
        break;

    case DISP_STAT_ACTIVE_SELECT_MON_WAIT:	// active, no select, WAIT
        blink_delay[i]++;
        if (blink_delay[i] > BLINK_DELAY) {
//printf("PEI MON CALL 1, i: %d, MON blinking\n", i);
            blink_delay[i] = 0;
            if (blink_val[i] == 0) {
                bg_idx = COLOR_HEAD_BG_2_idx;
                fg_idx = COLOR_black_idx;
            }
            else {
                bg_idx = COLOR_blue2_idx;
                fg_idx = COLOR_white_idx;
            }
            blink_val[i] ^= 1;
        }
        break;

    case DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT:	// active, no select, WAIT
        blink_delay[i]++;
        if (blink_delay[i] > BLINK_DELAY) {
//printf("PEI MON CALL 1, i: %d, MON blinking\n", i);
            blink_delay[i] = 0;
            if (blink_val[i] == 0) {
                bg_idx = COLOR_HEAD_BG_2_idx;
                fg_idx = COLOR_black_idx;
            }
            else {
                //bg_idx = COLOR_orange_idx;
                bg_idx = COLOR_blue2_idx;
                fg_idx = COLOR_white_idx;
            }
            blink_val[i] ^= 1;
        }
        break;

    case DISP_STAT_BLINK:
        blink_delay[i]++;
        if (blink_delay[i] > BLINK_DELAY) {
//printf("CAB CALL 1, i: %d, blinking\n", i);
            blink_delay[i] = 0;
            if (blink_val[i] == 0) {
                bg_idx = COLOR_HEAD_BG_2_idx;
                fg_idx = COLOR_black_idx;
            }
            else {
                if (emergency[i]) {
                    bg_idx = COLOR_red_idx;
                    fg_idx = COLOR_white_idx;
                }
                else {
                    bg_idx = COLOR_HEAD_BG_1_idx;
                    fg_idx = COLOR_white_idx;
                }
            }
            blink_val[i] ^= 1;
        }
        break;
    }

//printf("CAB CALL 2, i: %d, stat: %d\n", i, stat);
    if (   ((stat == DISP_STAT_BLINK) || (stat == DISP_STAT_ACTIVE_SELECT_MON_WAIT) || (stat == DISP_STAT_ACTIVE_SELECT_MON_EMER_WAIT))
        && blink_delay[i] != 0
       )
        continue;

//printf("CAB CALL, bg_idx: %d, fg_idx: %d\n", bg_idx, fg_idx);
    if (i == 0) {
#if 0
        if (is_pei_half) {
            x1 = pei_talk_cab_area1[0];
            y1 = pei_talk_cab_area1[1];
            x2 = pei_talk_cab_area1[2];
            y2 = pei_talk_cab_area1[3];
            if (is_talk)
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue_idx);
            else
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue2_idx);

            x1 = pei_talk_cab_area2[0];
            y1 = pei_talk_cab_area2[1];
            x2 = pei_talk_cab_area2[2];
            y2 = pei_talk_cab_area2[3];
            if (is_talk)
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue_idx);
            else
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue2_idx);

            x1 = pei_talk_cab_area3[0];
            y1 = pei_talk_cab_area3[1];
            x2 = pei_talk_cab_area3[2];
            y2 = pei_talk_cab_area3[3];
            if (is_talk)
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue_idx);
            else
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue2_idx);

            x1 = pei_talk_cab_area4[0];
            y1 = pei_talk_cab_area4[1];
            x2 = pei_talk_cab_area4[2];
            y2 = pei_talk_cab_area4[3];
            if (is_talk)
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue_idx);
            else
                lcd_fillrect (x1, y1, x2, y2, COLOR_blue2_idx);
        }
#endif
    }

    if (selected_language == 1) {
        x1 = turkish_pei_cab_area[i][0];
        y1 = turkish_pei_cab_area[i][1];
        x2 = turkish_pei_cab_area[i][2];
        y2 = turkish_pei_cab_area[i][3];
    } else {
        x1 = eng_pei_cab_area[i][0];
        y1 = eng_pei_cab_area[i][1];
        x2 = eng_pei_cab_area[i][2];
        y2 = eng_pei_cab_area[i][3];
    }

#if 0
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

    if ((stat == DISP_STAT_NO_ACTIVE_SELECT) || (stat == DISP_STAT_ACTIVE_SELECT)) {
        bg_idx = COLOR_HEAD_BG_1_idx;
        lcd_fillrect (x1, y1, x2, y1+3, bg_idx);
        lcd_fillrect (x1, y2-3, x2, y2, bg_idx);
        lcd_fillrect (x1, y1, x1+3, y2, bg_idx);
        lcd_fillrect (x2-3, y1, x2, y2, bg_idx);
    }
#endif
    car = pei_dev_num[i][0];
    id = pei_dev_num[i][1];
	train_num = pei_dev_num[i][2];

    if (car < 0 || car > MAX_CAR_NUM) car = 0;
    if (id < 0 || id > MAX_PEI_ID_NUM) id = 0;
#if 0
    if (car > 0 && id > 0)
        sprintf(msg, "%s %d-%d", "PEI", car, id);
    else
        sprintf(msg, "%s", "PEI");

    ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 1, 1);
#else
	if (car > 0 && id > 0) {
		pei_call_status[i].stat = stat;
		pei_call_status[i].emergency = (int)emergency[i];
		pei_call_status[i].car = car;
		pei_call_status[i].id = id;
		pei_call_status[i].train_num = train_num;
	} else {
		pei_call_status[i].stat = stat;
		pei_call_status[i].emergency = (int)emergency[i];
		pei_call_status[i].car = 0;
		pei_call_status[i].id = 0;
		pei_call_status[i].train_num = 0;
	}
#endif
}

}

void clear_menu_screen(void)
{
	return;

    int x1, y1, x2, y2;

#if 0
    x1 = 5;
    y1 = 46;
    x2 = 320 - 5;
    y2 = 240 - 40;
#else
    x1 = 2;
    y1 = 43;
    x2 = 320 - 3;
    y2 = 240 - 37;
#endif
    lcd_fillrect (x1, y1, x2, y2, COLOR_black_idx);
}

void clear_alarm_screen(void)
{
	return;

	lcd_fillrect(2, 2, 316, 203, COLOR_black_idx);
}

void menu_head_draw(int menu_active, int bcast_status, int func_status, int start_status, int station_status, int call_status, int vol_status, int iovol_status)
{
	return;

    static int old_menu_active = -1;
    static int old_bcast_status = -1;
    static int old_call_status = -1;
    static int old_vol_status = -1;
	static int old_iovol_status = -1;
    int x1, y1, x2, y2;

    int menu_select;
    int menu_live;

    if (    old_menu_active == menu_active
         && old_bcast_status == bcast_status
         && old_call_status == call_status
         && old_vol_status == vol_status
		 && old_iovol_status == iovol_status
		 && menu_force_redraw == 0
       ) {
		//DBG_LOG("-ERR- : old_menu_active(%d), old_bcast_status(%d), old_call_status(%d), old_vol_status(%d), old_iovol_status(%d)\n",
		//		old_menu_active, old_bcast_status, old_call_status, old_vol_status, old_iovol_status);
		//DBG_LOG("-ERR- : menu_active(%d), bcast_status(%d), call_status(%d), vol_status(%d), iovol_status(%d)\n",
		//		menu_active, bcast_status, call_status, vol_status, iovol_status);
		//DBG_LOG("-ERR- : menu_force_redraw(%d)\n", menu_force_redraw);
        return;
	}

    //menu_select = menu_active & 3;
    menu_select = menu_active & 7;
    menu_live = (menu_active & (1<<7)) ? 1 : 0;

    old_menu_active = menu_active;
    old_bcast_status = bcast_status;
    old_call_status = call_status;
    old_vol_status = vol_status;
	old_iovol_status = iovol_status;

	//printf("\n%s, menu_select: %d, menu_live:%d\n", __func__, menu_select, menu_live);
	//printf("%s, bcast_status: %d, call_status: %d\n", __func__, bcast_status, call_status);
    //clear_menu_screen();

    switch (menu_select) {
    case 1:
        lcd_draw_bcast_head_area(menu_live, bcast_status);
        x1 = bcast_head_area[2] + 1;
        y1 = 0;
        x2 = x1+1;
        y2 = bcast_head_area[3];
        lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);
        break;

	case 2: 	// Function Broadcast(홍보방송)
        lcd_draw_func_bcast_head_area(menu_live, func_status);
        x1 = bcast_head_area[2] + 1;
        y1 = 0;
        x2 = x1+1;
        y2 = bcast_head_area[3];
        lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);
		break;

	case 3:		// Start Broadcasr(출발방송)
        lcd_draw_start_bcast_head_area(menu_live, start_status);
        x1 = bcast_head_area[2] + 1;
        y1 = 0;
        x2 = x1+1;
        y2 = bcast_head_area[3];
        lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);
		break;

	case 4:		// Station Broadcast(역안내방송)
        lcd_draw_station_bcast_head_area(menu_live, station_status);
        x1 = bcast_head_area[2] + 1;
        y1 = 0;
        x2 = x1+1;
        y2 = bcast_head_area[3];
        lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);
		break;

    case 5:
		lcd_draw_call_head_area(menu_live, call_status);

        x1 = bcast_head_area[2] + 1;
        y1 = 0;
        x2 = x1+1;
        y2 = bcast_head_area[3];
        lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);
        break;
    case 6:
        lcd_draw_vol_ctrl_head_area(menu_live, vol_status);
		break;
    case 7:
        lcd_draw_iovol_ctrl_head_area(menu_live, iovol_status);
        break;
	default:
		break;
    }
}

static int bcast_menu_active = 0;
static int call_menu_active = 0;

static short unicode_bmenu_1_area[] =  {14,     65,         14+141,     65+43};
static short unicode_bmenu_2_area[] =  {14+151, 65,         24+141+141, 65+43};
static short unicode_bmenu_3_area[] =  {14,     65+43+30,   24+141+141, 65+43+30+43};

static short unicode_station_1_area[] =  {14,     65,         14+141,     65+43};
static short unicode_station_2_area[] =  {14+151, 65,         24+141+141, 65+43};
static short unicode_station_3_area[] =  {14,     65+43+30,   14+141,     65+43+30+43};
static short unicode_station_4_area[] =  {14+151, 65+43+30,   24+141+141, 65+43+30+43};

static char start_broadcast_hangle_str[] = {0xB9, 0xE6, 0xBC, 0xDB, 0xBD, 0xC3, 0xC0, 0xDB, 0,0};

static char func_str_numbers[10][4] = {
	{0,0x30, 0,0},        {0,0x31, 0,0},        {0,0x32, 0,0},        {0,0x33, 0,0},        {0,0x34, 0,0},          // 0 ~ 4
	{0,0x35, 0,0},        {0,0x36, 0,0},        {0,0x37, 0,0},        {0,0x38, 0,0},        {0,0x39, 0,0},          // 5 ~ 9
};

static short bmenu_func_num_area[4][4] = {
	{0,                      0,      	0,                          0},
	{15,                     75,     	15+70,                      75+75},
	{15+70+15,               75,     	15+70+15+70,                75+75},
	{15+70+15+70+17,         75+12,     15+70+15+70+17+120,         75+12+50},
};

static char start_1_hangle_str[] = {0xB3, 0xEB, 0xC8, 0xFD, 0xC6, 0xF7, 0,0};   // 노포
static char start_2_hangle_str[] = {0xB4, 0xD9, 0xB4, 0xEB, 0xC6, 0xF7, 0,0};   // 다대포
static char start_3_hangle_str[] = {0xC3, 0xE2, 0xB9, 0xDF, 0xC8, 0xFD, 0xBE, 0xC8, 0xB3, 0xBB, 0xB9, 0xE6, 0xBC, 0xDB, 0,0};   // 춣발 안내방송

static char station_stop_hangle_str[] = {0xB5, 0xB5, 0xC8, 0xFD, 0xC8, 0xFD, 0xC2, 0xF8, 0,0};  // 도착
static char station_door_hangle_str[] = {0xB9, 0xAE, 0xC8, 0xFD, 0xB4, 0xDD, 0xC8, 0xFB, 0,0};  // 문 닫힘

static char station_hangle_str[BMENU_STATION_END+1][16] = {
	{ 0,0 },
	{ 0xB3, 0xEB, 0xC8, 0xFD, 0xC6, 0xF7, 0,0 },    // 노포     1
	{ 0xB9, 0xFC, 0xBE, 0xEE, 0xBB, 0xE7, 0,0 },    // 범어사   2
	{ 0xB3, 0xB2, 0xC8, 0xFD, 0xBB, 0xEA, 0,0 },    // 남산     3
	{ 0xB5, 0xCE, 0xC8, 0xFD, 0xBD, 0xC7, 0,0 },    // 두실     4
	{ 0xB1, 0xB8, 0xC8, 0xFD, 0xBC, 0xAD, 0,0 },    // 구서     5
	{ 0xC0, 0xE5, 0xC8, 0xFD, 0xC0, 0xFC, 0,0 },    // 장전     6
	{ 0xBA, 0xCE, 0xBB, 0xEA, 0xB4, 0xEB, 0,0 },    // 부산대   7
	{ 0xBF, 0xC2, 0xC3, 0xB5, 0xC0, 0xE5, 0,0 },    // 온천장   8
	{ 0xB8, 0xED, 0xC8, 0xFD, 0xB7, 0xFB, 0,0 },    // 명륜     9
	{ 0xB5, 0xBF, 0xC8, 0xFD, 0xB7, 0xA1, 0,0 },    // 동래     10
	{ 0xB1, 0xB3, 0xC8, 0xFD, 0xB4, 0xEB, 0,0 },    // 교대     11
	{ 0xBF, 0xAC, 0xC8, 0xFD, 0xBB, 0xEA, 0,0 },    // 연산     12
	{ 0xBD, 0xC3, 0xC8, 0xFD, 0xC3, 0xBB, 0,0 },    // 시청     13
	{ 0xBE, 0xE7, 0xC8, 0xFD, 0xC1, 0xA4, 0,0 },    // 양정     14
	{ 0xBA, 0xCE, 0xC8, 0xFD, 0xC0, 0xFC, 0,0 },    // 부전     15
	{ 0xBC, 0xAD, 0xC8, 0xFD, 0xB8, 0xE9, 0,0 },    // 서면     16
	{ 0xB9, 0xFC, 0xB3, 0xBB, 0xB0, 0xF1, 0,0 },    // 범내골   17
	{ 0xB9, 0xFC, 0xC8, 0xFD, 0xC0, 0xCF, 0,0 },    // 범일     18
	{ 0xC1, 0xC2, 0xC8, 0xFD, 0xC3, 0xB5, 0,0 },    // 좌천     19
	{ 0xBA, 0xCE, 0xBB, 0xEA, 0xC1, 0xF8, 0,0 },    // 부산진   20
	{ 0xC3, 0xCA, 0xC8, 0xFD, 0xB7, 0xAE, 0,0 },    // 초량     21
	{ 0xBA, 0xCE, 0xBB, 0xEA, 0xBF, 0xAA, 0,0 },    // 부산역   22
	{ 0xC1, 0xDF, 0xBE, 0xD3, 0xB5, 0xBF, 0,0 },    // 중앙동   23
	{ 0xB3, 0xB2, 0xC8, 0xFD, 0xC6, 0xF7, 0,0 },    // 남포     24
	{ 0xC0, 0xDA, 0xB0, 0xA5, 0xC4, 0xA1, 0,0 },    // 자갈치   25
	{ 0xC5, 0xE4, 0xC8, 0xFD, 0xBC, 0xBA, 0,0 },    // 토성     26
	{ 0xB5, 0xBF, 0xB4, 0xEB, 0xBD, 0xC5, 0xB5, 0xBF, 0,0 },    // 동대신동 27
	{ 0xBC, 0xAD, 0xB4, 0xEB, 0xBD, 0xC5, 0xB5, 0xBF, 0,0 },    // 서대신동 28
	{ 0xB4, 0xEB, 0xC8, 0xFD, 0xC6, 0xBC, 0,0 },    // 대티 29
	{ 0xB1, 0xAB, 0xC8, 0xFD, 0xC1, 0xA4, 0,0 },    // 괴정 30
	{ 0xBB, 0xE7, 0xC8, 0xFD, 0xC7, 0xCF, 0,0 },    // 사하 31
	{ 0xB4, 0xE7, 0xC8, 0xFD, 0xB8, 0xAE, 0,0 },    // 당리 32
	{ 0xC7, 0xCF, 0xC8, 0xFD, 0xB4, 0xDC, 0,0 },    // 하단 33
	{ 0xBD, 0xC5, 0xC8, 0xFD, 0xC6, 0xF2, 0,0 },    // 신평 34 
	{ 0xBD, 0xC5, 0xB1, 0xD4, 0xC8, 0xFD, 0xC0, 0xCF, 0,0 },    // 신규 1 35 
	{ 0xBD, 0xC5, 0xB1, 0xD4, 0xC8, 0xFD, 0xC0, 0xCC, 0,0 },    // 신규 2 36 
	{ 0xBD, 0xC5, 0xB1, 0xD4, 0xC8, 0xFD, 0xBB, 0xEF, 0,0 },    // 신규 3 37 
	{ 0xBD, 0xC5, 0xB1, 0xD4, 0xC8, 0xFD, 0xBB, 0xE7, 0,0 },    // 신규 4 38 
	{ 0xBD, 0xC5, 0xB1, 0xD4, 0xC8, 0xFD, 0xBF, 0xC0, 0,0 },    // 신규 5 39 
	{ 0xB4, 0xD9, 0xB4, 0xEB, 0xC6, 0xF7, 0,0 },    // 다대포 40 
};


void write_font_bcast_page_menu(int x1, int y1, int x2, int y2, char *msg, int bg_idx, int fg_idx)
{
	return;

	    lcd_fillrect (x1, y1, x2, y2, bg_idx);
		    __lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg, fg_idx, 4, 4);
}

void broadcast_draw_station_menu(int station_1, int station_2, int select_station, int menu_select)
{
	return;

	int x1, y1, x2, y2;
	int bg_idx1, fg_idx1;
	int bg_idx2, fg_idx2;
	int bg_idx3, fg_idx3;
	int bg_idx4, fg_idx4;

	char *msg1;
	char *msg2;
	char *msg3;
	char *msg4;

	bg_idx1 = COLOR_HEAD_BG_2_idx;
	bg_idx2 = COLOR_HEAD_BG_2_idx;
	bg_idx3 = COLOR_HEAD_BG_2_idx;
	bg_idx4 = COLOR_HEAD_BG_2_idx;
	fg_idx1 = COLOR_black_idx;
	fg_idx2 = COLOR_black_idx;
	fg_idx3 = COLOR_black_idx;
	fg_idx4 = COLOR_black_idx;

	switch(menu_select) {
		case 1:	bg_idx1 = COLOR_HEAD_BG_1_idx;	fg_idx1 = COLOR_white_idx;	break;
		case 2:	bg_idx2 = COLOR_HEAD_BG_1_idx;	fg_idx2 = COLOR_white_idx;	break;
		case 3:	bg_idx3 = COLOR_HEAD_BG_1_idx;	fg_idx3 = COLOR_white_idx;	break;
		case 4:	bg_idx4 = COLOR_HEAD_BG_1_idx;	fg_idx4 = COLOR_white_idx;	break;
		default:	break;
	}

	if(menu_select > 2) {
		if(select_station == station_1) {
			bg_idx1 = COLOR_HEAD_BG_1_idx;	fg_idx1 = COLOR_black_idx;
		} else {
			bg_idx2 = COLOR_HEAD_BG_1_idx;	fg_idx2 = COLOR_black_idx;
		}
	}

	msg1 = &station_hangle_str[station_1][0];
	msg2 = &station_hangle_str[station_2][0];
	msg3 = station_stop_hangle_str;
	msg4 = station_door_hangle_str;

	write_font_bcast_page_menu(unicode_station_1_area[0], unicode_station_1_area[1], unicode_station_1_area[2], unicode_station_1_area[3],
							   msg1, bg_idx1, fg_idx1);
	write_font_bcast_page_menu(unicode_station_2_area[0], unicode_station_2_area[1], unicode_station_2_area[2], unicode_station_2_area[3],
							   msg2, bg_idx2, fg_idx2);
	write_font_bcast_page_menu(unicode_station_3_area[0], unicode_station_3_area[1], unicode_station_3_area[2], unicode_station_3_area[3],
							   msg3, bg_idx3, fg_idx3);
	write_font_bcast_page_menu(unicode_station_4_area[0], unicode_station_4_area[1], unicode_station_4_area[2], unicode_station_4_area[3],
							   msg4, bg_idx4, fg_idx4);
}

void broadcast_draw_start_menu(int start, int menu_select)
{
	return;

	int x1, y1, x2, y2;
	int bg_idx1, fg_idx1;
	int bg_idx2, fg_idx2;
	int bg_idx3, fg_idx3;

	char *msg1;
	char *msg2;
	char *msg3;

	bg_idx1 = COLOR_HEAD_BG_2_idx;
	bg_idx2 = COLOR_HEAD_BG_2_idx;
	bg_idx3 = COLOR_HEAD_BG_2_idx;
	fg_idx1 = COLOR_black_idx;
	fg_idx2 = COLOR_black_idx;
	fg_idx3 = COLOR_black_idx;

	switch(menu_select) {
		case 1:	bg_idx1 = COLOR_HEAD_BG_1_idx;	fg_idx1 = COLOR_white_idx;	break;
		case 2:	bg_idx2 = COLOR_HEAD_BG_1_idx;	fg_idx2 = COLOR_white_idx;	break;
		case 3:	bg_idx3 = COLOR_HEAD_BG_1_idx;	fg_idx3 = COLOR_white_idx;
				if(start == 1)
					bg_idx1 = COLOR_HEAD_BG_1_idx;  fg_idx1 = COLOR_black_idx;
				if (start == 2)
					bg_idx2 = COLOR_HEAD_BG_1_idx;  fg_idx2 = COLOR_black_idx;
				break;
		default:	break;
	}

	msg1 = start_1_hangle_str;
	msg2 = start_2_hangle_str;
	msg3 = start_3_hangle_str;

	write_font_bcast_page_menu(unicode_bmenu_1_area[0], unicode_bmenu_1_area[1], unicode_bmenu_1_area[2], unicode_bmenu_1_area[3],msg1, bg_idx1, fg_idx1);
	write_font_bcast_page_menu(unicode_bmenu_2_area[0], unicode_bmenu_2_area[1], unicode_bmenu_2_area[2], unicode_bmenu_2_area[3],msg2, bg_idx2, fg_idx2);
	write_font_bcast_page_menu(unicode_bmenu_3_area[0], unicode_bmenu_3_area[1], unicode_bmenu_3_area[2], unicode_bmenu_3_area[3],msg3, bg_idx3, fg_idx3);
}

void broadcast_draw_func_menu(int digit_10, int digit_1, int menu_select)
{
	return;

	int x1, y1, x2, y2;
	int bg_idx1, fg_idx1;
	int bg_idx2, fg_idx2;
	int bg_idx3, fg_idx3;

	char *msg1;
	char *msg2;
	char *msg3;

	if((digit_10 > 9) || (digit_1 > 9))
		return 0;
	if((digit_10 < 0) || (digit_1 < 0))
		return 0;

	bg_idx1 = COLOR_HEAD_BG_2_idx;
	bg_idx2 = COLOR_HEAD_BG_2_idx;
	bg_idx3 = COLOR_HEAD_BG_2_idx;
	fg_idx1 = COLOR_black_idx;
	fg_idx2 = COLOR_black_idx;
	fg_idx3 = COLOR_black_idx;

	switch(menu_select) {
		case 1:	bg_idx1 = COLOR_HEAD_BG_1_idx;	fg_idx1 = COLOR_white_idx;	break;
		case 2:	bg_idx2 = COLOR_HEAD_BG_1_idx;	fg_idx2 = COLOR_white_idx;	break;
		case 3:	bg_idx3 = COLOR_HEAD_BG_1_idx;	fg_idx3 = COLOR_white_idx;	break;
		default:	break;
	}

	msg1 = &func_str_numbers[digit_10][0];
	msg2 = &func_str_numbers[digit_1][0];
	msg3 = start_broadcast_hangle_str;

	x1= bmenu_func_num_area[1][0];
	y1= bmenu_func_num_area[1][1];
	x2= bmenu_func_num_area[1][2];
	y2= bmenu_func_num_area[1][3];
	lcd_fillrect (x1, y1, x2, y2, bg_idx1);
	__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg1, fg_idx1, 5, 5);

	x1= bmenu_func_num_area[2][0];
	y1= bmenu_func_num_area[2][1];
	x2= bmenu_func_num_area[2][2];
	y2= bmenu_func_num_area[2][3];
	lcd_fillrect (x1, y1, x2, y2, bg_idx2);
	__lcd_put_string_center(x1, y1, x2-x1, y2-y1, msg2, fg_idx2, 5, 5);

	x1= bmenu_func_num_area[3][0];
	y1= bmenu_func_num_area[3][1];
	x2= bmenu_func_num_area[3][2];
	y2= bmenu_func_num_area[3][3];
	lcd_fillrect (x1, y1, x2, y2, bg_idx3);
	__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, msg3, fg_idx3, 4, 4);
}

void broadcast_draw_menu(int inout_stat1, int inout_stat2, int occ_stat, int func, int force_active, int active, int func_start)
{
	return;

    int inout_stat_aux = 0;
    int inout_stat = INOUT_BUTTON_STATUS_ALLOFF;

	//printf("\n%s, 1,===== inout_stat1: %d, inout_stat2:0x%X\n", __func__, inout_stat1,inout_stat2);

    if (   (inout_stat2 & INOUT_BUTTON_STATUS_IN_ON_MASK)
        && (inout_stat2 & INOUT_BUTTON_STATUS_OUT_ON_MASK)
       )
        inout_stat_aux = INOUT_BUTTON_STATUS_INOUT_ON;
    else if (inout_stat2 & INOUT_BUTTON_STATUS_IN_ON_MASK)
        inout_stat_aux = INOUT_BUTTON_STATUS_IN_ON;
    else if (inout_stat2 & INOUT_BUTTON_STATUS_OUT_ON_MASK)
        inout_stat_aux = INOUT_BUTTON_STATUS_OUT_ON;
    else
        inout_stat_aux = INOUT_BUTTON_STATUS_ALLOFF;
	//printf("%s, 2,===== inout_stat_aux:0x%X\n", __func__, inout_stat_aux);

    if (inout_stat1 == INOUT_BUTTON_STATUS_ALLOFF) {
        inout_stat = inout_stat_aux;
    }
    else if (inout_stat_aux == INOUT_BUTTON_STATUS_ALLOFF) {
        inout_stat = inout_stat1;
    }
    else {
        inout_stat = inout_stat1 | inout_stat_aux;
    }

	//printf("%s, 3, ===== inout_stat1: %d, inout_stat_aux:%d, inout_stat: %d\n", __func__,
							//inout_stat1,inout_stat_aux, inout_stat);
	//printf("%s, 4, inout_stat: %d, func: %d, force_active: %d\n", __func__, inout_stat, func, force_active);

#ifdef __HANGLE__
	lcd_draw_image(7, 56, 312, 145, 0);	/* Small Logo : 0 */
    //write_font_func_area(func, force_active, active, func_start);
#else
    write_font_func_area(func, force_active, active, func_start);
#endif

    write_font_pa_inout_area(inout_stat, force_active);

    //write_font_pa_occ_area(occ_stat);
}

void call_draw_menu(struct multi_call_status *cab_list, struct multi_call_status *pei_list, int re_new, int is_pei_half, int is_talk)
{
    int x1, y1, x2, y2;
    int bg_idx = 0; //, fg_idx = 0;
    int adj = 0;

#if 0
    if (re_new) {
        if (selected_language == 1)
            adj = 26;

        bg_idx = COLOR_line_1_idx;
        x1 = 320/2 - 1 + adj;
        y1 = 50;
        x2 = x1 + 1;
        y2 = 240 - 45;
        lcd_fillrect (x1, y1, x2, y2, bg_idx);
    }
#endif

    lcd_draw_cab_call_area(cab_list, re_new);
    lcd_draw_pei_call_area(pei_list, re_new, is_pei_half, is_talk);
}


static void lcd_draw_iovol_ctrl_area(int nth, int active_n, int max_n, int vol_val, int active, int force_draw)
{
	return;

    int i;
    int x1, y1, x2, y2;
    int xx, yy;
    int bg_idx;
    int fg_idx;
	int active_idx;
    int y_add;
    char vol_str[16], *msg;

    if (nth == 0) {
        y_add = 0;

		switch(selected_language) {
			default:
			case 0:
            	msg = inspk_eng_str;
				break;
			case 1:
            	msg = inspk_turkish_str;
				break;
#ifdef __HANGLE__
			case 2:
            	msg = inspk_hangle_str;
				break;
#endif
		}
    }
    else if (nth == 1) {
        y_add = 70; // Ver 0.71, 2014/01/13, 70->55, in case of version string on LCD
		switch(selected_language) {
			default:
			case 0:
            	msg = outspk_eng_str;
				break;
			case 1:
            	msg = outspk_turkish_str;
				break;
#ifdef __HANGLE__
			case 2:
            	msg = outspk_hangle_str;
				break;
#endif
		}
    }
    else
        return;

    if (active) {
        bg_idx = COLOR_line_1_idx;
        fg_idx = COLOR_white_idx;
    }
    else {
        bg_idx = COLOR_HEAD_BG_6_idx;
        fg_idx = COLOR_HEAD_BG_2_idx;
    }

	switch(selected_language) {
		default:
		case 0:
        	x1 = eng_vol_area[0][0];
        	y1 = eng_vol_area[0][1] + y_add;
        	y2 = eng_vol_area[0][3] + y_add;

        	xx = eng_vol_area[max_n-1][2];
        	yy = eng_vol_area[max_n-1][3] + y_add;
			break;

		case 1:
        	x1 = turkish_vol_area[0][0];
        	y1 = turkish_vol_area[0][1] + y_add;
        	y2 = turkish_vol_area[0][3] + y_add;

        	xx = turkish_vol_area[max_n-1][2];
        	yy = turkish_vol_area[max_n-1][3] + y_add;
			break;
#ifdef __HANGLE__
		case 2:
        	x1 = hangle_pamp_vol_area[0][0];
        	y1 = hangle_pamp_vol_area[0][1] + y_add;
        	y2 = hangle_pamp_vol_area[0][3] + y_add;

        	xx = hangle_pamp_vol_area[max_n-1][2];
        	yy = hangle_pamp_vol_area[max_n-1][3] + y_add;
			break;
#endif
	}

    lcd_fillrect (2, y1, xx, yy, COLOR_black_idx);

#ifdef __HANGLE__
	if(selected_language == 2)
		__lcd_hangle_put_string_center(2, y1, x1-2, y2-y1, msg, fg_idx, 4, 4);
	else
#endif
    	__lcd_put_string_center(2, y1, x1-2, y2-y1, msg, fg_idx, 1, 1);

    for (i = 0; i < active_n; i++) {
		switch(selected_language) {
			case 1:
            	x1 = turkish_vol_area[i][0];
            	y1 = turkish_vol_area[i][1] + y_add;
            	x2 = turkish_vol_area[i][2];
            	y2 = turkish_vol_area[i][3] + y_add;
				break;

			default:
			case 0:
            	x1 = eng_vol_area[i][0];
            	y1 = eng_vol_area[i][1] + y_add;
            	x2 = eng_vol_area[i][2];
            	y2 = eng_vol_area[i][3] + y_add;
				break;
#ifdef __HANGLE__
			case 2:
            	x1 = hangle_pamp_vol_area[i][0];
            	y1 = hangle_pamp_vol_area[i][1] + y_add;
            	x2 = hangle_pamp_vol_area[i][2];
            	y2 = hangle_pamp_vol_area[i][3] + y_add;
				break;
#endif
        }

        lcd_fillrect (x1, y1, x2, y2, bg_idx);
    }

    bg_idx = COLOR_HEAD_BG_7_idx;
    for ( ; i < max_n; i++) {
		switch(selected_language) {
			case 1:
            	x1 = turkish_vol_area[i][0];
            	y1 = turkish_vol_area[i][1] + y_add;
            	x2 = turkish_vol_area[i][2];
            	y2 = turkish_vol_area[i][3] + y_add;
				break;
			
			default:
			case 0:
            	x1 = eng_vol_area[i][0];
            	y1 = eng_vol_area[i][1] + y_add;
            	x2 = eng_vol_area[i][2];
            	y2 = eng_vol_area[i][3] + y_add;
				break;
#ifdef __HANGLE__
			case 2:
            	x1 = hangle_pamp_vol_area[i][0];
            	y1 = hangle_pamp_vol_area[i][1] + y_add;
            	x2 = hangle_pamp_vol_area[i][2];
            	y2 = hangle_pamp_vol_area[i][3] + y_add;
				break;
#endif
		}

        lcd_fillrect (x1, y1, x2, y2, bg_idx);
    }

    sprintf(vol_str, "%d %%", vol_val);

	switch(selected_language) {
		case 1:
        	x1 = turkish_vol_area[0][0];
        	y1 = turkish_vol_area[0][1] + y_add;
        	x2 = turkish_vol_area[max_n-1][2];
        	y2 = turkish_vol_area[max_n-1][3] + y_add;
			break;

		default:
		case 0:
        	x1 = eng_vol_area[0][0];
        	y1 = eng_vol_area[0][1] + y_add;
        	x2 = eng_vol_area[max_n-1][2];
        	y2 = eng_vol_area[max_n-1][3] + y_add;
			break;
#ifdef __HANGLE__
		case 2:
        	x1 = hangle_pamp_vol_area[0][0];
        	y1 = hangle_pamp_vol_area[0][1] + y_add;
        	x2 = hangle_pamp_vol_area[max_n-1][2];
        	y2 = hangle_pamp_vol_area[max_n-1][3] + y_add;
			break;
#endif
    }

    fg_idx = COLOR_black_idx;
   	ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, vol_str, fg_idx, 0, 0);
}


static void lcd_draw_vol_ctrl_area(int nth, int active_n, int max_n, int vol_val, int active, int force_draw)
{
	return;

    int i;
    int x1, y1, x2, y2;
    int xx, yy;
    int bg_idx;
    int fg_idx;
    int y_add;
    char vol_str[16], *msg;

    if (nth == 0) {
        y_add = 0;
		switch(selected_language) {
			case 1:
            	msg = mic_turkish_str;
				break;
			default:
			case 0:
            	msg = mic_eng_str;
				break;
#ifdef __HANGLE__
			case 2:
				msg = mic_hangle_str;
				break;
#endif
		}
    }
    else if (nth == 1) {
        y_add = 70; // Ver 0.71, 2014/01/13, 70->55, in case of version string on LCD
		switch(selected_language) {
			case 1:
            	msg = spk_turkish_str;
				break;
			default:
			case 0:
            	msg = spk_eng_str;
				break;
#ifdef __HANGLE__
			case 2:
				msg = spk_hangle_str;
				break;
#endif
		}
    }
    else
        return;

    if (active) {
        bg_idx = COLOR_line_1_idx;
        fg_idx = COLOR_white_idx;
    }
    else {
        //bg_idx = COLOR_HEAD_BG_2_idx;
        bg_idx = COLOR_HEAD_BG_6_idx;
        fg_idx = COLOR_HEAD_BG_2_idx;
    }

	switch(selected_language) {
		case 1:
        	x1 = turkish_vol_area[0][0];
        	y1 = turkish_vol_area[0][1] + y_add;
        	y2 = turkish_vol_area[0][3] + y_add;

        	xx = turkish_vol_area[max_n-1][2];
        	yy = turkish_vol_area[max_n-1][3] + y_add;
			break;

		default:
		case 0:
        	x1 = eng_vol_area[0][0];
        	y1 = eng_vol_area[0][1] + y_add;
        	y2 = eng_vol_area[0][3] + y_add;

        	xx = eng_vol_area[max_n-1][2];
        	yy = eng_vol_area[max_n-1][3] + y_add;
			break;
#ifdef __HANGLE__
		case 2:
        	x1 = hangle_vol_area[0][0];
        	y1 = hangle_vol_area[0][1] + y_add;
        	y2 = hangle_vol_area[0][3] + y_add;

        	xx = hangle_vol_area[max_n-1][2];
        	yy = hangle_vol_area[max_n-1][3] + y_add;
			break;
#endif
    }

    lcd_fillrect (2, y1, xx, yy, COLOR_black_idx);

#ifdef __HANGLE__
	if(selected_language == 2)
		__lcd_hangle_put_string_center(2, y1, x1-2, y2-y1, msg, fg_idx, 4, 4);
	else
#endif
    	__lcd_put_string_center(2, y1, x1-2, y2-y1, msg, fg_idx, 1, 1);

    for (i = 0; i < active_n; i++) {
		switch(selected_language) {
			case 1:
            	x1 = turkish_vol_area[i][0];
            	y1 = turkish_vol_area[i][1] + y_add;
            	x2 = turkish_vol_area[i][2];
            	y2 = turkish_vol_area[i][3] + y_add;
				break;

			default:
			case 0:
            	x1 = eng_vol_area[i][0];
            	y1 = eng_vol_area[i][1] + y_add;
            	x2 = eng_vol_area[i][2];
            	y2 = eng_vol_area[i][3] + y_add;
				break;
#ifdef __HANGLE__
			case 2:
            	x1 = hangle_vol_area[i][0];
            	y1 = hangle_vol_area[i][1] + y_add;
            	x2 = hangle_vol_area[i][2];
            	y2 = hangle_vol_area[i][3] + y_add;
				break;
#endif
        }

        lcd_fillrect (x1, y1, x2, y2, bg_idx);
    }

    bg_idx = COLOR_HEAD_BG_7_idx;
    for ( ; i < max_n; i++) {
		switch(selected_language) {
			case 1:
            	x1 = turkish_vol_area[i][0];
            	y1 = turkish_vol_area[i][1] + y_add;
            	x2 = turkish_vol_area[i][2];
            	y2 = turkish_vol_area[i][3] + y_add;
				break;

			default:
			case 0:
            	x1 = eng_vol_area[i][0];
            	y1 = eng_vol_area[i][1] + y_add;
            	x2 = eng_vol_area[i][2];
            	y2 = eng_vol_area[i][3] + y_add;
				break;
#ifdef __HANGLE__
			case 2:
            	x1 = hangle_vol_area[i][0];
            	y1 = hangle_vol_area[i][1] + y_add;
            	x2 = hangle_vol_area[i][2];
            	y2 = hangle_vol_area[i][3] + y_add;
				break;
#endif
        }

        lcd_fillrect (x1, y1, x2, y2, bg_idx);
    }

    sprintf(vol_str, "%d %%", vol_val);

	switch(selected_language) {
		case 1:
        	x1 = turkish_vol_area[0][0];
        	y1 = turkish_vol_area[0][1] + y_add;
        	x2 = turkish_vol_area[max_n-1][2];
        	y2 = turkish_vol_area[max_n-1][3] + y_add;
			break;

		default:
		case 0:
       		x1 = eng_vol_area[0][0];
        	y1 = eng_vol_area[0][1] + y_add;
        	x2 = eng_vol_area[max_n-1][2];
        	y2 = eng_vol_area[max_n-1][3] + y_add;
			break;
#ifdef __HANGLE__
		case 2:
       		x1 = hangle_vol_area[0][0];
        	y1 = hangle_vol_area[0][1] + y_add;
        	x2 = hangle_vol_area[max_n-1][2];
        	y2 = hangle_vol_area[max_n-1][3] + y_add;
			break;
#endif
    }

    fg_idx = COLOR_black_idx;
    ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, vol_str, fg_idx, 0, 0);
}


void iovol_ctrl_draw_menu(int inspk_vol_value, int inspk_active, int outspk_vol_value, int outspk_active, int re_new)
{
	return;

    int max_n = 10;
    int active_n;

    if (inspk_vol_value >= 0) {
        active_n = inspk_vol_value / 10;
        lcd_draw_iovol_ctrl_area(0, active_n, max_n, inspk_vol_value, inspk_active, re_new);
    }

    if (outspk_vol_value >= 0) {
        active_n = outspk_vol_value / 10;
        lcd_draw_iovol_ctrl_area(1, active_n, max_n, outspk_vol_value, outspk_active, re_new);
    }
}

void vol_ctrl_draw_menu(int mic_vol_value, int mic_active, int spk_vol_value, int spk_active, int re_new)
{
	return;

    int max_n = 20;
    int active_n;

    if (mic_vol_value >= 0) {
        active_n = mic_vol_value / 5;
        lcd_draw_vol_ctrl_area(0, active_n, max_n, mic_vol_value, mic_active, re_new);
    }

    if (spk_vol_value >= 0) {
        active_n = spk_vol_value / 5;
        lcd_draw_vol_ctrl_area(1, active_n, max_n, spk_vol_value, spk_active, re_new);
    }
}

#if 0
/* Ver 0.71, Add, 2014/01/13 */
extern unsigned short get_version_digit(void);
void display_cop_version(void)
{
    unsigned short ver = get_version_digit();
    int x1, y1, x2, y2;
    char str[64];

    x2 = log_msg_head_area[2];
    y2 = log_msg_head_area[1];

    x1 = x2 - 80;
    y1 = y2 - 50;

    sprintf(str,  "Ver %d.%02d\n", (ver>>8), (ver&0xff));
    ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, str, COLOR_white_idx, 2, 2);
}
#endif

/*
#define LOG_MSG_STR_READY		1
#define LOG_MSG_STR_IP_SETUP_ERR	2
#define LOG_MSG_STR_NOT_CON_AVC		3
#define LOG_MSG_STR_PEI_HALF		4
#define LOG_MSG_STR_CAB_ID_ERROR	5
#define LOG_MSG_STR_CABCAB_WAITING	6
#define LOG_MSG_STR_CAB_REJECT_WAITING	7
#define LOG_MSG_STR_BUSY		8
#define LOG_MSG_STR_OCC_PA_BUSY		9
#define LOG_MSG_STR_CAB_PA_BUSY		10
#define LOG_MSG_STR_CABCAB_BUSY		11
#define LOG_MSG_STR_PEI_BUSY		12
#define LOG_MSG_STR_PEICAB_BUSY		13
#define LOG_MSG_STR_EMER_PEICAB_BUSY	14
#define LOG_MSG_STR_PEIOCC_BUSY		15
#define LOG_MSG_STR_FUNC_BUSY		16
#define LOG_MSG_STR_HELP_BUSY		17
*/

static char *eng_lcd_log_msg[] = {
    "NOT Defined",
    "READY",					/* 1  : 대기 : LOG_MSG_STR_READY */
    "IP SETUP ERROR", 			/* 2  : 주소설정 오류 : LOG_MSG_STR_IP_SETUP_ERR */
    "NOT READY", 				/* 3  : 준비중 : LOG_MSG_STR_NOT_CON_AVC */
    "Push 'S' button to talk", 	/* 4  : 알수없는 오류*/
    "CAB ID ERROR",				/* 5  : 운전실 ID 오류*/
    "CAB to CAB WAIT",			/* 6  : 운전실 통화 대기중*/
    "CAB REJECT WAIT",			/* 7  : 운전실 통화 불가*/
    "BUSY",						/* 8  : 통화중 */
    "OCC PA BUSY",				/* 9  : 사령 방송중*/
    "CAB PA BUSY",				/* 10 : 수동 방송중 */
    "CAB to CAB BUSY",			/* 11 : 운전실 통화중 */
    "PEI BUSY",					/* 12 : 승객 통화중 */
    "PEI to CAB BUSY",			/* 13 : 승객-운전실 통화중*/
    "EMER PEI BUSY",			/* 14 : 비상 통화중 */
    "PEI to OCC BUSY",			/* 15 : 사령실승객 통화중 */
    "FUNC BUSY",				/* 16 : 기능방송중*/
	"HELP BUSY",				/* 17 : 구원운전 통화중*/
};

static char unicode_hangle_lcd_log_msg[][128] = {
	{0, 0},
	/* 1  : 대  기          */
	{0xB4, 0xEB, 0xC8, 0xFD, 0xC8, 0xFD, 0xB1, 0xE2, 0, 0},

	/* 2  : 주소설정 오류   */
	{0xC1, 0xD6, 0xBC, 0xD2, 0xBC, 0xB3, 0xC1, 0xA4, 0x0D, 0x0A, 0xC8, 0xFD, 0xBF, 0xC0, 0xB7, 0xF9, 0, 0},

	/* 3  : 준 비 중        */
	{0xC1, 0xD8, 0xC8, 0xFD, 0xBA, 0xF1, 0xC8, 0xFD, 0xC1, 0xDF, 0, 0},

	/* 4  : 알수없는 오류   */
	{0xBE, 0xCB, 0xBC, 0xF6, 0xBE, 0xF8, 0xB4, 0xC2, 0xC8, 0xFD, 0xBF, 0xC0, 0xB7, 0xF9, 0, 0},

	/* 5  : 운전실ID 오류   */
	{0xBF, 0xEE, 0xC0, 0xFC, 0xBD, 0xC7, 0x0D, 0x0A, 0xC8, 0xE8, 0xC8, 0xE9, 0xC8, 0xFD, 0xBF, 0xC0, 0xB7, 0xF9, 0, 0},

	/* 6  : 운전실 통화 대기중 */
	{0xBF, 0xEE, 0xC0, 0xFC, 0xBD, 0xC7, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC8, 0xFD, 0xB4, 0xEB, 0xB1, 0xE2, 0xC1, 0xDF, 0, 0},

	/* 7  : 운전실 통화 불가 */
	{0xBF, 0xEE, 0xC0, 0xFC, 0xBD, 0xC7, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC8, 0xFD, 0xBA, 0xD2, 0xB0, 0xA1, 0, 0},

	/* 8  : 통화/방송중     */
	{0xC5, 0xEB, 0xC8, 0xAD, 0xC8, 0xEA, 0xB9, 0xE6, 0xBC, 0xDB, 0xC1, 0xDF, 0, 0},

	/* 9  : 사령 방송중     */
	{0xBB, 0xE7, 0xB7, 0xC9, 0xC8, 0xFD, 0xB9, 0xE6, 0xBC, 0xDB, 0xC1, 0xDF, 0, 0},

	/* 10 : 수동 방송중     */
	{0xBC, 0xF6, 0xB5, 0xBF, 0xC8, 0xFD, 0xB9, 0xE6, 0xBC, 0xDB, 0xC1, 0xDF, 0, 0},

	/* 11 : 운전실 통화중   */
	{0xBF, 0xEE, 0xC0, 0xFC, 0xBD, 0xC7, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},

	/* 12 : 승객 통화중     */
	{0xBD, 0xC2, 0xB0, 0xB4, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},

	
	/* 13 : 승객-운전실 통화중  */
	{0xBD, 0xC2, 0xB0, 0xB4, 0xC8, 0xEB, 0xBF, 0xEE, 0xC0, 0xFC, 0xBD, 0xC7, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},

	/* 14 : 승객-비상 통화중*/
	{0xBD, 0xC2, 0xB0, 0xB4, 0xC8, 0xEB, 0xBA, 0xF1, 0xBB, 0xF3, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},

	/* 15 : 승객-사령 통화중*/
	{0xBD, 0xC2, 0xB0, 0xB4, 0xC8, 0xEB, 0xBB, 0xE7, 0xB7, 0xC9, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},

	/* 16 : 기능방송중      */
	{0xB1, 0xE2, 0xB4, 0xC9, 0xB9, 0xE6, 0xBC, 0xDB, 0xC1, 0xDF, 0, 0},

	/* 17 : 구원운전 통화중 */
	{0xB1, 0xB8, 0xBF, 0xF8, 0xBF, 0xEE, 0xC0, 0xFC, 0xC8, 0xFD, 0xC5, 0xEB, 0xC8, 0xAD, 0xC1, 0xDF, 0 ,0},
};

static char unicode_turkish_lcd_log_msg[][128] = {
    {0,0},
    {0,'H', 0,'A', 0,'Z', 0, 'I', 0,'R', 0,0}, //"READY",		/* 1 */

    {0,'I', 0,'P', 0,' ', 0,'Y', 0,0xDC, 0,'K', 0,'L', 0,'E', 0,'M', 0,'E', 0,' ', 0,'H', 0,'A', 0,'T', 0,'A', 0,'S', 0,'I', 0,0}, 	/* 2 */
    {0,'H', 0,'A', 0,'Z', 0, 'I', 0,'R', 0, ' ', 0,'D', 0,'E', 1,0x1E, 1,0x30, 0,'L', 0,0}, //"NOT READY", 	/* 3 */

    {0,'P', 0,'u', 0,'s', 0,'h', 0,' ', 0,'S', 0,' ', 0,'b', 0,'u', 0,'t', 0,'t', 0,'o', 0,'n', 0,' ',
     0,'t', 0,'o', 0,' ', 0,'t', 0,'a', 0,'l', 0,'k', 0,0}, 	/* 4 */

    {0,0},	/* 5 */

    {0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,'d', 0,'e', 0,'n', 0,' ',
     0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z',0,'e', 0,' ',
     0,'B', 0,'E', 0,'K', 0,'L', 0,'E', 0,'M', 0, 'E', 0,0}, // "CAB to CAB WAIT" /* 6 */

    {0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,' ', 0,'R', 0,'E', 0,'D', 0,' ',
     0,'B', 0,'E', 0,'K', 0,'L', 0,'E', 0,'M', 0,'E', 0,'S', 1,0x30, 0,0},/* 7 */

    {0,0},		/* 8 */

    {0,'O', 0,'C', 0,'C', 0,' ', 0,'P', 0,'A', 0,' ', 0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0},	/* 9 */

    {0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,' ', 0,'P', 0,'A', 0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, //"CAB PA BUSY",	/* 10 */

    {0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,'d', 0,'e', 0,'n', 0,' ',
     0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,'e', 0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, //"CAB to CAB BUSY",	/* 11 */

    {0,'P', 0,'E', 0,'I', 0,' ', 0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, //"PEI BUSY",	/* 12 */

    {0,'P', 0,'E', 0,'I', 0,'d', 0,'e', 0,'n', 0,' ',
     0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,'e', 0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, // "PEI to CAB BUSY",	/* 13 */

    {0,'E', 0,'M', 0,'E', 0,'R', 0,' ', 0,'P', 0,'E', 0,'I',  0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, //"EMER PEI BUSY",	/* 14 */

    {0,'P', 0,'E', 0,'I', 0, 'd', 0,'e', 0,'n', 0,' ',
     0,'O', 0,'C', 0,'C', 0,'y', 0,'e', 0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, // "PEI to OCC BUSY", /* 15 */

    {0,'M', 0,'A', 0,'R', 0,'K', 1,0x30, 0,'Z', 0,' ', 0,'F', 0,'U', 0, 'N', 0, 'C', 0,' ',
     0,'M', 0,'E', 1,0x5E, 0,'G', 0,'U', 0,'L', 0,0}, //"CAB FUNC BUSY",	/* 16 */
};

void log_msg_draw(int str_idx)
{
	return;

    int x1, y1, x2, y2;
    int bg_idx, fg_idx;
    char *str = NULL;
    static int old_str_idx = -1;

    if (old_str_idx == str_idx)
        return;

    old_str_idx = str_idx;

    if (str_idx < 0 || str_idx > LOG_MSG_STR_MAX)
        return;

    switch(selected_language) {
		default:
		case 0:     /* Engligh */
			str = eng_lcd_log_msg[str_idx];
			break;

		case 1:     /* Turkish */
			str = &unicode_turkish_lcd_log_msg[str_idx][0];
			break;

#ifdef __HANGLE__
		case 2:     /* Hangul */
			str = &unicode_hangle_lcd_log_msg[str_idx][0];
			break;
#endif
	}   


    fg_idx = COLOR_black_idx;
    switch (str_idx) {
        case LOG_MSG_STR_READY:
            bg_idx = COLOR_HEAD_BG_2_idx;
            break;

        case LOG_MSG_STR_NOT_CON_AVC:
        case LOG_MSG_STR_PEI_HALF:
            fg_idx = COLOR_blue_idx;
            break;

        case LOG_MSG_STR_CAB_ID_ERROR:
            fg_idx = COLOR_blue_idx;
            break;

        case LOG_MSG_STR_CAB_REJECT_WAITING:
        case LOG_MSG_STR_CABCAB_WAITING:
            fg_idx = COLOR_blue_idx;
            break;

        case LOG_MSG_STR_IP_SETUP_ERR:
        case LOG_MSG_STR_BUSY:
        case LOG_MSG_STR_OCC_PA_BUSY:
        case LOG_MSG_STR_CAB_PA_BUSY:
        case LOG_MSG_STR_CABCAB_BUSY:
        case LOG_MSG_STR_PEI_BUSY:
        case LOG_MSG_STR_PEICAB_BUSY:
        case LOG_MSG_STR_EMER_PEICAB_BUSY:
        case LOG_MSG_STR_PEIOCC_BUSY:
        case LOG_MSG_STR_FUNC_BUSY:
		case LOG_MSG_STR_HELP_BUSY:
            fg_idx = COLOR_red_idx;
            break;
    }

    x1 = log_msg_head_area[0];
    y1 = log_msg_head_area[1];
    x2 = log_msg_head_area[2];
    y2 = log_msg_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, bg_idx);

	switch(selected_language) {
		default:
		case 0:     /* Engligh */
			ascii_to_unicode_lcd_put_string_center(x1, y1, x2-x1, y2-y1, str, fg_idx, 1, 2);
			break;

		case 1:     /* Turkish */
			__lcd_put_string_center(x1, y1, x2-x1, y2-y1, str, fg_idx, 1, 2);
			break;

#ifdef __HANGLE__
		case 2:     /* Hangul */
			__lcd_hangle_put_string_center(x1, y1, x2-x1, y2-y1, str, fg_idx, 4, 4);
			break;
#endif
	}
}

void draw_lines(void)
{
	return;

    int x1, x2, y1, y2;

    lcd_fillrect (0, 0, 320-1, 1, COLOR_line_1_idx);
    lcd_fillrect (0, 240-2, 320-1, 240-1, COLOR_line_1_idx);

    lcd_fillrect (0, 0, 1, 240-1, COLOR_line_1_idx);
    lcd_fillrect (320-2, 0, 320-1, 240-1, COLOR_line_1_idx);

    x1 = bcast_head_area[2] + 1;
    y1 = 0;
    x2 = x1+1;
    y2 = bcast_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);

    x1 = call_head_area[2] + 1;
    y1 = 0;
    x2 = x1+1;
    y2 = call_head_area[3];
    lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);

    x1 = 2;
    y1 = call_head_area[3] + 1;
    x2 = 320-1;
    y2 = y1 + 1;
    lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);

    x1 = 2;
    y1 = log_msg_head_area[1] - 2;
    x2 = 320-1;
    y2 = y1 + 1;
    lcd_fillrect (x1, y1, x2, y2, COLOR_line_1_idx);

}

