add_llvm_component_library(LLVMEmex64Desc
    Emex64AsmBackend.cpp
    Emex64ELFObjectWriter.cpp
    Emex64InstPrinter.cpp
    Emex64MCAsmInfo.cpp
    Emex64MCCodeEmitter.cpp
    Emex64MCTargetDesc.cpp

    LINK_COMPONENTS
    Emex64Info
    MC
    Support
    TargetParser

    ADD_TO_COMPONENT
    Emex64
)
