cmake_minimum_required(VERSION 3.13)
project(LLVMFoldDCE CXX)

# Point CMake at your LLVM install, e.g.
#   cmake -S . -B build -DLLVM_DIR=/usr/lib/llvm-18/lib/cmake/llvm
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION} (${LLVM_DIR})")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFS UNIX_COMMAND "${LLVM_DEFINITIONS}")
add_definitions(${LLVM_DEFS})

# Distro LLVM builds are compiled without RTTI; the plugin must match.
if(NOT LLVM_ENABLE_RTTI)
  add_compile_options(-fno-rtti)
endif()

add_library(FoldDCE MODULE src/FoldDCEPass.cpp)
set_target_properties(FoldDCE PROPERTIES PREFIX "" POSITION_INDEPENDENT_CODE ON)
target_compile_options(FoldDCE PRIVATE -Wall -Wextra)
