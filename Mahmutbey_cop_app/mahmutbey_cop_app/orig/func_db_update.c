#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>

#include "cob_process.h"

#include <my_global.h>
#include <mysql.h>

#include "ftpget.h"
#include "software_update.h"

#include "func_db_update.h"
#include "send_cob2avc_cmd.h"

#define DEBUG_LINE                      1
#if (DEBUG_LINE == 1)
	#define DBG_LOG(fmt, args...)   \
		do {                    \
			fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
		} while(0)
#else
	#define DBG_LOG(fmt, args...)
#endif

/*
 * Thread primitives
 */
#define THREAD_HANDLE	pthread_t
#define PTHREAD_FN      void *

#define OsCreateThread(_PHANDLE_, _PFUNc_, _PCONTEXT_)	pthread_create(_PHANDLE_, NULL, (void* (*)(void*))_PFUNc_, _PCONTEXT_)
#define OsJoinThread(_HANDLE_)							pthread_join(_HANDLE_,NULL)
#define OsExitThread(_EXIT_CODE_)						pthread_exit(NULL)

static THREAD_HANDLE	FuncUpdateThreadHandle;
static int				FuncUpdateThreadCreated;
static void 			*FuncDatabaseUpdateMySqlThread(void *param);

static int				FuncDatabase_BufLen;
static int				FuncDatabase_idx;;
static char				*BufFuncDatabase;

static char 			*buf = "";
static int				DB_func_no;
static char				DB_Utf8_Str[150];
static char				DB_EUC_KR_Str[150];
static char				DB_Tmp_Str[150];
static int				converted_StrLen;

extern char AVC_Master_IPName[16];				/* In the main.c	*/
//extern int done_func_db_update_completed;		/* In the main.c	*/
//extern char func_no_hangle_str[100][50];		/* In the lcd.c		*/
//extern int convert_utf8_init(void);

unsigned int line_count;
char line_name[LINE_COUNT_MAX][STATION_NAME_LEN_MAX];

unsigned int station_count[LINE_COUNT_MAX];
struct station_data station_info[LINE_COUNT_MAX][STATION_COUNT_MAX];
char	AVC_ver[10];

unsigned int di_manual_display_count;
struct station_data di_manual_display_info[DI_MANUAL_DISPLAY_MAX];

unsigned int func_broadcast_count;
struct func_broadcast_data func_broadcast_info[FUNC_BROADCAST_COUNT_MAX];

unsigned int station_fpa_count;
struct func_broadcast_data station_fpa_info[FUNC_BROADCAST_COUNT_MAX];

unsigned int special_route_count;
struct station_data special_route_info[SPECIAL_ROUTE_COUNT_MAX];

void clear_db_data(void)
{
	printf("Clear DB Data - station name, special DI, functional broadcast name\n");
	memset(station_count, 0, sizeof(station_count));
	memset(station_info, 0, sizeof(station_info));

	di_manual_display_count = 0;
	memset(di_manual_display_info, 0, sizeof(di_manual_display_info));

	func_broadcast_count = 0;
	memset(func_broadcast_info, 0, sizeof(func_broadcast_info));
}

void FuncUpdateThread_Start(void)
{
	int status;

	clear_db_data();
	status = OsCreateThread(&FuncUpdateThreadHandle, (PTHREAD_FN)FuncDatabaseUpdateMySqlThread, (void *)NULL);
	printf("Function Database Update Thread created, status = %d\n", status);
	FuncUpdateThreadCreated = 1;
}

void FuncUpdateThread_Stop(void)
{
	if(FuncUpdateThreadCreated) {
		OsJoinThread(FuncUpdateThreadHandle);
		printf("Function Database Update Thread closed...\n");
		FuncUpdateThreadCreated = 0;
	}
}

#if 0
void finish_with_error(MYSQL *con)
{
	DBG_LOG("%s\n", mysql_error(con));
	mysql_close(con);
	//exit(1);
}
#endif

