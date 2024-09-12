# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations
# under the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

cmake_minimum_required(VERSION 3.0.0)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(hal_refsi VERSION 1.0.0)

  if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "In-tree builds are not supported")
  endif()

  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
  string(APPEND CMAKE_INSTALL_RPATH :$ORIGIN/../lib)
endif()

set(HAL_REFSI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
list(APPEND CMAKE_MODULE_PATH ${HAL_REFSI_SOURCE_DIR}/cmake)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
string(APPEND CMAKE_C_FLAGS " -fvisibility=hidden")
string(APPEND CMAKE_CXX_FLAGS " -fvisibility=hidden")

set_property(GLOBAL APPEND PROPERTY KNOWN_HAL_DEVICES "refsi")
set_property(GLOBAL PROPERTY HAL_REFSI_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

set(HAL_REFSI_SOC "M1" CACHE STRING "Name of the RefSi SoC family to target")
set_property(CACHE HAL_REFSI_SOC PROPERTY STRINGS "M1")

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../driver PARENT_SCOPE)

include(Bin2H)
include(BakedFile)

# Report a HAL description and device name based on which SoC family has been
# selected.
get_property(REFSIDRV_M1_ISA GLOBAL PROPERTY REFSIDRV_M1_ISA)
set_property(GLOBAL PROPERTY HAL_REFSI_DESCRIPTION ${REFSIDRV_M1_ISA})
set_property(GLOBAL PROPERTY HAL_REFSI_DEVICE_NAME "RefSi M1")
set(TRIPLE "riscv64")

add_subdirectory(source)
