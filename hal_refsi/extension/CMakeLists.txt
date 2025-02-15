# Copyright (C) Codeplay Software Limited
#
# Licensed under the Apache License, Version 2.0 (the "License") with LLVM
# Exceptions; you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

add_ca_cl_compiler_extension(
  ext_riscv_nupu
  EXTENSIONS
  cl_nupu_dummy
  INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/include/extension/cl_nupu_dummy.h
  ${CMAKE_CURRENT_SOURCE_DIR}/source/cl_nupu_dummy.cpp)

add_ca_force_header(
  PREFIX "nupu_builtins_h"
  DEVICE_NAME "${CA_RISCV_DEVICE}"
  PATH "${CMAKE_CURRENT_SOURCE_DIR}/include/extension/nupu_builtins.h")
