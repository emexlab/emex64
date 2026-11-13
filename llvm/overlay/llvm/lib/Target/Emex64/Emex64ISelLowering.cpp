//===-- Emex64ISelLowering.cpp --------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "Emex64ISelLowering.h"
#include "Emex64MachineFunctionInfo.h"
#include "Emex64SelectionDAGInfo.h"
#include "Emex64Subtarget.h"
#include "Emex64TargetMachine.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/CodeGen/CallingConvLower.h"
#include "llvm/CodeGen/MachineFrameInfo.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/TargetLoweringObjectFileImpl.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

#define DEBUG_TYPE "emex64-lower"

#define GET_CALLING_CONV_IMPL
#include "Emex64GenCallingConv.inc"

Emex64TargetLowering::Emex64TargetLowering(const TargetMachine &TM,
                                           const Emex64Subtarget &STI) : TargetLowering(TM, STI), Subtarget(STI)
{
    addRegisterClass(MVT::i64, &Emex64::GPRRegClass);
    if(STI.hasFPU())
    {
        addRegisterClass(MVT::f64, &Emex64::FPRRegClass);
        addRegisterClass(MVT::f32, &Emex64::FPRRegClass);
    }
    computeRegisterProperties(STI.getRegisterInfo());

    setStackPointerRegisterToSaveRestore(Emex64::SP);
    setBooleanContents(ZeroOrOneBooleanContent);
    setSchedulingPreference(Sched::Source);
    setMinFunctionAlignment(Align(1));
    setPrefFunctionAlignment(Align(1));
    setMinStackArgumentAlignment(Align(8));

    for(MVT VT : {MVT::i64, MVT::f32, MVT::f64})
    {
        setOperationAction(ISD::BR_CC, VT, Custom);
        setOperationAction(ISD::SELECT_CC, VT, Custom);
        setOperationAction(ISD::SETCC, VT, Expand);
        setOperationAction(ISD::SELECT, VT, Expand);
    }

    setOperationAction(ISD::BRCOND, MVT::Other, Expand);
    setOperationAction(ISD::BR_JT, MVT::Other, Expand);

    setOperationAction(ISD::GlobalAddress, MVT::i64, Custom);
    setOperationAction(ISD::ExternalSymbol, MVT::i64, Custom);
    setOperationAction(ISD::BlockAddress, MVT::i64, Custom);
    setOperationAction(ISD::JumpTable, MVT::i64, Custom);
    setOperationAction(ISD::ConstantPool, MVT::i64, Custom);
    setOperationAction(ISD::GlobalTLSAddress, MVT::i64, Custom);

    setOperationAction(ISD::VASTART, MVT::Other, Custom);
    setOperationAction(ISD::VAARG, MVT::Other, Expand);
    setOperationAction(ISD::VACOPY, MVT::Other, Expand);
    setOperationAction(ISD::VAEND, MVT::Other, Expand);
    setOperationAction(ISD::DYNAMIC_STACKALLOC, MVT::i64, Custom);
    setOperationAction(ISD::STACKSAVE, MVT::Other, Expand);
    setOperationAction(ISD::STACKRESTORE, MVT::Other, Expand);
    setOperationAction(ISD::FRAMEADDR, MVT::i64, Custom);
    setOperationAction(ISD::RETURNADDR, MVT::i64, Custom);

    for(unsigned Op : {ISD::SREM, ISD::SDIVREM, ISD::UDIVREM, ISD::UMUL_LOHI,
                       ISD::SMUL_LOHI, ISD::ROTL, ISD::SHL_PARTS,
                       ISD::SRL_PARTS, ISD::SRA_PARTS})
    {
        setOperationAction(Op, MVT::i64, Expand);
    }

    if(STI.hasISA19())
    {
        for(unsigned Op : {ISD::ADDC, ISD::ADDE, ISD::SUBC, ISD::SUBE})
        {
            setOperationAction(Op, MVT::i64, Legal);
        }
    }
    else
    {
        for(unsigned Op : {ISD::MULHU, ISD::MULHS, ISD::CTPOP, ISD::CTLZ, ISD::CTTZ})
        {
            setOperationAction(Op, MVT::i64, Expand);
        }
        for(MVT VT : {MVT::i1, MVT::i8, MVT::i16, MVT::i32})
        {
            setOperationAction(ISD::SIGN_EXTEND_INREG, VT, Expand);
        }
    }
    
    for(MVT VT : MVT::integer_valuetypes())
    {
        setLoadExtAction(ISD::SEXTLOAD, VT, MVT::i1, Promote);
        setLoadExtAction(ISD::ZEXTLOAD, VT, MVT::i1, Promote);
        setLoadExtAction(ISD::EXTLOAD, VT, MVT::i1, Promote);
        for(MVT MemVT : {MVT::i8, MVT::i16, MVT::i32})
        {
            setLoadExtAction(ISD::SEXTLOAD, VT, MemVT, STI.hasISA20() && VT == MVT::i64 ? Legal : Expand);
        }
    }
    
    setOperationAction(ISD::READCYCLECOUNTER, MVT::i64, STI.hasISA20() ? Legal : Expand);
    
    if(STI.hasFPU())
    {
        for(MVT VT : {MVT::f32, MVT::f64})
        {
            for(unsigned Op : {ISD::FREM, ISD::FPOW, ISD::FPOWI, ISD::FSIN, ISD::FCOS,
                               ISD::FSINCOS, ISD::FTAN, ISD::FEXP, ISD::FEXP2, ISD::FEXP10,
                               ISD::FLOG, ISD::FLOG2, ISD::FLOG10, ISD::FCOPYSIGN, ISD::FFLOOR,
                               ISD::FCEIL, ISD::FTRUNC, ISD::FRINT, ISD::FNEARBYINT, ISD::FROUND,
                               ISD::FROUNDEVEN, ISD::FMINIMUM, ISD::FMAXIMUM, ISD::FLDEXP,
                               ISD::FFREXP, ISD::LROUND, ISD::LLROUND, ISD::LRINT, ISD::LLRINT,
                               ISD::FP16_TO_FP, ISD::FP_TO_FP16})
            {
                setOperationAction(Op, VT, Expand);
            }
            for (unsigned Op : {ISD::STRICT_FADD, ISD::STRICT_FSUB, ISD::STRICT_FMUL,
                                ISD::STRICT_FDIV, ISD::STRICT_FSQRT, ISD::STRICT_FMA})
            {
                setOperationAction(Op, VT, Legal);
            }

            setOperationAction(ISD::FMINNUM, VT, Legal);
            setOperationAction(ISD::FMAXNUM, VT, Legal);
            setOperationAction(ISD::FMA, VT, Legal);
            setOperationAction(ISD::ConstantFP, VT, Legal);
            setLoadExtAction(ISD::EXTLOAD, VT, MVT::f16, Expand);
            setTruncStoreAction(VT, MVT::f16, Expand);
        }

        setLoadExtAction(ISD::EXTLOAD, MVT::f64, MVT::f32, Expand);
        setTruncStoreAction(MVT::f64, MVT::f32, Expand);
        setOperationAction(ISD::BITCAST, MVT::i32, Custom);
        setOperationAction(ISD::BITCAST, MVT::f32, Custom);

        for(unsigned Op : {ISD::STRICT_FP_ROUND, ISD::STRICT_FP_EXTEND,
                           ISD::STRICT_SINT_TO_FP, ISD::STRICT_UINT_TO_FP,
                           ISD::STRICT_FP_TO_SINT, ISD::STRICT_FP_TO_UINT})
        {
            setOperationAction(Op, MVT::f32, Legal);
            setOperationAction(Op, MVT::f64, Legal);
            setOperationAction(Op, MVT::i64, Legal);
        }

        setOperationAction(ISD::FP_TO_SINT_SAT, MVT::i64, Legal);
        setOperationAction(ISD::FP_TO_UINT_SAT, MVT::i64, Legal);
    }

    setMaxAtomicSizeInBitsSupported(64);
    setMinCmpXchgSizeInBits(64);
    setOperationAction(ISD::ATOMIC_FENCE, MVT::Other, Legal);

    MaxStoresPerMemcpy = MaxStoresPerMemcpyOptSize = 4;
    MaxStoresPerMemmove = MaxStoresPerMemmoveOptSize = 4;
    MaxStoresPerMemset = MaxStoresPerMemsetOptSize = 4;
}

