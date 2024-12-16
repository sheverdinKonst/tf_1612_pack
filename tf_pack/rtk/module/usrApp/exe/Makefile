# Copyright (C) 2016 Realtek Semiconductor Corp.
#
# Makefile for Application
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings
  include             $(ROOT_PATH)/sdk/config/.config

  #******************************************************
  # module
  #******************************************************
    MODULE_NAME         := usrApp
    MODULE_DIR          := module/$(MODULE_NAME)/exe
    BUILD_OUTPUT_DIR    := build
ifeq ($(CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE),y)
	MODULE_SHARED_LIB   := -L$(ROOT_PATH)/module/rtusr/a -lrtusr -lpthread -lm
else
    MODULE_SHARED_LIB   := -L$(ROOT_PATH)/module/rtsdk/a -lrtsdk -lpthread -lm
endif

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/include \
                           -I$(ROOT_PATH)/sdk/src/app/diag_v2/include \
                           -I$(ROOT_PATH)/sdk/system/linux
ifeq ($(SDK_TARGET_ARCH),TYPE_ARM_CA)
    EXTRA_LDFLAGS       += --sysroot=/home/sdk/g3hgu-eng_gpon.sdk-image.R0.7.27.0.sdk/build/tmp/sysroots/g3hgu-eng
else
    EXTRA_LDFLAGS       +=
endif

  #******************************************************
  # object files
  #******************************************************
ifeq ($(CONFIG_SDK_KERNEL_LINUX_KERNEL_MODE),y)
    EXTRA_CFLAGS        += -DRTUSR -I$(ROOT_PATH)/sdk/include/dal
    include $(ROOT_PATH)/sdk/build/Makefile.diag
    USRAPP_OBJ          := $(DIAG_OBJ)
else
    USRAPP_OBJ          := sdk/system/linux/usrAppStart.o
endif
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(USRAPP_OBJ)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(USRAPP_OBJ)
    vpath %.c $(ALL_SRC_PATH)


#-----------------------------------------------------------------------------
# Build Application
#-----------------------------------------------------------------------------
    executable          := yes
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif

    include             $(ROOT_PATH)/make/Makefile.lib


