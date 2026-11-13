//===-- Emex64InstrInfo.td ---------------------------------*- tablegen -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Formats
//===----------------------------------------------------------------------===//

class E64Inst<bits<8> op, bits<6> maxargs, dag outs, dag ins, string asmstr, list<dag> pattern = []> : Instruction
{
    let Namespace = "Emex64";
    let OutOperandList = outs;
    let InOperandList = ins;
    let AsmString = asmstr;
    let Pattern = pattern;
    let Size = 0;
    let TSFlags{7-0} = op;
    let TSFlags{13-8} = maxargs;
    let hasSideEffects = 0;
    let mayLoad = 0;
    let mayStore = 0;
}

class E64Pseudo<dag outs, dag ins, string asmstr, list<dag> pattern = []> : E64Inst<0, 0, outs, ins, asmstr, pattern>
{
    let isPseudo = 1;
    let isCodeGenOnly = 1;
}

//===----------------------------------------------------------------------===//
// Operands
//===----------------------------------------------------------------------===//

def Emex64ImmAsmOperand : AsmOperandClass { let Name = "Imm64"; let PredicateMethod = "isImm"; let RenderMethod = "addImmOperands"; }
def Emex64MemRIAsmOperand : AsmOperandClass { let Name = "MemRI"; let RenderMethod = "addMemRIOperands"; }
def Emex64MemRRAsmOperand : AsmOperandClass { let Name = "MemRR"; let RenderMethod = "addMemRROperands"; }
def Emex64TargetAsmOperand : AsmOperandClass { let Name = "Target"; let RenderMethod = "addImmOperands"; }

def imm64 : Operand<i64>
{
    let ParserMatchClass = Emex64ImmAsmOperand;
    let OperandType = "OPERAND_IMMEDIATE";
}

def memri : Operand<iPTR>
{
    let PrintMethod = "printMemOperand";
    let MIOperandInfo = (ops GPR:$base, i64imm:$off);
    let ParserMatchClass = Emex64MemRIAsmOperand;
    let OperandType = "OPERAND_MEMORY";
    let EncoderMethod = "getMemEncoding";
}

def memrr : Operand<iPTR>
{
    let PrintMethod = "printMemOperand";
    let MIOperandInfo = (ops GPR:$base, GPR:$index);
    let ParserMatchClass = Emex64MemRRAsmOperand;
    let OperandType = "OPERAND_MEMORY";
    let EncoderMethod = "getMemEncoding";
}

def brtarget : Operand<OtherVT>
{
    let ParserMatchClass = Emex64TargetAsmOperand;
    let OperandType = "OPERAND_PCREL";
}

def calltarget : Operand<i64>
{
    let ParserMatchClass = Emex64TargetAsmOperand;
    let OperandNamespace = "Emex64Op";
    let OperandType = "OPERAND_CALLTARGET";
}

def condcode : Operand<i64>
{
    let PrintMethod = "printCondCode";
}

def addr : ComplexPattern<iPTR, 2, "SelectAddr", [frameindex], []>;
def addrrr : ComplexPattern<iPTR, 2, "SelectAddrRR", [], [], 10>;

def fpimm64bits : SDNodeXForm<fpimm, [{
    return CurDAG->getTargetConstant(N->getValueAPF().bitcastToAPInt().getZExtValue(), SDLoc(N), MVT::i64);
}]>;

//===----------------------------------------------------------------------===//
// Target nodes
//===----------------------------------------------------------------------===//

def SDT_E64Call : SDTypeProfile<0, -1, [SDTCisVT<0, iPTR>]>;
def SDT_E64CallSeqStart : SDCallSeqStart<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def SDT_E64CallSeqEnd : SDCallSeqEnd<[SDTCisVT<0, i64>, SDTCisVT<1, i64>]>;
def SDT_E64Wrapper : SDTypeProfile<1, 1, [SDTCisSameAs<0, 1>, SDTCisPtrTy<0>]>;
def SDT_E64Cmp : SDTypeProfile<0, 2, [SDTCisSameAs<0, 1>]>;
def SDT_E64BrCC : SDTypeProfile<0, 2, [SDTCisVT<0, OtherVT>, SDTCisVT<1, i64>]>;
def SDT_E64BrZ : SDTypeProfile<0, 2, [SDTCisVT<0, i64>, SDTCisVT<1, OtherVT>]>;
def SDT_E64CBLT : SDTypeProfile<0, 3, [SDTCisVT<0, i64>, SDTCisVT<1, i64>, SDTCisVT<2, OtherVT>]>;
def SDT_E64SelectCC : SDTypeProfile<1, 3, [SDTCisSameAs<0, 1>, SDTCisSameAs<1, 2>, SDTCisVT<3, i64>]>;
def SDT_E64FMVXW : SDTypeProfile<1, 1, [SDTCisVT<0, i64>, SDTCisVT<1, f32>]>;
def SDT_E64FMVWX : SDTypeProfile<1, 1, [SDTCisVT<0, f32>, SDTCisVT<1, i64>]>;
def SDT_E64Mem3 : SDTypeProfile<0, 3, [SDTCisVT<0, i64>, SDTCisVT<1, i64>, SDTCisVT<2, i64>]>;

