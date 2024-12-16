cmake_minimum_required(VERSION 3.26)
project(tf_device_monitor C)

set(CMAKE_C_STANDARD 11)

add_executable(tf_device_monitor main.c
                         net_utils.c
                         net_utils.h
                         dm_mainHeader.h
                         run_CLIcmd.c
                         run_CLIcmd.h
                         parsing.c
                         parsing.h
                         state_handler.c
                         state_handler.h
                         settings_module.c
                         settings_module.h
                         version.h
                        )

target_link_libraries(tf_device_monitor jansson)

INSTALL(TARGETS tf_device_monitor RUNTIME DESTINATION /usr/bin/)
