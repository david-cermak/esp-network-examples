cmake_minimum_required(VERSION 3.16)

set(srs_dir bacnet-stack/bacnet-stack/src)
set(demo_dir bacnet-stack/bacnet-stack/demo/handler)
set(port_dir bacnet-stack/bacnet-stack/ports/esp32/src)
#set(port_lwip_dir bacnet-stack/bacnet-stack/ports/lwip)

idf_component_register(SRCS
        ${srs_dir}/abort.c
        ${srs_dir}/address.c
        ${srs_dir}/apdu.c
        ${srs_dir}/bacaddr.c
        ${srs_dir}/bacapp.c
        ${srs_dir}/bacdcode.c
        ${srs_dir}/bacerror.c
        ${srs_dir}/bacint.c
        ${srs_dir}/bacreal.c
        ${srs_dir}/bacstr.c
        ${srs_dir}/bip.c
        ${srs_dir}/bvlc.c
        ${srs_dir}/cov.c
        ${srs_dir}/datetime.c
        ${srs_dir}/bacdevobjpropref.c
        ${srs_dir}/dcc.c
        ${srs_dir}/debug.c
        ${demo_dir}/h_cov.c
        ${demo_dir}/h_ucov.c
        ${demo_dir}/h_npdu.c
        ${demo_dir}/h_rp.c
        ${demo_dir}/h_rpm.c
        ${demo_dir}/h_whois.c
        ${demo_dir}/h_wp.c
        ${srs_dir}/iam.c
        ${srs_dir}/lighting.c
        ${srs_dir}/memcopy.c
        ${demo_dir}/noserv.c
        ${srs_dir}/npdu.c
        ${srs_dir}/proplist.c
        ${srs_dir}/reject.c
        ${srs_dir}/rp.c
        ${srs_dir}/rpm.c
        ${demo_dir}/s_iam.c
        ${srs_dir}/tsm.c
        ${srs_dir}/whois.c
        ${srs_dir}/wp.c
        ${port_dir}/ai.c
#        ${port_dir}/bip_init.c
        ${port_dir}/bo.c
        ${port_dir}/device.c
        #${port_dir}/main.c
        INCLUDE_DIRS include bacnet-stack/bacnet-stack/include
        ${port_dir}
        REQUIRES main)
#        ${port_lwip_dir})
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
target_compile_definitions(${COMPONENT_LIB} PRIVATE "-DMAX_TSM_TRANSACTIONS=10" "-DMAX_ADDRESS_CACHE=10")