bool Emex64TargetLowering::isFMAFasterThanFMulAndFAdd(const MachineFunction &MF,
                                                      EVT VT) const
{
    VT = VT.getScalarType();
    return Subtarget.hasFPU() && (VT == MVT::f32 || VT == MVT::f64);
}

bool Emex64TargetLowering::shouldInsertFencesForAtomic(const Instruction *I) const
{
    return isa<LoadInst>(I) || isa<StoreInst>(I);
}

TargetLowering::AtomicExpansionKind Emex64TargetLowering::shouldExpandAtomicRMWInIR(const AtomicRMWInst *AI) const
{
    unsigned Size = AI->getType()->getPrimitiveSizeInBits();
    if(AI->getOperation() == AtomicRMWInst::Xchg && Size == 64)
    {
        return AtomicExpansionKind::None;
    }
    if(Subtarget.hasISA20() && (Size == 64 || Size == 32))
    {
        switch(AI->getOperation())
        {
            case AtomicRMWInst::Add:
            case AtomicRMWInst::Sub:
            case AtomicRMWInst::And:
            case AtomicRMWInst::Or:
            case AtomicRMWInst::Xor:
                return AtomicExpansionKind::None;
            default:
                break;
        }
    }
    return AtomicExpansionKind::CmpXChg;
}

bool Emex64TargetLowering::isLegalAddressingMode(const DataLayout &DL,
                                                 const AddrMode &AM,
                                                 Type *Ty,
                                                 unsigned AS,
                                                 Instruction *I) const
{
    if(AM.Scale > 1)
    {
        return false;
    }
    if(AM.Scale == 1 && (AM.HasBaseReg ? 1 : 0) + (AM.BaseOffs ? 1 : 0) + (AM.BaseGV ? 1 : 0) > 1)
    {
        return false;
    }
    if(AM.BaseGV && AM.HasBaseReg && AM.BaseOffs)
    {
        return false;
    }
    if(AM.BaseGV && isPositionIndependent() && (AM.HasBaseReg || AM.Scale))
    {
        return false;
    }
    return true;
}

//===----------------------------------------------------------------------===//
// Calling convention
//===----------------------------------------------------------------------===//

static SDValue convertLocToVal(SelectionDAG &DAG,
                               const SDLoc &DL,
                               const CCValAssign &VA,
                               SDValue Val)
{
    switch(VA.getLocInfo())
    {
        case CCValAssign::SExt:
            Val = DAG.getNode(ISD::AssertSext, DL, VA.getLocVT(), Val, DAG.getValueType(VA.getValVT()));
            break;
        case CCValAssign::ZExt:
            Val = DAG.getNode(ISD::AssertZext, DL, VA.getLocVT(), Val, DAG.getValueType(VA.getValVT()));
            break;
        case CCValAssign::BCvt:
            return DAG.getNode(ISD::BITCAST, DL, VA.getValVT(), Val);
        default:
            break;
    }
    if(VA.getValVT() != VA.getLocVT())
    {
        if(VA.getValVT().isFloatingPoint())
        {
            return DAG.getNode(ISD::FP_ROUND, DL, VA.getValVT(), Val, DAG.getIntPtrConstant(0, DL));
        }
        Val = DAG.getNode(ISD::TRUNCATE, DL, VA.getValVT(), Val);
    }
    return Val;
}

