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
    MODULE_NAME         := rtusr
    MODULE_DIR          := module/$(MODULE_NAME)/so
    BUILD_OUTPUT_DIR    := build

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -DRTUSR -I$(ROOT_PATH)/sdk/system/linux -I$(ROOT_PATH)/sdk/include -I$(ROOT_PATH)/sdk/system/include
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.rtrpc
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/sdk/src/dal/rtrpc/,$(RTRPC_SRC))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(addprefix sdk/src/dal/rtrpc/,$(subst .c,.o,$(notdir $(RTRPC_SRC))))
    vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := yes
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif

    include             $(ROOT_PATH)/make/Makefile.lib
