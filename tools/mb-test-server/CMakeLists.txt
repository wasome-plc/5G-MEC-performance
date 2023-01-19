cmake_minimum_required (VERSION 2.8.3)
message ("modbus-test-server..." )
project (modbus-test-server)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-literal-suffix ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wno-pointer-sign -Wno-unused-function")
add_definitions(-Wall  -D_GNU_SOURCE=1 )

set (IDRM_BASE  ${CMAKE_CURRENT_LIST_DIR}/../../../..)
set(EX_CJSON_DIR ${IDRM_BASE}/sdk/external/cJSON)
set (LIB_MODBUS_SOURCE  ${CMAKE_CURRENT_LIST_DIR}/../../../external/libmodbus/src)

include_directories(${CMAKE_CURRENT_LIST_DIR}/../common)
include_directories(${EX_CJSON_DIR} ${LIB_MODBUS_SOURCE} ${CMAKE_CURRENT_LIST_DIR})

include(${IDRM_BASE}/sdk/external/util_deps.cmake)
include(${IDRM_BASE}/sdk/idr-mgt-shared-libs/lib/ssg_lib.cmake)
include(${IDRM_BASE}/sdk/idr-mgt-shared-libs/utils/ssg_utils.cmake)
include(${IDRM_BASE}/sdk/idr-mgt-shared-libs/AMS-SDK/ex_ams.cmake)
#include(${IDRM_BASE}/sdk/app-sdk/c-api/client-api.cmake)

set(LIB_MODBUS_SOURCE
    ${LIB_MODBUS_SOURCE}/modbus.c
    ${LIB_MODBUS_SOURCE}/modbus-data.c
    ${LIB_MODBUS_SOURCE}/modbus-rtu.c
    ${LIB_MODBUS_SOURCE}/modbus-tcp.c
)

SET(SOURCES
    ${SSG_LIB_SOURCE}
    ${SOURCE_HIREDIS}
    ${SSG_UTILS_SOURCE}
    ${LIB_MODBUS_SOURCE}
    ${EX_PATH_SOURCE}
    ${EX_CJSON_DIR}/cJSON.c
    test_server.c
)

add_executable(modbus_test_server ${SOURCES})
target_link_libraries(modbus_test_server pthread dl)
