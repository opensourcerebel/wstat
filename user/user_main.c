/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2016 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "pass_router.h"
#include "ip_addr.h"
#include "espconn.h"

#include "user_interface.h"

#define DEV_220

#ifdef DEV_152
#define IP_SUFFIX 152 //stadalone module3, snadalone 220
#define HOSTNAMEST "espstationbat" //stadalone module3
#define FILE_RESULTS "results152.txt"
#define PRINT_TIMINGS false
#endif

#ifdef DEV_126
#define IP_SUFFIX 126 //stadalone module3, snadalone 220
#define HOSTNAMEST "espstationgreen" //stadalone module3
#define FILE_RESULTS "results126.txt"
#define PRINT_TIMINGS true
#endif

#ifdef DEV_220
#define IP_SUFFIX 220 //stadalone module3, snadalone 220
#define HOSTNAMEST "espstationtest" //stadalone module3
#define FILE_RESULTS "results220.txt"
#define PRINT_TIMINGS true
#endif

#define DO_NOT_REPEAT_T 0
#define NO_ARG  NULL

//struct espconn dweet_conn;
static struct espconn nwconn;
static esp_tcp conntcp;
static struct ip_info ipconfig;
LOCAL os_timer_t nw_close_timer;
LOCAL os_timer_t fake_weather_timer;
LOCAL os_timer_t wait3sec;
#define DBG os_printf
static uint32_t wakeup_start;
static uint32_t wifi_connect_start;
static uint32_t wifi_disconnect_start;
static uint32_t http_start;
static uint32_t wifiConnectionDuration;
static uint32_t currentSendingDuration;
static uint32_t currentDisconnectDuration;
static uint32_t currentTotalDuration;
static uint32_t resetReason;

struct {
  uint32_t previousSendDuration;
  uint32_t previousTotalDuration;
  uint32_t previousDisconnectDuration;
  uint32_t counterIterations;
  uint32_t workingMode;
  float h;
  float t;//TODO: test with - temp
  float p;
  uint32_t weatherReadingDuration;
  uint32_t originalResetReason;
  //uint32_t data[RTCMEMORYLEN*4];
} rtcData;

// ip_addr_t dweet_ip;
// esp_tcp dweet_tcp;

// char dweet_host[] = "192.168.1.107:8000";
// char dweet_path[] = "/test";
//char json_data[ 256 ];
//char buffer[ 2048 ];

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}



void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
}



// void data_received( void *arg, char *pdata, unsigned short len )
// {
//     struct espconn *conn = arg;
//     
//     os_printf( "%s: %s\n", __FUNCTION__, pdata );
//     
//     espconn_disconnect( conn );
// }
// 
// 
// void tcp_connected( void *arg )
// {
//     int temperature = 55;   // test data
//     struct espconn *conn = arg;
//     
//     os_printf( "%s\n", __FUNCTION__ );
//     espconn_regist_recvcb( conn, data_received );
// 
//     os_sprintf( json_data, "{\"temperature\": \"%d\" }", temperature );
//     os_sprintf( buffer, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", 
//                          dweet_path, dweet_host, os_strlen( json_data ), json_data );
//     
//     os_printf( "Sending: %s\n", buffer );
//     espconn_sent( conn, buffer, os_strlen( buffer ) );
// }
// 
// void tcp_disconnected( void *arg )
// {
//     struct espconn *conn = arg;
//     
//     os_printf( "%s\n", __FUNCTION__ );
//     wifi_station_disconnect();
// }
// 
// 
// void dns_done( const char *name, ip_addr_t *ipaddr, void *arg )
// {
//     struct espconn *conn = arg;
//     
//     os_printf( "%s\n", __FUNCTION__ );
//     
//     if ( ipaddr == NULL) 
//     {
//         os_printf("DNS lookup failed\n");
//         wifi_station_disconnect();
//     }
//     else
//     {
//         os_printf("Connecting...\n" );
//         
//         conn->type = ESPCONN_TCP;
//         conn->state = ESPCONN_NONE;
//         conn->proto.tcp=&dweet_tcp;
//         conn->proto.tcp->local_port = espconn_port();
//         conn->proto.tcp->remote_port = 80;
//         os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );
// 
//         espconn_regist_connectcb( conn, tcp_connected );
//         espconn_regist_disconcb( conn, tcp_disconnected );
//         
//         espconn_connect( conn );
//     }
// }

