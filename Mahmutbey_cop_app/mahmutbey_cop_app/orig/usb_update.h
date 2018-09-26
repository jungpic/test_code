#ifndef _USB_UPDATE_H_
#define _USB_UPDATE_H_

#define USB_PAR_BASE        "/sys/bus/usb/devices/usb1/1-2"
#define MAX_SERIAL_SIZE     30
#define DATA_SIZE_10MB      (long long int)(0xA00000LL)
#define DEVICE_COUNT 		10
/* PIB, SCAM, FCAM , COP, PAMP, PEI, LRM, PID, DIF, NVR sw version */


typedef struct {        
	double PIBSwVer; 
	double SCAMSwVer; 
	double FCAMSwVer; 
	double OCAMSwVer; 
	double COPSwVer; 
	double PAMPSwVer; 
	double PEISwVer; 
	double LRMSwVer; 
	double PIDSwVer; 
	double DIFSwVer; 
	double DISSwVer;
	double NVRSwVer;     		
} __attribute__((packed)) UsbUpdateSW_t;

extern UsbUpdateSW_t 	UsbSwVer; /*sw update version usb */
extern int iDetectFlag ; /* USB insert flag*/
typedef struct
{    
    double swVersion;
	long long int llCapacity;
    char strUSBDevName[20];
} SUSBStatus;

int Authentication( const SUSBStatus * pParameter );
int Download( SUSBStatus * const pParameter );
int UpdateApp( const SUSBStatus * pParameter );
int DetectUSB( SUSBStatus * const pParameter );
int CheckUSBRemove( const SUSBStatus * pParameter );
void init_UsbSwVer(void);
int copy_usb2tmp(void);
//int run_ftp_upload_execlp(void);
int run_ftp_upload_execlp(char* AVC_IP, char* Update_filename);

//int run_ftp_all_upload_execlp(void);
int run_ftp_all_mc1_upload_execlp(void);
int run_ftp_all_mc2_upload_execlp(void);
#endif /** _USB_UPDATE_H_ */
