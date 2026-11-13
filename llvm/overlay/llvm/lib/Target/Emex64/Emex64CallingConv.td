//===-- Emex64CallingConv.td -------------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

def CC_Emex64 : CallingConv<[
    CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,

    CCIfArgVarArg<CCIfType<[f32], CCPromoteToType<f64>>>,
    CCIfArgVarArg<CCAssignToStack<8, 8>>,
    
    CCIfType<[i64], CCAssignToReg<[R0, R1, R2, R3, R4, R5]>>,
    CCIfType<[f32, f64], CCAssignToReg<[FR0, FR1, FR2, FR3, FR4, FR5, FR6, FR7]>>,
    
    CCAssignToStack<8, 8>
]>;

def RetCC_Emex64 : CallingConv<[
    CCIfType<[i1, i8, i16, i32], CCPromoteToType<i64>>,
    CCIfType<[i64], CCAssignToReg<[RR, R0]>>,
    CCIfType<[f32, f64], CCAssignToReg<[FR0, FR1]>>
]>;

def CSR_Emex64 : CalleeSavedRegs<(add R6, R7, R8, R9, (sequence "ER%u", 8, 14), (sequence "FR%u", 8, 15))>;
def CSR_Emex64_AllRegs : CalleeSavedRegs<(add (sequence "R%u", 0, 9), RR, (sequence "ER%u", 0, 15), (sequence "FR%u", 0, 15))>;