LOCAL void ICACHE_FLASH_ATTR
nw_close_cb(void * arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;
    DBG("nw_close_cb\n");
    espconn_disconnect(p_nwconn);
}

/*
 * Data sent callback. 
 * Nothing to do...
 */
LOCAL void ICACHE_FLASH_ATTR
nw_sent_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_sent_cb\n");
}

/*
 * Data recvieved callback. 
 */
LOCAL void ICACHE_FLASH_ATTR
nw_recv_cb(void *arg, char *data, unsigned short len)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_recv_cb len=%u\n", len);
    DBG("Received: %s\n", data );
    // Start a timer to close the connection
    os_timer_disarm(&nw_close_timer);
    os_timer_setfn(&nw_close_timer, nw_close_cb, arg);
    os_timer_arm(&nw_close_timer, 100, 0);
}

// static char* ftoa(float num, uint8_t decimals) {
//   // float to string; no float support in esp8266 sdk printf
//   // warning: limited to 15 chars & non-reentrant
//   // e.g., dont use more than once per os_printf call
//   static char* buf[16];
//   int whole = num;
//   int decimal = (num - whole) * power(10, decimals);
//   if (decimal < 0) {
//     // get rid of sign on decimal portion
//     decimal -= 2 * decimal;
//   }
//   char* pattern[10]; // setup printf pattern for decimal portion
//   os_sprintf(pattern, "%%d.%%0%dd", decimals);
//   os_sprintf(buf, pattern, whole, decimal);
//   return (char *)buf;
// }

/*
 * Connect callback. 
 * At this point, the connection to the remote server is established
 * Time to send some data
 */
#define THINGSPEAK_API_KEY "xx"
LOCAL void ICACHE_FLASH_ATTR
nw_connect_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;
    static char data[512];
    static char url[512];
//     static char json[256];

    DBG("nw_connect_cb\n");

    espconn_regist_recvcb(p_nwconn, nw_recv_cb);
    espconn_regist_sentcb(p_nwconn, nw_sent_cb);
    uint16_t voltage = system_get_vdd33();
    DBG("volt%d\n", voltage);
    
//     os_sprintf( json, "{\"temperature\": \"%d\" }", 55 );
//     os_sprintf( data, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", 
//                          "/test", "192.168.1.107", os_strlen( json ), json );
    
    os_sprintf( url, "/iot/gate?file=%s&1=%s&2=%s&3=%s&4=%d&5=%d&6=%d&7=%d&8=%d&9=%d&10=%d&11=%d&12=%d", FILE_RESULTS, "999", "999", "999", voltage, wifiConnectionDuration, rtcData.weatherReadingDuration, rtcData.previousSendDuration,
                rtcData.previousTotalDuration, rtcData.previousDisconnectDuration, rtcData.counterIterations, rtcData.originalResetReason, resetReason);
    
    os_sprintf( data, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", 
                         url, "192.168.1.107", os_strlen( "" ), "" );
                
    
    DBG("Sending: %s\n", data );
    espconn_sent(p_nwconn, data, os_strlen(data));
}

/*
 * Re-Connect callback. 
 * Do nothing?
 */
LOCAL void ICACHE_FLASH_ATTR
nw_reconnect_cb(void *arg, int8_t errno)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_reconnect_cb errno=%d, is server running?\n", errno);
}

/*
 * Dis-Connect callback. 
 * Do nothing?
 */
LOCAL void ICACHE_FLASH_ATTR
nw_disconnect_cb(void *arg)
{
    currentSendingDuration = system_get_time() - http_start;
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_disconnect_cb\n");
    wifi_disconnect_start = system_get_time();
    wifi_station_disconnect();
    
}

