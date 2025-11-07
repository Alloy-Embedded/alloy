# Linux Platform CMake Test Example

add_executable(linux_cmake_test
    main.cpp
)

target_include_directories(linux_cmake_test
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(linux_cmake_test
    PRIVATE
        Threads::Threads
)

set_target_properties(linux_cmake_test PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

message(STATUS "Linux CMake Test: linux_cmake_test target created")
