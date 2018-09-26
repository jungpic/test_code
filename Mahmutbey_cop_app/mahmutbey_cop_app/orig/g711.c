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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "g711.h"

static int search(int val, int *table, int size)
{
	int	i;
    
    for (i = 0; i < size; i++) {
        if (val <= *table++)
            return (i);
    }
    return (size);
}

/*
 * linear2alaw() - Convert a 16-bit linear PCM value to 8-bit A-law
 *
 * linear2alaw() accepts an 16-bit integer and encodes it as A-law data.
 *
 *		Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	0000000wxyza			000wxyz
 *	0000001wxyza			001wxyz
 *	000001wxyzab			010wxyz
 *	00001wxyzabc			011wxyz
 *	0001wxyzabcd			100wxyz
 *	001wxyzabcde			101wxyz
 *	01wxyzabcdef			110wxyz
 *	1wxyzabcdefg			111wxyz
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
int linear2alaw(int	pcm_val)        /* 2's complement (16-bit range) */
{
	int	mask;
	int	seg;
	int	aval;
    
	pcm_val = pcm_val >> 3;
    
	if (pcm_val >= 0) {
		mask = 0xD5;		/* sign (7th) bit = 1 */
	} else {
		mask = 0x55;		/* sign bit = 0 */
		pcm_val = -pcm_val - 1;
	}
    
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_aend, 8);
	
	/* Combine the sign, segment, and quantization bits. */
	if (seg >= 8) {				/* out of range, return maximum value. */
		return (0x7F ^ mask);
	} else {
		aval = seg << SEG_SHIFT;
		if (seg < 2) {
			aval |= (pcm_val >> 1) & QUANT_MASK;
		} else {
			aval |= (pcm_val >> seg) & QUANT_MASK;
		}
		return (aval ^ mask);
	}
}


/*
 * alaw2linear() - Convert an A-law value to 16-bit linear PCM
 *
 */
int alaw2linear(int	a_val)
{
	int	t;
	int	seg;
	
	a_val ^= 0x55;
	
	t = (a_val & QUANT_MASK) << 4;
	seg = ((unsigned)a_val & SEG_MASK) >> SEG_SHIFT;
	
	switch (seg) {
		case 0:
			t += 8;
			break;
		case 1:
			t += 0x108;
			break;
		default:
			t += 0x108;
			t <<= seg - 1;
	}
	
	return ((a_val & SIGN_BIT) ? t : -t);
}


/*
 * linear2ulaw() - Convert a linear PCM value to u-law
 *
 * In order to simplify the encoding process, the original linear magnitude
 * is biased by adding 33 which shifts the encoding range from (0 - 8158) to
 * (33 - 8191). The result can be seen in the following encoding table:
 *
 *	Biased Linear Input Code	Compressed Code
 *	------------------------	---------------
 *	00000001wxyza			000wxyz
 *	0000001wxyzab			001wxyz
 *	000001wxyzabc			010wxyz
 *	00001wxyzabcd			011wxyz
 *	0001wxyzabcde			100wxyz
 *	001wxyzabcdef			101wxyz
 *	01wxyzabcdefg			110wxyz
 *	1wxyzabcdefgh			111wxyz
 *
 * Each biased linear code has a leading 1 which identifies the segment
 * number. The value of the segment number is equal to 7 minus the number
 * of leading 0's. The quantization interval is directly available as the
 * four bits wxyz.  * The trailing bits (a - h) are ignored.
 *
 * Ordinarily the complement of the resulting code word is used for
 * transmission, and so the code word is complemented before it is returned.
 *
 * For further information see John C. Bellamy's Digital Telephony, 1982,
 * John Wiley & Sons, pps 98-111 and 472-476.
 */
int linear2ulaw(int	pcm_val)	/* 2's complement (16-bit range) */
{
	int	mask;
	int	seg;
	int	uval;
    
	/* Get the sign and the magnitude of the value. */
	pcm_val = pcm_val >> 2;
	if (pcm_val < 0) {
		pcm_val = -pcm_val;
		mask = 0x7F;
	} else {
		mask = 0xFF;
	}
	
	if (pcm_val > CLIP)
		pcm_val = CLIP;		// clip the magnitude
	
	pcm_val += (BIAS >> 2);
	
	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm_val, seg_uend, 8);
    
	/*
   	 * Combine the sign, segment, quantization bits;
	 * and complement the code word.
	 */
	if (seg >= 8) {		// out of range, return maximum value.
		return (0x7F ^ mask);
	} else {
   		uval = (seg << 4) | ((pcm_val >> (seg + 1)) & 0xF);
		return (uval ^ mask);
	}
}

/*
 * ulaw2linear() - Convert a u-law value to 16-bit linear PCM
 *
 * First, a biased linear code is derived from the code word. An unbiased
 * output can then be obtained by subtracting 33 from the biased code.
 *
 * Note that this function expects to be passed the complement of the
 * original code word. This is in keeping with ISDN conventions.
 */
int ulaw2linear(int	u_val)
{
	int t;
	
	/* Complement to obtain normal u-law value. */
	u_val = ~u_val;
	
	/*
	 * Extract and bias the quantization bits. Then
	 * shift up by the segment number and subtract out the bias.
	 */
	t = ((u_val & QUANT_MASK) << 3) + BIAS;
	t <<= (u_val & SEG_MASK) >> SEG_SHIFT;
	
	return ((u_val & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

/* A-law to u-law conversion */
static int alaw2ulaw (int aval)
{
	aval &= 0xff;
	return ((aval & 0x80) ? (0xFF ^ a2u[aval ^ 0xD5]) :(0x7F ^ a2u[aval ^ 0x55]));
}

/* u-law to A-law conversion */
static int ulaw2alaw (int uval)
{
	uval &= 0xff;
	return ((uval & 0x80) ? (0xD5 ^ (u2a[0xFF ^ uval] - 1)) : (0x55 ^ (u2a[0x7F ^ uval] - 1)));
}
