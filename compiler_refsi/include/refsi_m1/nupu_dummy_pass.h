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

#ifndef REFSI_M1_NUPU_DUMMY_PASS
#define REFSI_M1_NUPU_DUMMY_PASS

#include <llvm/IR/PassManager.h>

namespace refsi_m1 {
class NupuDummyPass final : public llvm::PassInfoMixin<NupuDummyPass> {
 public:
  llvm::PreservedAnalyses run(llvm::Module &, llvm::ModuleAnalysisManager &);
};

}  // namespace refsi_m1

#endif  // REFSI_M1_NUPU_DUMMY_PASS
