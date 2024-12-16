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
    MODULE_NAME         := rtk
    MODULE_DIR          := module/$(MODULE_NAME)/so

  #******************************************************
  # FLAG
  #******************************************************
    EXTRA_CFLAGS        +=
    EXTRA_LDFLAGS       +=

  #******************************************************
  # object files
  #******************************************************
    include             $(ROOT_PATH)/sdk/build/Makefile.rtk
    MODULE_SRC          := $(addprefix $(ROOT_PATH)/sdk/src/,$(subst .o,.c,$(RTK_ALL_OBJ)))
    ALL_SRC_PATH        := $(sort $(dir $(MODULE_SRC)))
    MODULE_OBJ          := $(subst .c,.o,$(subst $(ROOT_PATH)/,,$(MODULE_SRC)))
    vpath %.c $(ALL_SRC_PATH)

#-----------------------------------------------------------------------------
# Build lib
#-----------------------------------------------------------------------------
    sharedlib           := yes
    ifeq ($(ROOT_PATH),)
        ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
    endif
    include             $(ROOT_PATH)/make/Makefile.lib
