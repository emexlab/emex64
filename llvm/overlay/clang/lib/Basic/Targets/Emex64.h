//===--- Emex64.h -----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_CLANG_LIB_BASIC_TARGETS_EMEX64_H
#define LLVM_CLANG_LIB_BASIC_TARGETS_EMEX64_H

#include "clang/Basic/TargetInfo.h"
#include "clang/Basic/TargetOptions.h"
#include "llvm/Support/Compiler.h"
#include "llvm/TargetParser/Triple.h"

namespace clang {
namespace targets {

class LLVM_LIBRARY_VISIBILITY Emex64TargetInfo : public TargetInfo
{
    static const char *const GCCRegNames[];
    std::string CPU = "generic";
    bool HasFPU = true;
    bool HasISA19 = true;
    bool HasISA20 = true;
    bool HasISA21 = true;
    bool HasPAuth = true;

public:
    Emex64TargetInfo(const llvm::Triple &Triple, const TargetOptions &);
    
    void getTargetDefines(const LangOptions &Opts, MacroBuilder &Builder) const override;
    bool isValidCPUName(StringRef Name) const override;
    void fillValidCPUList(SmallVectorImpl<StringRef> &Values) const override;
    bool setCPU(StringRef Name) override;

    bool initFeatureMap(llvm::StringMap<bool> &Features, DiagnosticsEngine &Diags, StringRef CPU, const std::vector<std::string> &FeaturesVec) const override;
    bool handleTargetFeatures(std::vector<std::string> &Features, DiagnosticsEngine &Diags) override;
    bool hasFeature(StringRef Feature) const override;
    
    ArrayRef<const char *> getGCCRegNames() const override;
    ArrayRef<TargetInfo::GCCRegAlias> getGCCRegAliases() const override
    {
        return {};
    }
    BuiltinVaListKind getBuiltinVaListKind() const override
    {
        return TargetInfo::VoidPtrBuiltinVaList;
    }
    llvm::SmallVector<Builtin::InfosShard> getTargetBuiltins() const override
    {
        return {};
    }
    
    bool validateAsmConstraint(const char *&Name, TargetInfo::ConstraintInfo &Info) const override;
    std::string_view getClobbers() const override
    {
        return "";
    }
    
    int getEHDataRegisterNumber(unsigned RegNo) const override
    {
        if(RegNo == 0)
        {
            return 5;
        }
        if(RegNo == 1)
        {
            return 6;
        }
        return -1;
    }
    
    bool hasBitIntType() const override
    {
        return true;
    }
    bool hasInt128Type() const override
    {
        return true;
    }
};

} /* namespace targets */
} /* namespace clang */

#endif /* LLVM_CLANG_LIB_BASIC_TARGETS_EMEX64_H */
