add_executable(lsfg-vk-device-features-test
    device_features_test.cpp)

set_target_properties(lsfg-vk-device-features-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)

target_include_directories(lsfg-vk-device-features-test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/framegen/include)
target_link_libraries(lsfg-vk-device-features-test PRIVATE volk)

add_test(NAME lsfg-vk-device-features
    COMMAND lsfg-vk-device-features-test)

add_executable(lsfg-vk-device-features-runtime-test
    device_features_runtime_test.cpp)

set_target_properties(lsfg-vk-device-features-runtime-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)

target_include_directories(lsfg-vk-device-features-runtime-test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/framegen/include)
target_link_libraries(lsfg-vk-device-features-runtime-test PRIVATE volk)

add_test(NAME lsfg-vk-device-features-runtime
    COMMAND lsfg-vk-device-features-runtime-test)
set_tests_properties(lsfg-vk-device-features-runtime
    PROPERTIES SKIP_RETURN_CODE 77)

add_executable(lsfg-vk-layer-exports-test
    layer_exports_test.cpp)

set_target_properties(lsfg-vk-layer-exports-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_link_libraries(lsfg-vk-layer-exports-test PRIVATE ${CMAKE_DL_LIBS})

add_test(NAME lsfg-vk-layer-exports
    COMMAND lsfg-vk-layer-exports-test $<TARGET_FILE:lsfg-vk>)
set_tests_properties(lsfg-vk-layer-exports
    PROPERTIES
        ENVIRONMENT "LSFG_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/valid_layer_config.toml;LSFG_PROCESS=lsfg-vk-layer-exports-test")

add_executable(lsfg-vk-layer-device-test
    layer_device_test.cpp)

set_target_properties(lsfg-vk-layer-device-test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON)
target_link_libraries(lsfg-vk-layer-device-test PRIVATE volk)

add_test(NAME lsfg-vk-layer-device
    COMMAND lsfg-vk-layer-device-test)
set_tests_properties(lsfg-vk-layer-device
    PROPERTIES
        SKIP_RETURN_CODE 77
        ENVIRONMENT "VK_INSTANCE_LAYERS=VK_LAYER_LS_frame_generation;LSFG_CONFIG=${CMAKE_CURRENT_SOURCE_DIR}/valid_enabled_layer_config.toml;LSFG_PROCESS=lsfg-vk-layer-device-test")
