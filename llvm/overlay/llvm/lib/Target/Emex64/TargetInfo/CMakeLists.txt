add_llvm_component_library(LLVMEmex64Info
    Emex64TargetInfo.cpp

    LINK_COMPONENTS
    MC
    Support

    ADD_TO_COMPONENT
    Emex64
)
