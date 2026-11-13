add_llvm_component_library(LLVMEmex64AsmParser
    Emex64AsmParser.cpp
    
    LINK_COMPONENTS
    Emex64Desc
    Emex64Info
    MC
    MCParser
    Support
    TargetParser
    
    ADD_TO_COMPONENT
    Emex64
)