static SDValue convertValToLoc(SelectionDAG &DAG,
                               const SDLoc &DL,
                               const CCValAssign &VA,
                               SDValue Val)
{
    switch(VA.getLocInfo())
    {
        case CCValAssign::SExt:
            return DAG.getNode(ISD::SIGN_EXTEND, DL, VA.getLocVT(), Val);
        case CCValAssign::ZExt:
            return DAG.getNode(ISD::ZERO_EXTEND, DL, VA.getLocVT(), Val);
        case CCValAssign::AExt:
            return DAG.getNode(ISD::ANY_EXTEND, DL, VA.getLocVT(), Val);
        case CCValAssign::FPExt:
            return DAG.getNode(ISD::FP_EXTEND, DL, VA.getLocVT(), Val);
        case CCValAssign::BCvt:
            return DAG.getNode(ISD::BITCAST, DL, VA.getLocVT(), Val);
        default:
            return Val;
    }
}

static const TargetRegisterClass *regClassFor(MVT VT)
{
    return VT.isFloatingPoint() ? &Emex64::FPRRegClass : &Emex64::GPRRegClass;
}

SDValue Emex64TargetLowering::LowerFormalArguments(SDValue Chain,
                                                   CallingConv::ID CallConv,
                                                   bool IsVarArg,
                                                   const SmallVectorImpl<ISD::InputArg> &Ins,
                                                   const SDLoc &DL,
                                                   SelectionDAG &DAG,
                                                   SmallVectorImpl<SDValue> &InVals) const
{
    MachineFunction &MF = DAG.getMachineFunction();
    MachineFrameInfo &MFI = MF.getFrameInfo();
    MachineRegisterInfo &RegInfo = MF.getRegInfo();
    
    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CallConv, IsVarArg, MF, ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeFormalArguments(Ins, CC_Emex64);
    
    for(unsigned I = 0, E = ArgLocs.size(); I != E; ++I)
    {
        CCValAssign &VA = ArgLocs[I];
        if(Ins[I].Flags.isByVal())
        {
            report_fatal_error("emex64: byval arguments are not supported");
        }
        if(VA.isRegLoc())
        {
            Register VReg = RegInfo.createVirtualRegister(regClassFor(VA.getLocVT()));
            RegInfo.addLiveIn(VA.getLocReg(), VReg);
            SDValue Val = DAG.getCopyFromReg(Chain, DL, VReg, VA.getLocVT());
            InVals.push_back(convertLocToVal(DAG, DL, VA, Val));
            continue;
        }
        
        assert(VA.isMemLoc());
        int FI = MFI.CreateFixedObject(8, 24 + VA.getLocMemOffset(), true);
        SDValue FIN = DAG.getFrameIndex(FI, MVT::i64);
        SDValue Val = DAG.getLoad(VA.getLocVT(), DL, Chain, FIN, MachinePointerInfo::getFixedStack(MF, FI));
        InVals.push_back(convertLocToVal(DAG, DL, VA, Val));
    }
    
    if(IsVarArg)
    {
        auto *FuncInfo = MF.getInfo<Emex64MachineFunctionInfo>();
        FuncInfo->setVarArgsFrameIndex(MFI.CreateFixedObject(8, 24 + CCInfo.getStackSize(), true));
    }
    
    return Chain;
}

