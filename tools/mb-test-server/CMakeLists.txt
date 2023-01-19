cmake_minimum_required (VERSION 2.8.3)
message ("modbus-test-server..." )
project (modbus-test-server)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -Wno-literal-suffix ")
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wno-pointer-sign -Wno-unused-function")
add_definitions(-Wall  -D_GNU_SOURCE=1 )

set (IDRM_BASE  ${CMAKE_CURRENT_LIST_DIR}/../../deps/iagent-sdk)
set (LIB_MODBUS_SOURCE  ${CMAKE_CURRENT_LIST_DIR}/../../deps/libmodbus/src)

include_directories(${LIB_MODBUS_SOURCE} ${CMAKE_CURRENT_LIST_DIR})

include(${IDRM_BASE}/external/util_deps.cmake)
include(${IDRM_BASE}/scripts/cmake/idrm_shared_libs.cmake)

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
    ${SOURCE_CJSON}
    test_server.c
)

add_executable(modbus_test_server ${SOURCES})
target_link_libraries(modbus_test_server pthread dl)
