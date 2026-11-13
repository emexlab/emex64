add_llvm_component_library(LLVMEmex64Disassembler
    Emex64Disassembler.cpp
    
    LINK_COMPONENTS
    Emex64Desc
    Emex64Info
    MC
    MCDisassembler
    Support
    
    ADD_TO_COMPONENT
    Emex64
)