SDValue Emex64TargetLowering::LowerCall(TargetLowering::CallLoweringInfo &CLI,
                                        SmallVectorImpl<SDValue> &InVals) const
{
    SelectionDAG &DAG = CLI.DAG;
    SDLoc &DL = CLI.DL;
    SmallVectorImpl<ISD::OutputArg> &Outs = CLI.Outs;
    SmallVectorImpl<SDValue> &OutVals = CLI.OutVals;
    SmallVectorImpl<ISD::InputArg> &Ins = CLI.Ins;
    SDValue Chain = CLI.Chain;
    SDValue Callee = CLI.Callee;
    MachineFunction &MF = DAG.getMachineFunction();

    CLI.IsTailCall = false;

    SmallVector<CCValAssign, 16> ArgLocs;
    CCState CCInfo(CLI.CallConv, CLI.IsVarArg, MF, ArgLocs, *DAG.getContext());
    CCInfo.AnalyzeCallOperands(Outs, CC_Emex64);
    unsigned NumBytes = CCInfo.getStackSize();

    for(const ISD::OutputArg &O : Outs)
    {
        if(O.Flags.isByVal())
        {
            report_fatal_error("emex64: byval arguments are not supported");
        }
    }
    
    Chain = DAG.getCALLSEQ_START(Chain, NumBytes, 0, DL);
    
    SmallVector<std::pair<unsigned, SDValue>, 8> RegsToPass;
    SmallVector<SDValue, 8> MemOpChains;
    SDValue StackPtr;
    
    for(unsigned I = 0, E = ArgLocs.size(); I != E; ++I)
    {
        CCValAssign &VA = ArgLocs[I];
        SDValue Arg = convertValToLoc(DAG, DL, VA, OutVals[I]);
        if(VA.isRegLoc())
        {
            RegsToPass.push_back({VA.getLocReg(), Arg});
            continue;
        }

        if(!StackPtr)
        {
            StackPtr = DAG.getCopyFromReg(Chain, DL, Emex64::SP, MVT::i64);
        }
        SDValue Addr = DAG.getNode(ISD::ADD, DL, MVT::i64, StackPtr, DAG.getIntPtrConstant(8 + VA.getLocMemOffset(), DL));
        MemOpChains.push_back(DAG.getStore(Chain, DL, Arg, Addr, MachinePointerInfo()));
    }
    if(!MemOpChains.empty())
    {
        Chain = DAG.getNode(ISD::TokenFactor, DL, MVT::Other, MemOpChains);
    }

    SDValue Glue;
    for(auto &Reg : RegsToPass)
    {
        Chain = DAG.getCopyToReg(Chain, DL, Reg.first, Reg.second, Glue);
        Glue = Chain.getValue(1);
    }
    
    if(GlobalAddressSDNode *G = dyn_cast<GlobalAddressSDNode>(Callee))
    {
        Callee = DAG.getTargetGlobalAddress(G->getGlobal(), DL, MVT::i64, G->getOffset(), Emex64II::MO_PLT);
    }
    else if (ExternalSymbolSDNode *S = dyn_cast<ExternalSymbolSDNode>(Callee))
    {
        Callee = DAG.getTargetExternalSymbol(S->getSymbol(), MVT::i64, Emex64II::MO_PLT);
    }

    SmallVector<SDValue, 8> Ops;
    Ops.push_back(Chain);
    Ops.push_back(Callee);
    for(auto &Reg : RegsToPass)
    {
        Ops.push_back(DAG.getRegister(Reg.first, Reg.second.getValueType()));
    }

    const TargetRegisterInfo *TRI = Subtarget.getRegisterInfo();
    Ops.push_back(DAG.getRegisterMask(TRI->getCallPreservedMask(MF, CLI.CallConv)));
    if(Glue.getNode())
    {
        Ops.push_back(Glue);
    }
    
    SDVTList NodeTys = DAG.getVTList(MVT::Other, MVT::Glue);
    Chain = DAG.getNode(Emex64ISD::CALL, DL, NodeTys, Ops);
    Glue = Chain.getValue(1);
    
    Chain = DAG.getCALLSEQ_END(Chain, NumBytes, 0, Glue, DL);
    Glue = Chain.getValue(1);
    
    SmallVector<CCValAssign, 4> RVLocs;
    CCState RetInfo(CLI.CallConv, CLI.IsVarArg, MF, RVLocs, *DAG.getContext());
    RetInfo.AnalyzeCallResult(Ins, RetCC_Emex64);
    for(CCValAssign &VA : RVLocs)
    {
        SDValue Val = DAG.getCopyFromReg(Chain, DL, VA.getLocReg(), VA.getLocVT(), Glue);
        Chain = Val.getValue(1);
        Glue = Val.getValue(2);
        InVals.push_back(convertLocToVal(DAG, DL, VA, Val));
    }

    return Chain;
}

bool Emex64TargetLowering::CanLowerReturn(CallingConv::ID CallConv,
                                          MachineFunction &MF,
                                          bool IsVarArg,
                                          const SmallVectorImpl<ISD::OutputArg> &Outs,
                                          LLVMContext &Context,
                                          const Type *RetTy) const
{
    SmallVector<CCValAssign, 4> RVLocs;
    CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, Context);
    return CCInfo.CheckReturn(Outs, RetCC_Emex64);
}

SDValue Emex64TargetLowering::LowerReturn(SDValue Chain,
                                          CallingConv::ID CallConv,
                                          bool IsVarArg,
                                          const SmallVectorImpl<ISD::OutputArg> &Outs,
                                          const SmallVectorImpl<SDValue> &OutVals,
                                          const SDLoc &DL,
                                          SelectionDAG &DAG) const
{
    MachineFunction &MF = DAG.getMachineFunction();
    if(MF.getFunction().hasFnAttribute("interrupt"))
    {
        report_fatal_error("emex64: interrupt handlers return through assembly; " "call them from the trap entry");
    }
    
    SmallVector<CCValAssign, 4> RVLocs;
    CCState CCInfo(CallConv, IsVarArg, MF, RVLocs, *DAG.getContext());
    CCInfo.AnalyzeReturn(Outs, RetCC_Emex64);
    
    SDValue Glue;
    SmallVector<SDValue, 4> RetOps(1, Chain);
    for(unsigned I = 0, E = RVLocs.size(); I != E; ++I)
    {
        CCValAssign &VA = RVLocs[I];
        SDValue Val = convertValToLoc(DAG, DL, VA, OutVals[I]);
        Chain = DAG.getCopyToReg(Chain, DL, VA.getLocReg(), Val, Glue);
        Glue = Chain.getValue(1);
        RetOps.push_back(DAG.getRegister(VA.getLocReg(), VA.getLocVT()));
    }
    RetOps[0] = Chain;
    if(Glue.getNode())
    {
        RetOps.push_back(Glue);
    }
    return DAG.getNode(Emex64ISD::RET_GLUE, DL, MVT::Other, RetOps);
}

//===----------------------------------------------------------------------===//
// Custom lowering
//===----------------------------------------------------------------------===//

SDValue Emex64TargetLowering::LowerOperation(SDValue Op,
                                             SelectionDAG &DAG) const
{
    switch(Op.getOpcode())
    {
        case ISD::GlobalAddress:
        case ISD::ExternalSymbol:
        case ISD::BlockAddress:
        case ISD::JumpTable:
        case ISD::ConstantPool:
            return lowerAddress(Op, DAG);
        case ISD::GlobalTLSAddress:
            return lowerGlobalTLSAddress(Op, DAG);
        case ISD::BR_CC:
            return lowerBR_CC(Op, DAG);
        case ISD::SELECT_CC:
            return lowerSELECT_CC(Op, DAG);
        case ISD::VASTART:
            return lowerVASTART(Op, DAG);
        case ISD::DYNAMIC_STACKALLOC:
            return lowerDYNAMIC_STACKALLOC(Op, DAG);
        case ISD::BITCAST:
            return lowerBITCAST(Op, DAG);
        case ISD::FRAMEADDR:
            return lowerFRAMEADDR(Op, DAG);
        case ISD::RETURNADDR:
            return lowerRETURNADDR(Op, DAG);
        default:
            llvm_unreachable("unexpected operation to lower");
    }
}

