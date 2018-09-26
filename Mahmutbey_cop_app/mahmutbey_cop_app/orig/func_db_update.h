#ifndef __FUNC_DB_UPDATE_H__
#define __FUNC_DB_UPDATE_H__

#define LINE_COUNT_MAX			2

#define STATION_COUNT_MAX       255
#define STATION_NAME_LEN_MAX    255
#define DI_MANUAL_DISPLAY_MAX   10

#define FUNC_BROADCAST_COUNT_MAX	255
#define FUNC_BROADCAST_NAME_LEN_MAX	255

#define SPECIAL_ROUTE_COUNT_MAX		20

struct station_data {
    unsigned int    code;
    char            name[STATION_NAME_LEN_MAX];
};

struct func_broadcast_data {
	unsigned int	code;
	char			name[FUNC_BROADCAST_NAME_LEN_MAX];
	unsigned char	color;
};

extern unsigned int station_count[LINE_COUNT_MAX];
extern struct station_data station_info[LINE_COUNT_MAX][STATION_COUNT_MAX];
extern unsigned int di_manual_display_count;
extern struct station_data di_manual_display_info[DI_MANUAL_DISPLAY_MAX];
extern unsigned int func_broadcast_count;
extern struct func_broadcast_data func_broadcast_info[FUNC_BROADCAST_COUNT_MAX];
extern unsigned int station_fpa_count;
extern struct func_broadcast_data station_fpa_info[FUNC_BROADCAST_COUNT_MAX];
extern unsigned int special_route_count;
extern struct station_data special_route[SPECIAL_ROUTE_COUNT_MAX];
#endif
