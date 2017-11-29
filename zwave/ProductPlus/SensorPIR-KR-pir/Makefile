#****************************************************************************
#
#  Copyright (c) 2001-2013
#  Sigma Designs, Inc.
#  All Rights Reserved
#
#----------------------------------------------------------------------------
#
# Description:
#    This makefile defines the targets built
#
# Author:   Erik Friis Harck
#
# Last Changed By:  $Author: efh $
# Revision:         $Revision: 19611 $
# Last Changed:     $Date: 2010-12-01 12:45:57 +0100 (on, 01 dec 2010) $
#
#***************************************************************************/

#---------------------------------------------------------------------------------------------------------
# HOW-TO: Specifying which subset of "everything", you wan't to build:
#
# You can specify a single frequency to build for in 3 different ways:
# 1. By specifying the frequency in your command line, like:
#    > make FREQUENCY=EU ....
# 2. By uncommenting and correcting the following line:
#FREQUENCY:=US
# 3. Alternativly you can do the same by setting your environment with:
#    > SET FREQUENCY=EU
#    Remember to UNSET this when you jump to work on other things.
#
# You can specify a single code memory mode to build for in 3 different ways:
# 1. By specifying the code memory mode in your command line, like:
#    > make CODE_MEMORY_MODE=banking ....
# 2. By uncommenting and correcting the following line:
CODE_MEMORY_MODE:=banking
# 3. Alternativly you can do the same by setting your environment with:
#    > SET CODE_MEMORY_MODE=banking
#    Remember to UNSET this when you jump to work on other things.
#
# You can specify a single library to build for in 3 different ways:
# 1. By specifying the library in your command line, like:
#    > make LIBRARY=controller_portable ....
# 2. By uncommenting and correcting the following line:
LIBRARY:=slave_enhanced_232
# 3. Alternativly you can do the same by setting your environment with:
#    > SET LIBRARY=controller_portable
#    Remember to UNSET this when you jump to work on other things.
#
# You can specify a single host interface to build for in 3 different ways:
# 1. By specifying the host interface in your command line, like:
#    > make HOST_INTERFACE=UART ....
# 2. By uncommenting and correcting the following line:
HOST_INTERFACE:=UART
# 3. Alternativly you can do the same by setting your environment with:
#    > SET HOST_INTERFACE=UART
#    Remember to UNSET this when you jump to work on other things.
#
# You can specify a single sensor type to build for in 3 different ways:
# 1. By specifying the sensor type in your command line, like:
#    > make SENSOR_TYPE=NON_BATT ....
# 2. By uncommenting and correcting the following line:
SENSOR_TYPE:=BATTERY
# 3. Alternativly you can do the same by setting your environment with:
#    > SET SENSOR_TYPE=NON_BATT
#    Remember to UNSET this when you jump to work on other things.
#
# You can specify a single board type to build for in 3 different ways:
# 1. By specifying the board type in your command line, like:
#    > make BOARD=ZDP03A ....
# 2. By uncommenting and correcting the following line:
#BOARD:=ZDP03A
# 3. Alternativly you can do the same by setting your environment with:
#    > SET BOARD=ZDP03A
#    Remember to UNSET this when you jump to work on other things.
#
# This application is only prepared for building as secure
#MAKESCHEME:=SCHEME_0
MAKESCHEME_DEFAULT:=SCHEME_0
#
# You can specify a single watchdog type to build for in 3 different ways:
# 1. By specifying the watchdog type in your command line, like:
#    > make WATCHDOG=WATCHDOG_ENABLED ....
# 2. By uncommenting and correcting the following line:
WATCHDOG:=WATCHDOG_DISABLED
# 3. Alternativly you can do the same by setting your environment with:
#    > SET WATCHDOG=WATCHDOG_ENABLED
#    Remember to UNSET this when you jump to work on other things.
#
# And yes... You can combine these methods in any way.
#
# If you do nothing of the above, you will be building target code for all combinations of frequencies and
# code memory modes in the lists below.
#
# And yes... You can of course manipulate the lists below, if you have special needs for ranges of targets.
#---------------------------------------------------------------------------------------------------------

# If not supplied by application here, the name of the application will be set equal to it's directory name.
APP_NAME:=SensorPIR

CHIPPACK:=ZW050x
LIST_OF_BOARD:=ZDP03A ZM5202


ifeq ($(BOARD),ZM5202)
 # Set the driver for external serial non-volatile memory for use of SPI port 1, and P0.4 as chip select.
 NVM_TYPE:=generic_p0_4_spi1
 CHIPPACK:=ZM5202
endif

ifneq ($(SCHEME),)
SEC=_$(SCHEME)
endif

# Target name (Name of your target directory and base name of your target files)
ifneq ($(BOOTLOADER_ENABLED),)
ifeq ($(LIBRARY),slave_routing)
LIBRARY:=slave_enhanced_232
ENH:=_enhanced_232
endif
TARGET:=$(APP)_$(LIB)_OTA_$(CHIPPACK)_$(COUNTRY)$(TEST)$(SEC)
else
TARGET:=$(APP)_$(LIB)_$(CHIPPACK)_$(COUNTRY)$(TEST)$(SEC)
endif