static void *FuncDatabaseUpdateMySqlThread(void *param)
{
	int status, size, start, i, j;
	int num_fields;
	unsigned long *length;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	MYSQL_RES *result;

	//DBG_LOG("Function Database Update Thread........\n");

	MYSQL *con = mysql_init(NULL);
	if (con == NULL)  {
		DBG_LOG("%s\n", mysql_error(con));
		return 0;
	}

	/* Create a connection */
	if (mysql_real_connect(con, &AVC_Master_IPName[0], "wbpa", "dowon1222", "wbpa", 0, NULL, 0) == NULL) {
		DBG_LOG("%s\n", mysql_error(con));
		mysql_close(con);
		return 0;
	}

	if (mysql_query(con, "SELECT content FROM avc_configure WHERE item=\"avc_version\"") == NULL)
	{
		result = mysql_store_result(con);
		if (result != NULL)
		{
			row = mysql_fetch_row(result);
			length = mysql_fetch_lengths(result);
			memcpy(&AVC_ver[0], row[0], (int)length[0]);

			printf("##############################\n");
			printf("  AVC %-13s/Ver %-4s \n", AVC_Master_IPName, AVC_ver);
			printf("##############################\n");
		}
		else DBG_LOG("%s\n", mysql_error(con));
	}
	else DBG_LOG("%s\n", mysql_error(con));

	/* line information */
	printf("\n*0/5 Line information from Master AVC DB\n");
	/* Execute query */
	if (mysql_query(con, "SELECT line FROM wbpa_station_cop GROUP BY line ORDER BY line") == NULL)
	{
		/* Get the result set */
		result = mysql_store_result(con);
		if (result != NULL)
		{
			line_count = mysql_num_rows(result);
			i=0;

			while((row = mysql_fetch_row(result)))
			{
				//station_info[i].code = atoi(row[0]);

				length = mysql_fetch_lengths(result);

				memcpy(&line_name[i], row[0], (int)length[0]);

				i++;

				if (i >= LINE_COUNT_MAX)
					break;
			}

			if(line_count != i)
				printf("Error : DB line_count = %d, read count= %d\n", line_count, i);
			else
				printf("Total line count = %d\n", line_count);

			for(i=0; i<line_count; i++)
				printf("[%d][%s(%d)]\n", i, line_name[i], strlen(line_name[i]));
		}
		else DBG_LOG("%s\n", mysql_error(con));
	}
	else DBG_LOG("%s\n", mysql_error(con));
	printf("\n");

	for (j=0; j< line_count; j++)
	{
		char query[1024]={0,};
		/* Every station name */
		printf("\n*1/5 line-%d Station name from Master AVC DB\n",j);
		/* Execute query */
		sprintf(query, "SELECT CODE, STATION_NAME FROM wbpa_station_cop WHERE line=\"%s\" ORDER BY idx", line_name[j]);
		printf("query=(%s)\n",query);
		if (mysql_query(con, query) == NULL)
		{
			/* Get the result set */
			result = mysql_store_result(con);
			if (result != NULL)
			{
				station_count[j] = mysql_num_rows(result);
				i=0;

				while((row = mysql_fetch_row(result)))
				{
					station_info[j][i].code = atoi(row[0]);

					length = mysql_fetch_lengths(result);

					memcpy(&station_info[j][i].name[0], row[1], (int)length[1]);

					i++;

					if (i >= STATION_COUNT_MAX)
						break;
				}

				if(station_count[j] != i)
					printf("Error : DB station_count = %d, read count= %d\n", station_count[j], i);
				else
					printf("Total station count = %d\n", station_count[j]);

				i=0;
				for(i=0; i<station_count[j]; i++)
					printf("[%d][%s(%d)]\n", station_info[j][i].code, station_info[j][i].name,strlen(station_info[j][i].name));
			}
			else DBG_LOG("%s\n", mysql_error(con));
		}
		else DBG_LOG("%s\n", mysql_error(con));
		printf("\n");
	}

	/* Func Broadcast name */
	printf("\n*2/5 Functional broadcast name from Master AVC DB\n");
    /* Execute query */
    if (mysql_query(con, "SELECT CODE, NAME, COLOR FROM wbpa_functional_broadcast ORDER BY CODE") == NULL)
	{
		/* Get the result set */
		result = mysql_store_result(con);
		if (result != NULL)
		{
			func_broadcast_count = mysql_num_rows(result);
			i=0;

			while((row = mysql_fetch_row(result)))
			{   
				func_broadcast_info[i].code = atoi(row[0]);

				length = mysql_fetch_lengths(result);

				memcpy(&func_broadcast_info[i].name[0], row[1], (int)length[1]);

				func_broadcast_info[i].color = atoi(row[2]);

				i++;

				if (i >= FUNC_BROADCAST_COUNT_MAX)
					break;
			}   

			if(func_broadcast_count != i)
				printf("Warning : DB func_broadcast_count = %d, read count= %d\n", func_broadcast_count, i); 
			else
				printf("func_broadcast_count= %d\n", func_broadcast_count);

			i=0;
			for(i=0; i<func_broadcast_count; i++)
			{
				if(i >= FUNC_BROADCAST_COUNT_MAX)
				{
					func_broadcast_count = FUNC_BROADCAST_COUNT_MAX;
					break;
				}
				printf("[%d][%d][%s(%d)]\n", func_broadcast_info[i].code, func_broadcast_info[i].color, func_broadcast_info[i].name, strlen(func_broadcast_info[i].name));
			}

		}   
		else DBG_LOG("%s\n", mysql_error(con));
    }   
	else DBG_LOG("%s\n", mysql_error(con));
	printf("\n");

	/* Station FPA name */
	printf("\n*3/5 Special DI message from Master AVC DB\n");
    /* Execute query */
    if (mysql_query(con, "SELECT CODE, NAME FROM wbpa_station_functional_broadcast ORDER BY CODE") == NULL)
	{
		/* Get the result set */
		result = mysql_store_result(con);
		if (result != NULL)
		{
			station_fpa_count = mysql_num_rows(result);
			i=0;

			while((row = mysql_fetch_row(result)))
			{   
				station_fpa_info[i].code = atoi(row[0]);

				length = mysql_fetch_lengths(result);

				memcpy(&station_fpa_info[i].name[0], row[1], (int)length[1]);

				station_fpa_info[i].color = atoi(row[2]);

				i++;

				if (i >= FUNC_BROADCAST_COUNT_MAX)
					break;
			}   

			if(station_fpa_count != i)
				printf("Warning : DB station_fpa_count = %d, read count= %d\n", station_fpa_count, i); 
			else
				printf("station_fpa_count= %d\n", station_fpa_count);

			i=0;
			for(i=0; i<station_fpa_count; i++)
			{
				if(i >= FUNC_BROADCAST_COUNT_MAX)
				{
					station_fpa_count = FUNC_BROADCAST_COUNT_MAX;
					break;
				}
				printf("[%d][%s(%d)]\n", station_fpa_info[i].code, station_fpa_info[i].name, strlen(station_fpa_info[i].name));
			}

		}   
		else DBG_LOG("%s\n", mysql_error(con));
    }   
	else DBG_LOG("%s\n", mysql_error(con));
	printf("\n");


	/* DIF manual display message */
	printf("\n*4/5 Special DI message from Master AVC DB\n");
    /* Execute query */
    if (mysql_query(con, "SELECT NO, CONTENT FROM config_fdi_caption_engine WHERE item LIKE \"manual_display%\"") == NULL)
	{
		/* Get the result set */
		result = mysql_store_result(con);
		if (result != NULL)
		{
			di_manual_display_count = mysql_num_rows(result);
			i=0;

			while((row = mysql_fetch_row(result)))
			{   
				di_manual_display_info[i].code = atoi(row[0]);

				length = mysql_fetch_lengths(result);

				memcpy(&di_manual_display_info[i].name[0], row[1], (int)length[1]);

				i++;

				if (i >= DI_MANUAL_DISPLAY_MAX)
					break;
			}   

			if(di_manual_display_count != i)
				printf("Warning : DB di_manual_display_count = %d, read count= %d\n", di_manual_display_count, i); 
			else
				printf("di_manual_display_count = %d\n", di_manual_display_count);

			i=0;
			for(i=0; i<di_manual_display_count; i++)
			{
				if(i >= DI_MANUAL_DISPLAY_MAX)
				{
					di_manual_display_count = DI_MANUAL_DISPLAY_MAX;
					break;
				}
				printf("[%d][%s(%d)]\n", di_manual_display_info[i].code, di_manual_display_info[i].name,strlen(di_manual_display_info[i].name));
			}
		}   
		else DBG_LOG("%s\n", mysql_error(con));
    }   
	else DBG_LOG("%s\n", mysql_error(con));
	printf("\n");

	/* Special route */
	printf("\n*5/5 Special Route from Master AVC DB\n");
    /* Execute query */
    if (mysql_query(con, "SELECT pattern_no, pattern_name FROM wbpa_pattern WHERE purpose=2") == NULL)
	{
		/* Get the result set */
		result = mysql_store_result(con);
		if (result != NULL)
		{
			special_route_count = mysql_num_rows(result);
			i=0;

			while((row = mysql_fetch_row(result)))
			{   
				special_route_info[i].code = atoi(row[0]);

				length = mysql_fetch_lengths(result);

				memcpy(&special_route_info[i].name[0], row[1], (int)length[1]);

				i++;

				if (i >= SPECIAL_ROUTE_COUNT_MAX)
					break;
			}   

			if(special_route_count != i)
				printf("Warning : DB special_route_count = %d, read count= %d\n", special_route_count, i); 
			else
				printf("station_fpa_count= %d\n", special_route_count);

			i=0;
			for(i=0; i<special_route_count; i++)
			{
				if(i >= SPECIAL_ROUTE_COUNT_MAX)
				{
					special_route_count = SPECIAL_ROUTE_COUNT_MAX;
					break;
				}
				printf("[%d][%s(%d)]\n", special_route_info[i].code, special_route_info[i].name, strlen(special_route_info[i].name));
			}

		}   
		else DBG_LOG("%s\n", mysql_error(con));
    }   
	else DBG_LOG("%s\n", mysql_error(con));
	printf("\n");

	printf("Sation name, Func.BCAST name, Special DI, Special Route read from AVC DB, completed--------.\n");
	mysql_free_result(result);
	mysql_close(con);
}

