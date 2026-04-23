cmake_minimum_required(VERSION 3.25)

project(alloy_downstream_package_smoke LANGUAGES CXX C ASM)

find_package(Alloy CONFIG REQUIRED)

alloy_add_runtime_executable(alloy_downstream_package_smoke
    SOURCES
        main.cpp
)