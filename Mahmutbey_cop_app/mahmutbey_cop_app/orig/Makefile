##

#CROSS_COMPILE=/home/jongkkim/local/arm-eabi-pis-sdi-linux-tools-2012.05/usr/bin/arm-linux-
CROSS_COMPILE=/opt/arm-eabi-broadcast-uclibc-linux-2012.11/usr/bin/arm-linux-
#CROSS_COMPILE 	= arm-linux-

AS	= $(CROSS_COMPILE)as
LD	= $(CROSS_COMPILE)ld
CC	= $(CROSS_COMPILE)gcc
CPP	= $(CC) -E
AR	= $(CROSS_COMPILE)ar
NM	= $(CROSS_COMPILE)nm
STRIP	= $(CROSS_COMPILE)strip
OBJCOPY	= $(CROSS_COMPILE)objcopy
OBJDUMP	= $(CROSS_COMPILE)objdump
RANLIB	= $(CROSS_COMPILE)ranlib

CFLAGS	= -O2 -Wall -fno-strength-reduce

LIBDIR	= 
LIBS	= -lm -lc -pthread -liconv -lrt
LDLIBS	= $(LIBS) #-L$(LIBDIR)

OBJS = main.o \
       avc_cycle_msg.o avc_tcp.o \
       send_cob2avc_cmd.o recv_avc2cob_cmd.o \
       audio_multicast.o \
       debug_multicast.o \
       gpio_key_dev.o audio_ctrl.o cob_process.o \
       bcd.o crc16.o \
       lcd.o font_8x8.o \
       bmp_24bpp_image.o \
       ftpget.o copyfd.o safe_read_write.o software_update.o \
       watchdog_trigger.o version.o mtd_nanddump.o mtd_info.o libmtd.o mtd_nandwrite.o \
       flash_erase.o \
       get_pid_by_name.o \
	   func_db_update.o \
	   g711.o \ 
	   StrLib.o usb_update.o \
	   

       #silence_mp3_rtp.o

TARGET = cob__app

export AS LD CC CPP AR NM STRIP OBJCOPY OBJDUMP RANLIB CFLAGS


all: $(TARGET)


$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)
	$(STRIP) $@
	tar zcf $(TARGET).tar.gz $(TARGET)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@
	
.s.o:
	$(AS) $< -o $@
	
clean:
	rm -f *.o *.elf *.dis
	rm -rf $(TARGET)
	rm -rf $(TARGET).tar.gz

cleanall:
	rm -f *.o *.elf *.dis
	rm -f $(TARGET)
	rm -f $(TARGET).tar.gz

