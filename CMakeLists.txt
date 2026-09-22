zephyr_library()

if(CONFIG_ZMK_SPLIT_ROLE_CENTRAL OR NOT CONFIG_ZMK_SPLIT)
    zephyr_library_sources(
        src/input_processor_continuous_temp_layer.c
    )
endif()

zephyr_include_directories(
    ${ZEPHYR_BASE}/include
    ${CMAKE_SOURCE_DIR}/include
)
