# Copyright (C) 2016 Realtek Semiconductor Corp.
#
# Makefile for SDK Linux Kernel Module
#

#-----------------------------------------------------------------------------
# module_name:
#-----------------------------------------------------------------------------
MODULE_NAME         := unittest
MODULE_DIR          := app/unittest/lkm


#-----------------------------------------------------------------------------
# parameters
#-----------------------------------------------------------------------------
ifeq ($(ROOT_PATH),)
    ROOT_PATH       := $(patsubst %/$(MODULE_DIR),%,$(shell pwd))
endif
include             $(ROOT_PATH)/make/Makefile.settings
include             $(SDK_CONFIG)


#-----------------------------------------------------------------------------
# FLAGS
#-----------------------------------------------------------------------------
EXTRA_CFLAGS        += -I$(ROOT_PATH)/app/unittest/src/dal -I$(ROOT_PATH)/app/unittest/src/common -I$(ROOT_PATH)/app/unittest/src/sdk -DRTK_LKM
EXTRA_CFLAGS        += -DCONFIG_SDK_DRIVER_TEST_MODULE -DCONFIG_SDK_DRIVER_TEST
EXTRA_LDFLAGS       +=




#-----------------------------------------------------------------------------
# object files
#-----------------------------------------------------------------------------
include             $(ROOT_PATH)/app/unittest/src/Makefile
MODULE_OBJ          := $(MODULE_DIR)/sdk_test_module.o
MODULE_OBJ          += $(addprefix app/unittest/src/sdk/,$(SDK_OBJ))
MODULE_OBJ          += $(addprefix app/unittest/src/dal/,$(DAL_OBJ))
MODULE_OBJ          += $(addprefix app/unittest/src/common/,$(COMMON_OBJ))

MODULE_SRC          := $(addprefix $(ROOT_PATH)/,$(subst .o,.c,$(MODULE_OBJ)))

#-----------------------------------------------------------------------------
# Build Linux Kernel Module
#-----------------------------------------------------------------------------
include             $(ROOT_PATH)/make/Makefile.lkm


