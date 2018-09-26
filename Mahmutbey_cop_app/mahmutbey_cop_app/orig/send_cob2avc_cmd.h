#ifndef __SEND_COP2AVC_CMD_H__
#define __SEND_COP2AVC_CMD_H__

void init_cop_multicast(void);

int send_cop2avc_packet_init(void);
int send_cop2avc_cmd_in_door_start_request(COP_Cur_Status_t __broadcast_status);
int send_cop2avc_cmd_in_door_stop_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_cmd_out_door_start_request(COP_Cur_Status_t __broadcast_status);
int send_cop2avc_cmd_out_door_stop_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_cmd_occ_pa_start_request(COP_Cur_Status_t __broadcast_status);
int send_cop2avc_cmd_occ_pa_stop_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_fire_flag_clear(COP_Cur_Status_t __broadcast_status, unsigned short status);

int send_cop2avc_cmd_cab2cab_call_start_request(COP_Cur_Status_t __broadcast_status);
int send_cop2avc_cmd_cab2cab_call_accept_request(COP_Cur_Status_t __broadcast_status,
        unsigned char car_num, unsigned char dev_num);
int send_cop2avc_cmd_cab2cab_call_stop_request(COP_Cur_Status_t __broadcast_status,
                                         unsigned char car_num, unsigned char dev_num);
int send_cop2avc_cmd_cab2cab_call_reject_request(COP_Cur_Status_t __broadcast_status,
                                       unsigned char car_num, unsigned char dev_num);

//int send_cop2avc_cmd_inout_door_start_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_cmd_braodcast_stop_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_cmd_func_bcast_start_request (COP_Cur_Status_t __broadcast_status, int func_option, int funcnumber, unsigned char is_station_fpa);
int send_cop2avc_cmd_func_bcast_stop_request (COP_Cur_Status_t __broadcast_status, int func_option, int funcnumber, unsigned char is_station_fpa);

int send_cop2avc_cmd_pei2cab_call_start_request(COP_Cur_Status_t __broadcast_status,
                             unsigned char car_num, unsigned char dev_num, int is_emergency, int is_join);
int send_cop2avc_cmd_pei2cab_call_stop_request(COP_Cur_Status_t __broadcast_status,
                             unsigned char car_num, unsigned char dev_num, int is_emergency, int same_id);
int send_cop2avc_cmd_pei2cab_can_talk_request(COP_Cur_Status_t __broadcast_status,
                               unsigned char car_num, unsigned char dev_num);
int send_cop2avc_cmd_pei2cab_cannot_talk_request(COP_Cur_Status_t __broadcast_status,
                               unsigned char car_num, unsigned char dev_num);
int send_cop2avc_cmd_pei2cab_can_talk_and_receive_request(COP_Cur_Status_t __broadcast_status,
                               unsigned char car_num, unsigned char dev_num);

int send_cop2avc_cmd_di_display_start_request(COP_Cur_Status_t __broadcast_status, unsigned int id);
int send_cop2avc_cmd_di_display_stop_request(COP_Cur_Status_t __broadcast_status);

int send_cop2avc_cmd_set_route_request(COP_Cur_Status_t __broadcast_status,
							unsigned int departure_id, unsigned int destination_id);//, unsigned int via);

int send_cop2avc_cmd_set_special_route_request(COP_Cur_Status_t __broadcast_status, unsigned short pattern_num, unsigned char is_start);

void update_inout_key_status(int whaton, char *where);

int send_all2avc_cycle_data(char *buf, int maxlen, unsigned short errcode, int errset, int errclear);
int send_cop2avc_cmd_sw_version_request(char SWversion[], char add4,char add3,char add2,char add1);
int send_cop2avc_cmd_sw_update_start(short SWversion, char add4,char add3,char add2,char add1,int DeviceId);
#endif  //__SEND_COP2AVC_CMD_H__