def Emex64retglue : SDNode<"Emex64ISD::RET_GLUE", SDTNone, [SDNPHasChain, SDNPOptInGlue, SDNPVariadic]>;
def Emex64call : SDNode<"Emex64ISD::CALL", SDT_E64Call, [SDNPHasChain, SDNPOutGlue, SDNPOptInGlue, SDNPVariadic]>;
def Emex64callseq_start : SDNode<"ISD::CALLSEQ_START", SDT_E64CallSeqStart, [SDNPHasChain, SDNPOutGlue]>;
def Emex64callseq_end : SDNode<"ISD::CALLSEQ_END", SDT_E64CallSeqEnd, [SDNPHasChain, SDNPOptInGlue, SDNPOutGlue]>;
def Emex64wrapper : SDNode<"Emex64ISD::WRAPPER", SDT_E64Wrapper>;
def Emex64pcwrapper : SDNode<"Emex64ISD::PCWRAPPER", SDT_E64Wrapper>;
def Emex64cmp : SDNode<"Emex64ISD::CMP", SDT_E64Cmp, [SDNPOutGlue]>;
def Emex64fcmp : SDNode<"Emex64ISD::FCMP", SDT_E64Cmp, [SDNPOutGlue]>;
def Emex64brcc : SDNode<"Emex64ISD::BR_CC", SDT_E64BrCC, [SDNPHasChain, SDNPInGlue]>;
def Emex64bz : SDNode<"Emex64ISD::BZ", SDT_E64BrZ, [SDNPHasChain]>;
def Emex64bnz : SDNode<"Emex64ISD::BNZ", SDT_E64BrZ, [SDNPHasChain]>;
def Emex64cblt : SDNode<"Emex64ISD::CBLT", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64cbeq : SDNode<"Emex64ISD::CBEQ", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64cbne : SDNode<"Emex64ISD::CBNE", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64cbge : SDNode<"Emex64ISD::CBGE", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64cbltu : SDNode<"Emex64ISD::CBLTU", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64cbgeu : SDNode<"Emex64ISD::CBGEU", SDT_E64CBLT, [SDNPHasChain]>;
def Emex64xpac : SDNode<"Emex64ISD::XPAC", SDTIntUnaryOp>;
def Emex64selectcc : SDNode<"Emex64ISD::SELECT_CC", SDT_E64SelectCC, [SDNPInGlue]>;
def Emex64fmvxw : SDNode<"Emex64ISD::FMV_X_W", SDT_E64FMVXW>;
def Emex64fmvwx : SDNode<"Emex64ISD::FMV_W_X", SDT_E64FMVWX>;
def Emex64cpym : SDNode<"Emex64ISD::CPYM", SDT_E64Mem3, [SDNPHasChain, SDNPMayLoad, SDNPMayStore]>;
def Emex64movm : SDNode<"Emex64ISD::MOVM", SDT_E64Mem3, [SDNPHasChain, SDNPMayLoad, SDNPMayStore]>;
def Emex64setm : SDNode<"Emex64ISD::SETM", SDT_E64Mem3, [SDNPHasChain, SDNPMayStore]>;

//===----------------------------------------------------------------------===//
// Pseudo instructions
//===----------------------------------------------------------------------===//

let Defs = [SP], Uses = [SP] in {
    def ADJCALLSTACKDOWN : E64Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2), "#ADJCALLSTACKDOWN $amt1 $amt2", [(Emex64callseq_start timm:$amt1, timm:$amt2)]>;
    def ADJCALLSTACKUP : E64Pseudo<(outs), (ins i64imm:$amt1, i64imm:$amt2), "#ADJCALLSTACKUP $amt1 $amt2", [(Emex64callseq_end timm:$amt1, timm:$amt2)]>;
}

let usesCustomInserter = 1, Uses = [CF] in {
    def SELECT_GPR : E64Pseudo<(outs GPR:$dst), (ins GPR:$t, GPR:$f, condcode:$cc), "# SELECT_GPR", [(set i64:$dst, (Emex64selectcc i64:$t, i64:$f, timm:$cc))]>;
    def SELECT_F64 : E64Pseudo<(outs FPR:$dst), (ins FPR:$t, FPR:$f, condcode:$cc), "# SELECT_F64", [(set f64:$dst, (Emex64selectcc f64:$t, f64:$f, timm:$cc))]>;
    def SELECT_F32 : E64Pseudo<(outs FPR:$dst), (ins FPR:$t, FPR:$f, condcode:$cc), "# SELECT_F32", [(set f32:$dst, (Emex64selectcc f32:$t, f32:$f, timm:$cc))]>;
}

let isBranch = 1, isTerminator = 1, Uses = [CF] in
def BRCC : E64Pseudo<(outs), (ins brtarget:$dst, condcode:$cc), "# BRCC $cc, $dst", [(Emex64brcc bb:$dst, timm:$cc)]>;
let Defs = [CF], Uses = [SP], hasSideEffects = 1 in
def FENCE : E64Pseudo<(outs), (ins), "# FENCE", [(atomic_fence timm, timm)]>;
let isCodeGenOnly = 1, hasSideEffects = 1, mayLoad = 1, mayStore = 1 in
def RAW : E64Inst<0, 0, (outs), (ins variable_ops), "#RAW">;

//===----------------------------------------------------------------------===//
// Core and system
//===----------------------------------------------------------------------===//

let hasSideEffects = 1 in {
def HLT : E64Inst<0x00, 0, (outs), (ins), "hlt">;
def NOP : E64Inst<0x01, 0, (outs), (ins), "nop">;
def SVC : E64Inst<0x3F, 0, (outs), (ins), "svc">;
let isReturn = 1, isTerminator = 1, isBarrier = 1 in
def ERET : E64Inst<0x40, 0, (outs), (ins), "eret">;
def CMOV : E64Inst<0x36, 2, (outs), (ins imm64:$cr, GPR:$src), "cmov $cr, $src">;
def CMOVB : E64Inst<0x37, 2, (outs GPR:$dst), (ins imm64:$cr), "cmovb $dst, $cr">;
def CSWP : E64Inst<0x41, 2, (outs GPR:$dst), (ins imm64:$cr, GPR:$src), "cswp $cr, $dst"> {
  let Constraints = "$dst = $src";
}
def RDRND : E64Inst<0x3B, 1, (outs GPR:$dst), (ins), "rdrnd $dst">;
}

//===----------------------------------------------------------------------===//
// Data
//===----------------------------------------------------------------------===//

let isMoveReg = 1 in
{
    def MOVrr : E64Inst<0x02, 2, (outs GPR:$dst), (ins GPR:$src), "mov $dst, $src">;
    def FMOVff : E64Inst<0x02, 2, (outs FPR:$dst), (ins FPR:$src), "mov $dst, $src">;
}

def FMOVfr : E64Inst<0x02, 2, (outs FPR:$dst), (ins GPR:$src), "mov $dst, $src", [(set f64:$dst, (bitconvert i64:$src))]>;
def FMOVrf : E64Inst<0x02, 2, (outs GPR:$dst), (ins FPR:$src), "mov $dst, $src", [(set i64:$dst, (bitconvert f64:$src))]>;
def FMOVfr32 : E64Inst<0x02, 2, (outs FPR:$dst), (ins GPR:$src), "mov $dst, $src", [(set f32:$dst, (Emex64fmvwx i64:$src))]>;
def FMOVrf32 : E64Inst<0x02, 2, (outs GPR:$dst), (ins FPR:$src), "mov $dst, $src", [(set i64:$dst, (Emex64fmvxw f32:$src))]>;

