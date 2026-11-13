//===-- Emex64.td ------------------------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

include "llvm/Target/Target.td"

//===----------------------------------------------------------------------===//
// Subtarget features
//===----------------------------------------------------------------------===//

def FeatureFPU : SubtargetFeature<"fpu", "HasFPU", "true", "The floating point unit (ISA v18)">;
def HasFPU : Predicate<"Subtarget->hasFPU()">, AssemblerPredicate<(all_of FeatureFPU), "fpu">;

def FeaturePAuth : SubtargetFeature<"pauth", "HasPAuth", "true", "emex64e pointer authentication: pacia, autia, pacda, autda, xpac, pacret, reta">;
def HasPAuth : Predicate<"Subtarget->hasPAuth()">, AssemblerPredicate<(all_of FeaturePAuth), "pauth">;

def FeatureISA19 : SubtargetFeature<"isa19", "HasISA19", "true", "ISA v19: sext, cblt, clz, ctz, popcnt, mulh, umulh, adc, sbb">;
def HasISA19 : Predicate<"Subtarget->hasISA19()">, AssemblerPredicate<(all_of FeatureISA19), "isa19">;

def FeatureISA20 : SubtargetFeature<"isa20", "HasISA20", "true", "ISA v20: rdtime, wfe/sev/pause, cbeq/cbne/cbge/cbltu/cbgeu, ldsb/ldsw/ldsd, ldadd/ldand/ldor/ldxor/swpadd, tlbi", [FeatureFPU, FeatureISA19]>;
def HasISA20 : Predicate<"Subtarget->hasISA20()">, AssemblerPredicate<(all_of FeatureISA20), "isa20">;

def FeatureISA21 : SubtargetFeature<"isa21", "HasISA21", "true", "ISA v21: double word arithmetic (addd/iaddd, subd/isubd, muld/imuld, shld/ishld, shrd, sard, divd, idivd, modd)", [FeatureISA20]>;
def HasISA21 : Predicate<"Subtarget->hasISA21()">, AssemblerPredicate<(all_of FeatureISA21), "isa21">;

include "Emex64RegisterInfo.td"
include "Emex64CallingConv.td"
include "Emex64InstrInfo.td"

def Emex64InstrInfo : InstrInfo;

defm : RemapAllTargetPseudoPointerOperands<GPR>;

def Emex64AsmParser : AsmParser
{
    let ShouldEmitMatchRegisterName = 1;
}

def Emex64AsmWriter : AsmWriter
{
    int PassSubtarget = 1;
}

//===----------------------------------------------------------------------===//
// Processors
//===----------------------------------------------------------------------===//

def : ProcessorModel<"generic", NoSchedModel, [FeatureFPU, FeatureISA19, FeatureISA20, FeatureISA21, FeaturePAuth]>;
def : ProcessorModel<"emex64-v17", NoSchedModel, []>;
def : ProcessorModel<"emex64-v18", NoSchedModel, [FeatureFPU]>;
def : ProcessorModel<"emex64-v19", NoSchedModel, [FeatureFPU, FeatureISA19]>;
def : ProcessorModel<"emex64-v20", NoSchedModel, [FeatureFPU, FeatureISA19, FeatureISA20]>;
def : ProcessorModel<"emex64e", NoSchedModel, [FeatureFPU, FeatureISA19, FeaturePAuth]>;
def : ProcessorModel<"emex64e-v20", NoSchedModel, [FeatureFPU, FeatureISA19, FeatureISA20, FeaturePAuth]>;
def : ProcessorModel<"emex64-v21", NoSchedModel, [FeatureFPU, FeatureISA19, FeatureISA20, FeatureISA21]>;
def : ProcessorModel<"emex64e-v21", NoSchedModel, [FeatureFPU, FeatureISA19, FeatureISA20, FeatureISA21, FeaturePAuth]>;

def Emex64 : Target {
    let InstructionSet = Emex64InstrInfo;
    let AssemblyParsers = [Emex64AsmParser];
    let AssemblyWriters = [Emex64AsmWriter];
    let AllowRegisterRenaming = 1;
}
