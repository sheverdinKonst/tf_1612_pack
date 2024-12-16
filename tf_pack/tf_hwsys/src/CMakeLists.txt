cmake_minimum_required(VERSION 3.25)
project(tf_hwsys C)
set(CMAKE_C_STANDARD 17)
# Add the libraries

#set(CMAKE_BUILD_TYPE Debug)
#set(CMAKE_C_FLAGS_DEBUG -g)

add_library(i2c_module STATIC
    lib/i2c_lib/i2c_module.c
    lib/i2c_lib/i2c_module.h
)

add_library(events_module STATIC
    lib/events_lib/events_handlers.c
    lib/events_lib/events_handlers.h
)

add_library(gpio_module STATIC
    lib/gpio_lib/gpio_module.c
    lib/gpio_lib/gpio_module.h
)

add_library(socket_module STATIC
    lib/socket_lib/socket_module.c
    lib/socket_lib/socket_module.h
)
add_library(utils_module STATIC
    lib/utils_lib/utils_module.c
    lib/utils_lib/utils_module.h
)

add_library(thread_module STATIC
    lib/thread_lib/thread_module.c
    lib/thread_lib/thread_module.h
)

# Add the include directories
target_include_directories(i2c_module    PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(events_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(gpio_module   PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(socket_module PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(utils_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(thread_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)

# Create the project_2 executable
add_executable(tf_hwsysd
        hwsysd/hwsysd.c
        hwsysd/hwsysd.h
        hwsysd/client_comm.c
        hwsysd/client_comm.h
        hwsysd/main_app.c
        hwsysd/main_app.h
        hwsysd/global_include.h)
target_link_libraries(tf_hwsysd i2c_module events_module gpio_module socket_module utils_module thread_module rt gpiod)
INSTALL(TARGETS tf_hwsysd RUNTIME DESTINATION /usr/bin/)


add_executable(tf_hwsys_ctrl
        hwsys_ctrl/hwsys_ctrl.c
        hwsys_ctrl/hwsys_ctrl.h
        hwsysd/global_include.h)

target_link_libraries(tf_hwsys_ctrl
        PRIVATE
        i2c_module
        events_module
        gpio_module
        socket_module
        utils_module
        thread_module
        rt
        gpiod)
INSTALL(TARGETS tf_hwsys_ctrl RUNTIME DESTINATION /usr/bin/)