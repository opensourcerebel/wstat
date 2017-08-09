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

//struct espconn dweet_conn;
static struct espconn nwconn;
static esp_tcp conntcp;
static struct ip_info ipconfig;
LOCAL os_timer_t nw_close_timer;
#define DBG os_printf

ip_addr_t dweet_ip;
esp_tcp dweet_tcp;

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
    static char data[256];
    static char json[256];

    uint32_t systime = system_get_time()/1000000;
    DBG("nw_connect_cb\n");

    espconn_regist_recvcb(p_nwconn, nw_recv_cb);
    espconn_regist_sentcb(p_nwconn, nw_sent_cb);
    
    os_sprintf( json, "{\"temperature\": \"%d\" }", 55 );
    os_sprintf( data, "POST %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s", 
                         "/test", "192.168.1.107", os_strlen( json ), json );
    
    DBG("Sending: %s\n", data );
    DBG("system_get_time() returns %lu\n", systime);
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

    DBG("nw_reconnect_cb errno=%d\n", errno);
}

/*
 * Dis-Connect callback. 
 * Do nothing?
 */
LOCAL void ICACHE_FLASH_ATTR
nw_disconnect_cb(void *arg)
{
    struct espconn *p_nwconn = (struct espconn *)arg;

    DBG("nw_disconnect_cb\n");
    wifi_station_disconnect();
    
}

//EVENT_STAMODE_CONNECTED - The ESP8266 has connected to the WiFi network as a station
//EVENT_STAMODE_DISCONNECTED - The ESP8266 has been disconnected from the WiFi network
//EVENT_STAMODE_AUTHMODE_CHANGE - The ESP8266 has a change in the authorisation mode
//EVENT_STAMODE_GOT_IP - The ESP8266 has an assigned IP address
//EVENT_STAMODE_DHCP_TIMEOUT - There was a timeout while trying to get an IP address via DHCP

void wifi_callback( System_Event_t *evt )
{
    os_printf( "here: %s: %d\n", __FUNCTION__, evt->event );
    
    switch ( evt->event )
    {
        case EVENT_STAMODE_CONNECTED:
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
            
            deep_sleep_set_option( 0 );
            system_deep_sleep( 5 * 1000 * 1000 );  // 60 seconds
            break;
        }

        case EVENT_STAMODE_GOT_IP:
        {
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
            espconn_connect(&nwconn);
            
            break;
        }
        
        default:
        {
            break;
        }
    }
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
    os_printf("Server My SDK version:%s\n", system_get_sdk_version());
    
    static struct station_config config;
    
    //uart_div_modify( 0, UART_CLK_FREQ / ( 115200 ) );
    os_printf( "%s\n", __FUNCTION__ );

    wifi_station_set_hostname( "TestSleep" );
    wifi_set_opmode_current( STATION_MODE );

    gpio_init();
    
    config.bssid_set = 0;
    os_memcpy( &config.ssid, ROUTER_NAME, 32 );
    os_memcpy( &config.password, ROUTER_PASS, 64 );
    wifi_station_set_config( &config );
    
    wifi_set_event_handler_cb( wifi_callback );
}

