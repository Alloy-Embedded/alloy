cmake_minimum_required(VERSION 3.25)

project(microcore_external_consumer LANGUAGES CXX C)

find_package(microcore CONFIG REQUIRED)

if(NOT TARGET microcore::hal)
    message(FATAL_ERROR "Expected exported target microcore::hal")
endif()

if(NOT TARGET microcore::rtos)
    message(FATAL_ERROR "Expected exported target microcore::rtos")
endif()

if(NOT TARGET microcore::board::host)
    message(FATAL_ERROR "Expected exported target microcore::board::host")
endif()

add_executable(find_package_consumer_smoke main.cpp)
target_compile_features(find_package_consumer_smoke PRIVATE cxx_std_23)
target_link_libraries(find_package_consumer_smoke PRIVATE
    microcore::hal
    microcore::rtos
    microcore::board::host
)
