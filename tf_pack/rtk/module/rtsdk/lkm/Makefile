#
# Copyright (C) 2017 Realtek Semiconductor Corp.
#
# Makefile for Linux Kernel Module
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings
  include             $(ROOT_PATH)/sdk/config/.config

  #******************************************************
  # module
  #******************************************************
    MODULE_NAME         := rtsdk
    MODULE_DIR          := module/$(MODULE_NAME)/lkm

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/include -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/system/linux \
                           -I$(KERNEL_PATH)/include -DCONFIG_SDK_SINGLE_LKM
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build

    include             $(ROOT_PATH)/sdk/build/Makefile.rtsdk_lkm
	MODULE_OBJ          += $(arch_A_lib)

    MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_drv.o
    ifeq ($(CONFIG_SDK_DRIVER_EXTC_PCI),y)
        MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_pci.o
        ifeq ($(CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE),y)
            ifneq ($(CONFIG_SDK_DRIVER_RTNIC),)
                MODULE_OBJ          += sdk/system/linux/rtnic/rtnic_drv.o
                EXTRA_CFLAGS        += -DCONFIG_SDK_DRIVER_RTNIC
            endif
        endif
    endif
    ifeq ($(CONFIG_SDK_DRIVER_UIO),y)
        MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_uio.o
    endif
	ifneq ($(CONFIG_SDK_EXTERNAL_CPU),y)
	    MODULE_OBJ          += sdk/system/linux/dev_config.o
	endif
	MODULE_OBJ			+= sdk/system/linux/rtk/module.o
	MODULE_OBJ			+= sdk/system/linux/rtsdk_lkm_start.o

    MODULE_SRC          += $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))

	ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
	vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lkm


