#include "ets_sys.h"
#include "osapi.h"
#include "pass_router.h"
#include "const.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "i2c_chirp.h"

#include "user_interface.h"


static struct espconn nwconn;
static esp_tcp conntcp;
static struct ip_info ipconfig;
LOCAL os_timer_t nw_close_timer;
LOCAL os_timer_t fake_weather_timer;
LOCAL os_timer_t wait3sec;
LOCAL os_timer_t totalTimeLimitTmr;
LOCAL os_timer_t sleepAfterSendTmr;
LOCAL os_timer_t sleepAfterReadTmr;
LOCAL os_timer_t sendDataTmr;

static uint32_t wakeup_start;
static uint32_t wifi_connect_start;
static uint32_t wifi_disconnect_start;
static uint32_t http_start;
static uint32_t wifiConnectionDuration;
static uint32_t currentSendingDuration;
static uint32_t currentDisconnectDuration;
static uint32_t currentTotalDuration;
static uint32_t resetReason;
static uint32_t currentWorkingMode;

struct {
  uint32_t previousSendDuration;
  uint32_t previousTotalDuration;
  uint32_t previousDisconnectDuration;
  uint32_t counterIterations;
  uint32_t counterCancelledIterations;
  uint32_t workingMode;
  uint32_t h;
  int32_t t;
  uint32_t p;
  uint32_t weatherReadingDuration;
  uint32_t originalResetReason;
  bool bmeInitOk;
  //uint32_t data[RTCMEMORYLEN*4];
} rtcData;


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

static void uart_ignore_char(char c)
{
    (void) c;
}

void ICACHE_FLASH_ATTR
user_rf_pre_init(void)
{
    struct rst_info *rtc_info = system_get_rst_info();
    resetReason = rtc_info->reason;
    
    wifi_connect_start = system_get_time(); 
    if(resetReason == HARDARE_RESET)
    {
        //DBG("!FC");
        system_phy_set_powerup_option(FULL_CALIBRATION);
    }
    else
    {
        //DBG("!MC");
        system_phy_set_powerup_option(MIN_CALIBRATION);
    }
    system_phy_set_max_tpw(82); // 82: max TX power
    
 #ifdef PROD_COMPILE
//     system_uart_swap();
     system_set_os_print(0);
     ets_install_putc1((void *) &uart_ignore_char);
 #endif
}

LOCAL void ICACHE_FLASH_ATTR nw_connect_cb(void *arg);
LOCAL void ICACHE_FLASH_ATTR nw_reconnect_cb(void *arg, int8_t errno);
LOCAL void ICACHE_FLASH_ATTR nw_disconnect_cb(void *arg);
LOCAL void ICACHE_FLASH_ATTR sleepAfterSend();

static bool ipok = 0;
static bool waitok = 0;
void sendDataGated()
{
    DBG("sendData: %d, %d\n", ipok, waitok);
    if(ipok && waitok)
    {
        uint8 stst = wifi_station_get_connect_status();
        DBG("wifi status %d \n", stst);
        
        const char hostip[4] = HOST_IP;
        nwconn.type = ESPCONN_TCP;
        nwconn.proto.tcp = &conntcp;
        os_memcpy(conntcp.remote_ip, hostip, 4);
#ifdef SEND_SECURE        
        conntcp.remote_port = 443;
#else
        conntcp.remote_port = 8000;
#endif        
        conntcp.local_port = espconn_port();
        
        
        espconn_regist_connectcb(&nwconn, nw_connect_cb);
        espconn_regist_disconcb(&nwconn, nw_disconnect_cb);
        espconn_regist_reconcb(&nwconn, nw_reconnect_cb);
        
        http_start = system_get_time();
        
#ifdef SEND_SECURE
        espconn_secure_set_size(ESPCONN_CLIENT,8192); 
        sint8 conStatus = espconn_secure_connect(&nwconn);        
        DBG("Con secure status %d \n", conStatus);
        if(conStatus == -16)
        {
            espconn_secure_disconnect(&nwconn);
            
            os_timer_disarm(&sendDataTmr);
            os_timer_setfn(&sendDataTmr, sendDataGated,  NO_ARG);
            os_timer_arm(&sendDataTmr, 100, DO_NOT_REPEAT_T); 
        }
#else
        sint8 conStatus = espconn_connect(&nwconn);        
        DBG("Con status %d \n", conStatus);
        if(conStatus != 0)
        {
            rtcData.counterCancelledIterations++;
            sleepAfterSend();
        }
#endif
        
    }
}


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
    //struct espconn *p_nwconn = (struct espconn *)arg;
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
    os_timer_arm(&nw_close_timer, 1, 0);
}

