# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)

project(tcp_client)

if(CONFIG_EXAMPLE_IPV4)
    target_link_libraries(${project_elf} PRIVATE
        "-Wl,--wrap=ip4_output_if"
        "-Wl,-u,__wrap_ip4_output_if")
elseif(CONFIG_EXAMPLE_IPV6)
    target_link_libraries(${project_elf} PRIVATE
        "-Wl,--wrap=ip6_output_if"
        "-Wl,-u,__wrap_ip6_output_if")
endif()
