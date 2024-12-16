cmake_minimum_required(VERSION 3.25)
project(tf_autorestart C)
set(CMAKE_C_STANDARD 17)

add_compile_options()

add_library(socket_module STATIC
    lib/socket_lib/socket_module.c
    lib/socket_lib/socket_module.h
)

add_library(thread_module STATIC
    lib/thread_lib/thread_module.c
    lib/thread_lib/thread_module.h
)

add_library(utils_module STATIC
    lib/utils_lib/utils_module.c
    lib/utils_lib/utils_module.h
)

# Add the include directories
target_include_directories(socket_module PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(thread_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(utils_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)

# Create the project_2 executable
add_executable(tf_autorestartd
    autorestartd/ar_maind.c
    autorestartd/ar_maind.h
    autorestartd/ar_mainApp.c
    autorestartd/ar_mainApp.h
    autorestartd/client_module.c
    autorestartd/client_module.h
    autorestartd/events_module.c
    autorestartd/events_module.h
    autorestartd/init_module.c
    autorestartd/init_module.h
    autorestartd/test_module.c
    autorestartd/test_module.h
    autorestartd/port_module.c
    autorestartd/port_module.h
    autorestartd/ping.c
    autorestartd/ping.h
)

target_link_libraries(tf_autorestartd  socket_module thread_module utils_module jansson)

INSTALL(TARGETS tf_autorestartd RUNTIME DESTINATION /usr/bin/)

add_executable(tf_autorestart_ctrl
    autorestart_ctrl/ar_main_ctrl.c
    autorestart_ctrl/client_module.c
    autorestart_ctrl/client_module.h
    autorestart_ctrl/parsing_command.c
    autorestart_ctrl/parsing_command.h
)

target_link_libraries(tf_autorestart_ctrl
    PRIVATE
    socket_module
    thread_module
    utils_module
    jansson
)

INSTALL(TARGETS tf_autorestart_ctrl RUNTIME DESTINATION /usr/bin/)