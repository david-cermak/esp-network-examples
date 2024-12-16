set(PLCTAG_VERSION "c55bc5876d938dda1c609750cde5ae4812d7b8a8")
set(PLCTAG_URL "https://github.com/libplctag/libplctag/archive/${PLCTAG_VERSION}.zip")

set(plctag_dir ${CMAKE_BINARY_DIR}/libplctag/libplctag-${PLCTAG_VERSION})

# Fetch the library
if(NOT EXISTS ${plctag_dir})
        message(STATUS "Downloading libplctag ${PLCTAG_VERSION}...")
        file(DOWNLOAD ${PLCTAG_URL} ${CMAKE_BINARY_DIR}/libplctag.zip SHOW_PROGRESS)
        execute_process(COMMAND unzip -o ${CMAKE_BINARY_DIR}/libplctag.zip -d ${CMAKE_BINARY_DIR}/libplctag
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
endif()

set ( base_SRC_PATH "${plctag_dir}/src" )
set ( lib_SRC_PATH "${base_SRC_PATH}/lib" )
set ( protocol_SRC_PATH "${base_SRC_PATH}/protocols" )
set ( ab_SRC_PATH "${protocol_SRC_PATH}/ab" )
set ( mb_SRC_PATH "${protocol_SRC_PATH}/mb" )
set ( util_SRC_PATH "${base_SRC_PATH}/util" )
set ( example_SRC_PATH "${base_SRC_PATH}/examples" )
set ( test_SRC_PATH "${base_SRC_PATH}/tests" )
set ( cli_SRC_PATH "${base_SRC_PATH}/contrib/cli" )
set ( platform_SRC_PATH "${base_SRC_PATH}/platform/posix" )

set ( libplctag_SRCS "${lib_SRC_PATH}/init.c"
        "${lib_SRC_PATH}/init.h"
        "${lib_SRC_PATH}/libplctag.h"
        "${lib_SRC_PATH}/lib.c"
        "${lib_SRC_PATH}/tag.h"
        "${lib_SRC_PATH}/version.h"
        "${lib_SRC_PATH}/version.c"
        "${ab_SRC_PATH}/ab.h"
        "${ab_SRC_PATH}/ab_common.c"
        "${ab_SRC_PATH}/ab_common.h"
        "${ab_SRC_PATH}/cip.c"
        "${ab_SRC_PATH}/cip.h"
        "${ab_SRC_PATH}/defs.h"
        "${ab_SRC_PATH}/eip_cip.c"
        "${ab_SRC_PATH}/eip_cip.h"
        "${ab_SRC_PATH}/eip_cip_special.c"
        "${ab_SRC_PATH}/eip_cip_special.h"
        "${ab_SRC_PATH}/eip_lgx_pccc.c"
        "${ab_SRC_PATH}/eip_lgx_pccc.h"
        "${ab_SRC_PATH}/eip_plc5_dhp.c"
        "${ab_SRC_PATH}/eip_plc5_dhp.h"
        "${ab_SRC_PATH}/eip_plc5_pccc.c"
        "${ab_SRC_PATH}/eip_plc5_pccc.h"
        "${ab_SRC_PATH}/eip_slc_dhp.c"
        "${ab_SRC_PATH}/eip_slc_dhp.h"
        "${ab_SRC_PATH}/eip_slc_pccc.c"
        "${ab_SRC_PATH}/eip_slc_pccc.h"
        "${ab_SRC_PATH}/error_codes.c"
        "${ab_SRC_PATH}/error_codes.h"
        "${ab_SRC_PATH}/pccc.c"
        "${ab_SRC_PATH}/pccc.h"
        "${ab_SRC_PATH}/session.c"
        "${ab_SRC_PATH}/session.h"
        "${ab_SRC_PATH}/tag.h"
        "${protocol_SRC_PATH}/omron/omron.h"
        "${protocol_SRC_PATH}/omron/omron_common.c"
        "${protocol_SRC_PATH}/omron/omron_common.h"
        "${protocol_SRC_PATH}/omron/cip.c"
        "${protocol_SRC_PATH}/omron/cip.h"
        "${protocol_SRC_PATH}/omron/conn.c"
        "${protocol_SRC_PATH}/omron/conn.h"
        "${protocol_SRC_PATH}/omron/defs.h"
        "${protocol_SRC_PATH}/omron/omron_raw_tag.c"
        "${protocol_SRC_PATH}/omron/omron_raw_tag.h"
        "${protocol_SRC_PATH}/omron/omron_standard_tag.c"
        "${protocol_SRC_PATH}/omron/omron_standard_tag.h"
        "${protocol_SRC_PATH}/omron/tag.h"
        "${mb_SRC_PATH}/modbus.c"
        "${mb_SRC_PATH}/modbus.h"
        "${protocol_SRC_PATH}/system/system.c"
        "${protocol_SRC_PATH}/system/system.h"
        "${protocol_SRC_PATH}/system/tag.h"
        "${util_SRC_PATH}/atomic_int.c"
        "${util_SRC_PATH}/atomic_int.h"
        "${util_SRC_PATH}/attr.c"
        "${util_SRC_PATH}/attr.h"
        "${util_SRC_PATH}/byteorder.h"
        "${util_SRC_PATH}/debug.c"
        "${util_SRC_PATH}/debug.h"
        "${util_SRC_PATH}/hash.c"
        "${util_SRC_PATH}/hash.h"
        "${util_SRC_PATH}/hashtable.c"
        "${util_SRC_PATH}/hashtable.h"
        "${util_SRC_PATH}/macros.h"
        "${util_SRC_PATH}/rc.c"
        "${util_SRC_PATH}/rc.h"
        "${util_SRC_PATH}/vector.c"
        "${util_SRC_PATH}/vector.h"
        "${platform_SRC_PATH}/platform.c"
        "${platform_SRC_PATH}/platform.h" )

idf_component_register(SRCS         ${libplctag_SRCS}
                       INCLUDE_DIRS "include" "include/lib"  "${base_SRC_PATH}"
                       PRIV_INCLUDE_DIRS "${platform_SRC_PATH}" "${protocol_SRC_PATH}"
                       REQUIRES esp_netif
                       PRIV_REQUIRES sock_utils)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-char-subscripts")