// EVENT_STAMODE_CONNECTED 0
// EVENT_STAMODE_DISCONNECTED 1
// EVENT_STAMODE_AUTHMODE_CHANGE 2
// EVENT_STAMODE_GOT_IP 3
// EVENT_STAMODE_DHCP_TIMEOUT 4
// EVENT_SOFTAPMODE_STACONNECTED 5
// EVENT_SOFTAPMODE_STADISCONNECTED 6
// EVENT_SOFTAPMODE_PROBEREQRECVED 7
// EVENT_OPMODE_CHANGED 8
// EVENT_MAX 9



//#define SLEEP_TIME 500
#define SLEEP_TIME 5000
#define WAKE_WITH_WIFI_AND_DEF_CAL 0
#define WAKE_WITHOUT_WIFI 4
#define RTCMEMORYSTART 64
#define MODE_SEND 100
#define MODE_READ 200
void wifi_callback( System_Event_t *evt )
{
    os_printf( "here: %s: %d\n", __FUNCTION__, evt->event );
    
    switch ( evt->event )
    {
        /*0*/ case EVENT_STAMODE_CONNECTED:
        {
            os_printf("EVENT_STAMODE_CONNECTED connect to ssid %s, channel %d\n",
                        evt->event_info.connected.ssid,
                        evt->event_info.connected.channel);
            break;
        }

        case EVENT_STAMODE_DISCONNECTED:
        {
            os_printf("EVENT_STAMODE_DISCONNECTED disconnect from ssid %s, reason %d\n",
                        evt->event_info.disconnected.ssid,
                        evt->event_info.disconnected.reason);
            
            uint32_t wakup_end = system_get_time();
            currentTotalDuration = wakup_end - wakeup_start;
            currentDisconnectDuration = wakup_end - wifi_disconnect_start;
            //setup for the next wake
            rtcData.workingMode = MODE_READ;
            rtcData.previousSendDuration = currentSendingDuration;
            rtcData.previousTotalDuration = currentTotalDuration;
            rtcData.previousDisconnectDuration = currentDisconnectDuration;
            rtcData.counterIterations = rtcData.counterIterations + 1;
            
            deep_sleep_set_option( WAKE_WITH_WIFI_AND_DEF_CAL );//TODO: count before rf cal!!!
            
            uint32_t wakup_end2 = system_get_time();
            os_printf("e2eWi1 %d", wakup_end - wakeup_start);
            os_printf("e2eWi2 %d", wakup_end2 - wakup_end);
            system_deep_sleep( SLEEP_TIME * 1000 ); 
            break;
        }

        /*3*/case EVENT_STAMODE_GOT_IP:
        {
            uint32_t wifi_end = system_get_time();
            wifiConnectionDuration = wifi_end - wifi_connect_start;
            os_printf("EVENT_STAMODE_GOT_IP ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR,
                        IP2STR(&evt->event_info.got_ip.ip),
                        IP2STR(&evt->event_info.got_ip.mask),
                        IP2STR(&evt->event_info.got_ip.gw));
            os_printf("\n");
            
            //TODO: change here to direct IP without DNS!
            //espconn_gethostbyname( &dweet_conn, dweet_host, &dweet_ip, dns_done );
            
            const char thingspeak_ip[4] = {192, 168, 1, 107};
            nwconn.type = ESPCONN_TCP;
            nwconn.proto.tcp = &conntcp;
            os_memcpy(conntcp.remote_ip, thingspeak_ip, 4);
            conntcp.remote_port = 8000;
            conntcp.local_port = espconn_port();
            espconn_regist_connectcb(&nwconn, nw_connect_cb);
            espconn_regist_disconcb(&nwconn, nw_disconnect_cb);
            espconn_regist_reconcb(&nwconn, nw_reconnect_cb);
            
            http_start = system_get_time();
            espconn_connect(&nwconn);
            
            break;
        }
        
        default:
        {
            break;
        }
    }
}

