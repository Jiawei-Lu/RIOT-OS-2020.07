/*
 * Copyright (c) 2015-2016 Ken Bannister. All rights reserved.
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       gcoap example
 *
 * @author      Ken Bannister <kb2ma@runbox.com>
 *
 * @}
 */

#include <stdio.h>
#include "msg.h"

#include "net/gcoap.h"
#include "shell.h"

/*RPL*/
#include "net/gnrc/rpl.h"
#include "net/gnrc/rpl/structs.h"
#include "net/gnrc/rpl/dodag.h"

/*RTC*/
#include "periph_conf.h"
#include "periph/rtc.h"
// #include "periph/rtc_mem.h"

/*NETIF AND PM*/
#include "xtimer.h"

#include "periph/gpio.h"
#include "net/gnrc/netif.h"
#include "net/netif.h"
#include "net/gnrc/netapi.h"

#include "periph/pm.h"
#ifdef MODULE_PM_LAYERED
#include "pm_layered.h"
#endif
/******/

#define PERIOD              (2U)
#define REPEAT              (4U)

#define TM_YEAR_OFFSET      (1900)

gnrc_netif_t* netif = NULL;
int wakeup_gap =200;
static unsigned cnt = 0;
struct tm alarm_time;
struct tm current_time;

#define MAIN_QUEUE_SIZE (512)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

/*RTC struct defination*/
struct tm coaptime;

/*RPL*/
gnrc_ipv6_nib_ft_t entry;      
void *state = NULL;
uint8_t dst_address[] = {0};
    // uint8_t nexthop_address[] = {0};
    // int i = 0;
unsigned iface = 0U;
/* import "ifconfig" shell command, used for printing addresses */
extern int _gnrc_netif_config(int argc, char **argv);

extern int gcoap_cli_cmd(int argc, char **argv);
extern void gcoap_cli_init(void);

static const shell_command_t shell_commands[] = {
    { "coap", "CoAP example", gcoap_cli_cmd },
    { NULL, NULL, NULL }
};


// /*RTC */
// #ifdef MODULE_PERIPH_RTC_MEM
// static const uint8_t riot_msg_offset = 1;
// static const char riot_msg[] = "RIOT";
// static void _set_rtc_mem(void)
// {
//     /* first fill the whole memory */
//     uint8_t size = rtc_mem_size();
//     while (size--) {
//         rtc_mem_write(size, &size, sizeof(size));
//     }

//     /* write test data */
//     rtc_mem_write(riot_msg_offset, riot_msg, sizeof(riot_msg) - 1);
// }


// static void _get_rtc_mem(void)
// {
//     char buf[4];
//     rtc_mem_read(riot_msg_offset, buf, sizeof(buf));

//     if (memcmp(buf, riot_msg, sizeof(buf))) {
//         puts("RTC mem content does not match");
//         for (unsigned i = 0; i < sizeof(buf); ++i) {
//             printf("%02x - %02x\n", riot_msg[i], buf[i]);
//         }
//         return;
//     }

//     uint8_t size = rtc_mem_size();
//     while (size--) {
//         uint8_t data;

//         if (size >= riot_msg_offset &&
//             size < riot_msg_offset + sizeof(riot_msg)) {
//             continue;
//         }

//         rtc_mem_read(size, &data, 1);
//         if (data != size) {
//             puts("RTC mem content does not match");
//             printf("%02x: %02x\n", size, data);
//         }
//     }


//     puts("RTC mem OK");
// }
// #else
// static inline void _set_rtc_mem(void) {}
// static inline void _get_rtc_mem(void) {}
// #endif
void print_time(const char *label, const struct tm *time)
{
    printf("%s  %04d-%02d-%02d %02d:%02d:%02d\n", label,
            time->tm_year + TM_YEAR_OFFSET,
            time->tm_mon + 1,
            time->tm_mday,
            time->tm_hour,
            time->tm_min,
            time->tm_sec);
}

