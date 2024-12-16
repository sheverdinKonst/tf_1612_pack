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
    MODULE_NAME         := rtcoreu_lkm
    MODULE_DIR          := module/$(MODULE_NAME)/lkm


  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/include -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/system/linux \
	                       -I$(KERNEL_PATH)/include
    EXTRA_CFLAGS        += -I$(SDK_KRN_INCLUDE)
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_drv.o
    ifeq ($(CONFIG_SDK_DRIVER_EXTC_PCI),y)
        MODULE_OBJ          += sdk/system/linux/rtcore/rtcore_pci.o
    endif

	ifneq ($(CONFIG_SDK_EXTERNAL_CPU),y)
	    MODULE_OBJ          += sdk/system/linux/dev_config.o
	endif

    include             $(ROOT_PATH)/sdk/build/Makefile.rtcoreu_lkm
    MODULE_OBJ          += $(USER_MODE_EXTRA)
    MODULE_SRC          += $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))

#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lkm


