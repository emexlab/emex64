add_llvm_component_group(Emex64)

set(LLVM_TARGET_DEFINITIONS Emex64.td)

tablegen(LLVM Emex64GenAsmWriter.inc -gen-asm-writer)
tablegen(LLVM Emex64GenCallingConv.inc -gen-callingconv)
tablegen(LLVM Emex64GenDAGISel.inc -gen-dag-isel)
tablegen(LLVM Emex64GenInstrInfo.inc -gen-instr-info)
tablegen(LLVM Emex64GenRegisterInfo.inc -gen-register-info)
tablegen(LLVM Emex64GenSDNodeInfo.inc -gen-sd-node-info)
tablegen(LLVM Emex64GenSubtargetInfo.inc -gen-subtarget)

add_public_tablegen_target(Emex64CommonTableGen)

add_llvm_target(Emex64CodeGen
    Emex64AsmPrinter.cpp
    Emex64FrameLowering.cpp
    Emex64InstrInfo.cpp
    Emex64ISelDAGToDAG.cpp
    Emex64ISelLowering.cpp
    Emex64MCInstLower.cpp
    Emex64RegisterInfo.cpp
    Emex64SelectionDAGInfo.cpp
    Emex64Subtarget.cpp
    Emex64TargetMachine.cpp

    LINK_COMPONENTS
    Analysis
    AsmPrinter
    CodeGen
    CodeGenTypes
    Core
    Emex64AsmParser
    Emex64Desc
    Emex64Info
    MC
    SelectionDAG
    Support
    Target
    TargetParser

    ADD_TO_COMPONENT
    Emex64
)

add_subdirectory(AsmParser)
add_subdirectory(Disassembler)
add_subdirectory(MCTargetDesc)
add_subdirectory(TargetInfo)