static void cb_rtc(void *arg)
{
    puts(arg);
}
// static void cb_rtc_sleep(void *arg)
// {
//     puts(arg);
// }
void radio_off(gnrc_netif_t *netif){
    netopt_state_t state = NETOPT_STATE_SLEEP;
    while ((netif = gnrc_netif_iter(netif))) {
            /* retry if busy */
            while (gnrc_netapi_set(netif->pid, NETOPT_STATE, 0,
                &state, sizeof(state)) == -EBUSY) {}
    }
}
void radio_on(gnrc_netif_t *netif){
    netopt_state_t state = NETOPT_STATE_IDLE;
    while ((netif = gnrc_netif_iter(netif))) {
            /* retry if busy */
            while (gnrc_netapi_set(netif->pid, NETOPT_STATE, 0,
                &state, sizeof(state)) == -EBUSY) {}
    }
}

int main(void)
{

    /* for the thread running the shell */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);
        struct tm time = {
        .tm_year = 2020 - TM_YEAR_OFFSET,   /* years are counted from 1900 */
        .tm_mon  =  4,                      /* 0 = January, 11 = December */
        .tm_mday = 28,
        .tm_hour = 23,
        .tm_min  = 58,
        .tm_sec  = 57
    };   
    rtc_init();
    puts("1111111111111111111111111111111111");
    print_time("  Setting clock to ", &time);
    rtc_set_time(&time);
    rtc_get_time(&time);	
    print_time("Clock value is now ", &time);
    gcoap_cli_init();
    puts("gcoap example app");   
    
    puts("Waiting for address autoconfiguration...");
    xtimer_sleep(3);

    /* print network addresses */
    puts("Configured network interfaces:");
    _gnrc_netif_config(0, NULL);

    puts("Configured rpl:");
    gnrc_rpl_init(7);
    puts("printing route:");
    while (gnrc_ipv6_nib_ft_iter(NULL, iface, &state, &entry)) {
        char addr_str[IPV6_ADDR_MAX_STR_LEN];
        if ((entry.dst_len == 0) || ipv6_addr_is_unspecified(&entry.dst)) {
            printf("default%s ", (entry.primary ? "*" : ""));
             puts("printing route:");   

        }
        else {
            printf("%s/%u ", ipv6_addr_to_str(addr_str, &entry.dst, sizeof(addr_str)),entry.dst_len);
            puts("printing route:");
        }
        if (!ipv6_addr_is_unspecified(&entry.next_hop)) {
           printf("via %s ", ipv6_addr_to_str(addr_str, &entry.next_hop, sizeof(addr_str)));
           puts("printing route:");
        }
        // printf("dev #%u\n", fte->iface);
        // char a[i][4] = entry->iface;
       // i++;
    }
    /*RTC initialisation*/
    // struct tm coaptime = {
    //     .tm_year = 2022 - TM_YEAR_OFFSET,   /* years are counted from 1900 */
    //     .tm_mon  =  3,                      /* 0 = January, 11 = December */
    //     .tm_mday = 30,
    //     .tm_hour = 12,
    //     .tm_min  = 00,
    //     .tm_sec  = 00
    // };

    // _set_rtc_mem();
    // _get_rtc_mem();

        // printf("dev #%u\n", fte->iface);
        // char a[i][4] = entry->iface;
       // i++;
 
    // while (1) {
    //     // mutex_lock(&rtc_mtx);
    //     // puts("Alarm!");
    //     puts("wake up!");
    //     ++cnt;
    //     rtc_get_time(&current_time);
    //     print_time("currenttime:\n", &current_time);
    //     int current_timestamp= mktime(&current_time);
    //     printf("current time stamp: %d\n", current_timestamp);
    //     int alarm_timestamp = 0;
    //     if ((int)(current_timestamp % 120) < (wakeup_gap*1)){
    //         radio_on(netif);
    //         int chance = ( wakeup_gap ) - ( current_timestamp % 120 );
    //         alarm_timestamp = (current_timestamp / 120) *120+ (wakeup_gap * 1);
    //         alarm_timestamp = alarm_timestamp- 1577836800;
    //         rtc_localtime(alarm_timestamp, &alarm_time);
    //         print_time("alarm time:\n", &alarm_time);
    //         printf("---------%ds",chance);
    //         puts("xtimer sleep");
            
    //         xtimer_sleep(chance);
           
    //         rtc_set_alarm(&alarm_time, cb_rtc, "111");
    //         radio_off(netif);
    //         // puts("radio is off");
    //         // puts("radio is offfffffffffffffffffffffffff");
    //         // puts("zzzzzzzzzzzzzzzzzzzz");
    //         pm_set(1);
    //         /*源代码*/
    //         // rtc_get_alarm(&time);
    //         // inc_secs(&time, PERIOD);
    //         // rtc_set_alarm(&time, cb, &rtc_mtx);
    //     }
    //     else{
    //         printf("fflush");
    //         fflush(stdout);
    //         radio_off(netif);
    //         alarm_timestamp =  current_timestamp+(120- (current_timestamp % 120));
    //         // alarm_timestamp = (current_timestamp / 360) *360+ (wakeup_gap * 1);
    //         alarm_timestamp = alarm_timestamp - 1577836800;
    //         rtc_localtime(alarm_timestamp, &alarm_time);
    //         print_time("alarm time:\n", &alarm_time);
            
    //         int modetest =1;
    //         rtc_set_alarm(&alarm_time, cb_rtc, (void *)modetest);
    //         // rtc_set_alarm(&alarm_time, cb_rtc_sleep, "222");
    //         // radio_on(netif);
    //         pm_set(1);
    //         // puts("radio is off");
    //         // puts("radio is offfffffffffffffffffffffffff");
    //         // puts("zzzzzzzzzzzzzzzzzzzz");
    //         // puts("3333333333333");
    //     }
    // }

