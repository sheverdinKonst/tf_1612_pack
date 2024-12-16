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
    MODULE_NAME         := rtusr
    MODULE_DIR          := module/$(MODULE_NAME)/a
    BUILD_OUTPUT_DIR    := build

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -DRTUSR -I$(ROOT_PATH)/sdk/include  -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/system/linux \
							-I$(ROOT_PATH)/sdk/include/dal -I$(ROOT_PATH)/sdk/src/app/diag_v2/include
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.rtrpc
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/sdk/src/dal/rtrpc/,$(RTRPC_SRC))
    MODULE_OBJ          := $(addprefix $(BUILD_OUTPUT_DIR)/sdk/src/dal/rtrpc/,$(subst .c,.o,$(notdir $(RTRPC_SRC))))



    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := no
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif

    include             $(ROOT_PATH)/make/Makefile.lib
