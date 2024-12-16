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
    BUILD_OUTPUT_DIR    := build
    MODULE_NAME         := diag
    MODULE_DIR          := module/$(MODULE_NAME)/a
    MODULE_SRC_PATH     := module/$(MODULE_NAME)/src
    MODULE_SHARED_LIB   :=
    ifeq ($(CONFIG_SDK_RTSDN),y)
        MODULE_SHARED_LIB   += -L$(ROOT_PATH)/module/rtsdn/so -lrtsdn
    endif
    MODULE_EXTRA_TARGET := diag-tree


  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -DRTUSR -I$(ROOT_PATH)/sdk/system/linux -I$(ROOT_PATH)/sdk/include/dal \
                           -I$(ROOT_PATH)/sdk/include -I$(ROOT_PATH)/sdk/system/include \
                           -I$(ROOT_PATH)/sdk/src/app/diag_v2/include -I$(ROOT_PATH)/sdk/src/app/diag_v2/include/parser \
                           -I$(ROOT_PATH)/module  -DMAPLE_TEST
    ifeq ($(CONFIG_SDK_RTSDN),y)
        EXTRA_CFLAGS    += -I$(ROOT_PATH)/module/rtsdn/src/include
    endif
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.diag
    ALL_SRC_PATH        := $(sort $(dir $(DIAG_OBJ)))
    MODULE_OBJ          := $(DIAG_OBJ)
    vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build Application
#-----------------------------------------------------------------------------
    executable          := no
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif

    include             $(ROOT_PATH)/make/Makefile.lib
    include             $(ROOT_PATH)/sdk/build/Makefile.diagtree
