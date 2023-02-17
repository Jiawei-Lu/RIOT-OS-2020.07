/*
 * Copyright (C) 2016 Michel Rottleuthner <michel.rottleuthner@haw-hamburg.de>
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     tests
 * @{
 *
 * @file
 * @brief       Test application for the sd-card spi driver
 *
 * @author      Michel Rottleuthner <michel.rottleuthner@haw-hamburg.de>
 *
 * @}
 */
 
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>


#include "shell.h"
#include "board.h" /* MTD_0 is defined in board.h */
/* Configure MTD device for SD card if none is provided */
#if !defined(MTD_0) && MODULE_MTD_SDCARD
#include "mtd_sdcard.h"
#include "sdcard_spi.h"
#include "sdcard_spi_params.h"
#include "io1_xplained.h"
#include "io1_xplained_params.h"

#define SDCARD_SPI_NUM ARRAY_SIZE(sdcard_spi_params)
static io1_xplained_t dev;
/* SD card devices are provided by drivers/sdcard_spi/sdcard_spi.c */
extern sdcard_spi_t sdcard_spi_devs[SDCARD_SPI_NUM];

/* Configure MTD device for the first SD card */
static mtd_sdcard_t mtd_sdcard_dev = {
    .base = {
        .driver = &mtd_sdcard_driver
    },
    .sd_card = &dev.sdcard,
    .params = &sdcard_spi_params[0],
};
static mtd_dev_t *mtd0 = (mtd_dev_t*)&mtd_sdcard_dev;
#define MTD_0 mtd0
#endif

/* Flash mount point */
#define FLASH_MOUNT_POINT   "/nvm0"

/* In this example, MTD_0 is used as mtd interface for littlefs or spiffs */
/* littlefs and spiffs basic usage are shown */
#ifdef MTD_0
/* File system descriptor initialization */
#if defined(MODULE_LITTLEFS)
/* include file system header for driver */
#include "fs/littlefs_fs.h"

/* file system specific descriptor
 * for littlefs, some fields can be tweaked to define the size
 * of the partition, see header documentation.
 * In this example, default behavior will be used, i.e. the entire
 * memory will be used (parameters come from mtd) */
static littlefs_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* littlefs file system driver will be used */
#define FS_DRIVER littlefs_file_system

#elif defined(MODULE_LITTLEFS2)
/* include file system header for driver */
#include "fs/littlefs2_fs.h"

/* file system specific descriptor
 * for littlefs2, some fields can be tweaked to define the size
 * of the partition, see header documentation.
 * In this example, default behavior will be used, i.e. the entire
 * memory will be used (parameters come from mtd) */
static littlefs2_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* littlefs file system driver will be used */
#define FS_DRIVER littlefs2_file_system

#elif defined(MODULE_SPIFFS)
/* include file system header */
#include "fs/spiffs_fs.h"

/* file system specific descriptor
 * as for littlefs, some fields can be changed if needed,
 * this example focus on basic usage, i.e. entire memory used */
static spiffs_desc_t fs_desc = {
    .lock = MUTEX_INIT,
};

/* spiffs driver will be used */
#define FS_DRIVER spiffs_file_system

#elif defined(MODULE_FATFS_VFS)
/* include file system header */
#include "fs/fatfs.h"

/* file system specific descriptor
 * as for littlefs, some fields can be changed if needed,
 * this example focus on basic usage, i.e. entire memory used */
static fatfs_desc_t fs_desc;

/* provide mtd devices for use within diskio layer of fatfs */
mtd_dev_t *fatfs_mtd_devs[FF_VOLUMES];

/* fatfs driver will be used */
#define FS_DRIVER fatfs_file_system
#endif

/* this structure defines the vfs mount point:
 *  - fs field is set to the file system driver
 *  - mount_point field is the mount point name
 *  - private_data depends on the underlying file system. For both spiffs and
 *  littlefs, it needs to be a pointer to the file system descriptor */
static vfs_mount_t flash_mount = {
    .fs = &FS_DRIVER,
    .mount_point = FLASH_MOUNT_POINT,
    .private_data = &fs_desc,
};
#endif /* MTD_0 */