void Emex64TargetLowering::ReplaceNodeResults(SDNode *N,
                                              SmallVectorImpl<SDValue> &Results,
                                              SelectionDAG &DAG) const
{
    SDLoc DL(N);
    switch(N->getOpcode())
    {
        case ISD::BITCAST:
        {
            SDValue Src = N->getOperand(0);
            if(N->getValueType(0) == MVT::i32 && Src.getValueType() == MVT::f32)
            {
                SDValue Bits = DAG.getNode(Emex64ISD::FMV_X_W, DL, MVT::i64, Src);
                Results.push_back(DAG.getNode(ISD::TRUNCATE, DL, MVT::i32, Bits));
            }
            return;
        }
        default:
            return;
    }
}

SDValue Emex64TargetLowering::lowerAddress(SDValue Op,
                                           SelectionDAG &DAG) const
{
    SDLoc DL(Op);
    bool PIC = isPositionIndependent();
    unsigned Flag = PIC ? Emex64II::MO_PCREL : Emex64II::MO_NO_FLAG;
    SDValue T;

    switch(Op.getOpcode())
    {
        case ISD::GlobalAddress:
        {
            auto *N = cast<GlobalAddressSDNode>(Op);
            const GlobalValue *GV = N->getGlobal();
            if(PIC && !getTargetMachine().shouldAssumeDSOLocal(GV))
            {
                SDValue G = DAG.getTargetGlobalAddress(GV, DL, MVT::i64, 0, Emex64II::MO_GOTPCREL);
                SDValue Addr = DAG.getLoad(MVT::i64, DL, DAG.getEntryNode(), DAG.getNode(Emex64ISD::PCWRAPPER, DL, MVT::i64, G), MachinePointerInfo::getGOT(DAG.getMachineFunction()));
                if(N->getOffset())
                {
                    Addr = DAG.getNode(ISD::ADD, DL, MVT::i64, Addr, DAG.getConstant(N->getOffset(), DL, MVT::i64));
                }
                return Addr;
            }
            T = DAG.getTargetGlobalAddress(GV, DL, MVT::i64, N->getOffset(), Flag);
            break;
        }
        case ISD::ExternalSymbol:
        {
            const char *Sym = cast<ExternalSymbolSDNode>(Op)->getSymbol();
            if(PIC)
            {
                SDValue E = DAG.getTargetExternalSymbol(Sym, MVT::i64, Emex64II::MO_GOTPCREL);
                return DAG.getLoad(MVT::i64, DL, DAG.getEntryNode(), DAG.getNode(Emex64ISD::PCWRAPPER, DL, MVT::i64, E), MachinePointerInfo::getGOT(DAG.getMachineFunction()));
            }
            T = DAG.getTargetExternalSymbol(Sym, MVT::i64);
            break;
        }
        case ISD::BlockAddress:
        {
            auto *N = cast<BlockAddressSDNode>(Op);
            T = DAG.getTargetBlockAddress(N->getBlockAddress(), MVT::i64, N->getOffset(), Flag);
            break;
        }
        case ISD::JumpTable:
            T = DAG.getTargetJumpTable(cast<JumpTableSDNode>(Op)->getIndex(), MVT::i64, Flag);
            break;
        case ISD::ConstantPool:
        {
            auto *N = cast<ConstantPoolSDNode>(Op);
            T = DAG.getTargetConstantPool(N->getConstVal(), MVT::i64, N->getAlign(), N->getOffset(), Flag);
            break;
        }
    }

    return DAG.getNode(PIC ? Emex64ISD::PCWRAPPER : Emex64ISD::WRAPPER, DL, MVT::i64, T);
}

SDValue Emex64TargetLowering::lowerGlobalTLSAddress(SDValue Op,
                                                    SelectionDAG &DAG) const
{
    auto *N = cast<GlobalAddressSDNode>(Op);
    const GlobalValue *GV = N->getGlobal();
    SDLoc DL(Op);
    if(DAG.getTarget().useEmulatedTLS())
    {
        return LowerToTLSEmulatedModel(N, DAG);
    }

    TLSModel::Model Model = getTargetMachine().getTLSModel(GV);
    SDValue TP = DAG.getCopyFromReg(DAG.getEntryNode(), DL, Emex64::ER15, MVT::i64);
    SDValue Addr;
    switch(Model)
    {
        case TLSModel::LocalExec:
        {
            SDValue Off = DAG.getTargetGlobalAddress(GV, DL, MVT::i64, 0, Emex64II::MO_TPOFF);
            Addr = DAG.getNode(ISD::ADD, DL, MVT::i64, TP, DAG.getNode(Emex64ISD::WRAPPER, DL, MVT::i64, Off));
            break;
        }
        case TLSModel::InitialExec:
        {
            SDValue G = DAG.getTargetGlobalAddress(GV, DL, MVT::i64, 0, Emex64II::MO_GOTTPOFF);
            SDValue Off = DAG.getLoad(MVT::i64, DL, DAG.getEntryNode(), DAG.getNode(Emex64ISD::PCWRAPPER, DL, MVT::i64, G), MachinePointerInfo::getGOT(DAG.getMachineFunction()));
            Addr = DAG.getNode(ISD::ADD, DL, MVT::i64, TP, Off);
            break;
        }
        case TLSModel::GeneralDynamic:
        case TLSModel::LocalDynamic:
        {
            SDValue G = DAG.getTargetGlobalAddress(GV, DL, MVT::i64, 0, Emex64II::MO_TLSGD);
            SDValue Arg = DAG.getNode(Emex64ISD::PCWRAPPER, DL, MVT::i64, G);
            ArgListTy Args;
            Args.emplace_back(Arg, Type::getInt64Ty(*DAG.getContext()));
            TargetLowering::CallLoweringInfo CLI(DAG);
            CLI.setDebugLoc(DL).setChain(DAG.getEntryNode()).setLibCallee(CallingConv::C, Type::getInt64Ty(*DAG.getContext()), DAG.getExternalSymbol("__tls_get_addr", MVT::i64), std::move(Args));
            Addr = LowerCallTo(CLI).first;
            break;
        }
    }

    if(N->getOffset())
    {
        Addr = DAG.getNode(ISD::ADD, DL, MVT::i64, Addr, DAG.getConstant(N->getOffset(), DL, MVT::i64));
    }

    return Addr;
}

