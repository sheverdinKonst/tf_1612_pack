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
    MODULE_NAME         := rtk
    MODULE_DIR          := module/$(MODULE_NAME)/lkm

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(DRV_INCLUDE)
    EXTRA_LDFLAGS       +=
    KBUILD_EXTRA_SYMBOLS += $(ROOT_PATH)/module/rtcore/lkm/Module.symvers
    export KBUILD_EXTRA_SYMBOLS

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    MODULE_OBJ          := sdk/system/linux/rtk/module.o
    CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE:=y
    export SDK_TOP_DIR  := $(ROOT_PATH)
    include             $(ROOT_PATH)/sdk/build/Makefile.rtk
    include             $(ROOT_PATH)/sdk/build/Makefile.phy
    MODULE_OBJ          += $(addprefix sdk/src/,$(RTK_ALL_OBJ) $(PHY_OBJ))
    #MODULE_OBJ          += $(ROOT_PATH)/module/phy/a/libphy.a
	MODULE_SRC          += $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))

#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lkm


