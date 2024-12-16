cmake_minimum_required(VERSION 3.26)
project(tf_rtk_test C)

set(CMAKE_C_STANDARD 17)
include_directories(sdk/system sdk/system/include sdk/include/dal/rtrpc sdk/include/common sdk/include sdk/src)

add_executable(tf_rtk_test
        sdk/src/main.c
        sdk/src/dal/rtrpc/rtrpc_diag.c
        sdk/src/dal/rtrpc/rtrpc_msg.c

        sdk/system/osal.c

        sdk/include/rtk/phy.h
        sdk/include/rtk/time.h
        sdk/include/rtk/sds.h

        sdk/include/common/rt_type.h
        sdk/include/common/rt_error.h
        sdk/include/common/util/rt_bitop.h
        sdk/include/common/hal/chipdef/chipdef.h
        sdk/include/common/hal/chipdef/chip.h

        sdk/include/dal/rtrpc/rtrpc_msg.h
        sdk/include/dal/dal_mapper.h

        sdk/system/rtdrv/rtdrv_netfilter.c
        sdk/system/rtdrv/rtdrv_netfilter.h

        sdk/system/include/osal/time.h
        sdk/system/include/osal/memory.h
        sdk/system/include/osal/print.h
        sdk/system/include/osal/lib.h
        sdk/system/include/osal/sem.h

        sdk/system/include/common/error.h
        sdk/system/include/common/debug/rt_log.h

        sdk/include/dal/dal_mgmt.h
        )

INSTALL(TARGETS tf_rtk_test RUNTIME DESTINATION /usr/bin/)
