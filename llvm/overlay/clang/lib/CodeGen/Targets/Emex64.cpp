//===- Emex64.cpp ---------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "ABIInfoImpl.h"
#include "TargetInfo.h"

using namespace clang;
using namespace clang::CodeGen;

namespace {

class Emex64ABIInfo : public DefaultABIInfo
{
public:
    Emex64ABIInfo(CodeGenTypes &CGT) : DefaultABIInfo(CGT) {}
    
    ABIArgInfo classifyArgumentType(QualType Ty) const;
    ABIArgInfo classifyReturnType(QualType RetTy) const;
    
    void computeInfo(CGFunctionInfo &FI) const override
    {
        if(!getCXXABI().classifyReturnType(FI))
        {
            FI.getReturnInfo() = classifyReturnType(FI.getReturnType());
        }
        for(auto &I : FI.arguments())
        {
            I.info = classifyArgumentType(I.type);
        }
    }
    
    RValue EmitVAArg(CodeGenFunction &CGF, Address VAListAddr, QualType Ty, AggValueSlot Slot) const override;

private:
    ABIArgInfo coerceToQuads(QualType Ty) const;
};

class Emex64TargetCodeGenInfo : public TargetCodeGenInfo
{
public:
    Emex64TargetCodeGenInfo(CodeGenTypes &CGT) : TargetCodeGenInfo(std::make_unique<Emex64ABIInfo>(CGT)) {}
};

} /* namespace */

ABIArgInfo Emex64ABIInfo::coerceToQuads(QualType Ty) const
{
    uint64_t Size = getContext().getTypeSize(Ty);
    llvm::LLVMContext &Ctx = getVMContext();
    if(Size <= 64)
    {
        return ABIArgInfo::getDirect(llvm::IntegerType::get(Ctx, 64));
    }
    return ABIArgInfo::getDirect(llvm::ArrayType::get(llvm::IntegerType::get(Ctx, 64), 2));
}

ABIArgInfo Emex64ABIInfo::classifyArgumentType(QualType Ty) const
{
    Ty = useFirstFieldIfTransparentUnion(Ty);
    
    if(isAggregateTypeForABI(Ty))
    {
        if(CGCXXABI::RecordArgABI RAA = getRecordArgABI(Ty, getCXXABI()))
        {
            return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(), RAA == CGCXXABI::RAA_DirectInMemory);
        }
        if(isEmptyRecord(getContext(), Ty, true))
        {
            return ABIArgInfo::getIgnore();
        }
        if(getContext().getTypeSize(Ty) <= 128)
        {
            return coerceToQuads(Ty);
        }
        return getNaturalAlignIndirect(Ty, getDataLayout().getAllocaAddrSpace(), false);
    }
    
    return DefaultABIInfo::classifyArgumentType(Ty);
}

ABIArgInfo Emex64ABIInfo::classifyReturnType(QualType RetTy) const
{
    if(RetTy->isVoidType())
    {
        return ABIArgInfo::getIgnore();
    }
    if(isAggregateTypeForABI(RetTy))
    {
        if(isEmptyRecord(getContext(), RetTy, true))
        {
            return ABIArgInfo::getIgnore();
        }
        if(getContext().getTypeSize(RetTy) <= 128)
        {
            return coerceToQuads(RetTy);
        }
        return getNaturalAlignIndirect(RetTy, getDataLayout().getAllocaAddrSpace());
    }
    return DefaultABIInfo::classifyReturnType(RetTy);
}

RValue Emex64ABIInfo::EmitVAArg(CodeGenFunction &CGF,
                                Address VAListAddr,
                                QualType Ty,
                                AggValueSlot Slot) const
{
    bool Indirect = isAggregateTypeForABI(Ty) && getContext().getTypeSize(Ty) > 128;
    return emitVoidPtrVAArg(CGF, VAListAddr, Ty, Indirect, getContext().getTypeInfoInChars(Ty), CharUnits::fromQuantity(8), true, Slot);
}

std::unique_ptr<TargetCodeGenInfo> CodeGen::createEmex64TargetCodeGenInfo(CodeGenModule &CGM)
{
    return std::make_unique<Emex64TargetCodeGenInfo>(CGM.getTypes());
}
