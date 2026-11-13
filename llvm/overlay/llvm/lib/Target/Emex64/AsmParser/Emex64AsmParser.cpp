//===-- Emex64AsmParser.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/Emex64MCTargetDesc.h"
#include "MCTargetDesc/Emex64OpcodeTable.h"
#include "TargetInfo/Emex64TargetInfo.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCParser/AsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCParser/MCTargetAsmParser.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/Compiler.h"

using namespace llvm;

#define DEBUG_TYPE  "emex64-asm-parser"

static MCRegister matchRegister(StringRef Name)
{
    std::string N = Name.lower();
    MCRegister R = StringSwitch<MCRegister>(N)
                    .Case("pc", Emex64::PC)
                    .Case("sp", Emex64::SP)
                    .Case("fp", Emex64::FP)
                    .Case("cf", Emex64::CF)
                    .Case("fpc", Emex64::FPC)
                    .Case("rr", Emex64::RR)
                    .Case("tp", Emex64::ER15)
                    .Default(MCRegister());
    if(R)
    {
        return R;
    }

    StringRef S(N);
    unsigned Num;
    static const MCPhysReg Main[] = {
        Emex64::R0,     Emex64::R1,     Emex64::R2,     Emex64::R3,
        Emex64::R4,     Emex64::R5,     Emex64::R6,     Emex64::R7,
        Emex64::R8,     Emex64::R9
    };
    static const MCPhysReg Ext[] = {
        Emex64::ER0,    Emex64::ER1,    Emex64::ER2,    Emex64::ER3,
        Emex64::ER4,    Emex64::ER5,    Emex64::ER6,    Emex64::ER7,
        Emex64::ER8,    Emex64::ER9,    Emex64::ER10,   Emex64::ER11,
        Emex64::ER12,   Emex64::ER13,   Emex64::ER14,   Emex64::ER15
    };
    static const MCPhysReg Flt[] = {
        Emex64::FR0,    Emex64::FR1,    Emex64::FR2,    Emex64::FR3,
        Emex64::FR4,    Emex64::FR5,    Emex64::FR6,    Emex64::FR7,
        Emex64::FR8,    Emex64::FR9,    Emex64::FR10,   Emex64::FR11,
        Emex64::FR12,   Emex64::FR13,   Emex64::FR14,   Emex64::FR15
    };

    if(S.consume_front("er"))
    {
        if(!S.getAsInteger(10, Num) && Num < 16)
        {
            return Ext[Num];
        }
    }
    else if (S.consume_front("fr"))
    {
        if(!S.getAsInteger(10, Num) && Num < 16)
        {
            return Flt[Num];
        }
    }
    else if(S.consume_front("r"))
    {
        if(!S.getAsInteger(10, Num) && Num < 10)
        {
            return Main[Num];
        }
    }

    return MCRegister();
}

namespace {

class Emex64Operand : public MCParsedAsmOperand
{
    SMLoc Start, End;

public:
    MCInst Inst;
    Emex64Operand(SMLoc S, SMLoc E) : Start(S), End(E) {}
    bool isToken() const override
    {
        return false;
    }
    bool isReg() const override
    {
        return false;
    }
    bool isImm() const override
    {
        return false;
    }
    bool isMem() const override
    {
        return false;
    }
    MCRegister getReg() const override
    {
        return MCRegister();
    }
    SMLoc getStartLoc() const override
    {
        return Start;
    }
    SMLoc getEndLoc() const override
    {
        return End;
    }
    void print(raw_ostream &OS, const MCAsmInfo &MAI) const override
    {
        OS << "emex64 instruction";
    }
};

class Emex64AsmParser : public MCTargetAsmParser
{
    MCAsmParser &Parser;
    
    AsmLexer &getLexer() const
    {
        return Parser.getLexer();
    }
    
    bool parseRegister(MCRegister &Reg,
                       SMLoc &StartLoc,
                       SMLoc &EndLoc) override
    {
        if(!tryParseRegister(Reg, StartLoc, EndLoc).isSuccess())
        {
            return Error(StartLoc, "invalid register name");
        }
        return false;
    }
    
    ParseStatus tryParseRegister(MCRegister &Reg,
                                 SMLoc &StartLoc,
                                 SMLoc &EndLoc) override
    {
        const AsmToken &Tok = getLexer().getTok();
        StartLoc = Tok.getLoc();
        if(Tok.isNot(AsmToken::Identifier))
        {
            return ParseStatus::NoMatch;
        }
        Reg = matchRegister(Tok.getIdentifier());
        if(!Reg)
        {
            return ParseStatus::NoMatch;
        }
        EndLoc = Tok.getEndLoc();
        getLexer().Lex();
        return ParseStatus::Success;
    }
    
    bool parseValue(MCInst &Inst,
                    bool &IsReg,
                    MCRegister &Reg)
    {
        SMLoc S, E;
        IsReg = false;
        if(tryParseRegister(Reg, S, E).isSuccess())
        {
            IsReg = true;
            Inst.addOperand(MCOperand::createReg(Reg));
            return false;
        }
        S = getLexer().getLoc();
        const MCExpr *V;
        if(Parser.parseExpression(V))
        {
            return Error(S, "expected a register or an expression");
        }
        int64_t C;
        if(V->evaluateAsAbsolute(C))
        {
            Inst.addOperand(MCOperand::createImm(C));
        }
        else
        {
            Inst.addOperand(MCOperand::createExpr(V));
        }
        return false;
    }
    
