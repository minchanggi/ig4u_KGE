project(CFE_IG4U_APP C)

set(APP_SRC_FILES
  fsw/src/ig4u_task.c
  fsw/src/ig4u_cmds.c
  fsw/src/ig4u_dispatch.c
  fsw/src/ig4u_protocol.c
  fsw/src/ig4u_commands.c
  fsw/src/ig4u_hal.c
)

# Create the app module
add_cfe_app(ig4u ${APP_SRC_FILES})

target_include_directories(ig4u PUBLIC
  fsw/inc
  config
  ../../cfe/modules/srl/fsw/src
  ../../submodules/libgscsp/lib/libftp_client/include
  ../../submodules/libgscsp/lib/libgscsp/include
  ../../submodules/libgscsp/lib/libgscsp/lib/libcsp/include
  ../../submodules/libgscsp/lib/libparam_client/include
  ../../submodules/libgscsp/lib/libparam_client/include/deprecated
  ../../submodules/libgscsp/lib/libutil/include
  ../../submodules/libgscsp/build/lib/libgscsp/lib/libcsp/include
)

target_compile_options(ig4u PRIVATE -Wno-pedantic -Wno-error=pedantic)
target_compile_definitions(ig4u PRIVATE GS_PARAM_INTERNAL_USE=1)

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "arm")
    # OBC build
    target_link_libraries(ig4u ${PROJECT_SOURCE_DIR}/../../submodules/libgscsp/build/obc/lib/libcsp-client.a)
else()
    # CPU1 build
    target_link_libraries(ig4u ${PROJECT_SOURCE_DIR}/../../submodules/libgscsp/build/cpu1/lib/libcsp-client.a)
endif()
