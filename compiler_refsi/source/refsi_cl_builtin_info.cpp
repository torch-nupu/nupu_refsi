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

#include <compiler/utils/mangling.h>
#include <llvm/IR/IntrinsicsRISCV.h>
#include <llvm/IR/Module.h>
#include <refsi_m1/refsi_cl_builtin_info.h>

namespace refsi_m1 {

std::unique_ptr<compiler::utils::BILangInfoConcept> createRefSiM1CLBuiltinInfo(
    llvm::Module *builtins) {
  return std::make_unique<RefSiM1CLBuiltinInfo>(builtins);
}

compiler::utils::Builtin RefSiM1CLBuiltinInfo::analyzeBuiltin(
    const llvm::Function &Builtin) const {
  compiler::utils::NameMangler mangler(&Builtin.getParent()->getContext());
  llvm::StringRef BaseName = mangler.demangleName(Builtin.getName());

  if ((BaseName == "riscv_nu_nop")) {
    unsigned properties = compiler::utils::eBuiltinPropertyCanEmitInline;
    return compiler::utils::Builtin{
        Builtin, compiler::utils::eBuiltinUnknown,
        (compiler::utils::BuiltinProperties)properties};
  }
  return compiler::utils::CLBuiltinInfo::analyzeBuiltin(Builtin);
}

llvm::Value *RefSiM1CLBuiltinInfo::emitBuiltinInline(
    llvm::Function *Builtin, llvm::IRBuilder<> &B,
    llvm::ArrayRef<llvm::Value *> Args) {
#if defined(REFSI_LLVM_ENABLE_NU)
  if (Builtin) {
    compiler::utils::NameMangler mangler(&Builtin->getParent()->getContext());
    llvm::StringRef BaseName = mangler.demangleName(Builtin->getName());

    if (BaseName == "riscv_nu_nop") {
      return B.CreateIntrinsic(llvm::Intrinsic::riscv_nu_nop, {}, Args);
    }
  }
#endif
  return compiler::utils::CLBuiltinInfo::emitBuiltinInline(Builtin, B, Args);
}

}  // namespace refsi_m1