#---------------------------------------------------------------------------------------------------------
# build the project the original way for OTP. "SensorPIR"
ifeq ($(BOOTLOADER),)
RELFILES:=\
 SensorPIR.obj\
 eeprom.obj\
 ZW_nvr_app_api.obj\
 manufacturer_specific_device_id.obj\
 event_util.obj\
 keyman.obj\
 p_button.obj\
 one_button.obj\
 ZW_string.obj\
 slave_learn_plus.obj\
 association_plus.obj\
 ZW_tx_mutex.obj\
 agi.obj\
 CommandClassVersion.obj\
 CommandClassPowerLevel.obj\
 CommandClassDeviceResetLocally.obj\
 CommandClassAssociation.obj\
 CommandClassAssociationGroupInfo.obj\
 CommandClassManufacturerSpecific.obj\
 CommandClassZWavePlusInfo.obj\
 CommandClassBasic.obj\
 CommandClassBattery.obj\
 CommandClassNotification.obj\
 CommandClassWakeUp.obj\
 notification.obj\
 battery_monitor.obj\
 battery_non_zero_vars.obj\
 battery_plus.obj
endif

ifneq ($(BOOTLOADER_ENABLED),)
RELFILES+=\
 CommandClassFirmwareUpdate.obj\
 ota_util.obj
endif

ifneq ($(SCHEME),)
RELFILES+=\
 ZW_Security_AES_module.obj \
 ZW_TransportSecurity.obj\
 ZW_Security_FSM.obj\
 ZW_cmd_class_list.obj
else
RELFILES+=\
 ZW_TransportNative.obj
endif

#---------------------------------------------------------------------------------------------------------

# Find the current directory
# used as a base for all other paths.
BASEDIR=$(shell cd)

# Find the product name from the subdirectoryname
BASEDIRWORDLIST=$(subst \, ,$(BASEDIR))
BASEDIRLASTWORDINDEX=$(words $(BASEDIRWORDLIST))
PRODUCTDIR=$(word $(BASEDIRLASTWORDINDEX),$(BASEDIRWORDLIST))

# Find the Z-Wave library location (if not forced by setting the environment variable ZWLIBROOT)
ifndef ZWLIBROOT
ZWLIBROOT:=$(subst \$(PRODUCTDIR),,$(subst \ProductPlus,\Z-Wave,$(BASEDIR)))
endif

# Library location when making release of application
ifdef SVNLIBRELEASEDIRBIN
ZWLIBROOT:=$(SVNLIBRELEASEDIRBIN)
endif

# Export the variables declared above to the other makefiles.
export BASEDIR ZWLIBROOT UNDERSTAND_C ZM4102

# Utility location
ifeq ($(ZWUTILDIR),)
ZWUTILDIR:=..\ApplicationUtilities
endif

-include $(ZWUTILDIR)\Makefile.common_bootloader

include $(ZWUTILDIR)\Makefile.common

# Filter out unsupported frequencies for the Z-Wave application.
# Full list of all supported frequencies can be found in $(ZWUTILDIR)\Makefile.common
# (this filter-out must be done after/below the inclusion of $(ZWUTILDIR)\Makefile.common)
LIST_OF_FREQUENCY:=$(filter-out ALL ALL_3CH,$(LIST_OF_FREQUENCY))
LIST_OF_BOARD:=$(filter-out ZM5304,$(LIST_OF_BOARD))

#---------------------------------------------------------------------------------------------------------
# ADDITION TO THE VARIANTS TO BUILD
#---------------------------------------------------------------------------------------------------------
# Add a dimension "BOOT" to the build array
#
# BOOT variant must be added in front of LIST_OF_VARIANTS to be handled last,
# and for the filtering below in this Makefile to work. (LIST_OF_VARIANTS+= BOOT # does not work correctly)
LIST_OF_VARIANTS:=BOOT $(LIST_OF_VARIANTS)
# A full list of values for each VariantName must be assigned.
LIST_OF_BOOT:=BOOTLOADER BOOTLOADER_ENABLED
# A default value for each VariantName must be assigned. They are used in the help texts.
BOOT_DEFAULT:=BOOTLOADER_ENABLED
# An option variable for each VariantName must be assigned to be considered during "make",
# and of course, the application source code must be prepared to distinguish between variants.
ifeq ($(BOOT),BOOTLOADER)
BOOT_OPTION:=BOOTLOADER=1
BOOTLOADER:=1
endif
ifeq ($(BOOT),BOOTLOADER_ENABLED)
BOOT_OPTION:=BOOTLOADER_ENABLED=1
BOOTLOADER_ENABLED:=1
endif
#---------------------------------------------------------------------------------------------------------

#---------------------------------------------------------------------------------------------------------
# Filtering out targets, which have no meaning, or are just not needed.
#
# The controlling variant name must be listed before the controlled variant name in the LIST_OF_VARIANTS
# for this to work.
#
#LIST_OF_VARIANTS:=BOARD HOST_INTERFACE LIBRARY FREQUENCY CODE_MEMORY_MODE SENSOR_TYPE CHIP
ifneq ($(BOOTLOADER),)
LIST_OF_LIBRARY:=nolib
LIST_OF_FREQUENCY:=EU
LIST_OF_CODE_MEMORY_MODE=nonbanking
LIST_OF_SENSOR_TYPE:=NON_BATT
LIST_OF_MAKESCHEME:=NO_SCHEME
LIST_OF_WATCHDOG:=WATCHDOG_DISABLED
endif
ifneq ($(BOOTLOADER_ENABLED),)
# Only generate UART OTA targets - for now
LIST_OF_HOST_INTERFACE:=UART
endif
#---------------------------------------------------------------------------------------------------------

CDEFINES+=,\
 $(SENSOR_TYPE),\
 $(BOARD)

#Test if we should build with security
ifneq ($(SCHEME),)
CDEFINES+=,\
 SECURITY=$(SCHEME),\
 $(SCHEME),\
 AES_ON_CHIP
ADEFINES+=,\
 SECURITY=$(SCHEME),\
 $(SCHEME)
endif