int convert_ascii_to_euc_kr(char *euc_kr_str, char *out_str, int olen)
{
    int i, j;
#if 0
	char *obuf = out_str;
#endif
	size_t readBytes;
	int str_len;
	
	readBytes = strlen(euc_kr_str);
#if 0
	printf("\n##<< CONVERT ASCII to EUC-KR >> ###############");
	for (i = 0; i < readBytes; i++) {
		if ((i % 16) == 0)
		printf("\n");
		printf("%02X ", (unsigned char)euc_kr_str[i]);
	}   
	printf("\n##########################################\n");
#endif	
	str_len = 0;
	for(i=0, j =0; i<readBytes; ) { 
		if((unsigned char)euc_kr_str[i] < 0x80) {
			if((unsigned char)euc_kr_str[i] == 0x20) {
				DB_EUC_KR_Str[j++] = 0xA1;
				DB_EUC_KR_Str[j++] = 0xA1;
			} else {
				DB_EUC_KR_Str[j++] = 0xA3;
				DB_EUC_KR_Str[j++] = euc_kr_str[i] + 0x80;
			}   
			i++;
		} else {
			DB_EUC_KR_Str[j++] = euc_kr_str[i++];
			DB_EUC_KR_Str[j++] = euc_kr_str[i++];
		}   
		str_len += 2;
	}   
	DB_EUC_KR_Str[j++] = 0x00;
	DB_EUC_KR_Str[j++] = 0x00;
	str_len += 2;
#if 0
	printf("--------------------------");
	for (i = 0; i < str_len; i++) {
		if ((i % 16) == 0)
			printf("\n");
		printf("%02X ", (unsigned char)obuf[i]);
	}   
	printf("\n----------------------------------\n");
#endif	
	return str_len;
}

int cmd_di_display_start(unsigned int id)
{
	int ret=0;

	ret = send_cop2avc_cmd_di_display_start_request(0, di_manual_display_info[id-1].code);

	return ret;
}

int cmd_di_display_stop(void)
{
	int ret=0;

	ret = send_cop2avc_cmd_di_display_stop_request(0);

	return ret;
}

int cmd_set_route(unsigned int departure, unsigned int destination)//, unsigned int via)
{
	int ret=0;

	ret = send_cop2avc_cmd_set_route_request(0, departure, destination);//, via);

	return ret;
}

int cmd_set_special_route_start(unsigned short code)
{
	int ret=0;

	printf("cmd_set_special_route_start code =%d\n", code);
	ret = send_cop2avc_cmd_set_special_route_request(0, code, 1);

	return ret;
}

int cmd_set_special_route_stop(unsigned short code)
{
	int ret=0;

	printf("cmd_set_special_route_stop code =%d\n", code);
	ret = send_cop2avc_cmd_set_special_route_request(0, code, 0);

	return ret;
}
