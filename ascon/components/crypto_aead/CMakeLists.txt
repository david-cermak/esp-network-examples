set(srcs)
set(reqs)

if(CONFIG_AEAD_IMPL_ASCON)
    list(APPEND srcs provider_ascon.c)
    list(APPEND reqs ascon)
elseif(CONFIG_AEAD_IMPL_AES_GCM)
    list(APPEND srcs provider_aes_gcm.c)
    list(APPEND reqs mbedtls)
elseif(CONFIG_AEAD_IMPL_CHACHA20_POLY1305)
    list(APPEND srcs provider_chacha20poly1305.c)
    list(APPEND reqs mbedtls)
elseif(CONFIG_AEAD_IMPL_MOCK)
    list(APPEND srcs provider_mock.c)
endif()

idf_component_register(
    SRCS ${srcs}
    INCLUDE_DIRS "include"
    REQUIRES ascon mbedtls
)