static unsigned fpCondCode(ISD::CondCode CC)
{
    switch(CC)
    {
        case ISD::SETOEQ:
        case ISD::SETEQ: 
            return Emex64CC::EQ;
        case ISD::SETUNE:
        case ISD::SETNE:
            return Emex64CC::NE;
        case ISD::SETOLT:
        case ISD::SETLT: 
            return Emex64CC::LT;
        case ISD::SETOGT:
        case ISD::SETGT:
            return Emex64CC::GT;
        case ISD::SETOLE:
        case ISD::SETLE:
            return Emex64CC::LE;
        case ISD::SETOGE:
        case ISD::SETGE:
            return Emex64CC::GE;
        case ISD::SETUO:
            return Emex64CC::UN;
        case ISD::SETO:
            return Emex64CC::ORD;
        case ISD::SETUEQ:
            return Emex64CC::UEQ;
        case ISD::SETULT:
            return Emex64CC::ULT;
        case ISD::SETUGT:
            return Emex64CC::UGT;
        case ISD::SETULE:
            return Emex64CC::ULE;
        case ISD::SETUGE:
            return Emex64CC::UGE;
        case ISD::SETONE:
            return Emex64CC::ONE;
        default:
            llvm_unreachable("unexpected floating point condition");
    }
}

SDValue Emex64TargetLowering::emitCompare(SDValue LHS,
                                          SDValue RHS,
                                          ISD::CondCode CC,
                                          unsigned &E64CC,
                                          const SDLoc &DL,
                                          SelectionDAG &DAG) const
{
    if(LHS.getValueType().isFloatingPoint())
    {
        E64CC = fpCondCode(CC);
        return DAG.getNode(Emex64ISD::FCMP, DL, MVT::Glue, LHS, RHS);
    }

    if(ISD::isUnsignedIntSetCC(CC))
    {
        SDValue Sign = DAG.getConstant(APInt::getSignMask(64), DL, MVT::i64);
        LHS = DAG.getNode(ISD::XOR, DL, MVT::i64, LHS, Sign);
        RHS = DAG.getNode(ISD::XOR, DL, MVT::i64, RHS, Sign);
        switch(CC)
        {
            case ISD::SETULT:
                CC = ISD::SETLT;
                break;
            case ISD::SETUGT:
                CC = ISD::SETGT;
                break;
            case ISD::SETULE:
                CC = ISD::SETLE;
                break;
            case ISD::SETUGE:
                CC = ISD::SETGE;
                break;
            default:
                break;
        }
    }

    switch(CC)
    {
        case ISD::SETEQ:
            E64CC = Emex64CC::EQ;
            break;
        case ISD::SETNE:
            E64CC = Emex64CC::NE;
            break;
        case ISD::SETLT:
            E64CC = Emex64CC::LT;
            break;
        case ISD::SETGT:
            E64CC = Emex64CC::GT;
            break;
        case ISD::SETLE:
            E64CC = Emex64CC::LE;
            break;
        case ISD::SETGE:
            E64CC = Emex64CC::GE;
            break;
        default:
            llvm_unreachable("unexpected integer condition");
    }

    return DAG.getNode(Emex64ISD::CMP, DL, MVT::Glue, LHS, RHS);
}

