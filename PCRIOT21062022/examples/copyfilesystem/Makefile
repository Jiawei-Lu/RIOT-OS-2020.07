# name of your application
APPLICATION = filesystem

# If no BOARD is found in the environment, use this default:
BOARD ?= native


# Blacklisting msp430-based boards, as file syscalls are not supported
FEATURES_BLACKLIST += arch_msp430

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../..

# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

USEMODULE += xtimer

# Modules to include:
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += gnrc_pktdump
# Use MTD (flash layer)
USEMODULE += mtd
# USEMODULE += mtd_sdcard

# Use VFS
USEMODULE += vfs

# Use a file system
USEMODULE += littlefs
#USEMODULE += littlefs2
#USEMODULE += spiffs
#USEMODULE += fatfs_vfs
#USEMODULE += constfs
#USEMODULE += devfs
# rtc
FEATURES_REQUIRED += periph_rtc
FEATURES_OPTIONAL += periph_rtc_ms
FEATURES_OPTIONAL += periph_rtc_mem

DISABLE_MODULE += periph_init_rtc


include $(RIOTBASE)/Makefile.include
