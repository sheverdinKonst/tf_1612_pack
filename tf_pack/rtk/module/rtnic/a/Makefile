#
# Copyright (C) 2019 Realtek Semiconductor Corp.
#
# Makefile for static lib
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings
  include             $(ROOT_PATH)/sdk/config/.config

  #******************************************************
  # module
  #******************************************************
    BUILD_OUTPUT_DIR    := build
    MODULE_NAME         := rtnic
    MODULE_DIR          := module/$(MODULE_NAME)/a

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/system/linux
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.rtnic
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/sdk/system/drv/nic,$(subst .o,.c,$(NIC_OBJ)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(subst .c,.o,$(subst $(ROOT_PATH)/,,$(MODULE_SRC)))
    vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := no
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lib
