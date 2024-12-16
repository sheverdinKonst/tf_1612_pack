#
# Copyright (C) 2017 Realtek Semiconductor Corp.
#
# Makefile for Linux Kernel Module
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings

  #******************************************************
  # module
  #******************************************************
    MODULE_NAME         := rtnic
    MODULE_DIR          := module/$(MODULE_NAME)/lkm
    KBUILD_EXTRA_SYMBOLS += $(ROOT_PATH)/module/rtcore/lkm/Module.symvers
    KBUILD_EXTRA_SYMBOLS += $(ROOT_PATH)/module/rtk/lkm/Module.symvers
    export KBUILD_EXTRA_SYMBOLS

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(DRV_INCLUDE) -I$(SDK_INCLUDE) -I$(SYS_INCLUDE)
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    MODULE_OBJ          := sdk/system/linux/rtnic/rtnic_drv.o
    CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE:=y
#    include             $(ROOT_PATH)/sdk/build/Makefile.rtnic
#    MODULE_OBJ          += $(addprefix $(ROOT_PATH)/sdk/system/linux/rtdrv/,$(RTK_ALL_OBJ))
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))


#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lkm


