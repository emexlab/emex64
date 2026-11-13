//===-- Emex64RegisterInfo.td ------------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

class Emex64Reg<bits<6> Enc, string n, list<string> alt = []>: Register<n, alt>
{
    let HWEncoding{5-0} = Enc;
    let Namespace = "Emex64";
}

def PC : Emex64Reg<0,  "pc">,  DwarfRegNum<[0]>;
def SP : Emex64Reg<1,  "sp">,  DwarfRegNum<[1]>;
def FP : Emex64Reg<2,  "fp">,  DwarfRegNum<[2]>;
def CF : Emex64Reg<3,  "cf">,  DwarfRegNum<[3]>;
def FPC: Emex64Reg<4,  "fpc">, DwarfRegNum<[4]>;
foreach I = 0-9 in
{
    def R#I : Emex64Reg<!add(5, I), "r"#I>, DwarfRegNum<[!add(5, I)]>;
}
def RR  : Emex64Reg<15, "rr">,  DwarfRegNum<[15]>;

foreach I = 0-15 in
{
    def ER#I : Emex64Reg<!add(16, I), "er"#I>, DwarfRegNum<[!add(16, I)]>;
}

foreach I = 0-15 in
{
    def FR#I : Emex64Reg<!add(32, I), "fr"#I>, DwarfRegNum<[!add(32, I)]>;
}

def GPR : RegisterClass<"Emex64", [i64], 64,
    (add R0, R1, R2, R3, R4, R5, RR,
        (sequence "ER%u", 0, 7),
        R6, R7, R8, R9,
        (sequence "ER%u", 8, 15),
        SP, FP, PC)>;

def FPR : RegisterClass<"Emex64", [f64, f32], 64, (add (sequence "FR%u", 0, 15))>;

def CCR : RegisterClass<"Emex64", [i64], 64, (add CF)>
{
    let CopyCost = -1;
    let isAllocatable = 0;
}

def SPECIAL : RegisterClass<"Emex64", [i64], 64, (add PC, FPC)>
{
    let isAllocatable = 0;
}
