zephyr_library()

zephyr_library_sources(
    src/input_processor_continuous_temp_layer.c
)

zephyr_include_directories(
    ${ZEPHYR_BASE}/include
    ${CMAKE_SOURCE_DIR}/include
)