let isReMaterializable = 1, isAsCheapAsAMove = 1, isMoveImm = 1 in
{
    def MOVri : E64Inst<0x02, 2, (outs GPR:$dst), (ins imm64:$imm), "mov $dst, $imm", [(set i64:$dst, imm:$imm)]>;
    def FMOVfi : E64Inst<0x02, 2, (outs FPR:$dst), (ins imm64:$imm), "mov $dst, $imm">;
}

def : Pat<(f64 fpimm:$v), (FMOVfi (fpimm64bits fpimm:$v))>;
def : Pat<(f32 fpimm:$v), (FMOVfi (fpimm64bits fpimm:$v))>;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in
{
    def MOVrm : E64Inst<0x02, 2, (outs GPR:$dst), (ins memri:$addr), "mov $dst, $addr">;
}
def MOVrmm : E64Inst<0x02, 2, (outs GPR:$dst), (ins memrr:$addr), "mov $dst, $addr">;

def : Pat<(Emex64wrapper tglobaladdr:$g), (MOVri tglobaladdr:$g)>;
def : Pat<(Emex64wrapper texternalsym:$g), (MOVri texternalsym:$g)>;
def : Pat<(Emex64wrapper tblockaddress:$g), (MOVri tblockaddress:$g)>;
def : Pat<(Emex64wrapper tjumptable:$g), (MOVri tjumptable:$g)>;
def : Pat<(Emex64wrapper tconstpool:$g), (MOVri tconstpool:$g)>;