SDValue Emex64TargetLowering::lowerBR_CC(SDValue Op,
                                         SelectionDAG &DAG) const
{
    SDValue Chain = Op.getOperand(0);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(1))->get();
    SDValue LHS = Op.getOperand(2);
    SDValue RHS = Op.getOperand(3);
    SDValue Dest = Op.getOperand(4);
    SDLoc DL(Op);
    
    if(LHS.getValueType().isInteger() && (CC == ISD::SETEQ || CC == ISD::SETNE))
    {
        if(isNullConstant(LHS))
        {
            std::swap(LHS, RHS);
        }
        if(isNullConstant(RHS))
        {
            return DAG.getNode(CC == ISD::SETEQ ? Emex64ISD::BZ : Emex64ISD::BNZ, DL, MVT::Other, Chain, LHS, Dest);
        }
    }
    
    if(Subtarget.hasISA20() && LHS.getValueType().isInteger())
    {
        unsigned Opc;
        switch(CC)
        {
            case ISD::SETEQ:
                Opc = Emex64ISD::CBEQ;
                break;
            case ISD::SETNE:
                Opc = Emex64ISD::CBNE;
                break;
            case ISD::SETLT:
                Opc = Emex64ISD::CBLT;
                break;
            case ISD::SETGE:
                Opc = Emex64ISD::CBGE;
                break;
            case ISD::SETGT:
                Opc = Emex64ISD::CBLT;
                std::swap(LHS, RHS);
                break;
            case ISD::SETLE:
                Opc = Emex64ISD::CBGE;
                std::swap(LHS, RHS);
                break;
            case ISD::SETULT:
                Opc = Emex64ISD::CBLTU;
                break;
            case ISD::SETUGE:
                Opc = Emex64ISD::CBGEU;
                break;
            case ISD::SETUGT:
                Opc = Emex64ISD::CBLTU;
                std::swap(LHS, RHS);
                break;
            case ISD::SETULE:
                Opc = Emex64ISD::CBGEU;
                std::swap(LHS, RHS);
                break;
            default:
                Opc = 0;
                break;
        }
        
        if(Opc)
        {
            return DAG.getNode(Opc, DL, MVT::Other, Chain, LHS, RHS, Dest);
        }
    }
    
    if(Subtarget.hasISA19() && LHS.getValueType().isInteger())
    {
        if(auto *C = dyn_cast<ConstantSDNode>(RHS))
        {
            const APInt &V = C->getAPIntValue();
            if(CC == ISD::SETLE && !V.isMaxSignedValue())
            {
                CC = ISD::SETLT;
                RHS = DAG.getConstant(V + 1, DL, MVT::i64);
            }
            else if (CC == ISD::SETGE && !V.isMinSignedValue())
            {
                CC = ISD::SETGT;
                RHS = DAG.getConstant(V - 1, DL, MVT::i64);
            }
            else if (CC == ISD::SETULE && !V.isMaxValue())
            {
                CC = ISD::SETULT;
                RHS = DAG.getConstant(V + 1, DL, MVT::i64);
            }
            else if (CC == ISD::SETUGE && !V.isMinValue())
            {
                CC = ISD::SETUGT;
                RHS = DAG.getConstant(V - 1, DL, MVT::i64);
            }
        }
        
        if(CC == ISD::SETLT || CC == ISD::SETGT || CC == ISD::SETULT || CC == ISD::SETUGT)
        {
            if(CC == ISD::SETULT || CC == ISD::SETUGT)
            {
                SDValue Sign = DAG.getConstant(APInt::getSignMask(64), DL, MVT::i64);
                LHS = DAG.getNode(ISD::XOR, DL, MVT::i64, LHS, Sign);
                RHS = DAG.getNode(ISD::XOR, DL, MVT::i64, RHS, Sign);
            }
            if(CC == ISD::SETGT || CC == ISD::SETUGT)
            {
                std::swap(LHS, RHS);
            }
            
            return DAG.getNode(Emex64ISD::CBLT, DL, MVT::Other, Chain, LHS, RHS, Dest);
        }
    }
    
    unsigned E64CC;
    SDValue Glue = emitCompare(LHS, RHS, CC, E64CC, DL, DAG);
    return DAG.getNode(Emex64ISD::BR_CC, DL, MVT::Other, Chain, Dest, DAG.getTargetConstant(E64CC, DL, MVT::i64), Glue);
}

SDValue Emex64TargetLowering::lowerSELECT_CC(SDValue Op,
                                             SelectionDAG &DAG) const
{
    SDValue LHS = Op.getOperand(0);
    SDValue RHS = Op.getOperand(1);
    SDValue TrueV = Op.getOperand(2);
    SDValue FalseV = Op.getOperand(3);
    ISD::CondCode CC = cast<CondCodeSDNode>(Op.getOperand(4))->get();
    SDLoc DL(Op);

    unsigned E64CC;
    SDValue Glue = emitCompare(LHS, RHS, CC, E64CC, DL, DAG);
    return DAG.getNode(Emex64ISD::SELECT_CC, DL, Op.getValueType(), TrueV, FalseV, DAG.getTargetConstant(E64CC, DL, MVT::i64), Glue);
}

SDValue Emex64TargetLowering::lowerVASTART(SDValue Op,
                                           SelectionDAG &DAG) const
{
    MachineFunction &MF = DAG.getMachineFunction();
    auto *FuncInfo = MF.getInfo<Emex64MachineFunctionInfo>();
    SDLoc DL(Op);
    SDValue FI = DAG.getFrameIndex(FuncInfo->getVarArgsFrameIndex(), MVT::i64);
    const Value *SV = cast<SrcValueSDNode>(Op.getOperand(2))->getValue();
    return DAG.getStore(Op.getOperand(0), DL, FI, Op.getOperand(1), MachinePointerInfo(SV));
}

SDValue Emex64TargetLowering::lowerDYNAMIC_STACKALLOC(SDValue Op,
                                                      SelectionDAG &DAG) const
{
    SDValue Chain = Op.getOperand(0);
    SDValue Size = Op.getOperand(1);
    MaybeAlign A = cast<ConstantSDNode>(Op.getOperand(2))->getMaybeAlignValue();
    SDLoc DL(Op);

    SDValue SP = DAG.getCopyFromReg(Chain, DL, Emex64::SP, MVT::i64);
    Chain = SP.getValue(1);
    Size = DAG.getNode(ISD::AND, DL, MVT::i64, DAG.getNode(ISD::ADD, DL, MVT::i64, Size, DAG.getConstant(7, DL, MVT::i64)), DAG.getConstant(~7ULL, DL, MVT::i64));
    SDValue Ptr = DAG.getNode(ISD::ADD, DL, MVT::i64, DAG.getNode(ISD::SUB, DL, MVT::i64, SP, Size), DAG.getConstant(8, DL, MVT::i64));
    if(A && A->value() > 8)
    {
        Ptr = DAG.getNode(ISD::AND, DL, MVT::i64, Ptr, DAG.getSignedConstant(-(int64_t)A->value(), DL, MVT::i64));
    }
    SDValue NewSP = DAG.getNode(ISD::SUB, DL, MVT::i64, Ptr, DAG.getConstant(8, DL, MVT::i64));
    Chain = DAG.getCopyToReg(Chain, DL, Emex64::SP, NewSP);
    return DAG.getMergeValues({Ptr, Chain}, DL);
}

