#ifndef __RECV_AVC2COP_CMD_H__
#define __RECV_AVC2COP_CMD_H__

int check_avc2cop_cmd(char *buf, int len, COP_Cur_Status_t __broadcast_status, int data);
unsigned char get_car_number(char *buf, int len);
unsigned char get_device_number(char *buf, int len);

unsigned int get_rx_mult_addr(char *buf, int len);
unsigned int put_rx_mult_addr(char *buf, unsigned int rx_addr);

unsigned int get_tx_mult_addr(char *buf, int len);
unsigned int get_mon_mult_addr(char *buf, int len);
unsigned int get_train_number(char *buf, int len);

#endif // __RECV_AVC2COP_CMD_H__
