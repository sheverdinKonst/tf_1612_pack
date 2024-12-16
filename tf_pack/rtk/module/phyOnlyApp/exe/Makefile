# Copyright (C) 2016 Realtek Semiconductor Corp.
#
# Makefile for Application
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings
  include             $(ROOT_PATH)/sdk/config/.config

  PHY_ONLY := y

  #******************************************************
  # module
  #******************************************************
    MODULE_NAME         := phyOnlyApp
    MODULE_DIR          := module/$(MODULE_NAME)/exe
    MODULE_SRC_PATH     := $(ROOT_PATH)/module/$(MODULE_NAME)/src
    BUILD_OUTPUT_DIR    := build
    MODULE_SHARED_LIB   := -L$(ROOT_PATH)/module/phy/a -lphy -lpthread

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/module/phy/inc
    EXTRA_CFLAGS        += -I$(ROOT_PATH)/sdk/system/include -I$(ROOT_PATH)/sdk/include \
                           -I$(ROOT_PATH)/sdk/src/app/diag_v2/include \
                           -I$(ROOT_PATH)/sdk/system/linux
    EXTRA_CFLAGS        += -I$(MODULE_SRC_PATH)
    EXTRA_CFLAGS        += -DPHY_ONLY

    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.diag
    include             $(MODULE_SRC_PATH)/Makefile

    MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(PHYONLYAPP_OBJ)))
    MODULE_SRC          += $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(DIAG_OBJ)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(addprefix $(BUILD_OUTPUT_DIR)/, $(PHYONLYAPP_OBJ))
    MODULE_OBJ          += $(addprefix $(BUILD_OUTPUT_DIR)/, $(DIAG_OBJ))
    vpath %.c $(ALL_SRC_PATH)


#-----------------------------------------------------------------------------
# Build Application
#-----------------------------------------------------------------------------
    executable          := yes
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif

    include             $(ROOT_PATH)/make/Makefile.lib


