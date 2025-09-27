set(ASCONC_AEAD_DIR ${CMAKE_CURRENT_LIST_DIR}/ascon-c/crypto_aead/asconaead128)

file(GLOB ASCONC_ESP32_SRCS
    ${ASCONC_AEAD_DIR}/esp32/*.c
)

idf_component_register(
    SRCS ${ASCONC_ESP32_SRCS} "ascon_wrapper.c"
    INCLUDE_DIRS "include" ${ASCONC_AEAD_DIR}/esp32
)