    bool parseOperand(MCInst &Inst,
                      unsigned ArgNo,
                      int TargetArg)
    {
        if(getLexer().is(AsmToken::LBrac))
        {
            getLexer().Lex();
            unsigned KindIdx = Inst.getNumOperands();
            Inst.addOperand(MCOperand::createImm(Emex64Raw::Value));
            bool IsReg;
            MCRegister Reg;
            if(parseValue(Inst, IsReg, Reg))
            {
                return true;
            }
            if(getLexer().is(AsmToken::Plus) || getLexer().is(AsmToken::Minus))
            {
                bool Minus = getLexer().is(AsmToken::Minus);
                getLexer().Lex();
                Inst.getOperand(KindIdx).setImm(Minus ? Emex64Raw::OffsetSub : Emex64Raw::OffsetAdd);
                if(parseValue(Inst, IsReg, Reg))
                {
                    return true;
                }
            }
            if(getLexer().isNot(AsmToken::RBrac))
            {
                return Error(getLexer().getLoc(), "expected ']'");
            }
            getLexer().Lex();
            return false;
        }
        
        unsigned KindIdx = Inst.getNumOperands();
        Inst.addOperand(MCOperand::createImm(Emex64Raw::Value));
        bool IsReg;
        MCRegister Reg;
        if(parseValue(Inst, IsReg, Reg))
        {
            return true;
        }
        
        if((int)ArgNo == TargetArg && !IsReg && Inst.getOperand(KindIdx + 1).isExpr())
        {
            uint16_t S = getEmex64Specifier(Inst.getOperand(KindIdx + 1).getExpr());
            if(S == Emex64S::PCREL || S == Emex64S::PLT)
            {
                Inst.getOperand(KindIdx).setImm(Emex64Raw::PCRel8);
            }
        }
        
        if(IsReg && (getLexer().is(AsmToken::Plus) || getLexer().is(AsmToken::Minus)) && getLexer().peekTok().is(getLexer().getKind()))
        {
            bool Dec = getLexer().is(AsmToken::Minus);
            getLexer().Lex();
            getLexer().Lex();
            if((getContext().getRegisterInfo()->getEncodingValue(Reg) >> 4) != 0)
            {
                return Error(getLexer().getLoc(), "++ and -- need a register of the main file");
            }
            Inst.getOperand(KindIdx).setImm(Dec ? Emex64Raw::RegDec : Emex64Raw::RegInc);
        }

        return false;
    }
    
    bool parseInstruction(ParseInstructionInfo &Info,
                          StringRef Name,
                          SMLoc NameLoc,
                          OperandVector &Operands) override
    {
        int Op = getEmex64Opcode(Name);
        if(Op < 0)
        {
            return Error(NameLoc, "unknown instruction '" + Name + "'");
        }
        
        const Emex64OpcodeInfo &OI = getEmex64OpcodeInfo(Op);
        
        auto Result = std::make_unique<Emex64Operand>(NameLoc, NameLoc);
        MCInst &Inst = Result->Inst;
        Inst.setOpcode(Emex64::RAW);
        Inst.addOperand(MCOperand::createImm(Op));
        
        unsigned Count = 0;
        if(getLexer().isNot(AsmToken::EndOfStatement))
        {
            do {
                if(Count++ && getLexer().is(AsmToken::Comma))
                {
                    getLexer().Lex();
                }
                if(parseOperand(Inst, Count - 1, OI.TargetArg))
                {
                    return true;
                }
            } while(getLexer().is(AsmToken::Comma));
        }
        if(getLexer().isNot(AsmToken::EndOfStatement))
        {
            return Error(getLexer().getLoc(), "unexpected token after the operands");
        }
        getLexer().Lex();

        if(Count > OI.MaxArgs)
        {
            return Error(NameLoc, "too many operands for '" + Name + "'");
        }
        Operands.push_back(std::move(Result));
        return false;
    }

    bool matchAndEmitInstruction(SMLoc IDLoc,
                                 unsigned &Opcode,
                                 OperandVector &Operands,
                                 MCStreamer &Out,
                                 uint64_t &ErrorInfo,
                                 bool MatchingInlineAsm) override
    {
        MCInst &Inst = static_cast<Emex64Operand &>(*Operands[0]).Inst;
        Inst.setLoc(IDLoc);
        Out.emitInstruction(Inst, getSTI());
        return false;
    }
    
    ParseStatus parseDirective(AsmToken DirectiveID) override
    {
        return ParseStatus::NoMatch;
    }
    
    void convertToMapAndConstraints(unsigned Kind, const OperandVector &Operands) override {}

public:
    Emex64AsmParser(const MCSubtargetInfo &STI,
                    MCAsmParser &Parser,
                    const MCInstrInfo &MII) : MCTargetAsmParser(STI, MII), Parser(Parser)
    {
        MCAsmParserExtension::Initialize(Parser);
    }
};

} /* namespace*/

extern "C" LLVM_ABI LLVM_EXTERNAL_VISIBILITY void
LLVMInitializeEmex64AsmParser()
{
    RegisterMCAsmParser<Emex64AsmParser> X(getTheEmex64Target());
}