/* Add simple macro to check if an MTD device together with a filesystem is
 * compiled in */
#if  defined(MTD_0) && \
     (defined(MODULE_SPIFFS) || \
      defined(MODULE_LITTLEFS) || \
      defined(MODULE_LITTLEFS2) || \
      defined(MODULE_FATFS_VFS))
#define FLASH_AND_FILESYSTEM_PRESENT    1
#else
#define FLASH_AND_FILESYSTEM_PRESENT    0
#endif
/*******************************************************************************************************************/
/* constfs example */
#include "fs/constfs.h"

#define HELLO_WORLD_CONTENT "Hello World!\n"
#define HELLO_RIOT_CONTENT  "Hello RIOT!\n"
#define TEMP_CONTENT  "TEMPXXX TIMESTAMPXXX\n"
/* this defines two const files in the constfs */
static constfs_file_t constfs_files[] = {
    {
        .path = "/hello-world",
        .size = sizeof(HELLO_WORLD_CONTENT),
        .data = (const uint8_t *)HELLO_WORLD_CONTENT,
    },
    {
        .path = "/hello-riot",
        .size = sizeof(HELLO_RIOT_CONTENT),
        .data = (const uint8_t *)HELLO_RIOT_CONTENT,
    },
    {
        .path = "/temp",
        .size = sizeof(TEMP_CONTENT),
        .data = (const uint8_t *)TEMP_CONTENT,
    }
};

/* this is the constfs specific descriptor */
static constfs_t constfs_desc = {
    .nfiles = ARRAY_SIZE(constfs_files),
    .files = constfs_files,
};

/* constfs mount point, as for previous example, it needs a file system driver,
 * a mount point and private_data as a pointer to the constfs descriptor */
static vfs_mount_t const_mount = {
    .fs = &constfs_file_system,
    .mount_point = "/const",
    .private_data = &constfs_desc,
};


#include "shell.h"
#include "sdcard_spi.h"
#include "sdcard_spi_internal.h"
#include "sdcard_spi_params.h"
#include "fmt.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#include "periph/gpio.h"

#include "xtimer.h"
#include "at30tse75x.h"
#include "sdcard_spi.h"
#include "io1_xplained.h"
#include "io1_xplained_params.h"
#include "vfs_default.h"

/* independent of what you specify in a r/w cmd this is the maximum number of blocks read at once.
   If you call read with a bigger blockcount the read is performed in chunks*/
#define MAX_BLOCKS_IN_BUFFER 4
#define BLOCK_PRINT_BYTES_PER_LINE 16
#define FIRST_PRINTABLE_ASCII_CHAR 0x20
#define ASCII_UNPRINTABLE_REPLACEMENT "."

#define DELAY_1S   (1U) /* 1 seconds delay between each test */


/* this is provided by the sdcard_spi driver
 * see drivers/sdcard_spi/sdcard_spi.c */
// extern dev->sdcard[ARRAY_SIZE(sdcard_spi_params)];
//sdcard_spi_t *dev.sdcard = &sdcard_spi_devs[0];
// card = dev->sdcard;
uint8_t buffer[SD_HC_BLOCK_SIZE * MAX_BLOCKS_IN_BUFFER];

#define DELAY_1S   (1U) /* 1 seconds delay between each test */
static int _init(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("Initializing SD-card at SPI_%i...", sdcard_spi_params[0].spi_dev);

    if (io1_xplained_init(&dev, &io1_xplained_params[0]) != IO1_XPLAINED_OK) {
        puts("[Error] Cannot initialize the IO1 Xplained extension\n");
        #if ENABLE_DEBUG != 1
        puts("enable debugging in sdcard_spi.c for further information!");
        #endif
        return -2;
    }
    puts("[OK]");
    return 0;
}

