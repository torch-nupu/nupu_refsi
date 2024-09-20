// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <refsi_m1/target.h>

namespace refsi_m1 {

RefSiM1Target::RefSiM1Target(
    const compiler::Info *compiler_info,
    const riscv::hal_device_info_riscv_t *hal_device_info,
    compiler::Context *context, compiler::NotifyCallbackFn callback)
    : riscv::RiscvTarget(compiler_info, hal_device_info, context, callback) {
  // TODO: merge with cmake option `REFSIDRV_M1_ISA`
  if (const char *val = getenv("REFSI_LLVM_ENABLE_NU")) {
    // TODO: enable by default
    if (strcmp(val, "1") == 0) {
      llvm_features += ",+xnu";
    }
  }
}

}  // namespace refsi_m1