/*
 * Connect callback. 
 * At this point, the connection to the remote server is established
 * Time to send some data
 */
LOCAL void ICACHE_FLASH_ATTR
nw_connect_cb(void *arg)
{
    DBG("nw_connect_cb\n");
    
    struct espconn *p_nwconn = (struct espconn *)arg;
    
    char *data = (char *)os_zalloc(packet_size);    
    char *url = (char *)os_zalloc(packet_size);

    espconn_regist_recvcb(p_nwconn, nw_recv_cb);
    espconn_regist_sentcb(p_nwconn, nw_sent_cb);
    uint16_t voltage = system_get_vdd33() + CORRECTION;
    DBG("volt%d\n", voltage);
    
    os_sprintf( url, "/iot/gate?file=%s&1=%d&2=%d&3=%d&4=%d&5=%d&6=%d&7=%d&8=%d&9=%d&10=%d&11=%d&12=%d&13=%d", 
                FILE_RESULTS, rtcData.t, rtcData.h, rtcData.p, voltage, wifiConnectionDuration, rtcData.weatherReadingDuration, rtcData.previousSendDuration,
                rtcData.previousTotalDuration, rtcData.previousDisconnectDuration, rtcData.counterIterations, rtcData.counterCancelledIterations, rtcData.originalResetReason, resetReason);    
    
    os_sprintf( data, "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: text/html\r\nContent-Length: %d\r\n\r\n%s", url, HOST_NAME, os_strlen( "" ), "" );
                
    
    DBG("Sending: %s\n", data );
#ifdef SEND_SECURE
    sint8 sentResult = espconn_secure_send(p_nwconn, data, os_strlen(data));
#else    
    sint8 sentResult = espconn_send(p_nwconn, data, os_strlen(data));
#endif
    
    os_free(data);
    os_free(url);
    
    if(sentResult != 0)
    {
        rtcData.counterCancelledIterations++;
        sleepAfterSend();
    }
}

LOCAL void ICACHE_FLASH_ATTR
actualSleepAfterSend()
{
    DBG("sleepAfterSend\n");
    uint32_t wakup_end = system_get_time();
    currentTotalDuration = wakup_end - wakeup_start;
    currentDisconnectDuration = wakup_end - wifi_disconnect_start;
    //setup for the next wake
    rtcData.workingMode = MODE_READ;
    rtcData.previousSendDuration = currentSendingDuration;
    rtcData.previousTotalDuration = currentTotalDuration;
    rtcData.previousDisconnectDuration = currentDisconnectDuration;
    rtcData.counterIterations = rtcData.counterIterations + 1;
    system_rtc_mem_write(RTCMEMORYSTART, &rtcData, sizeof(rtcData));
    
    uint32_t wakup_end2 = system_get_time();
    DBG_TIME("e2eWiAttach %d ms\n", wifiConnectionDuration/1000);            
    DBG_TIME("e2eWiSend %d ms\n", currentSendingDuration/1000);
    DBG_TIME("e2eWiDisc+Write %d ms\n", currentDisconnectDuration/1000);
    DBG_TIME("e2eWiTot %d ms\n", currentTotalDuration/1000);
    DBG_TIME("e2eWiWrite %d us\n", wakup_end2 - wakup_end);
    
    deep_sleep_set_option( WAKE_WITHOUT_WIFI );
    uint64 desiredSleep = SLEEP_TIME * 1000;
    if(desiredSleep + SLEEP_TIME_MIN > currentTotalDuration)
    {
        //lower the sleep time
        DEEP_SLEEP(desiredSleep - currentTotalDuration); 
    }
    else
    {
        //do not sleep as the current run took too much time
        DEEP_SLEEP(SLEEP_TIME_MIN); 
    }
}

LOCAL void ICACHE_FLASH_ATTR
sleepAfterSend()
{
    os_timer_disarm(&sleepAfterSendTmr);
    os_timer_setfn(&sleepAfterSendTmr, actualSleepAfterSend,  NO_ARG);
    os_timer_arm(&sleepAfterSendTmr, 1, DO_NOT_REPEAT_T);
}

LOCAL void ICACHE_FLASH_ATTR
nw_reconnect_cb(void *arg, int8_t errno)
{
    DBG("nw_reconnect_cb errno=%d, is server running, retry?\n", errno);
    sleepAfterSend();
}

