/****************************************************************************
 *
 *      Copyright (c) InterCon Systems Co., Ltd.  All rights reserved.
 *
 *      THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *      KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *      IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *      PURPOSE.
 *
 ****************************************************************************/

#ifndef __ICS_G711_H__
#define __ICS_G711_H__

#include <stdio.h>

#define bcopy(s,d,l)	_fmemcpy(d,s,l)
#define bzero(d,l)		_fmemset(d,0,l)


#define	SIGN_BIT		(0x80)		/* Sign bit for a A-law byte. */
#define	QUANT_MASK		(0xf)		/* Quantization field mask. */
#define	NSEGS			(8)		/* Number of A-law segments. */
#define	SEG_SHIFT		(4)		/* Left shift for segment number. */
#define	SEG_MASK		(0x70)		/* Segment field mask. */
#define	BIAS			(0x84)		/* Bias for linear code. */
#define CLIP            8159

#ifndef PROTOTYPES
#define PROTOTYPES 0
#endif


/* PROTO_LIST is defined depending on how PROTOTYPES is defined above.
 If using PROTOTYPES, then PROTO_LIST returns the list, otherwise it
 returns an empty list.
 */
#if PROTOTYPES
#define PROTO_LIST(list) list
#else
#define PROTO_LIST(list) ()
#endif

#ifndef max
#define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
#define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

static int seg_aend[8] = {0x1F, 0x3F, 0x7F, 0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF};
static int seg_uend[8] = {0x3F, 0x7F, 0xFF, 0x1FF,0x3FF, 0x7FF, 0xFFF, 0x1FFF};
static short seg_end[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};
static unsigned char PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
/* copy from CCITT G.711 specifications */
static unsigned char u2a[128] = {			/* u- to A-law conversions */
    1,		1,		2,		2,		3,		3,		4,		4,
    5,		5,		6,		6,		7,		7,		8,		8,
    9,		10,		11,		12,		13,		14,		15,		16,
    17,		18,		19,		20,		21,		22,		23,		24,
    25,		27,		29,		31,		33,		34,		35,		36,
    37,		38,		39,		40,		41,		42,		43,		44,
    46,		48,		49,		50,		51,		52,		53,		54,
    55,		56,		57,		58,		59,		60,		61,		62,
    64,		65,		66,		67,		68,		69,		70,		71,
    72,		73,		74,		75,		76,		77,		78,		79,
    /* corrected:
     81,		82,		83,		84,		85,		86,		87,		88,
     should be: */
    80,		82,		83,		84,		85,		86,		87,		88,
    89,		90,		91,		92,		93,		94,		95,		96,
    97,		98,		99,		100,	101,	102,	103,	104,
    105,	106,	107,	108,	109,	110,	111,	112,
    113,	114,	115,	116,	117,	118,	119,	120,
    121,	122,	123,	124,	125,	126,	127,	128};

static unsigned char a2u[128] = {			/* A- to u-law conversions */
    1,		3,		5,		7,		9,		11,		13,		15,
    16,		17,		18,		19,		20,		21,		22,		23,
    24,		25,		26,		27,		28,		29,		30,		31,
    32,		32,		33,		33,		34,		34,		35,		35,
    36,		37,		38,		39,		40,		41,		42,		43,
    44,		45,		46,		47,		48,		48,		49,		49,
    50,		51,		52,		53,		54,		55,		56,		57,
    58,		59,		60,		61,		62,		63,		64,		64,
    65,		66,		67,		68,		69,		70,		71,		72,
    /* corrected:
     73,		74,		75,		76,		77,		78,		79,		79,
     should be: */
    73,		74,		75,		76,		77,		78,		79,		80,
    80,		81,		82,		83,		84,		85,		86,		87,
    88,		89,		90,		91,		92,		93,		94,		95,
    96,		97,		98,		99,		100,	101,	102,	103,
    104,	105,	106,	107,	108,	109,	110,	111,
    112,	113,	114,	115,	116,	117,	118,	119,
    120,	121,	122,	123,	124,	125,	126,	127};

static int search(int val, int *table, int size);

int linear2alaw(int	pcm_val);
int alaw2linear(int	a_val);
int linear2ulaw(int pcm_val);
int ulaw2linear(int u_val);

#endif /* defined(__ICS_G711_H__) */
