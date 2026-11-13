//===--- Emex64.cpp -------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64.h"
#include "clang/Basic/MacroBuilder.h"
#include "llvm/ADT/StringSwitch.h"

using namespace clang;
using namespace clang::targets;

const char *const Emex64TargetInfo::GCCRegNames[] = {
    "pc",   "sp",   "fp",   "cf",   "fpc",  "r0",   "r1",   "r2",
    "r3",   "r4",   "r5",   "r6",   "r7",   "r8",   "r9",   "rr",
    "er0",  "er1",  "er2",  "er3",  "er4",  "er5",  "er6",  "er7",
    "er8",  "er9",  "er10", "er11", "er12", "er13", "er14", "er15",
    "fr0",  "fr1",  "fr2",  "fr3",  "fr4",  "fr5",  "fr6",  "fr7",
    "fr8",  "fr9",  "fr10", "fr11", "fr12", "fr13", "fr14", "fr15"
};

Emex64TargetInfo::Emex64TargetInfo(const llvm::Triple &Triple,
                                   const TargetOptions &): TargetInfo(Triple)
{
    LongWidth = LongAlign = 64;
    PointerWidth = PointerAlign = 64;
    IntMaxType = SignedLong;
    Int64Type = SignedLong;
    SizeType = UnsignedLong;
    PtrDiffType = SignedLong;
    IntPtrType = SignedLong;
    WCharType = SignedInt;
    WIntType = UnsignedInt;
    LongDoubleWidth = LongDoubleAlign = 64;
    LongDoubleFormat = &llvm::APFloat::IEEEdouble();
    SuitableAlign = 64;
    MaxAtomicPromoteWidth = MaxAtomicInlineWidth = 64;
    resetDataLayout();
}

void Emex64TargetInfo::getTargetDefines(const LangOptions &Opts,
                                        MacroBuilder &Builder) const
{
    Builder.defineMacro("__emex64__");
    Builder.defineMacro("__EMEX64__");
    Builder.defineMacro("__la64__");
    Builder.defineMacro("__emex64_isa__", HasISA21 ? "21" : HasISA20 ? "20" : HasISA19 ? "19" : HasFPU ? "18" : "17");
    if(HasPAuth)
    {
        Builder.defineMacro("__emex64_pauth__");
    }
    if(getTriple().isEmex64e())
    {
        Builder.defineMacro("__emex64e__");
        Builder.defineMacro("__la64e__");
    }
    if(HasFPU)
    {
        Builder.defineMacro("__emex64_fpu__");
    }
    else
    {
        Builder.defineMacro("__emex64_soft_float__");
        Builder.defineMacro("__SOFTFP__");
    }
    Builder.defineMacro("__REGISTER_PREFIX__", "");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_1");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_2");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4");
    Builder.defineMacro("__GCC_HAVE_SYNC_COMPARE_AND_SWAP_8");
}

bool Emex64TargetInfo::isValidCPUName(StringRef Name) const
{
    return llvm::StringSwitch<bool>(Name)
        .Cases({"generic", "emex64-v17", "emex64-v18", "emex64-v19", "emex64-v20", "emex64-v21", "emex64e", "emex64e-v20", "emex64e-v21"}, true)
        .Default(false);
}

void Emex64TargetInfo::fillValidCPUList(SmallVectorImpl<StringRef> &Values) const
{
    Values.append({"generic", "emex64-v17", "emex64-v18", "emex64-v19", "emex64-v20", "emex64-v21", "emex64e", "emex64e-v20", "emex64e-v21"});
}

bool Emex64TargetInfo::setCPU(StringRef Name)
{
    if(!isValidCPUName(Name))
    {
        return false;
    }
    CPU = Name.str();
    return true;
}

bool Emex64TargetInfo::initFeatureMap(llvm::StringMap<bool> &Features,
                                      DiagnosticsEngine &Diags,
                                      StringRef CPU, const std::vector<std::string> &FeaturesVec) const
{
    bool Generic = CPU.empty() || CPU == "generic";
    bool V21 = Generic || CPU == "emex64-v21" || CPU == "emex64e-v21";
    bool V20 = V21 || CPU == "emex64-v20" || CPU == "emex64e-v20";
    Features["fpu"] = CPU != "emex64-v17";
    Features["isa19"] = V20 || CPU == "emex64-v19" || CPU == "emex64e";
    Features["isa20"] = V20;
    Features["isa21"] = V21;
    Features["pauth"] = Generic || CPU == "emex64e" || CPU == "emex64e-v20" || CPU == "emex64e-v21";
    return TargetInfo::initFeatureMap(Features, Diags, CPU, FeaturesVec);
}

bool Emex64TargetInfo::handleTargetFeatures(std::vector<std::string> &Features,
                                            DiagnosticsEngine &Diags)
{
    HasFPU = false;
    HasISA19 = false;
    HasISA20 = false;
    HasISA21 = false;
    HasPAuth = false;
    for(const std::string &F : Features)
    {
        if(F == "+fpu")
        {
            HasFPU = true;
        }
        else if(F == "-fpu")
        {
            HasFPU = false;
        }
        else if(F == "+isa19")
        {
            HasISA19 = true;
        }
        else if(F == "-isa19")
        {
            HasISA19 = false;
        }
        else if(F == "+isa20")
        {
            HasISA20 = true;
        }
        else if(F == "-isa20")
        {
            HasISA20 = false;
        }
        else if(F == "+isa21")
        {
            HasISA21 = true;
        }
        else if(F == "-isa21")
        {
            HasISA21 = false;
        }
        else if(F == "+pauth")
        {
            HasPAuth = true;
        }
        else if(F == "-pauth")
        {
            HasPAuth = false;
        }
    }
    return true;
}

bool Emex64TargetInfo::hasFeature(StringRef Feature) const
{
    return llvm::StringSwitch<bool>(Feature)
        .Case("emex64", true)
        .Case("fpu", HasFPU)
        .Case("isa19", HasISA19)
        .Case("isa20", HasISA20)
        .Case("isa21", HasISA21)
        .Case("pauth", HasPAuth)
        .Default(false);
}

ArrayRef<const char *> Emex64TargetInfo::getGCCRegNames() const
{
  return llvm::ArrayRef(GCCRegNames);
}

bool Emex64TargetInfo::validateAsmConstraint(const char *&Name,
                                             TargetInfo::ConstraintInfo &Info) const
{
    switch(*Name)
    {
        case 'r':
            Info.setAllowsRegister();
            return true;
        case 'f':
            if(!HasFPU)
            {
                return false;
            }
            Info.setAllowsRegister();
            return true;
        case 'I':
        case 'i':
        case 'n':
            Info.setRequiresImmediate();
            return true;
        default:
            return false;
    }
}