while (1) {
        // mutex_lock(&rtc_mtx);
        // puts("Alarm!");
        puts("wake up!");
        ++cnt;
        rtc_get_time(&current_time);
        print_time("currenttime:\n", &current_time);
        int current_timestamp= mktime(&current_time);
        printf("current time stamp: %d\n", current_timestamp);
        int alarm_timestamp = 0;
        if ((int)(current_timestamp % 300) < (wakeup_gap*1)){
            radio_on(netif);
            int chance = ( wakeup_gap ) - ( current_timestamp % 300 );
            alarm_timestamp = (current_timestamp / 300) *300+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp- 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            printf("---------%ds",chance);
            puts("xtimer sleep");
            
            xtimer_sleep(chance);
           
            rtc_set_alarm(&alarm_time, cb_rtc, "111");
            radio_off(netif);
            puts("radio is off");
            puts("radio is offfffffffffffffffffffffffff");
            puts("zzzzzzzzzzzzzzzzzzzz");
            pm_set(1);
            /*源代码*/
            // rtc_get_alarm(&time);
            // inc_secs(&time, PERIOD);
            // rtc_set_alarm(&time, cb, &rtc_mtx);
        }
        else{
            printf("fflush");
            fflush(stdout);
            radio_off(netif);
            alarm_timestamp =  current_timestamp+(300- (current_timestamp % 300));
            // alarm_timestamp = (current_timestamp / 360) *360+ (wakeup_gap * 1);
            alarm_timestamp = alarm_timestamp - 1577836800;
            rtc_localtime(alarm_timestamp, &alarm_time);
            print_time("alarm time:\n", &alarm_time);
            
            int modetest =1;
            rtc_set_alarm(&alarm_time, cb_rtc, (void *)modetest);
            // radio_on(netif);
            pm_set(1);
            puts("radio is off");
            puts("radio is offfffffffffffffffffffffffff");
            puts("zzzzzzzzzzzzzzzzzzzz");
            puts("3333333333333");
        }
    }

    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should never be reached */
    return 0;
}