LOCAL void ICACHE_FLASH_ATTR
nw_disconnect_cb(void *arg)
{
    currentSendingDuration = system_get_time() - http_start;
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_disconnect_cb\n");
    wifi_disconnect_start = system_get_time();
    //wifi_station_disconnect();    
    sleepAfterSend();
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
void wifi_callback( System_Event_t *evt )
{
    DBG( "here: %s: %d\n", __FUNCTION__, evt->event );
    
    switch ( evt->event )
    {
        /*0*/ case EVENT_STAMODE_CONNECTED:
        {
            DBG("EVENT_STAMODE_CONNECTED\n");
            break;
        }

        /*1*/ case EVENT_STAMODE_DISCONNECTED:
        {
            DBG("EVENT_STAMODE_DISCONNECTED\n");
//             if(currentWorkingMode == MODE_SEND)
//             {
//                 sleepAfterSend();
//             }
            break;
        }

        /*3*/ case EVENT_STAMODE_GOT_IP:
        
        {
            uint32_t wifi_end = system_get_time();

            wifiConnectionDuration = wifi_end - wifi_connect_start;
            DBG("EVENT_STAMODE_GOT_IP %d\n", wifiConnectionDuration / 1000);
            ipok = 1;
            os_timer_disarm(&sendDataTmr);
            os_timer_setfn(&sendDataTmr, sendDataGated,  NO_ARG);
            os_timer_arm(&sendDataTmr, 1, DO_NOT_REPEAT_T);
            
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

  if (resetReason != WAKE_FROM_DEEP_SLEEP)
  {
    DBG("Reset rtc data\n");
    //reset the counter on initial reset
    rtcData.counterIterations = 1;
    rtcData.counterCancelledIterations = 0;
    
    rtcData.previousSendDuration = 0;
    rtcData.previousTotalDuration = 0;
    rtcData.previousDisconnectDuration = 0;
    rtcData.workingMode = MODE_READ;
    rtcData.h = 0;
    rtcData.t = 0;
    rtcData.p = 0;
    rtcData.weatherReadingDuration = 0;
    rtcData.originalResetReason = 0;    
  }
  currentWorkingMode = rtcData.workingMode;
  
  DBG("Working mode %d\n", rtcData.workingMode);
}

LOCAL void ICACHE_FLASH_ATTR
readDataActual()
{   
    if(resetReason == WAKE_FROM_DEEP_SLEEP)
    {
        if(rtcData.bmeInitOk)
        {
            CHIRP_InitFromSleep();
        }
    }
    else
    {
        rtcData.bmeInitOk = CHIRP_Init();
    }
    
    if(rtcData.bmeInitOk)
    {
        rtcData.h = CHIRP_GetHumidityRaw();
        rtcData.t = CHIRP_GetTemperatureRaw();
        //rtcData.p = CHIRP_GetLightRaw();
        CHIRP_Sleep();

        CHIRP_Sleep();

        DBG("+++Temp: %d \r\n", rtcData.t);
        DBG("+++Hum: %d \r\n",rtcData.h);
        //         DBG("+++Light: %d\r\n", rtcData.p);
    }
    else
    {
        DBG("CHIRP init error.\r\n");
    }
  
    uint32_t wakup_end = system_get_time();
    rtcData.workingMode = MODE_READ;
    rtcData.originalResetReason = resetReason;   
    rtcData.weatherReadingDuration = wakup_end - wakeup_start;

    system_rtc_mem_write(RTCMEMORYSTART, &rtcData, sizeof(rtcData));
    DBG_TIME("e2eWe %d\n", rtcData.weatherReadingDuration / 1000);
    
    deep_sleep_set_option( DESIRED_WIFI_WAKE_MODE );
    DEEP_SLEEP(SLEEP_TIME_MIN); 
}

#define WEATHER_READ_IN_MS 110
LOCAL void ICACHE_FLASH_ATTR
readData(void)
{
    os_timer_disarm(&sleepAfterReadTmr);
    os_timer_setfn(&sleepAfterReadTmr, readDataActual,  NO_ARG);
    os_timer_arm(&sleepAfterReadTmr, 1, DO_NOT_REPEAT_T);
}

LOCAL void ICACHE_FLASH_ATTR 
work(void)
{
    //uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    fillPreviousSendDuration();    
    
    if (rtcData.workingMode == MODE_SEND)
    { 
        waitok = 1;
        os_timer_disarm(&sendDataTmr);
        os_timer_setfn(&sendDataTmr, sendDataGated,  NO_ARG);
        os_timer_arm(&sendDataTmr, 1, DO_NOT_REPEAT_T);
    }
    else
    {
        readData();        
    }
}

LOCAL void ICACHE_FLASH_ATTR
initialWait(void * arg)
{
    DBG( "\n+Initial wait over+\n" );
    work();
}

ICACHE_FLASH_ATTR
bool sta_config_differ(struct station_config * lhs, struct station_config * rhs) 
{
    if(strcmp(lhs->ssid, rhs->ssid) != 0) {
        return true;
    }

    if(strcmp(lhs->password, rhs->password) != 0) {
        return true;
    }

    if(lhs->bssid_set != rhs->bssid_set) {
        return true;
    }

    if(lhs->bssid_set) {
        if(memcmp(lhs->bssid, rhs->bssid, 6) != 0) {
            return true;
        }
    }

    return false;
}

LOCAL void ICACHE_FLASH_ATTR
system_operational(void)
{
    DBG("Reset %d time[%d]\n", resetReason, ((system_get_time() - wakeup_start) / 1000));
    
    if(wifi_station_get_auto_connect() != DESIRED_AUTOCONNECT)
    {
        DBG("reseting auto connect to %d, old %d\n", DESIRED_AUTOCONNECT, wifi_station_get_auto_connect());
        wifi_station_set_auto_connect(DESIRED_AUTOCONNECT);
    }
    
    struct station_config flashConfig;
    wifi_station_get_config(&flashConfig);
    if(resetReason == 6)
    {
        struct station_config config;
        os_memcpy( &config.ssid, ROUTER_NAME, 32 );        
        os_memcpy( &config.password, ROUTER_PASS, 64 );
        config.bssid_set = 0; 
        
        DBG("current ssid %s\n", flashConfig.ssid);
        DBG("current pssw %s\n", flashConfig.password);
        DBG("current bset %d\n", flashConfig.bssid_set);
        DBG("current bssd %d\n", flashConfig.bssid);
        
        if(sta_config_differ(&flashConfig, &config))
        {
            wifi_station_set_config(&config);
            wifi_station_get_config(&flashConfig);
            
            DBG("new ssid %s\n", flashConfig.ssid);
            DBG("new pssw %s\n", flashConfig.password);
            DBG("new bset %d\n", flashConfig.bssid_set);
            DBG("new bssd %d\n", flashConfig.bssid);
        }
    }
    
    if(wifi_station_dhcpc_status() != 0)
    {
        DBG("disable dhcp %d\n", wifi_station_dhcpc_status());
        wifi_station_dhcpc_stop();//disable dhcp
        struct ip_info info;
        IP4_ADDR(&info.ip,192,168,IP_3,IP_SUFFIX);
        IP4_ADDR(&info.gw,192,168,IP_3,1);
        IP4_ADDR(&info.netmask,255,255,255,0);
        wifi_set_ip_info(STATION_IF,&info);//does not write to flash
    }
    
    uint8 currentOpMode = wifi_get_opmode_default();
    
    if(currentOpMode != DESIRED_WIFI_MODE)
    {
        DBG("currentOpMode %d, change to %d\n", currentOpMode, DESIRED_WIFI_MODE);
        wifi_set_opmode(DESIRED_WIFI_MODE);
    }
    
    
    if(resetReason == HARDARE_RESET)
    {
        os_timer_disarm(&wait3sec);
        os_timer_setfn(&wait3sec, initialWait,  NO_ARG);
        os_timer_arm(&wait3sec, 1000, DO_NOT_REPEAT_T);
    }     
    else
    {
        work();
    }
    
}

void ICACHE_FLASH_ATTR
totalTimeLimit()
{
    rtcData.counterCancelledIterations++;
    sleepAfterSend();
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
<<<<<<< 3a29ce7fc62c5ad2b420f463039196f6c5159b32
#ifdef DEV_COMPILE    
    char macaddr[6];
    wifi_get_macaddr(STATION_IF, macaddr);
    DBG("::::MAC:" MACSTR "\r\n", MAC2STR(macaddr));
#endif 
=======
>>>>>>> working read
    wifi_set_event_handler_cb( wifi_callback );
    wakeup_start = system_get_time();    
    system_init_done_cb(system_operational);
    
    
#ifdef START_TOTAL_TIME_LIMIT
    if(resetReason != HARDARE_RESET)
    {
        os_timer_disarm(&totalTimeLimitTmr);
        os_timer_setfn(&totalTimeLimitTmr, totalTimeLimit,  NO_ARG);
        os_timer_arm(&totalTimeLimitTmr, 5000, DO_NOT_REPEAT_T);
    }
#endif    
}