LOCAL void ICACHE_FLASH_ATTR
fillPreviousSendDuration()
{
  system_rtc_mem_read(RTCMEMORYSTART, &rtcData, sizeof(rtcData));

  if (rtcData.originalResetReason != 5)
  {
    //reset the counter on unexpected event
    rtcData.counterIterations = 1;
  }

  if (rtcData.workingMode != MODE_READ && rtcData.workingMode != MODE_SEND)
  {
    os_printf("Reseting mode %d to read\n", rtcData.workingMode);
    rtcData.workingMode = MODE_READ;
  }
  
  os_printf("Working mode %d\n", rtcData.workingMode);
}

// enum	rst_reason	{
// 	 REANSON_DEFAULT_RST	 =	0,	 //	normal	startup	by	power	on
// 	 REANSON_WDT_RST	=	1,	 //	hardware	watch	dog	reset	exception	reset,	GPIO	status	won’t	change		
// 	 REANSON_EXCEPTION_RST	 =	2,	 	 //	software	watch	dog	reset,	GPIO	status	won’t	change		
// 	 REANSON_SOFT_WDT_RST	 =	3,	 	 //	software	restart	,system_restart	,	GPIO	status	won’t change
// 	 REANSON_SOFT_RESTART	 =	4,	 	
// 	 REANSON_DEEP_SLEEP_AWAKE=	5,				//	wake	up	from	deep-sleep	
// 	 REANSON_EXT_SYS_RST	 =	6,	 //	external	system	reset
// };


LOCAL void ICACHE_FLASH_ATTR
readComplete(void * arg)
{    
    rtcData.workingMode = MODE_SEND;
    rtcData.originalResetReason = resetReason;
    system_rtc_mem_write(RTCMEMORYSTART, &rtcData, sizeof(rtcData));
    
    deep_sleep_set_option( WAKE_WITHOUT_WIFI );
    uint32_t wakup_end = system_get_time();
    os_printf("e2eWe %d", wakup_end - wakeup_start);
    system_deep_sleep( 1 ); 
}

#define WEATHER_READ_IN_MS 110
LOCAL void ICACHE_FLASH_ATTR
readData(void)
{
    os_timer_disarm(&fake_weather_timer);
    os_timer_setfn(&fake_weather_timer, readComplete,  NO_ARG);
    os_timer_arm(&fake_weather_timer, WEATHER_READ_IN_MS, DO_NOT_REPEAT_T);
}

LOCAL void ICACHE_FLASH_ATTR
work(void)
{
    os_printf( "+working\n" );
    static struct station_config config;
    
    //uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    //os_printf( "%s\n", __FUNCTION__ );
    fillPreviousSendDuration();
    
    if (rtcData.workingMode == MODE_SEND)
    { 
        wifi_connect_start = system_get_time();
        
        wifi_station_set_hostname( "TestSleep" );
        wifi_set_opmode_current( STATION_MODE );

        gpio_init();
        
        config.bssid_set = 0;
        os_memcpy( &config.ssid, ROUTER_NAME, 32 );
        os_memcpy( &config.password, ROUTER_PASS, 64 );
        wifi_station_set_config( &config );
        
        wifi_set_event_handler_cb( wifi_callback );
    }
    else
    {
        readData();        
    }
}

LOCAL void ICACHE_FLASH_ATTR
initialWait(void * arg)
{
    os_printf( "\n+wait over+\n" );
    work();
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    
    wakeup_start = system_get_time();
    struct rst_info *rtc_info = system_get_rst_info();
    resetReason = rtc_info->reason;
    os_printf("Reset %d\n", resetReason);
    
    if(resetReason == 6)
    {
        os_printf( "\n+wait\n" );
        os_timer_disarm(&wait3sec);
        os_timer_setfn(&wait3sec, initialWait,  NO_ARG);
        os_timer_arm(&wait3sec, 3000, DO_NOT_REPEAT_T);
    }
    //rtc_info->exccause;
   
    else{
        os_printf( "\n+run\n" );
        work();
    }
}