static int _cid(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    puts("----------------------------------------");
    printf("MID: %d\n", dev.sdcard.cid.MID);
    printf("OID: %c%c\n", dev.sdcard.cid.OID[0], dev.sdcard.cid.OID[1]);
    printf("PNM: %c%c%c%c%c\n", dev.sdcard.cid.PNM[0], dev.sdcard.cid.PNM[1], dev.sdcard.cid.PNM[2],
                                dev.sdcard.cid.PNM[3], dev.sdcard.cid.PNM[4]);
    printf("PRV: %u\n", dev.sdcard.cid.PRV);
    printf("PSN: %" PRIu32 "\n", dev.sdcard.cid.PSN);
    printf("MDT: %u\n", dev.sdcard.cid.MDT);
    printf("CRC: %u\n", dev.sdcard.cid.CID_CRC);
    puts("----------------------------------------");
    return 0;
}
static int _csd(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (dev.sdcard.csd_structure == SD_CSD_V1) {
        puts("CSD V1\n----------------------------------------");
        printf("CSD_STRUCTURE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.CSD_STRUCTURE);
        printf("TAAC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.TAAC);
        printf("NSAC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.NSAC);
        printf("TRAN_SPEED: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.TRAN_SPEED);
        printf("CCC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.CCC);
        printf("READ_BL_LEN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.READ_BL_LEN);
        printf("READ_BL_PARTIAL: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.READ_BL_PARTIAL);
        printf("WRITE_BLK_MISALIGN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.WRITE_BLK_MISALIGN);
        printf("READ_BLK_MISALIGN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.READ_BLK_MISALIGN);
        printf("DSR_IMP: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.DSR_IMP);
        printf("C_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.C_SIZE);
        printf("VDD_R_CURR_MIN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.VDD_R_CURR_MIN);
        printf("VDD_R_CURR_MAX: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.VDD_R_CURR_MAX);
        printf("VDD_W_CURR_MIN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.VDD_W_CURR_MIN);
        printf("VDD_W_CURR_MAX: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.VDD_W_CURR_MAX);
        printf("C_SIZE_MULT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.C_SIZE_MULT);
        printf("ERASE_BLK_EN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.ERASE_BLK_EN);
        printf("SECTOR_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.SECTOR_SIZE);
        printf("WP_GRP_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.WP_GRP_SIZE);
        printf("WP_GRP_ENABLE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.WP_GRP_ENABLE);
        printf("R2W_FACTOR: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.R2W_FACTOR);
        printf("WRITE_BL_LEN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.WRITE_BL_LEN);
        printf("WRITE_BL_PARTIAL: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.WRITE_BL_PARTIAL);
        printf("FILE_FORMAT_GRP: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.FILE_FORMAT_GRP);
        printf("COPY: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.COPY);
        printf("PERM_WRITE_PROTECT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.PERM_WRITE_PROTECT);
        printf("TMP_WRITE_PROTECT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.TMP_WRITE_PROTECT);
        printf("FILE_FORMAT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.FILE_FORMAT);
        printf("CRC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v1.CSD_CRC);
    }
    else if (dev.sdcard.csd_structure == SD_CSD_V2) {
        puts("CSD V2:\n----------------------------------------");
        printf("CSD_STRUCTURE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.CSD_STRUCTURE);
        printf("TAAC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.TAAC);
        printf("NSAC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.NSAC);
        printf("TRAN_SPEED: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.TRAN_SPEED);
        printf("CCC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.CCC);
        printf("READ_BL_LEN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.READ_BL_LEN);
        printf("READ_BL_PARTIAL: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.READ_BL_PARTIAL);
        printf("WRITE_BLK_MISALIGN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.WRITE_BLK_MISALIGN);
        printf("READ_BLK_MISALIGN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.READ_BLK_MISALIGN);
        printf("DSR_IMP: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.DSR_IMP);
        printf("C_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.C_SIZE);
        printf("ERASE_BLK_EN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.ERASE_BLK_EN);
        printf("SECTOR_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.SECTOR_SIZE);
        printf("WP_GRP_SIZE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.WP_GRP_SIZE);
        printf("WP_GRP_ENABLE: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.WP_GRP_ENABLE);
        printf("R2W_FACTOR: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.R2W_FACTOR);
        printf("WRITE_BL_LEN: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.WRITE_BL_LEN);
        printf("WRITE_BL_PARTIAL: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.WRITE_BL_PARTIAL);
        printf("FILE_FORMAT_GRP: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.FILE_FORMAT_GRP);
        printf("COPY: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.COPY);
        printf("PERM_WRITE_PROTECT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.PERM_WRITE_PROTECT);
        printf("TMP_WRITE_PROTECT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.TMP_WRITE_PROTECT);
        printf("FILE_FORMAT: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.FILE_FORMAT);
        printf("CRC: 0x%0lx\n", (unsigned long)dev.sdcard.csd.v2.CSD_CRC);
    }
    puts("----------------------------------------");
    return 0;
}

static int _sds(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    sd_status_t sds;

    if (sdcard_spi_read_sds(&dev.sdcard, &sds) == SD_RW_OK) {
        puts("SD status:\n----------------------------------------");
        printf("SIZE_OF_PROTECTED_AREA: 0x%0lx\n", (unsigned long)sds.SIZE_OF_PROTECTED_AREA);
        printf("SUS_ADDR: 0x%0lx\n", (unsigned long)sds.SUS_ADDR);
        printf("VSC_AU_SIZE: 0x%0lx\n", (unsigned long)sds.VSC_AU_SIZE);
        printf("SD_CARD_TYPE: 0x%0lx\n", (unsigned long)sds.SD_CARD_TYPE);
        printf("ERASE_SIZE: 0x%0lx\n", (unsigned long)sds.ERASE_SIZE);
        printf("SPEED_CLASS: 0x%0lx\n", (unsigned long)sds.SPEED_CLASS);
        printf("PERFORMANCE_MOVE: 0x%0lx\n", (unsigned long)sds.PERFORMANCE_MOVE);
        printf("VIDEO_SPEED_CLASS: 0x%0lx\n", (unsigned long)sds.VIDEO_SPEED_CLASS);
        printf("ERASE_TIMEOUT: 0x%0lx\n", (unsigned long)sds.ERASE_TIMEOUT);
        printf("ERASE_OFFSET: 0x%0lx\n", (unsigned long)sds.ERASE_OFFSET);
        printf("UHS_SPEED_GRADE: 0x%0lx\n", (unsigned long)sds.UHS_SPEED_GRADE);
        printf("UHS_AU_SIZE: 0x%0lx\n", (unsigned long)sds.UHS_AU_SIZE);
        printf("AU_SIZE: 0x%0lx\n", (unsigned long)sds.AU_SIZE);
        printf("DAT_BUS_WIDTH: 0x%0lx\n", (unsigned long)sds.DAT_BUS_WIDTH);
        printf("SECURED_MODE: 0x%0lx\n", (unsigned long)sds.SECURED_MODE);
        puts("----------------------------------------");
        return 0;
    }
    return -1;
}

static int _size(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    uint64_t bytes = sdcard_spi_get_capacity(&dev.sdcard);

    uint32_t gib_int = bytes / (SDCARD_SPI_IEC_KIBI * SDCARD_SPI_IEC_KIBI * SDCARD_SPI_IEC_KIBI);
    uint32_t gib_frac = ( (((bytes/(SDCARD_SPI_IEC_KIBI * SDCARD_SPI_IEC_KIBI))
                            - gib_int * SDCARD_SPI_IEC_KIBI) * SDCARD_SPI_SI_KILO)
                          / SDCARD_SPI_IEC_KIBI);

    uint32_t gb_int = bytes / (SDCARD_SPI_SI_KILO * SDCARD_SPI_SI_KILO * SDCARD_SPI_SI_KILO);
    uint32_t gb_frac = (bytes / (SDCARD_SPI_SI_KILO * SDCARD_SPI_SI_KILO))
                       - (gb_int * SDCARD_SPI_SI_KILO); /* [MB] */

    puts("\nCard size: ");
    print_u64_dec( bytes );
    printf(" bytes (%" PRIu32 ",%03" PRIu32 " GiB | %" PRIu32 ",%03" PRIu32 " GB)\n", gib_int,
           gib_frac, gb_int, gb_frac);
    return 0;
}

static int _read(int argc, char **argv)
{
    int blockaddr;
    int cnt;
    bool print_as_char = false;

    if ((argc == 3) || (argc == 4)) {
        blockaddr = atoi(argv[1]);
        cnt = atoi(argv[2]);
        if (argc == 4 && (strcmp("-c", argv[3]) == 0)) {
            print_as_char = true;
        }
    }
    else {
        printf("usage: %s blockaddr cnt [-c]\n", argv[0]);
        return -1;
    }

    int total_read = 0;
    while (total_read < cnt) {
        int chunk_blocks = cnt - total_read;
        if (chunk_blocks > MAX_BLOCKS_IN_BUFFER) {
            chunk_blocks = MAX_BLOCKS_IN_BUFFER;
        }
        sd_rw_response_t state;
        int chunks_read = sdcard_spi_read_blocks(&dev.sdcard, blockaddr + total_read, buffer,
                                                 SD_HC_BLOCK_SIZE, chunk_blocks, &state);

        if (state != SD_RW_OK) {
            printf("read error %d (block %d/%d)\n", state, total_read + chunks_read, cnt);
            return -1;
        }

        for (int i = 0; i < chunk_blocks * SD_HC_BLOCK_SIZE; i++) {

            if ((i % SD_HC_BLOCK_SIZE) == 0) {
                printf("BLOCK %d:\n", blockaddr + total_read + i / SD_HC_BLOCK_SIZE);
            }

            if (print_as_char) {
                if (buffer[i] >= FIRST_PRINTABLE_ASCII_CHAR) {
                    printf("%c", buffer[i]);
                }
                else {
                    printf(ASCII_UNPRINTABLE_REPLACEMENT);
                }
            }
            else {
                printf("%02x ", buffer[i]);
            }

            if ((i % BLOCK_PRINT_BYTES_PER_LINE) == (BLOCK_PRINT_BYTES_PER_LINE - 1)) {
                puts(""); /* line break after BLOCK_PRINT_BYTES_PER_LINE bytes */
            }

            if ((i % SD_HC_BLOCK_SIZE) == (SD_HC_BLOCK_SIZE - 1)) {
                puts(""); /* empty line after each printed block */
            }
        }
        total_read += chunks_read;
    }
    return 0;
}

static int _write(int argc, char **argv)
{
    int bladdr;
    char *data;
    int size;
    bool repeat_data = false;

    if (argc == 3 || argc == 4) {
        bladdr = atoi(argv[1]);
        data = argv[2];
        size = strlen(argv[2]);
        printf("will write '%s' (%d chars) at start of block %d\n", data, size, bladdr);
        if (argc == 4 && (strcmp("-r", argv[3]) == 0)) {
            repeat_data = true;
            puts("the rest of the block will be filled with copies of that string");
        }
        else {
            puts("the rest of the block will be filled with zeros");
        }
    }
    else {
        printf("usage: %s blockaddr string [-r]\n", argv[0]);
        return -1;
    }

    if (size > SD_HC_BLOCK_SIZE) {
        printf("maximum stringsize to write at once is %d ...aborting\n", SD_HC_BLOCK_SIZE);
        return -1;
    }

    /* copy data to a full-block-sized buffer an fill remaining block space according to -r param*/
    uint8_t write_buffer[SD_HC_BLOCK_SIZE];
    for (unsigned i = 0; i < sizeof(write_buffer); i++) {
        if (repeat_data || ((int)i < size)) {
            write_buffer[i] = data[i % size];
        }
        else {
            write_buffer[i] = 0;
        }
    }

    sd_rw_response_t state;
    int chunks_written = sdcard_spi_write_blocks(&dev.sdcard, bladdr, write_buffer, SD_HC_BLOCK_SIZE, 1, &state);

    if (state != SD_RW_OK) {
        printf("write error %d (wrote %d/%d blocks)\n", state, chunks_written, 1);
        return -1;
    }

    printf("write block %d [OK]\n", bladdr);
    return 0;
}

static int _copy(int argc, char **argv)
{
    int src_block;
    int dst_block;
    uint8_t tmp_copy[SD_HC_BLOCK_SIZE];

    if (argc != 3) {
        printf("usage: %s src_block dst_block\n", argv[0]);
        return -1;
    }

    src_block = atoi(argv[1]);
    dst_block = atoi(argv[2]);

    sd_rw_response_t rd_state;
    sdcard_spi_read_blocks(&dev.sdcard, src_block, tmp_copy, SD_HC_BLOCK_SIZE, 1, &rd_state);

    if (rd_state != SD_RW_OK) {
        printf("read error %d (block %d)\n", rd_state, src_block);
        return -1;
    }

    sd_rw_response_t wr_state;
    sdcard_spi_write_blocks(&dev.sdcard, dst_block, tmp_copy, SD_HC_BLOCK_SIZE, 1, &wr_state);

    if (wr_state != SD_RW_OK) {
        printf("write error %d (block %d)\n", wr_state, dst_block);
        return -2;
    }

    printf("copy block %d to %d [OK]\n", src_block, dst_block);
    return 0;
}

static int _sector_count(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    printf("available sectors on card: %" PRIu32 "\n",
           sdcard_spi_get_sector_count(&dev.sdcard));
    return 0;
}
static int _mount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_mount(&flash_mount);
    if (res < 0) {
        printf("Error while mounting %s...try format\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully mounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _format(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_format(&flash_mount);
    if (res < 0) {
        printf("Error while formatting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully formatted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _umount(int argc, char **argv)
{
    (void)argc;
    (void)argv;
#if FLASH_AND_FILESYSTEM_PRESENT
    int res = vfs_umount(&flash_mount);
    if (res < 0) {
        printf("Error while unmounting %s\n", FLASH_MOUNT_POINT);
        return 1;
    }

    printf("%s successfully unmounted\n", FLASH_MOUNT_POINT);
    return 0;
#else
    puts("No external flash file system selected");
    return 1;
#endif
}

static int _cat(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <file>\n", argv[0]);
        return 1;
    }
    /* With newlib or picolibc, low-level syscalls are plugged to RIOT vfs
     * on native, open/read/write/close/... are plugged to RIOT vfs */
#if defined(MODULE_NEWLIB) || defined(MODULE_PICOLIBC)
    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (fread(&c, 1, 1, f) != 0) {
        putchar(c);
    }
    fclose(f);
#else
    int fd = open(argv[1], O_RDWR | O_CREAT);//O_RDONLY);
    if (fd < 0) {
        printf("file %s does not exist\n", argv[1]);
        return 1;
    }
    char c;
    while (read(fd, &c, 1) != 0) {
        putchar(c);
    }
    close(fd);
#endif
    fflush(stdout);
    return 0;
}

static int _tee(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: %s <file> <str>\n", argv[0]);
        return 1;
    }

#if defined(MODULE_NEWLIB) || defined(MODULE_PICOLIBC)
    FILE *f = fopen(argv[1], "w+");
    if (f == NULL) {
        printf("error while trying to create1 %s\n", argv[1]);
        return 1;
    }
    if (fwrite(argv[2], 1, strlen(argv[2]), f) != strlen(argv[2])) {
        puts("Error while writing");
    }
    fclose(f);
#else
    int fd = open(argv[1], O_RDWR | O_CREAT);
    if (fd < 0) {
        printf("error while trying to create2 %s\n", argv[1]);
        return 1;
    }
    if (write(fd, argv[2], strlen(argv[2])) != (ssize_t)strlen(argv[2])) {
        puts("Error while writing");
    }
    close(fd);
#endif
    return 0;
}

static void _sd_card_cid(void)
{
    puts("SD Card CID info:");
    printf("MID: %d\n", dev.sdcard.cid.MID);
    printf("OID: %c%c\n", dev.sdcard.cid.OID[0], dev.sdcard.cid.OID[1]);
    printf("PNM: %c%c%c%c%c\n",
           dev.sdcard.cid.PNM[0], dev.sdcard.cid.PNM[1], dev.sdcard.cid.PNM[2],
           dev.sdcard.cid.PNM[3], dev.sdcard.cid.PNM[4]);
    printf("PRV: %u\n", dev.sdcard.cid.PRV);
    printf("PSN: %" PRIu32 "\n", dev.sdcard.cid.PSN);
    printf("MDT: %u\n", dev.sdcard.cid.MDT);
    printf("CRC: %u\n", dev.sdcard.cid.CID_CRC);
    puts("+----------------------------------------+\n");
}


static const shell_command_t shell_commands[] = {
    { "init", "initializes default card", _init },
    { "cid",  "print content of CID (Card IDentification) register", _cid },
    { "csd", "print content of CSD (Card-Specific Data) register", _csd },
    { "sds", "print SD status", _sds },
    { "size", "print card size", _size },
    { "sectors", "print sector count of card", _sector_count },
    { "read", "'read n m' reads m blocks beginning at block address n and prints the result. "
              "Append -c option to print data readable chars", _read },
    { "write", "'write n data' writes data to block n. Append -r option to "
               "repeatedly write data to complete block", _write },
    { "copy", "'copy src dst' copies block src to block dst", _copy },
    { "mount", "mount flash filesystem", _mount },
    { "format", "format flash file system", _format },
    { "umount", "unmount flash filesystem", _umount },
    { "cat", "print the content of a file", _cat },
    { "tee", "write a string in a file", _tee },
    { NULL, NULL, NULL }
};


int main(void)
{
#if defined(MTD_0) && (defined(MODULE_SPIFFS) || defined(MODULE_LITTLEFS) || defined(MODULE_LITTLEFS2))
    /* spiffs and littlefs need a mtd pointer
     * by default the whole memory is used */
    fs_desc.dev = MTD_0;
#elif defined(MTD_0) && defined(MODULE_FATFS_VFS)
    fatfs_mtd_devs[fs_desc.vol_idx] = MTD_0;
#endif
    int res = vfs_mount(&const_mount);
    if (res < 0) {
        puts("Error while mounting constfs");
    }
    else {
        puts("constfs mounted successfully");
    }

    puts("SD-card spi driver test application");

    dev.sdcard.init_done = false;

    puts("insert SD-card and use 'init' command to set card to spi mode");
    puts("WARNING: using 'write' or 'copy' commands WILL overwrite data on your sd-card and");
    puts("almost for sure corrupt existing filesystems, partitions and contained data!");

float temperature;

if (io1_xplained_init(&dev, &io1_xplained_params[0]) != IO1_XPLAINED_OK) {
        puts("[Error] Cannot initialize the IO1 Xplained extension\n");
        return 1;
    }
puts("Initialization successful");
    puts("\n+--------Starting tests --------+");
    
        /* Get temperature in degrees celsius */
        at30tse75x_get_temperature(&dev.temp, &temperature);
        printf("Temperature [°C]: %i.%03u\n"
               "+-------------------------------------+\n",
               (int)temperature,
               (unsigned)((temperature - (int)temperature) * 1000));
        xtimer_sleep(DELAY_1S);

        /* Card detect pin is inverted */
        if (!gpio_read(IO1_SDCARD_SPI_PARAM_DETECT)) {
            _sd_card_cid();
            xtimer_sleep(DELAY_1S);
        }

        uint16_t light;
        io1_xplained_read_light_level(&light);
        printf("Light level: %i\n"
               "+-------------------------------------+\n",
               light);
        xtimer_sleep(DELAY_1S);

        /* set led */
        gpio_set(IO1_LED_PIN);
        xtimer_sleep(DELAY_1S);

        /* clear led */
        gpio_clear(IO1_LED_PIN);
        xtimer_sleep(DELAY_1S);

        /* toggle led */
        gpio_toggle(IO1_LED_PIN);
        xtimer_sleep(DELAY_1S);

        /* toggle led again */
        gpio_toggle(IO1_LED_PIN);
        xtimer_sleep(DELAY_1S);

    vfs_DIR mount = {0};

    /* list mounted file systems */
    puts("mount points:");
    while (vfs_iterate_mount_dirs(&mount)) {
        printf("\t%s\n", mount.mp->mount_point);
    }
    printf("\ndata dir: %s\n", VFS_DEFAULT_DATA);

char line_buf[SHELL_DEFAULT_BUFSIZE];
shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

return 0;

}