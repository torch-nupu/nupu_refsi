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

#include <compiler/utils/attributes.h>
#include <compiler/utils/pass_functions.h>
#include <compiler/utils/scheduling.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include <refsi_m1/huca_dummy_pass.h>

#include <iostream>

using namespace llvm;

namespace refsi_m1 {

llvm::PreservedAnalyses HucaDummyPass::run(llvm::Module &M,
                                           llvm::ModuleAnalysisManager &) {
  llvm::dbgs() << "--- start HucaDummyPass\n";
  (void)M;
  // M.print(llvm::dbgs(), nullptr);
  llvm::dbgs() << "--- fin HucaDummyPass\n";

  return PreservedAnalyses::all();
}
}  // namespace refsi_m1