multiclass Load<bits<8> op, string name, RegisterClass RC, list<PatFrag> frags, ValueType vt>
{
  let mayLoad = 1 in
  {
    def ri : E64Inst<op, 2, (outs RC:$dst), (ins memri:$addr), name # " $dst, $addr">;
    def rr : E64Inst<op, 2, (outs RC:$dst), (ins memrr:$addr), name # " $dst, $addr">;
  }
  foreach frag = frags in
  {
    def : Pat<(vt (frag addrrr:$addr)), (!cast<Instruction>(NAME # "rr") addrrr:$addr)>;
    def : Pat<(vt (frag addr:$addr)), (!cast<Instruction>(NAME # "ri") addr:$addr)>;
  }
}

multiclass Store<bits<8> op, string name, RegisterClass RC, PatFrag frag, ValueType vt>
{
  let mayStore = 1 in
  {
    def ri : E64Inst<op, 2, (outs), (ins memri:$addr, RC:$src), name # " $addr, $src">;
    def rr : E64Inst<op, 2, (outs), (ins memrr:$addr, RC:$src), name # " $addr, $src">;
  }
  
  def : Pat<(frag vt:$src, addrrr:$addr), (!cast<Instruction>(NAME # "rr") addrrr:$addr, RC:$src)>;
  def : Pat<(frag vt:$src, addr:$addr), (!cast<Instruction>(NAME # "ri") addr:$addr, RC:$src)>;
}

defm LDB : Load<0x07, "ldb", GPR, [zextloadi8, extloadi8], i64>;
defm LDW : Load<0x08, "ldw", GPR, [zextloadi16, extloadi16], i64>;
defm LDD : Load<0x09, "ldd", GPR, [zextloadi32, extloadi32], i64>;
defm LDQ : Load<0x0A, "ldq", GPR, [load], i64>;
defm STB : Store<0x0B, "stb", GPR, truncstorei8, i64>;
defm STW : Store<0x0C, "stw", GPR, truncstorei16, i64>;
defm STD : Store<0x0D, "std", GPR, truncstorei32, i64>;
defm STQ : Store<0x0E, "stq", GPR, store, i64>;

let Predicates = [HasFPU] in {
    defm FLDD : Load<0x09, "ldd", FPR, [load], f32>;
    defm FLDQ : Load<0x0A, "ldq", FPR, [load], f64>;
    defm FSTD : Store<0x0D, "std", FPR, store, f32>;
    defm FSTQ : Store<0x0E, "stq", FPR, store, f64>;
}

let mayStore = 1 in {
    def STBi : E64Inst<0x0B, 2, (outs), (ins memri:$addr, imm64:$imm), "stb $addr, $imm">;
    def STWi : E64Inst<0x0C, 2, (outs), (ins memri:$addr, imm64:$imm), "stw $addr, $imm">;
    def STDi : E64Inst<0x0D, 2, (outs), (ins memri:$addr, imm64:$imm), "std $addr, $imm">;
    def STQi : E64Inst<0x0E, 2, (outs), (ins memri:$addr, imm64:$imm), "stq $addr, $imm">;
}

def : Pat<(truncstorei8 (i64 imm:$v), addr:$a), (STBi addr:$a, imm:$v)>;
def : Pat<(truncstorei16 (i64 imm:$v), addr:$a), (STWi addr:$a, imm:$v)>;
def : Pat<(truncstorei32 (i64 imm:$v), addr:$a), (STDi addr:$a, imm:$v)>;
def : Pat<(store (i64 imm:$v), addr:$a), (STQi addr:$a, imm:$v)>;

let mayLoad = 1, mayStore = 1, hasSideEffects = 1 in
{
    def XCHG : E64Inst<0x42, 2, (outs GPR:$dst), (ins GPR:$val, memri:$addr), "xchg $dst, $addr", [(set i64:$dst, (atomic_swap_i64 addr:$addr, i64:$val))]>
    {
        let Constraints = "$dst = $val";
    }
    let Defs = [CF] in
    def CAS : E64Inst<0x43, 3, (outs GPR:$dst), (ins GPR:$cmp, memri:$addr, GPR:$new), "cas $dst, $addr, $new", [(set i64:$dst, (atomic_cmp_swap_i64 addr:$addr, i64:$cmp, i64:$new))]>
    {
        let Constraints = "$dst = $cmp";
    }
}

def : Pat<(i64 (atomic_load_nonext_64 addr:$a)), (LDQri addr:$a)>;
def : Pat<(i64 (atomic_load_zext_8 addr:$a)), (LDBri addr:$a)>;
def : Pat<(i64 (atomic_load_zext_16 addr:$a)), (LDWri addr:$a)>;
def : Pat<(i64 (atomic_load_zext_32 addr:$a)), (LDDri addr:$a)>;
def : Pat<(i64 (atomic_load_nonext_8 addr:$a)), (LDBri addr:$a)>;
def : Pat<(i64 (atomic_load_nonext_16 addr:$a)), (LDWri addr:$a)>;
def : Pat<(i64 (atomic_load_nonext_32 addr:$a)), (LDDri addr:$a)>;
def : Pat<(atomic_store_64 i64:$v, addr:$a), (STQri addr:$a, GPR:$v)>;
def : Pat<(atomic_store_32 i64:$v, addr:$a), (STDri addr:$a, GPR:$v)>;
def : Pat<(atomic_store_16 i64:$v, addr:$a), (STWri addr:$a, GPR:$v)>;
def : Pat<(atomic_store_8 i64:$v, addr:$a), (STBri addr:$a, GPR:$v)>;

let mayLoad = 1, mayStore = 1 in {
    def MOVM : E64Inst<0x3C, 3, (outs), (ins GPR:$dst, GPR:$src, GPR:$len), "movm $dst, $src, $len", [(Emex64movm i64:$dst, i64:$src, i64:$len)]>;
    def CPYM : E64Inst<0x3D, 3, (outs), (ins GPR:$dst, GPR:$src, GPR:$len), "cpym $dst, $src, $len", [(Emex64cpym i64:$dst, i64:$src, i64:$len)]>;
}

let mayStore = 1 in
{
    def SETM : E64Inst<0x3E, 3, (outs), (ins GPR:$dst, GPR:$val, GPR:$len), "setm $dst, $val, $len", [(Emex64setm i64:$dst, i64:$val, i64:$len)]>;
}

let mayStore = 1, Defs = [SP], Uses = [SP] in
def PUSH : E64Inst<0x05, 16, (outs), (ins GPR:$src), "push $src">;
let mayLoad = 1, Defs = [SP], Uses = [SP] in
def POP : E64Inst<0x06, 16, (outs GPR:$dst), (ins), "pop $dst">;

//===----------------------------------------------------------------------===//
// ALU
//===----------------------------------------------------------------------===//

multiclass ALU<bits<8> op, string name, SDNode node, bit commutable = 0, bit revimm = 0>
{
  let isCommutable = commutable in
  {
    def rr : E64Inst<op, 3, (outs GPR:$dst), (ins GPR:$a, GPR:$b), name # " $dst, $a, $b", [(set i64:$dst, (node i64:$a, i64:$b))]>;
  }
  def ri : E64Inst<op, 3, (outs GPR:$dst), (ins GPR:$a, imm64:$b), name # " $dst, $a, $b", [(set i64:$dst, (node i64:$a, (i64 imm:$b)))]>;
  if revimm then
  {
    def ir : E64Inst<op, 3, (outs GPR:$dst), (ins imm64:$a, GPR:$b), name # " $dst, $a, $b", [(set i64:$dst, (node (i64 imm:$a), i64:$b))]>;
  }
}

defm ADD : ALU<0x0F, "add", add, 1>;
defm SUB : ALU<0x10, "sub", sub, 0, 1>;
defm MUL : ALU<0x11, "mul", mul, 1>;
defm DIV : ALU<0x12, "div", udiv, 0, 1>;
defm IDIV : ALU<0x13, "idiv", sdiv, 0, 1>;
defm MOD : ALU<0x14, "mod", urem, 0, 1>;
defm AND : ALU<0x17, "and", and, 1>;
defm OR : ALU<0x18, "or", or, 1>;
defm XOR : ALU<0x19, "xor", xor, 1>;
defm SHR : ALU<0x1A, "shr", srl, 0, 1>;
defm SHL : ALU<0x1B, "shl", shl, 0, 1>;
defm SAR : ALU<0x1C, "sar", sra, 0, 1>;

let Constraints = "$dst = $src" in
{
    def NOT : E64Inst<0x15, 16, (outs GPR:$dst), (ins GPR:$src), "not $dst", [(set i64:$dst, (not i64:$src))]>;
    def BSWAPQ : E64Inst<0x23, 1, (outs GPR:$dst), (ins GPR:$src), "bswapq $dst", [(set i64:$dst, (bswap i64:$src))]>;
    def RORrr : E64Inst<0x1D, 3, (outs GPR:$dst), (ins GPR:$src, GPR:$n), "ror $dst, $n", [(set i64:$dst, (rotr i64:$src, i64:$n))]>;
    def RORri : E64Inst<0x1D, 3, (outs GPR:$dst), (ins GPR:$src, imm64:$n), "ror $dst, $n", [(set i64:$dst, (rotr i64:$src, (i64 imm:$n)))]>;
}
def : Pat<(sub 0, i64:$x), (SUBir 0, GPR:$x)>;

def neg_imm32 : PatLeaf<(imm), [{
  int64_t V = N->getSExtValue();
  return V < 0 && V >= -0xFFFFFFFFLL;
}]>;

def negate_imm : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant(-N->getSExtValue(), SDLoc(N), MVT::i64);
}]>;

let isReMaterializable = 1, isAsCheapAsAMove = 1 in
def SUBii : E64Inst<0x10, 3, (outs GPR:$dst), (ins imm64:$a, imm64:$b), "sub $dst, $a, $b">;

let AddedComplexity = 10 in
{
    def : Pat<(add i64:$a, (i64 neg_imm32:$b)), (SUBri GPR:$a, (negate_imm neg_imm32:$b))>;
    def : Pat<(i64 neg_imm32:$v), (SUBii 0, (negate_imm neg_imm32:$v))>;
}

def : Pat<(Emex64pcwrapper tglobaladdr:$g), (ADDri PC, tglobaladdr:$g)>;
def : Pat<(Emex64pcwrapper texternalsym:$g), (ADDri PC, texternalsym:$g)>;
def : Pat<(Emex64pcwrapper tblockaddress:$g), (ADDri PC, tblockaddress:$g)>;
def : Pat<(Emex64pcwrapper tjumptable:$g), (ADDri PC, tjumptable:$g)>;
def : Pat<(Emex64pcwrapper tconstpool:$g), (ADDri PC, tconstpool:$g)>;
def : Pat<(Emex64pcwrapper tglobaltlsaddr:$g), (ADDri PC, tglobaltlsaddr:$g)>;

def : Pat<(add i64:$a, (Emex64wrapper tglobaladdr:$g)), (ADDri GPR:$a, tglobaladdr:$g)>;
def : Pat<(add i64:$a, (Emex64wrapper texternalsym:$g)), (ADDri GPR:$a, texternalsym:$g)>;
def : Pat<(add i64:$a, (Emex64wrapper tconstpool:$g)), (ADDri GPR:$a, tconstpool:$g)>;
def : Pat<(add i64:$a, (Emex64wrapper tglobaltlsaddr:$g)), (ADDri GPR:$a, tglobaltlsaddr:$g)>;
def : Pat<(Emex64wrapper tglobaltlsaddr:$g), (MOVri tglobaltlsaddr:$g)>;

//===----------------------------------------------------------------------===//
// Compares and branches
//===----------------------------------------------------------------------===//

let Defs = [CF] in
{
    def CMPrr : E64Inst<0x27, 2, (outs), (ins GPR:$a, GPR:$b), "cmp $a, $b", [(Emex64cmp i64:$a, i64:$b)]>;
    def CMPri : E64Inst<0x27, 2, (outs), (ins GPR:$a, imm64:$b), "cmp $a, $b", [(Emex64cmp i64:$a, (i64 imm:$b))]>;
    def CMPir : E64Inst<0x27, 2, (outs), (ins imm64:$a, GPR:$b), "cmp $a, $b", [(Emex64cmp (i64 imm:$a), i64:$b)]>;
}

let isBranch = 1, isTerminator = 1 in
{
    let isBarrier = 1 in
    {
        let isCodeGenOnly = 1 in
        def B : E64Inst<0x26, 1, (outs), (ins brtarget:$dst), "b $dst", [(br bb:$dst)]>;
        let isIndirectBranch = 1 in
        def BR : E64Inst<0x26, 1, (outs), (ins GPR:$dst), "b $dst", [(brind i64:$dst)]>;
        def BA : E64Inst<0x26, 1, (outs), (ins calltarget:$dst), "b $dst">;
    }

    let Uses = [CF] in
    {
        def BE : E64Inst<0x28, 1, (outs), (ins brtarget:$dst), "be $dst">;
        def BNE : E64Inst<0x29, 1, (outs), (ins brtarget:$dst), "bne $dst">;
        def BLT : E64Inst<0x2A, 1, (outs), (ins brtarget:$dst), "blt $dst">;
        def BGT : E64Inst<0x2B, 1, (outs), (ins brtarget:$dst), "bgt $dst">;
        def BLE : E64Inst<0x2C, 1, (outs), (ins brtarget:$dst), "ble $dst">;
        def BGE : E64Inst<0x2D, 1, (outs), (ins brtarget:$dst), "bge $dst">;
    }

    def BZ : E64Inst<0x2E, 2, (outs), (ins GPR:$a, brtarget:$dst), "bz $a, $dst", [(Emex64bz i64:$a, bb:$dst)]>;
    def BNZ : E64Inst<0x2F, 2, (outs), (ins GPR:$a, brtarget:$dst), "bnz $a, $dst", [(Emex64bnz i64:$a, bb:$dst)]>;
    def BBZ : E64Inst<0x39, 3, (outs), (ins GPR:$a, imm64:$mask, brtarget:$dst), "bbz $a, $mask, $dst">;
    def BBNZ : E64Inst<0x3A, 3, (outs), (ins GPR:$a, imm64:$mask, brtarget:$dst), "bbnz $a, $mask, $dst">;
}

def : Pat<(Emex64bz (and i64:$a, (i64 imm:$m)), bb:$dst), (BBZ GPR:$a, imm:$m, bb:$dst)>;
def : Pat<(Emex64bnz (and i64:$a, (i64 imm:$m)), bb:$dst), (BBNZ GPR:$a, imm:$m, bb:$dst)>;

let isCall = 1, Uses = [SP], Defs = [CF] in
{
    def BL : E64Inst<0x33, 1, (outs), (ins calltarget:$dst, variable_ops), "bl $dst">;
    def BLr : E64Inst<0x33, 1, (outs), (ins GPR:$dst, variable_ops), "bl $dst">;
}

def : Pat<(Emex64call tglobaladdr:$dst), (BL tglobaladdr:$dst)>;
def : Pat<(Emex64call texternalsym:$dst), (BL texternalsym:$dst)>;
def : Pat<(Emex64call i64:$dst), (BLr GPR:$dst)>;

let isReturn = 1, isTerminator = 1, isBarrier = 1 in
def RET : E64Inst<0x34, 0, (outs), (ins), "ret", [(Emex64retglue)]>;

//===----------------------------------------------------------------------===//
// ISA v19
//===----------------------------------------------------------------------===//

let Predicates = [HasISA19] in
{
    def SEXT : E64Inst<0x64, 3, (outs GPR:$dst), (ins GPR:$a, imm64:$n), "sext $dst, $a, $n">;
    def CLZ : E64Inst<0x66, 2, (outs GPR:$dst), (ins GPR:$a), "clz $dst, $a", [(set i64:$dst, (ctlz i64:$a))]>;
    def CTZ : E64Inst<0x67, 2, (outs GPR:$dst), (ins GPR:$a), "ctz $dst, $a", [(set i64:$dst, (cttz i64:$a))]>;
    def POPCNT : E64Inst<0x68, 2, (outs GPR:$dst), (ins GPR:$a), "popcnt $dst, $a", [(set i64:$dst, (ctpop i64:$a))]>;

    defm MULH  : ALU<0x69, "mulh", mulhs, 1>;
    defm UMULH : ALU<0x6A, "umulh", mulhu, 1>;

    let Uses = [CF], Defs = [CF] in
    {
        defm ADC : ALU<0x6B, "adc", adde, 1>;
        defm SBB : ALU<0x6C, "sbb", sube>;
    }

    let Defs = [CF] in
    {
        def ADDCrr : E64Pseudo<(outs GPR:$dst), (ins GPR:$a, GPR:$b), "# ADDC $dst, $a, $b", [(set i64:$dst, (addc i64:$a, i64:$b))]>;
        def ADDCri : E64Pseudo<(outs GPR:$dst), (ins GPR:$a, imm64:$b), "# ADDC $dst, $a, $b", [(set i64:$dst, (addc i64:$a, (i64 imm:$b)))]>;
        def SUBCrr : E64Pseudo<(outs GPR:$dst), (ins GPR:$a, GPR:$b), "# SUBC $dst, $a, $b", [(set i64:$dst, (subc i64:$a, i64:$b))]>;
        def SUBCri : E64Pseudo<(outs GPR:$dst), (ins GPR:$a, imm64:$b), "# SUBC $dst, $a, $b", [(set i64:$dst, (subc i64:$a, (i64 imm:$b)))]>;
    }

    let isBranch = 1, isTerminator = 1 in
    {
        def CBLTrr : E64Inst<0x65, 3, (outs), (ins GPR:$a, GPR:$b, brtarget:$dst), "cblt $a, $b, $dst", [(Emex64cblt i64:$a, i64:$b, bb:$dst)]>;
        def CBLTri : E64Inst<0x65, 3, (outs), (ins GPR:$a, imm64:$b, brtarget:$dst), "cblt $a, $b, $dst", [(Emex64cblt i64:$a, (i64 imm:$b), bb:$dst)]>;
        def CBLTir : E64Inst<0x65, 3, (outs), (ins imm64:$a, GPR:$b, brtarget:$dst), "cblt $a, $b, $dst", [(Emex64cblt (i64 imm:$a), i64:$b, bb:$dst)]>;
    }

    def : Pat<(sext_inreg i64:$a, i1), (SEXT GPR:$a, 1)>;
    def : Pat<(sext_inreg i64:$a, i8), (SEXT GPR:$a, 8)>;
    def : Pat<(sext_inreg i64:$a, i16), (SEXT GPR:$a, 16)>;
    def : Pat<(sext_inreg i64:$a, i32), (SEXT GPR:$a, 32)>;
}

//===----------------------------------------------------------------------===//
// ISA v20
//===----------------------------------------------------------------------===//

multiclass CB<bits<8> op, string name, SDNode node>
{
    let isBranch = 1, isTerminator = 1 in
    {
        def rr : E64Inst<op, 3, (outs), (ins GPR:$a, GPR:$b, brtarget:$dst), name # " $a, $b, $dst", [(node i64:$a, i64:$b, bb:$dst)]>;
        def ri : E64Inst<op, 3, (outs), (ins GPR:$a, imm64:$b, brtarget:$dst), name # " $a, $b, $dst", [(node i64:$a, (i64 imm:$b), bb:$dst)]>;
        def ir : E64Inst<op, 3, (outs), (ins imm64:$a, GPR:$b, brtarget:$dst), name # " $a, $b, $dst", [(node (i64 imm:$a), i64:$b, bb:$dst)]>;
    }
}

class E64AMO<bits<8> op, string name> : E64Inst<op, 3, (outs GPR:$dst), (ins memri:$addr, GPR:$val), name # " $dst, $addr, $val">
{
    let mayLoad = 1;
    let mayStore = 1;
    let hasSideEffects = 1;
}

let Predicates = [HasISA20] in
{
    let hasSideEffects = 1 in
    def RDTIME : E64Inst<0x74, 1, (outs GPR:$dst), (ins), "rdtime $dst">;
    def : Pat<(i64 (readcyclecounter)), (RDTIME)>;

    defm CBEQ : CB<0x78, "cbeq", Emex64cbeq>;
    defm CBNE : CB<0x79, "cbne", Emex64cbne>;
    defm CBGE : CB<0x7A, "cbge", Emex64cbge>;
    defm CBLTU : CB<0x7B, "cbltu", Emex64cbltu>;
    defm CBGEU : CB<0x7C, "cbgeu", Emex64cbgeu>;

    defm LDSB : Load<0x7D, "ldsb", GPR, [sextloadi8], i64>;
    defm LDSW : Load<0x7E, "ldsw", GPR, [sextloadi16], i64>;
    defm LDSD : Load<0x7F, "ldsd", GPR, [sextloadi32], i64>;

    def LDADD : E64AMO<0x80, "ldadd">;
    def LDAND : E64AMO<0x81, "ldand">;
    def LDOR : E64AMO<0x82, "ldor">;
    def LDXOR : E64AMO<0x83, "ldxor">;
    def LDADDD : E64AMO<0x85, "ldaddd">;
    def LDANDD : E64AMO<0x86, "ldandd">;
    def LDORD : E64AMO<0x87, "ldord">;
    def LDXORD : E64AMO<0x88, "ldxord">;

    def : Pat<(atomic_load_add_i64 addr:$a, i64:$v), (LDADD addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_sub_i64 addr:$a, i64:$v), (LDADD addr:$a, (SUBir 0, GPR:$v))>;
    def : Pat<(atomic_load_and_i64 addr:$a, i64:$v), (LDAND addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_or_i64 addr:$a, i64:$v), (LDOR addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_xor_i64 addr:$a, i64:$v), (LDXOR addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_add_i32 addr:$a, i64:$v), (LDADDD addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_sub_i32 addr:$a, i64:$v), (LDADDD addr:$a, (SUBir 0, GPR:$v))>;
    def : Pat<(atomic_load_and_i32 addr:$a, i64:$v), (LDANDD addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_or_i32 addr:$a, i64:$v), (LDORD addr:$a, GPR:$v)>;
    def : Pat<(atomic_load_xor_i32 addr:$a, i64:$v), (LDXORD addr:$a, GPR:$v)>;
}

//===----------------------------------------------------------------------===//
// ISA v21 (double word arithmetic)
//===----------------------------------------------------------------------===//

def zext32 : PatFrag<(ops node:$v), (and node:$v, 0xFFFFFFFF)>;
def sext32 : PatFrag<(ops node:$v), (sext_inreg node:$v, i32)>;

def add_1u : PatFrag<(ops node:$a, node:$b), (add node:$a, node:$b), [{ return N->hasOneUse(); }]>;
def sub_1u : PatFrag<(ops node:$a, node:$b), (sub node:$a, node:$b), [{ return N->hasOneUse(); }]>;
def mul_1u : PatFrag<(ops node:$a, node:$b), (mul node:$a, node:$b), [{ return N->hasOneUse(); }]>;
def shl_1u : PatFrag<(ops node:$a, node:$b), (shl node:$a, node:$b), [{ return N->hasOneUse(); }]>;

def lo32_imm : SDNodeXForm<imm, [{
    return CurDAG->getTargetConstant(N->getZExtValue() & 0xFFFFFFFFULL, SDLoc(N), MVT::i64);
}]>;

def uimm32 : PatLeaf<(imm), [{ return isUInt<32>(N->getZExtValue()); }]>;
def dw_neg_imm : PatLeaf<(imm), [{ return (int32_t)N->getZExtValue() < 0; }]>;
def dw_negate_imm : SDNodeXForm<imm, [{
  return CurDAG->getTargetConstant((uint32_t)(0 - (uint32_t)N->getZExtValue()), SDLoc(N), MVT::i64);
}]>;

let Predicates = [HasISA21] in
{
    multiclass DW<bits<8> op, string name, bit commutable = 0>
    {
        let isCommutable = commutable in
        def rr : E64Inst<op, 3, (outs GPR:$dst), (ins GPR:$a, GPR:$b), name # " $dst, $a, $b">;
        def ri : E64Inst<op, 3, (outs GPR:$dst), (ins GPR:$a, imm64:$b), name # " $dst, $a, $b">;
        def ir : E64Inst<op, 3, (outs GPR:$dst), (ins imm64:$a, GPR:$b), name # " $dst, $a, $b">;
    }

    defm ADDD : DW<0x8B, "addd", 1>;
    defm IADDD : DW<0x8C, "iaddd", 1>;
    defm SUBD : DW<0x8D, "subd">;
    defm ISUBD : DW<0x8E, "isubd">;
    defm MULD : DW<0x8F, "muld", 1>;
    defm IMULD : DW<0x90, "imuld", 1>;
    defm SHLD : DW<0x91, "shld">;
    defm ISHLD : DW<0x92, "ishld">;
    defm SHRD : DW<0x93, "shrd">;
    defm SARD : DW<0x94, "sard">;
    defm DIVD : DW<0x95, "divd">;
    defm IDIVD : DW<0x96, "idivd">;
    defm MODD : DW<0x97, "modd">;

    multiclass DWPats<PatFrag op, Instruction Zrr, Instruction Zri, Instruction Srr, Instruction Sri>
    {
        def : Pat<(zext32 (op i64:$a, i64:$b)), (Zrr GPR:$a, GPR:$b)>;
        def : Pat<(zext32 (op i64:$a, (i64 imm:$b))), (Zri GPR:$a, (lo32_imm imm:$b))>;
        def : Pat<(sext32 (op i64:$a, i64:$b)), (Srr GPR:$a, GPR:$b)>;
        def : Pat<(sext32 (op i64:$a, (i64 imm:$b))), (Sri GPR:$a, (lo32_imm imm:$b))>;
    }
    defm : DWPats<add_1u, ADDDrr, ADDDri, IADDDrr, IADDDri>;
    defm : DWPats<sub_1u, SUBDrr, SUBDri, ISUBDrr, ISUBDri>;
    defm : DWPats<mul_1u, MULDrr, MULDri, IMULDrr, IMULDri>;
    def : Pat<(zext32 (sub_1u (i64 imm:$a), i64:$b)), (SUBDir (lo32_imm imm:$a), GPR:$b)>;
    def : Pat<(sext32 (sub_1u (i64 imm:$a), i64:$b)), (ISUBDir (lo32_imm imm:$a), GPR:$b)>;

    let AddedComplexity = 10 in {
        def : Pat<(zext32 (add_1u i64:$a, (i64 dw_neg_imm:$b))), (SUBDri GPR:$a, (dw_negate_imm dw_neg_imm:$b))>;
        def : Pat<(sext32 (add_1u i64:$a, (i64 dw_neg_imm:$b))), (ISUBDri GPR:$a, (dw_negate_imm dw_neg_imm:$b))>;
    }

    def : Pat<(zext32 (shl_1u i64:$a, i64:$b)), (SHLDrr GPR:$a, GPR:$b)>;
    def : Pat<(zext32 (shl_1u i64:$a, (i64 imm:$b))), (SHLDri GPR:$a, imm:$b)>;
    def : Pat<(zext32 (shl_1u (i64 imm:$a), i64:$b)), (SHLDir (lo32_imm imm:$a), GPR:$b)>;
    def : Pat<(sext32 (shl_1u i64:$a, i64:$b)), (ISHLDrr GPR:$a, GPR:$b)>;
    def : Pat<(sext32 (shl_1u i64:$a, (i64 imm:$b))), (ISHLDri GPR:$a, imm:$b)>;
    def : Pat<(sext32 (shl_1u (i64 imm:$a), i64:$b)), (ISHLDir (lo32_imm imm:$a), GPR:$b)>;
    def : Pat<(srl (zext32 i64:$a), i64:$b), (SHRDrr GPR:$a, GPR:$b)>;
    def : Pat<(srl (zext32 i64:$a), (i64 imm:$b)), (SHRDri GPR:$a, imm:$b)>;
    def : Pat<(sra (sext32 i64:$a), i64:$b), (SARDrr GPR:$a, GPR:$b)>;
    def : Pat<(sra (sext32 i64:$a), (i64 imm:$b)), (SARDri GPR:$a, imm:$b)>;

    def : Pat<(udiv (zext32 i64:$a), (zext32 i64:$b)), (DIVDrr GPR:$a, GPR:$b)>;
    def : Pat<(udiv (zext32 i64:$a), (i64 uimm32:$b)), (DIVDri GPR:$a, imm:$b)>;
    def : Pat<(urem (zext32 i64:$a), (zext32 i64:$b)), (MODDrr GPR:$a, GPR:$b)>;
    def : Pat<(urem (zext32 i64:$a), (i64 uimm32:$b)), (MODDri GPR:$a, imm:$b)>;
    def : Pat<(sext32 (sdiv (sext32 i64:$a), (sext32 i64:$b))), (IDIVDrr GPR:$a, GPR:$b)>;
}

//===----------------------------------------------------------------------===//
// ISA v19 (pointer authentication)
//===----------------------------------------------------------------------===//

let Predicates = [HasPAuth] in
{
    let Constraints = "$dst = $src" in
    {
        def PACIA : E64Inst<0x6D, 2, (outs GPR:$dst), (ins GPR:$src, GPR:$mod), "pacia $dst, $mod">;
        def PACDA : E64Inst<0x6F, 2, (outs GPR:$dst), (ins GPR:$src, GPR:$mod), "pacda $dst, $mod">;
        let hasSideEffects = 1 in
        {
            def AUTIA : E64Inst<0x6E, 2, (outs GPR:$dst), (ins GPR:$src, GPR:$mod), "autia $dst, $mod">;
            def AUTDA : E64Inst<0x70, 2, (outs GPR:$dst), (ins GPR:$src, GPR:$mod), "autda $dst, $mod">;
        }
    }
    def XPAC : E64Inst<0x71, 2, (outs GPR:$dst), (ins GPR:$src), "xpac $dst, $src", [(set i64:$dst, (Emex64xpac i64:$src))]>;

    let Uses = [FP], hasSideEffects = 1, mayLoad = 1, mayStore = 1 in
    def PACRET : E64Inst<0x72, 0, (outs), (ins), "pacret">;
    let isReturn = 1, isTerminator = 1, isBarrier = 1 in
    def RETA : E64Inst<0x73, 0, (outs), (ins), "reta">;
}

//===----------------------------------------------------------------------===//
// ISA v18 (Floating point unit)
//===----------------------------------------------------------------------===//

multiclass FBin<bits<8> opd, bits<8> ops, string name, SDPatternOperator node, bit commutable = 0>
{
    let isCommutable = commutable, Predicates = [HasFPU] in
    {
        def D : E64Inst<opd, 3, (outs FPR:$dst), (ins FPR:$a, FPR:$b), name # " $dst, $a, $b", [(set f64:$dst, (node f64:$a, f64:$b))]>;
        def S : E64Inst<ops, 3, (outs FPR:$dst), (ins FPR:$a, FPR:$b), name # "s $dst, $a, $b", [(set f32:$dst, (node f32:$a, f32:$b))]>;
    }
}

multiclass FUn<bits<8> opd, bits<8> ops, string name, SDPatternOperator node>
{
    let Predicates = [HasFPU] in
    {
        def D : E64Inst<opd, 2, (outs FPR:$dst), (ins FPR:$a), name # " $dst, $a", [(set f64:$dst, (node f64:$a))]>;
        def S : E64Inst<ops, 2, (outs FPR:$dst), (ins FPR:$a), name # "s $dst, $a", [(set f32:$dst, (node f32:$a))]>;
    }
}

defm FADD : FBin<0x44, 0x4F, "fadd", any_fadd, 1>;
defm FSUB : FBin<0x45, 0x50, "fsub", any_fsub>;
defm FMUL : FBin<0x46, 0x51, "fmul", any_fmul, 1>;
defm FDIV : FBin<0x47, 0x52, "fdiv", any_fdiv>;
defm FMIN : FBin<0x49, 0x54, "fmin", fminnum, 1>;
defm FMAX : FBin<0x4A, 0x55, "fmax", fmaxnum, 1>;
defm FSQRT : FUn<0x48, 0x53, "fsqrt", any_fsqrt>;
defm FNEG : FUn<0x4C, 0x57, "fneg", fneg>;
defm FABS : FUn<0x4D, 0x58, "fabs", fabs>;

let Predicates = [HasFPU] in
{
    def FMADDD : E64Inst<0x4B, 4, (outs FPR:$dst), (ins FPR:$a, FPR:$b, FPR:$c), "fmadd $dst, $a, $b, $c", [(set f64:$dst, (any_fma f64:$a, f64:$b, f64:$c))]>;
    def FMADDS : E64Inst<0x56, 4, (outs FPR:$dst), (ins FPR:$a, FPR:$b, FPR:$c), "fmadds $dst, $a, $b, $c", [(set f32:$dst, (any_fma f32:$a, f32:$b, f32:$c))]>;

    let Defs = [CF] in
    {
        def FCMPD : E64Inst<0x4E, 2, (outs), (ins FPR:$a, FPR:$b), "fcmp $a, $b", [(Emex64fcmp f64:$a, f64:$b)]>;
        def FCMPS : E64Inst<0x59, 2, (outs), (ins FPR:$a, FPR:$b), "fcmps $a, $b", [(Emex64fcmp f32:$a, f32:$b)]>;
    }

    def FCVTDS : E64Inst<0x5A, 2, (outs FPR:$dst), (ins FPR:$a), "fcvtds $dst, $a", [(set f64:$dst, (any_fpextend f32:$a))]>;
    def FCVTSD : E64Inst<0x5B, 2, (outs FPR:$dst), (ins FPR:$a), "fcvtsd $dst, $a", [(set f32:$dst, (any_fpround f64:$a))]>;
    def FCVTDI : E64Inst<0x5C, 2, (outs FPR:$dst), (ins GPR:$a), "fcvtdi $dst, $a", [(set f64:$dst, (any_sint_to_fp i64:$a))]>;
    def FCVTDU : E64Inst<0x5D, 2, (outs FPR:$dst), (ins GPR:$a), "fcvtdu $dst, $a", [(set f64:$dst, (any_uint_to_fp i64:$a))]>;
    def FCVTID : E64Inst<0x5E, 2, (outs GPR:$dst), (ins FPR:$a), "fcvtid $dst, $a", [(set i64:$dst, (any_fp_to_sint f64:$a))]>;
    def FCVTUD : E64Inst<0x5F, 2, (outs GPR:$dst), (ins FPR:$a), "fcvtud $dst, $a", [(set i64:$dst, (any_fp_to_uint f64:$a))]>;
    def FCVTSI : E64Inst<0x60, 2, (outs FPR:$dst), (ins GPR:$a), "fcvtsi $dst, $a", [(set f32:$dst, (any_sint_to_fp i64:$a))]>;
    def FCVTSU : E64Inst<0x61, 2, (outs FPR:$dst), (ins GPR:$a), "fcvtsu $dst, $a", [(set f32:$dst, (any_uint_to_fp i64:$a))]>;
    def FCVTIS : E64Inst<0x62, 2, (outs GPR:$dst), (ins FPR:$a), "fcvtis $dst, $a", [(set i64:$dst, (any_fp_to_sint f32:$a))]>;
    def FCVTUS : E64Inst<0x63, 2, (outs GPR:$dst), (ins FPR:$a), "fcvtus $dst, $a", [(set i64:$dst, (any_fp_to_uint f32:$a))]>;
}

let Predicates = [HasFPU] in {
    def : Pat<(i64 (fp_to_sint_sat f64:$a, i64)), (FCVTID FPR:$a)>;
    def : Pat<(i64 (fp_to_uint_sat f64:$a, i64)), (FCVTUD FPR:$a)>;
    def : Pat<(i64 (fp_to_sint_sat f32:$a, i64)), (FCVTIS FPR:$a)>;
    def : Pat<(i64 (fp_to_uint_sat f32:$a, i64)), (FCVTUS FPR:$a)>;
}
