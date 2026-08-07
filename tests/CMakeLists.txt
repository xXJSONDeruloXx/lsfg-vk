add_executable(lsfg-vk-runtime-tests
    runtime_state_test.cpp
    ../lsfg-vk-layer/src/instance.cpp
    ../lsfg-vk-layer/src/swapchain.cpp)

target_include_directories(lsfg-vk-runtime-tests PRIVATE
    ../lsfg-vk-layer/src)

target_link_libraries(lsfg-vk-runtime-tests
    PRIVATE lsfg-vk-common
    PRIVATE lsfg-vk-backend)

target_compile_options(lsfg-vk-runtime-tests PRIVATE
    -Wno-unknown-warning-option
    -Wno-unsafe-buffer-usage)

add_test(NAME lsfg-vk-runtime-tests COMMAND lsfg-vk-runtime-tests)