SDValue Emex64TargetLowering::lowerBITCAST(SDValue Op,
                                           SelectionDAG &DAG) const
{
    SDLoc DL(Op);
    SDValue Src = Op.getOperand(0);
    if(Op.getValueType() == MVT::f32 && Src.getValueType() == MVT::i32)
    {
        SDValue Ext = DAG.getNode(ISD::ANY_EXTEND, DL, MVT::i64, Src);
        return DAG.getNode(Emex64ISD::FMV_W_X, DL, MVT::f32, Ext);
    }
    return SDValue();
}

SDValue Emex64TargetLowering::lowerFRAMEADDR(SDValue Op,
                                             SelectionDAG &DAG) const
{
    MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
    MFI.setFrameAddressIsTaken(true);
    SDLoc DL(Op);
    unsigned Depth = Op.getConstantOperandVal(0);
    SDValue Frame = DAG.getCopyFromReg(DAG.getEntryNode(), DL, Emex64::FP, MVT::i64);
    while(Depth--)
    {
        SDValue Ptr = DAG.getNode(ISD::ADD, DL, MVT::i64, Frame, DAG.getConstant(8, DL, MVT::i64));
        Frame = DAG.getLoad(MVT::i64, DL, DAG.getEntryNode(), Ptr, MachinePointerInfo());
    }
    return Frame;
}

SDValue Emex64TargetLowering::lowerRETURNADDR(SDValue Op,
                                              SelectionDAG &DAG) const
{
    MachineFrameInfo &MFI = DAG.getMachineFunction().getFrameInfo();
    MFI.setReturnAddressIsTaken(true);
    SDLoc DL(Op);
    SDValue Frame = lowerFRAMEADDR(Op, DAG);
    SDValue Ptr = DAG.getNode(ISD::ADD, DL, MVT::i64, Frame, DAG.getConstant(16, DL, MVT::i64));
    SDValue RA = DAG.getLoad(MVT::i64, DL, DAG.getEntryNode(), Ptr, MachinePointerInfo());
    if(Subtarget.hasPAuth())
    {
        RA = DAG.getNode(Emex64ISD::XPAC, DL, MVT::i64, RA);
    }
    return RA;
}

MachineBasicBlock *Emex64TargetLowering::EmitInstrWithCustomInserter(MachineInstr &MI,
                                                                     MachineBasicBlock *BB) const
{
    const TargetInstrInfo &TII = *Subtarget.getInstrInfo();
    DebugLoc DL = MI.getDebugLoc();
    assert((MI.getOpcode() == Emex64::SELECT_GPR || MI.getOpcode() == Emex64::SELECT_F64 || MI.getOpcode() == Emex64::SELECT_F32) && "unexpected custom inserter");
    
    const BasicBlock *LLVMBB = BB->getBasicBlock();
    MachineFunction::iterator It = ++BB->getIterator();
    MachineFunction *F = BB->getParent();
    MachineBasicBlock *ThisMBB = BB;
    MachineBasicBlock *FalseMBB = F->CreateMachineBasicBlock(LLVMBB);
    MachineBasicBlock *SinkMBB = F->CreateMachineBasicBlock(LLVMBB);
    F->insert(It, FalseMBB);
    F->insert(It, SinkMBB);

    SinkMBB->splice(SinkMBB->begin(), BB, std::next(MachineBasicBlock::iterator(MI)), BB->end());
    SinkMBB->transferSuccessorsAndUpdatePHIs(BB);

    BB->addSuccessor(FalseMBB);
    BB->addSuccessor(SinkMBB);
    BuildMI(BB, DL, TII.get(Emex64::BRCC)).addMBB(SinkMBB).addImm(MI.getOperand(3).getImm());

    FalseMBB->addSuccessor(SinkMBB);

    BuildMI(*SinkMBB, SinkMBB->begin(), DL, TII.get(Emex64::PHI), MI.getOperand(0).getReg()).addReg(MI.getOperand(1).getReg()).addMBB(ThisMBB).addReg(MI.getOperand(2).getReg()).addMBB(FalseMBB);
    
    MI.eraseFromParent();
    return SinkMBB;
}

//===----------------------------------------------------------------------===//
// Inline assembly
//===----------------------------------------------------------------------===//

TargetLowering::ConstraintType Emex64TargetLowering::getConstraintType(StringRef Constraint) const
{
    if(Constraint.size() == 1)
    {
        switch(Constraint[0])
        {
            case 'r':
            case 'f':
                return C_RegisterClass;
            default:
                break;
        }
    }
    return TargetLowering::getConstraintType(Constraint);
}

std::pair<unsigned, const TargetRegisterClass *> Emex64TargetLowering::getRegForInlineAsmConstraint(const TargetRegisterInfo *TRI,
                                                                                                    StringRef Constraint,
                                                                                                    MVT VT) const
{
    if(Constraint.size() == 1)
    {
        switch(Constraint[0])
        {
            case 'r':
                if(VT.isFloatingPoint() && Subtarget.hasFPU())
                {
                    return {0U, &Emex64::FPRRegClass};
                }
                return {0U, &Emex64::GPRRegClass};
            case 'f':
                if(Subtarget.hasFPU())
                {
                    return {0U, &Emex64::FPRRegClass};
                }
                break;
            default:
                break;
        }
    }
    return TargetLowering::getRegForInlineAsmConstraint(TRI, Constraint, VT);
}

Register Emex64TargetLowering::getRegisterByName(const char *RegName,
                                                 LLT VT,
                                                 const MachineFunction &MF) const
{
    Register Reg = StringSwitch<Register>(RegName).Case("sp", Emex64::SP).Case("fp", Emex64::FP).Default(Register());
    if(Reg)
    {
        return Reg;
    }
    report_fatal_error(Twine("invalid register name \"") + RegName + "\"");
}
