/* ------------------------------------------------------------------------------------------------------------------------------
    vs10xx bsp and ioctl functions.
    Copyright (C) 2010 Richard van Paasen <rvpaasen@t3i.nl>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
   ----------------------------------------------------------------------------------------------------------------------------- */

#ifndef VS1053_H
#define VS1053_H

/* ----------------------------------------------------------------------------------------------------------------------------- */
/* Board Definition                                                                                                              */
/* ----------------------------------------------------------------------------------------------------------------------------- */
#define VS1053_MAX_DEVICES 4

#define VS1053_SPI_CTRL "vs1053-ctrl"
#define VS1053_SPI_DATA "vs1053-data"

struct vs1053_board_info {
	int device_id;
	int gpio_reset;
	int gpio_dreq;
};

/* ----------------------------------------------------- */
/* IOCTLS                                                */
/* ----------------------------------------------------- */

#define VS1053_CTL_TYPE      'Q'
#define VS1053_CTL_RESET     _IO(VS1053_CTL_TYPE, 1)
#define VS1053_CTL_QUE_FLUSH _IO(VS1053_CTL_TYPE, 2)
#define VS1053_CTL_SWRESET   _IO(VS1053_CTL_TYPE, 3)

#define VS1053_CTL_SINETEST  _IO(VS1053_CTL_TYPE, 10)
#define VS1053_CTL_MEMTEST   _IO(VS1053_CTL_TYPE, 11)

#define VS1053_CTL_GETSCIREG 		_IOWR(VS1053_CTL_TYPE, 20, struct vs1053_scireg)
#define VS1053_CTL_SETSCIREG 		_IOWR(VS1053_CTL_TYPE, 21, struct vs1053_scireg)

#define VS1053_CTL_GETCLOCKF 		_IOR(VS1053_CTL_TYPE, 22, struct vs1053_clockf)
#define VS1053_CTL_SETCLOCKF 		_IOW(VS1053_CTL_TYPE, 23, struct vs1053_clockf)
#define VS1053_CTL_GETVOLUME 		_IOR(VS1053_CTL_TYPE, 24, struct vs1053_volume)
#define VS1053_CTL_SETVOLUME 		_IOW(VS1053_CTL_TYPE, 25, struct vs1053_volume)
#define VS1053_CTL_GETTONE   		_IOR(VS1053_CTL_TYPE, 26, struct vs1053_tone)
#define VS1053_CTL_SETTONE   		_IOW(VS1053_CTL_TYPE, 27, struct vs1053_tone)
#define VS1053_CTL_GETSCIBURSTREG 	_IOWR(VS1053_CTL_TYPE, 28, struct vs1053_sciburstreg)
#define VS1053_CTL_ISREADY 	_IOR(VS1053_CTL_TYPE, 29, int)

struct vs1053_scireg {
	unsigned char reg; /* 0..15  */
	unsigned char msb; /* 0..255 */
	unsigned char lsb; /* 0..255 */
};

struct vs1053_clockf {
	unsigned int mul; /* 0..7    fixed clock multiplication factor */
	unsigned int add; /* 0..3    additional multiplication factor  */
	unsigned int clk; /* 0..2048 input clock (XTALI) in 4kHz steps */
};

struct vs1053_volume {
	unsigned int left; /* 0..255 */
	unsigned int right; /* 0..255 */
};

struct vs1053_tone {
	unsigned int trebboost; /* 0..15 (x 1.5 dB) amount of boost for high frequencies       */
	unsigned int treblimit; /* 0..15 (x 1  kHz) frequency above which amplitude is boosted */
	unsigned int bassboost; /* 0..15 (x 1   dB) amount of boost for low frequencies        */
	unsigned int basslimit; /* 0..15 (x 10  Hz) frequency below which amplitude is boosted */
};

struct vs1053_sciburstreg {
	unsigned char reg; /* 0..15  */
	unsigned char size; /* 0..255 */
	//unsigned short data[255];
	unsigned short *data;
};

#endif
