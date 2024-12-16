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
    MODULE_NAME         := rtcore
    MODULE_DIR          := module/$(MODULE_NAME)/lkm

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(SDK_KRN_INCLUDE)
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    MODULE_OBJ          := sdk/system/linux/rtcore/rtcore_drv.o
    MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_export.o
ifeq ($(CONFIG_SDK_EXTERNAL_CPU),y)
else
    MODULE_OBJ          += sdk/system/linux/dev_config.o
endif
    CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE:=y
    include             $(ROOT_PATH)/sdk/build/Makefile.rtcore
    MODULE_OBJ          += $(RTCORE_OBJ)
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))

#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lkm


