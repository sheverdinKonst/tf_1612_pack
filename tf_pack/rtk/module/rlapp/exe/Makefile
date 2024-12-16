# Copyright (C) 2016 Realtek Semiconductor Corp.
#
# Makefile for Application
#

#-----------------------------------------------------------------------------
# Customization per module
#-----------------------------------------------------------------------------
  include             $(ROOT_PATH)/make/Makefile.settings
    
  #******************************************************
  # module
  #******************************************************
    MODULE_NAME         := rlapp
    MODULE_DIR          := module/$(MODULE_NAME)/exe
    MODULE_SRC_PATH     := $(ROOT_PATH)/module/$(MODULE_NAME)/src
    MODULE_SHARED_LIB   := -l$(MODULE_NAME) -L$(ROOT_PATH)/module/$(MODULE_NAME)/so \
                           -lrtk -L$(ROOT_PATH)/module/rtk/so \
                           -lrtcore -L$(ROOT_PATH)/module/rtcore/so \
                           -lpthread

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        += -I$(MODULE_SRC_PATH)
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(MODULE_SRC_PATH)/Makefile
    MODULE_OBJ          := $(ROOT_PATH)/$(MODULE_DIR)/$(MODULE_NAME)_main.o

#-----------------------------------------------------------------------------
# Build Application
#-----------------------------------------------------------------------------
    executable          := yes
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lib
