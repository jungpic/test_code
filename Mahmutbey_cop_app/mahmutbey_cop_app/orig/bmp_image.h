#ifndef __BMP_IMAGE_H__
#define __BMP_IMAGE_H__

typedef struct {
	/* BITMAPFILEHEADER*/
	unsigned char	bfType[2];
	unsigned int	bfSize;
	unsigned short	bfReserved1;
	unsigned short	bfReserved2;
	unsigned int	bfOffBits;
} __attribute__((aligned(4))) BmpFileHead;  /* 16 bytes */
#define FILEHEADSIZE 14

/* windows style*/
typedef struct {
	/* BITMAPINFOHEADER*/
	unsigned int	BiSize;
	unsigned int	BiWidth;
	unsigned int	BiHeight;
	unsigned short	BiPlanes;
	unsigned short	BiBitCount;
	unsigned int	BiCompression;
	unsigned int	BiSizeImage;
	unsigned int	BiXpelsPerMeter;
	unsigned int	BiYpelsPerMeter;
	unsigned int	BiClrUsed;
	unsigned int	BiClrImportant;
} BmpInfoHead;
#define INFOHEADSIZE 40

typedef struct __palentry {
	unsigned char	b;
	unsigned char	g;
	unsigned char	r;
	unsigned char _padding;
} PalEntry;

#if 0
/* if you not use any more SURFACE Structure, must free the DMA memory */
typedef struct __surface_dma_buffer {
	PalEntry *DMA_palette;	/* palette buffer in kernel*/
	unsigned short *DMA_pixels;	/* image buffer in kernel*/
} SURFACE_DMA_Buffer;

typedef struct {
	unsigned short  draw_x;
	unsigned short  draw_y;
	unsigned int    draw_flags;
	unsigned short 	w;	 /* image width in pixels*/
	unsigned short 	align_w; /* image width on 2^n aligned*/
	unsigned short 	h;  /* image height in pixels*/
	unsigned short 	align_h; /* image height on 2^n aligned */
	char  bpp;	 /* bits per pixel (1, 4 or 8)*/
	char  bytesperpixel; /* bytes per pixel*/
	int palsize; /* palette size*/
	unsigned int  pitch;  /* bytes per line*/

    unsigned char alpha;
    unsigned int drawmode; /* drawmode : transparency, shade, alpha , no-repeat */
    unsigned int TileXSize; 
    unsigned int TileYSize; 
    unsigned int LUT4Bank; 
    COLOR   ColorKey;
    COLOR   ShadeColor;
	PalEntry *palette;	/* palette temporay buffer in application */
                        /* ===> STATIC Memory */
	unsigned short *pixels;	/* temporay image bits (dword right aligned) in application*/
                        /* ===> STATIC Memory */
    struct __surface_dma_buffer dma_buffer;
} SURFACE;
#endif


#endif // __BMP_IMAGE_H__
