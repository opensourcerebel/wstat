 
#define DEV_COMPILE
#define DEV_220
//#define SLEEP_TIME 5000
#define SLEEP_TIME 5000


//#define SEND_SECURE

#ifdef DEV_COMPILE
#define DBG os_printf
#define DBG_TIME os_printf
#define DEEP_SLEEP system_deep_sleep
#endif

#ifdef PROD_COMPILE
#define DBG 
#define DBG_TIME 
#define DEEP_SLEEP system_deep_sleep_instant
#define START_TOTAL_TIME_LIMIT
#endif


#define IP_3 1

#ifdef DEV_152
#define IP_SUFFIX 152 
#define FILE_RESULTS "results152.txt"
#define CORRECTION -213 //3539-3328 //-211
#endif

#ifdef DEV_126
#define IP_SUFFIX 126 
#define FILE_RESULTS "results126.txt"
#define CORRECTION -357 //3328 2751
#endif

#ifdef DEV_210
#define IP_SUFFIX 210
#define FILE_RESULTS "results210.txt"
#define CORRECTION -216//3328 2751
#endif

#ifdef DEV_220
#define IP_SUFFIX 220
#define FILE_RESULTS "results220.txt"
#define CORRECTION -142//3328 2751
#endif

#ifdef DEV_234
#define IP_SUFFIX 234
#define FILE_RESULTS "results234.txt"
#define CORRECTION -142//3328 2751
#endif

#define DO_NOT_REPEAT_T 0
#define NO_ARG  NULL

#define DESIRED_AUTOCONNECT AUTOCONNECT_ON
#define AUTOCONNECT_ON 1
#define AUTOCONNECT_OFF 0

#define DESIRED_WIFI_MODE WIFI_MODE_STATON
#define WIFI_MODE_STATON 1
#define WIFI_MODE_SOFT_AP 2
#define WIFI_MODE_STATION_AND_SOFT_AP 3

#define RTCMEMORYSTART 64
#define MODE_SEND 100
#define MODE_READ 200

#define packet_size 2048

// RF calibration: 
// 0=do what byte 114 of esp_init_data_default says - The default value of byte 114 is 0, which has the same effect as option 2
// 1=calibrate VDD33 and TX power (18ms); 
// 2=calibrate VDD33 only (2ms); 
// 3=full calibration (200ms). 
#define MIN_CALIBRATION 2
#define FULL_CALIBRATION 3

#define DESIRED_WIFI_WAKE_MODE WAKE_WITH_WIFI_AND_DEF_CAL
#define WAKE_WITH_WIFI_AND_DEF_CAL 0
#define WAKE_WITH_WIFI_NO_CAL 2
#define WAKE_WITHOUT_WIFI 4

// enum	rst_reason	{
// 	 REANSON_DEFAULT_RST	 =	0,	 //	normal	startup	by	power	on
// 	 REANSON_WDT_RST	=	1,	 //	hardware	watch	dog	reset	exception	reset,	GPIO	status	won’t	change		
// 	 REANSON_EXCEPTION_RST	 =	2,	 	 //	software	watch	dog	reset,	GPIO	status	won’t	change		
// 	 REANSON_SOFT_WDT_RST	 =	3,	 	 //	software	restart	,system_restart	,	GPIO	status	won’t change
// 	 REANSON_SOFT_RESTART	 =	4,	 	
// 	 REANSON_DEEP_SLEEP_AWAKE=	5,				//	wake	up	from	deep-sleep	
// 	 REANSON_EXT_SYS_RST	 =	6,	 //	external	system	reset
// };
//0 Power reboot Changed
//1 Hardware WDT reset Changed
//2 Fatal exception Unchanged
//3 Software watchdog reset Unchanged
//4 Software reset Unchanged
//5 Deep-sleep Changed
//6 Hardware reset Changed
#define WAKE_FROM_DEEP_SLEEP 5
#define HARDARE_RESET 6