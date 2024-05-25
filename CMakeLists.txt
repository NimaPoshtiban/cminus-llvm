cmake_minimum_required(VERSION 3.28.1)
project(cminus)
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(LLVM_PATH "llvm-config --src-root")
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")

set(SOURCE_FILES cminus.cpp parser.h lexer.h ast.h)
add_executable(cminus ${SOURCE_FILES})
target_include_directories(cminus PUBLIC
                           "${LLVM_PATH}"
                           )

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(cminus ${llvm_libs})
set(CMAKE_BUILD_TYPE "Release")