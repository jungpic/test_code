#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "usb_update.h"
#include "StrLib.h"

#define DEBUG_LINE                      1
#if (DEBUG_LINE == 1)
#define DBG_LOG(fmt, args...)   \
	do {                    \
		fprintf(stderr, "\n%s-%s[%d] : " fmt, __FILE__, __FUNCTION__, __LINE__, ##args);        \
	} while(0)
#else
#define DBG_LOG(fmt, args...)
#endif

char gValidFileBin[DEVICE_COUNT] = {0};
char deviceVerName[DEVICE_COUNT][100] = {
    "/media/usb0/sw_update/pib__firmware_update_image.txt",
    "/media/usb0/sw_update/scam__firmware_update_image.txt",
    "/media/usb0/sw_update/fcam__firmware_update_image.txt",
    "/media/usb0/sw_update/ocam__firmware_update_image.txt",
    "/media/usb0/sw_update/cop__firmware_update_image.txt",
    "/media/usb0/sw_update/pamp__firmware_update_image.txt",
    "/media/usb0/sw_update/pei__firmware_update_image.txt",
    "/media/usb0/sw_update/lrm__firmware_update_image.txt",
    "/media/usb0/sw_update/pid__firmware_update_image.txt",
    "/media/usb0/sw_update/dif__firmware_update_image.txt",
    "/media/usb0/sw_update/dis__firmware_update_image.txt",
    "/media/usb0/sw_update/nvr__firmware_update_image.txt",
};
char deviceFileName[DEVICE_COUNT][100] = {
    "/media/usb0/sw_update/pib__firmware_update_image.bin",
    "/media/usb0/sw_update/scam__firmware_update_image.bin",
    "/media/usb0/sw_update/fcam__firmware_update_image.bin",
    "/media/usb0/sw_update/ocam__firmware_update_image.bin",
    "/media/usb0/sw_update/cop__firmware_update_image.bin",
    "/media/usb0/sw_update/pamp__firmware_update_image.bin",
    "/media/usb0/sw_update/pei__firmware_update_image.bin",
    "/media/usb0/sw_update/lrm__firmware_update_image.bin",
    "/media/usb0/sw_update/pid__firmware_update_image.bin",
    "/media/usb0/sw_update/dif__firmware_update_image.bin",
    "/media/usb0/sw_update/dis__firmware_update_image.bin",
    "/media/usb0/sw_update/nvr__firmware_update_image.bin",
};


//static int iUSBFd = -1;
//static long long int llTotalSize = 0;
//static char strDownDir[100];
UsbUpdateSW_t 	UsbSwVer; /*sw update version usb */


int gDownPercent = 0;
int ReadUSBParameter(const char * strFileName, SUSBStatus * const pParameter ,int count);
int ReadPartitions( char * const strBuf );
int CheckUSBCapacity( SUSBStatus * const pParameter );
int WriteUSB(const unsigned char * pBuf, int iCount);

void init_UsbSwVer(void)
{
	UsbSwVer.PIBSwVer  = 0;
	UsbSwVer.SCAMSwVer = 0;
	UsbSwVer.FCAMSwVer = 0;
	UsbSwVer.OCAMSwVer = 0;
	UsbSwVer.COPSwVer  = 0;
	UsbSwVer.PAMPSwVer = 0;
	UsbSwVer.PEISwVer  = 0;
	UsbSwVer.LRMSwVer  = 0;
	UsbSwVer.PIDSwVer  = 0;
	UsbSwVer.DIFSwVer  = 0;
	UsbSwVer.DISSwVer  = 0;
	UsbSwVer.NVRSwVer  = 0;	

}

int ReadUSBParameter(const char * strFileName, SUSBStatus * const pParameter ,int count)
{
    int iRet = 0;
    FILE * pFp = NULL;
    char strLine[100] = {0};  //파일로부터 읽은 string msg 
    char * strEqualPos = NULL;
    char strEqualArray[100] = {0};
    char strName[100] = {0};	//ver 이름
    char strValue[100] = {0};	//version 정보 
    unsigned char ucLenFlag = 0U;


    if( access( strFileName, R_OK ) != 0 ) {
        iRet = -1;        
    }
    else {
        pFp = fopen(strFileName, "r");
        if(pFp == NULL) {
            iRet = -1;
        }
        else
        {			
			printf("ReadUSBParameter strFileName %s count %d\r\n",strFileName,count );
            while (fgets(strLine, (int)sizeof(strLine), pFp)) {
				printf("check string %s \r\n",strLine);
                if(memset(strName, 0x00, sizeof(strName)) == NULL) {
                    iRet = -1;
                }
                if(memset(strValue, 0x00, sizeof(strValue)) == NULL) {
                    iRet = -1;
                }
                if(iRet != -1) {
                    iRet = TrimString(strLine); //문자열 공백제거 
                }
                if(iRet == -1) {
					printf("TrimString fail \r\n");
                    break;
                }
                if (strlen(strLine) < 1U) {
                    ucLenFlag = 0U;
                }
                else {
                    ucLenFlag = 1U;
                }
                if( (ucLenFlag == 1U) ) {					
                    strEqualPos = strchr(strLine, 'r'); //ver에서 r 위치 검색 후 버정 정보 파싱 
                    if(strEqualPos != NULL) {
                        if( strcpy(strEqualArray, strEqualPos) == NULL) {							
                            iRet = -1;
                        }
                        *(strEqualPos+1) = '\0';						
                        if( strcpy(strName, strLine) == NULL) {
                            iRet = -1;
                        }
                        if( TrimRightString(strName) == -1 ) {
                            iRet = -1;
                        }
                        if( strcpy(strValue,&strEqualArray[1]) == NULL) {
                            iRet = -1;
                        }
                        if( TrimLeftString(strValue) == -1) {
                            iRet = -1;
                        }

                        if(iRet != -1) {
                            /* User Define Start */							 							 
                            if (strcmp(strName, "ver") == 0) {
                                    if(gValidFileBin[count] == 0) { /* bin file exist */

                                        switch(count) {
                                        case 0:
                                            UsbSwVer.PIBSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.PIBSwVer : %f\n", UsbSwVer.PIBSwVer);
                                            break;
                                        case 1:
                                            UsbSwVer.SCAMSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.SCAMSwVer : %f\n", UsbSwVer.SCAMSwVer);
                                            break;
                                        case 2:
                                            UsbSwVer.FCAMSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.FCAMSwVer : %f\n", UsbSwVer.FCAMSwVer);
                                            break;
                                        case 3:
                                            UsbSwVer.OCAMSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.OCAMSwVer : %f\n", UsbSwVer.OCAMSwVer);
                                            break;
                                        case 4:
                                            UsbSwVer.COPSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.COPSwVer : %f\n", UsbSwVer.COPSwVer);
                                            break;
                                        case 5:
                                            UsbSwVer.PAMPSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.PAMPSwVer : %f\n", UsbSwVer.PAMPSwVer);
                                            break;
                                        case 6:
                                            UsbSwVer.PEISwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.PEISwVer : %f\n", UsbSwVer.PEISwVer);
                                            break;
                                        case 7:
                                            UsbSwVer.LRMSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.LRMSwVer : %f\n", UsbSwVer.LRMSwVer);
                                            break;
                                        case 8:
                                            UsbSwVer.PIDSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.PIDSwVer : %f\n", UsbSwVer.PIDSwVer);
                                            break;
                                        case 9:
                                            UsbSwVer.DIFSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.DIFSwVer : %f\n", UsbSwVer.DIFSwVer);
                                            break;
                                        case 10:
                                            UsbSwVer.DISSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.DISSwVer : %f\n", UsbSwVer.DISSwVer);
                                            break;
                                        case 11:
                                            UsbSwVer.NVRSwVer = (double)AtoF(strValue);
                                            printf("jhlee UsbSwVer.NVRSwVer : %f\n", UsbSwVer.NVRSwVer);
                                            break;
                                        default :
                                            printf("default jhlee\r\n");
                                            /*NULL*/
                                            break;
                                    }
                                }


                            }							                          
							
                            
                        }

                    }
                }
            }
        }
    }
#if 1
    if(pFp != NULL)
    {
        if( fclose(pFp) != 0 )
        {
            iRet = -1;
        }
    }
#endif	

    //printf(" readUSBparameter %d iRet\r\n",iRet );
    return iRet;
}


int ReadUSBFile(const char * strFileName, SUSBStatus * const pParameter ,int count)
{
    int iRet = 0;

    if( access( strFileName, R_OK ) != 0 ) {
        iRet = -1;
    }

  //  printf(" ReadUSBFile %d iRet\r\n",iRet );
    return iRet;
}

 int ReadPartitions( char * const strBuf )
{
    int iRet = -1;
    /* char *strLine = NULL; */
    char strLine[100];
    char strTokens[4][100];
    FILE    *pFp = NULL;

    pFp = fopen("/proc/partitions", "r");
    if(pFp == (FILE *)NULL)
    {
#ifdef DEBUG_LINE
    DBG_LOG("%s :: fopen error\n", __func__);
#endif
        iRet = -1;
    }
    else
    {
        while (fgets(strLine, (int)sizeof(strLine), pFp))
        {
#ifdef DEBUG_LINE
            //DBG_LOG("%s :: strLine : %s\n", __func__, strLine);
#endif
            if( BreakLine(strLine, strTokens) > 0 )
            {
                if(strncmp(strTokens[3], "sda1", 4U) == 0)
                {
                    if( iRet < AtoI(&strTokens[3][3]) )
                    {
                        iRet = AtoI(&strTokens[3][3]);
                        if( sprintf(strBuf, "/dev/%s", strTokens[3]) == 0)
                        {
#ifdef DEBUG_LINE
                            DBG_LOG("%s :: sprintf error\n", __func__);
#endif
                            iRet = -1;
                        }
                    }
                }
            }
        }

        if(fclose(pFp) != 0)
        {
#ifdef DEBUG_LINE
            DBG_LOG("%s :: fclose error\n", __func__);
#endif
            iRet = -1;
        }
    }

    return iRet;
}
#if 1

 int CheckUSBCapacity( SUSBStatus * const pParameter )
{
    int iRet = 0;
    struct statvfs FileSystem;

    iRet = statvfs("/mnt/usb",&FileSystem);

#ifdef DEBUG_LINE
    DBG_LOG("f_bsize   : %lu\n", FileSystem.f_bsize);
    DBG_LOG("f_frsize  : %lu\n", FileSystem.f_frsize);
    DBG_LOG("f_blocks  : %lu\n", FileSystem.f_blocks);
    DBG_LOG("f_bfree   : %lu\n", FileSystem.f_bfree);
    DBG_LOG("f_bavail  : %lu\n", FileSystem.f_bavail);
    DBG_LOG("f_files   : %lu\n", FileSystem.f_files);
    DBG_LOG("f_ffree   : %lu\n", FileSystem.f_ffree);
    DBG_LOG("f_favail  : %lu\n", FileSystem.f_favail);
    DBG_LOG("f_fsid    : %lu\n", FileSystem.f_fsid);
    DBG_LOG("f_flag    : %lu\n", FileSystem.f_flag);
    DBG_LOG("f_namemax : %lu\n", FileSystem.f_namemax);
#endif /* DEBUG_LINE */
    if(iRet != -1)
    {
        pParameter->llCapacity = (long long int)((long long int)FileSystem.f_bavail * (long long int)FileSystem.f_bsize);
        if( (pParameter->llCapacity))// < ( (DATA_END_ADDR-DATA_BASE_ADDR)+DATA_SIZE_10MB ) )
        {
            iRet = -1;
        }
    }
    return iRet;
}
#endif


 int CheckUSBRemove( const SUSBStatus * pParameter )
{
    int iRet = 0;
    int iReadRet = 0;
    char  strPartitionName[20];

    if( memcpy(strPartitionName,  pParameter->strUSBDevName, 20U) != NULL)
    {
        iReadRet = ReadPartitions(strPartitionName);

        if(iReadRet != -1)
        {
            iRet = -1;
        }
    }
    else
    {
        iRet = -1;
    }

    return iRet;
}


/* PIB, SCAM, FCAM ,OCAM, COP, PAMP, PEI, LRM, PID, DIF, DIS, NVR sw version */
 int DetectUSB( SUSBStatus * const pParameter )
{
    int iRet = 0;
	int count = 0;
    int iRetCount = 0;


    if(memset( pParameter->strUSBDevName, 0x00, sizeof(pParameter->strUSBDevName)) != 0)
    {
        iRet = ReadPartitions( pParameter->strUSBDevName);
    }
    else
    {
        iRet = -1;
    }

    if(iRet != -1)
    {
#ifdef DEBUG_LINE
 //       DBG_LOG("Found! %s\n",  pParameter->strUSBDevName);
#endif /* DEBUG_LINE */        
    }

    if(iRet != -1)
    {
        iRet = -1;
        for(count =0;count <DEVICE_COUNT;count++) {
            iRetCount = ReadUSBFile( &deviceFileName[count][0], pParameter,count );
                gValidFileBin[count] = iRetCount;
         //       printf("gvalgValidFileBin %d \r\n",gValidFileBin[count]);
            if(iRetCount == 0) {
                iRet = 0;
            }
        }

    }

    if(iRet != -1)
    {        
        iRet = -1;
		for(count =0;count <DEVICE_COUNT;count++) {			
            iRetCount = ReadUSBParameter( &deviceVerName[count][0], pParameter,count );            
            if(iRetCount == 0) {
                iRet = 0;
            }
		}        

    }




    return  iRet;
}


int run_ftp_upload_execlp(char* AVC_IP, char* Update_filename)
{
    pid_t childpid;

    printf("AVCIP %s\r\n",AVC_IP);
    printf("Update_filename %s\r\n",Update_filename);
    childpid = fork();

    if (childpid == -1) {
        printf("Failed to fork for running tar command\n");
        return -1;
    }

    if (childpid == 0) {
        printf("\n[ RUN NCFTPPUT ]\n");
        execlp("ncftpput", "ncftpput","-u", "wbpa","-p","dowon1222",AVC_IP,"/sw_update", Update_filename,NULL);
        printf("Failed ncftpput command\n");
        return -1;
    }

    if (childpid != wait(NULL)) {
        printf("Parent failed to wait due to signal or error while running tar command\n");
        return -1;
    }

    return 0;
}

// //execlp("ncftpput", "ncftpput","-u", "wbpa","-p","dowon1222",AVC_IP,"/sw_update", Update_filename);
int run_ftp_all_mc1_upload_execlp(void)
{
    pid_t childpid;

    childpid = fork();

    if (childpid == -1) {
        printf("Failed to fork for running tar command\n");
        return -1;
    }
    ///media/usb0/sw_update ,,, /tmp/sw_update/
    if (childpid == 0) {
        printf("\n[ COPY USB to TMP ]\n");
        execlp("ncftpput", "ncftpput","-R","-u", "wbpa","-p","dowon1222","10.128.9.33","/", "/media/usb0/sw_update/", NULL);
        printf("Failed running tar command\n");
        return -1;
    }

    if (childpid != wait(NULL)) {
        printf("Parent failed to wait due to signal or error while running tar command\n");
        return -1;
    }

    return 0;
}

int run_ftp_all_mc2_upload_execlp(void)
{
    pid_t childpid;

    childpid = fork();

    if (childpid == -1) {
        printf("Failed to fork for running tar command\n");
        return -1;
    }

    if (childpid == 0) {
        printf("\n[ COPY USB to TMP ]\n");
        execlp("ncftpput", "ncftpput","-R","-u", "wbpa","-p","dowon1222","10.128.12.33","/", "/media/usb0/sw_update/", NULL);
        printf("Failed running tar command\n");
        return -1;
    }

    if (childpid != wait(NULL)) {
        printf("Parent failed to wait due to signal or error while running tar command\n");
        return -1;
    }

    return 0;
}

int copy_usb2tmp(void)
{ 
	pid_t childpid; 

	childpid = fork(); 

	if (childpid == -1) { 
		printf("Failed to fork for running tar command\n"); 
		return -1; 
	}   

	if (childpid == 0) { 
		printf("\n[ COPY USB to TMP ]\n"); 
		execlp("cp", "cp","-rf", "/media/usb0/sw_update","/tmp/", NULL);  
		printf("Failed running tar command\n"); 
		return -1; 
	}   

	if (childpid != wait(NULL)) { 
		printf("Parent failed to wait due to signal or error while running tar command\n"); 
		return -1; 
	}   

	return 0;  
} 



