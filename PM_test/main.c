/*
 * Copyright (C) 2014 Freie Universität Berlin
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
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <stdlib.h>

#include "pm_layered.h"
#include "periph/pm.h"
#include "net/netdev.h"
#include "periph/gpio.h"
#include "common.h"
#include "periph/rtc.h"
#include <time.h>
#include "periph_conf.h"
#include "xtimer.h"
#include "shell.h"
#include "shell_commands.h"
at86rf2xx_t devs[AT86RF2XX_NUM];

// #define AT86RF2XX_PARAM_SLEEP  GPIO_PIN(PA,20)

// static const shell_command_t shell_commands[] = {
//     { NULL, NULL, NULL }
// };

// static void cb_rtc_puts(void *arg)
// {
//     puts(arg);
// }
// static int parse_mode(char *argv)
// {
//     uint8_t mode = atoi(argv);

//     if (mode >= PM_NUM_MODES) {
//         printf("Error: power mode not in range 0 - %d.\n", PM_NUM_MODES - 1);
//         return -1;
//     }

//     return mode;
// }
// static int parse_duration(char *argv)
// {
//     int duration = atoi(argv);

//     if (duration < 0) {
//         puts("Error: duration must be a positive number.");
//         return -1;
//     }

//     return duration;
// }
// static int check_mode_duration(int argc, char **argv)
// {
//     if (argc != 2) {
//         printf("Usage: %s <power mode> <duration (s)>\n", argv[0]);
//         return -1;
//     }

//     return 0;
// }
// static int cmd_set_rtc(int argc, char **argv)
// {
//     if (check_mode_duration(argc, argv) != 0) {
//         return 1;
//     }

//     int mode = parse_mode(argv[0]);
//     int duration = parse_duration(argv[1]);

//     if (mode < 0 || duration < 0) {
//         return 1;
//     }

//     printf("Setting power mode %d for %d seconds.\n", mode, duration);
//     fflush(stdout);

//     struct tm time;

//     rtc_get_time(&time);
//     time.tm_sec += duration;
//     rtc_set_alarm(&time, cb_rtc_puts, "The alarm rang");

//     at86rf2xx_set_state(devs, AT86RF2XX_STATE_FORCE_TRX_OFF);
//     pm_set(mode);

//     return 0;
// }
// int main(void)
// {
//     puts("Hello World!");

//     printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
//     printf("This board features a(n) %s MCU.\n", RIOT_MCU);
 
//     int argc1 = 2; 
//     char *argv1[] = {"0", "10"};
//     cmd_set_rtc(argc1, argv1);
//     char line_buf[SHELL_DEFAULT_BUFSIZE];
//     shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);
//     return 0;
// }



static void cb_rtc_puts(void *arg)
{
    puts(arg);
}
static int parse_mode(char *argv)
{
    uint8_t mode = atoi(argv);

    if (mode >= PM_NUM_MODES) {
        printf("Error: power mode not in range 0 - %d.\n", PM_NUM_MODES - 1);
        return -1;
    }

    return mode;
}
static int parse_duration(char *argv)
{
    int duration = atoi(argv);

    if (duration < 0) {
        puts("Error: duration must be a positive number.");
        return -1;
    }

    return duration;
}
static int check_mode_duration(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage: %s <power mode> <duration (s)>\n", argv[0]);
        return -1;
    }

    return 0;
}
static int cmd_set_rtc(int argc, char **argv)
{
    if (check_mode_duration(argc, argv) != 0) {
        return 1;
    }

    int mode = parse_mode(argv[0]);
    int duration = parse_duration(argv[1]);

    if (mode < 0 || duration < 0) {
        return 1;
    }

    printf("Setting power mode %d for %d seconds.\n", mode, duration);
    fflush(stdout);

    struct tm time;

    rtc_get_time(&time);
    time.tm_sec += duration;
    rtc_set_alarm(&time, cb_rtc_puts, "The alarm rang");

    at86rf2xx_set_state(devs, AT86RF2XX_STATE_FORCE_TRX_OFF);
    gpio_set(AT86RF2XX_PARAM_SLEEP);
    pm_set(mode);

    return 0;
}
int main(void)
{
    while(1){
    // uint8_t cmd = 'PREP_DEEP_SLEEP';
    unsigned dev_success = 0;
    for (unsigned i = 0; i < AT86RF2XX_NUM; i++) {
        netopt_enable_t en = NETOPT_ENABLE;
        const at86rf2xx_params_t *p = &at86rf2xx_params[i];
        netdev_t *dev = (netdev_t *)(&devs[i]);

        printf("Initializing AT86RF2xx radio at SPI_%d\n", p->spi);
        at86rf2xx_setup(&devs[i], p);
        // dev->event_callback = _event_cb;
        if (dev->driver->init(dev) < 0) {
            continue;
        }
        dev->driver->set(dev, NETOPT_RX_END_IRQ, &en, sizeof(en));
        dev_success++;
    }
    
    struct tm past, now;
    // struct tm test;
    // rtc_set_time(&test);
    // test.tm_sec = 0;
    // test.tm_sec -= 10;
    rtc_get_time(&past);
    // printf("%d\n", test.tm_sec);
    // int interval = 10;
    now.tm_sec = 0;
    int offset = 10;
    while (now.tm_sec - offset - past.tm_sec< 0){
        rtc_get_time(&now);
    }
    // at86rf2xx_set_state(devs, AT86RF2XX_STATE_FORCE_TRX_OFF);
    // gpio_set(AT86RF2XX_PARAM_SLEEP);
    int argc1 = 2; 
    char *argv1[] = {"0", "10"};
    cmd_set_rtc(argc1, argv1);
    }
    return 0;
}