#
# Copyright (C) 2017 Realtek Semiconductor Corp.
#
# Makefile for shared lib
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings

  #******************************************************
  # module
  #******************************************************
    BUILD_OUTPUT_DIR    := build
    MODULE_NAME         := rtcore
    MODULE_DIR          := module/$(MODULE_NAME)/a


  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        +=
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    SDK_CONFIG          := $(ROOT_PATH)/sdk/config/.config
    SDK_BUILD           := $(ROOT_PATH)/sdk/build
    include             $(ROOT_PATH)/sdk/build/Makefile.rtcore
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(RTCORE_OBJ)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(RTCORE_OBJ)
    vpath %.c $(ALL_SRC_PATH)


#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := no
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lib
