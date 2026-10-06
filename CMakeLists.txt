cmake_minimum_required(VERSION 3.21)

project(dce)

set(CMAKE_CXX_STANDARD 20)

option(QBE_ENABLE_LIBCXX "Use libc++ if available." OFF)

function(append value)
  foreach(variable ${ARGN})
    set(${variable} "${${variable}} ${value}" PARENT_SCOPE)
  endforeach(variable)
endfunction()

if (QBE_ENABLE_LIBCXX)
  append("-stdlib=libc++"
    CMAKE_CXX_FLAGS CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS)
endif()

list(INSERT CMAKE_MODULE_PATH 0
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(QBE_INCLUDE_DIR ${QBE_ROOT_DIR}/include)
set(QBE_OBJ_DIR ${QBE_ROOT_DIR}/obj)
set(QBE_LIB_DIR ${QBE_ROOT_DIR}/lib)

find_library(LIBQBE_PATH NAMES libqbe.a REQUIRED PATHS ${QBE_LIB_DIR})
if(NOT LIBQBE_PATH)
    message(FATAL_ERROR "Could not find libqbe.a!")
endif()
message(STATUS "Found libqbe.a: ${LIBQBE_PATH}")

include_directories(${QBE_INCLUDE_DIR})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(AddQBE)

add_subdirectory(src)

enable_testing()
add_subdirectory(test)
