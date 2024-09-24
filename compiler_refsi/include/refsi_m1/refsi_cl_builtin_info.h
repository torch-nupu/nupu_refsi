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

#ifndef REFSI_CL_BUILTIN_INFO_H_INCLUDED
#define REFSI_CL_BUILTIN_INFO_H_INCLUDED

#include <compiler/utils/cl_builtin_info.h>

namespace refsi_m1 {

std::unique_ptr<compiler::utils::BILangInfoConcept> createRefSiM1CLBuiltinInfo(
    llvm::Module *builtins);

class RefSiM1CLBuiltinInfo : public compiler::utils::CLBuiltinInfo {
 public:
  RefSiM1CLBuiltinInfo(llvm::Module *Builtins)
      : compiler::utils::CLBuiltinInfo(Builtins) {}

  RefSiM1CLBuiltinInfo(std::unique_ptr<compiler::utils::CLBuiltinLoader> L)
      : compiler::utils::CLBuiltinInfo(std::move(L)) {}

  compiler::utils::Builtin analyzeBuiltin(
      const llvm::Function &Builtin) const override;

  llvm::Value *emitBuiltinInline(llvm::Function *Builtin, llvm::IRBuilder<> &B,
                                 llvm::ArrayRef<llvm::Value *> Args) override;
};

}  // namespace refsi_m1

#endif  // REFSI_CL_BUILTIN_INFO_H_INCLUDED
