#
# Copyright (C) 2017 Realtek Semiconductor Corp.
#
# Makefile for shared lib
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
    MODULE_DIR          := module/$(MODULE_NAME)/a
    BUILD_OUTPUT_DIR    := build

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/src/app/diag_v2/include
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    include             $(ROOT_PATH)/sdk/build/Makefile.rtsdk
    arch_B_lib          += sdk/system/linux/rtcore/user/rtcore_drv_usr.o
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(arch_B_lib)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(arch_B_lib)
    vpath %.c $(ALL_SRC_PATH)


#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := no
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lib
