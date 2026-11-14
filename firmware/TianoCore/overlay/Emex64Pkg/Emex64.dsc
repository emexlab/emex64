## @file
#  Copyright (C) 2026 emexlab
#  SPDX-License-Identifier: BSD-2-Clause-Patent
##

[Defines]
    PLATFORM_NAME           = Emex64
    PLATFORM_GUID           = 0E640000-7E64-4C64-8E64-0000000000E6
    PLATFORM_VERSION        = 1.0
    DSC_SPECIFICATION       = 0x0001001C
    OUTPUT_DIRECTORY        = Build/Emex64
    SUPPORTED_ARCHITECTURES = EMEX64
    BUILD_TARGETS           = DEBUG|RELEASE|NOOPT
    SKUID_IDENTIFIER        = DEFAULT
    FLASH_DEFINITION        = Emex64Pkg/Emex64.fdf

[LibraryClasses.common]
    NULL|MdePkg/Library/CompilerIntrinsicsLib/CompilerIntrinsicsLib.inf

    BaseLib|MdePkg/Library/BaseLib/BaseLib.inf
    BaseMemoryLib|MdePkg/Library/BaseMemoryLib/BaseMemoryLib.inf
    SafeIntLib|MdePkg/Library/BaseSafeIntLib/BaseSafeIntLib.inf
    SynchronizationLib|MdePkg/Library/BaseSynchronizationLib/BaseSynchronizationLib.inf
    PrintLib|MdePkg/Library/BasePrintLib/BasePrintLib.inf
    IoLib|MdePkg/Library/BaseIoLibIntrinsic/BaseIoLibIntrinsic.inf
    CpuLib|MdePkg/Library/BaseCpuLib/BaseCpuLib.inf
    CacheMaintenanceLib|MdePkg/Library/BaseCacheMaintenanceLib/BaseCacheMaintenanceLib.inf
    PeCoffLib|MdePkg/Library/BasePeCoffLib/BasePeCoffLib.inf
    PeCoffGetEntryPointLib|MdePkg/Library/BasePeCoffGetEntryPointLib/BasePeCoffGetEntryPointLib.inf
    PeCoffExtraActionLib|MdePkg/Library/BasePeCoffExtraActionLibNull/BasePeCoffExtraActionLibNull.inf
    UefiDecompressLib|MdePkg/Library/BaseUefiDecompressLib/BaseUefiDecompressLib.inf
    RngLib|MdePkg/Library/BaseRngLib/BaseRngLib.inf
    RegisterFilterLib|MdePkg/Library/RegisterFilterLibNull/RegisterFilterLibNull.inf
    StackCheckLib|MdePkg/Library/StackCheckLibNull/StackCheckLibNull.inf
    StackCheckFailureHookLib|MdePkg/Library/StackCheckFailureHookLibNull/StackCheckFailureHookLibNull.inf
    OrderedCollectionLib|MdePkg/Library/BaseOrderedCollectionRedBlackTreeLib/BaseOrderedCollectionRedBlackTreeLib.inf

    DebugLib|MdePkg/Library/BaseDebugLibSerialPort/BaseDebugLibSerialPort.inf
    DebugPrintErrorLevelLib|MdePkg/Library/BaseDebugPrintErrorLevelLib/BaseDebugPrintErrorLevelLib.inf
    DebugAgentLib|MdeModulePkg/Library/DebugAgentLibNull/DebugAgentLibNull.inf
    ReportStatusCodeLib|MdePkg/Library/BaseReportStatusCodeLibNull/BaseReportStatusCodeLibNull.inf
    PerformanceLib|MdePkg/Library/BasePerformanceLibNull/BasePerformanceLibNull.inf

    # Machine
    SerialPortLib|Emex64Pkg/Library/Emex64SerialPortLib/Emex64SerialPortLib.inf
    TimerLib|Emex64Pkg/Library/Emex64TimerLib/Emex64TimerLib.inf
    RealTimeClockLib|Emex64Pkg/Library/Emex64RealTimeClockLib/Emex64RealTimeClockLib.inf
    ResetSystemLib|Emex64Pkg/Library/Emex64ResetSystemLib/Emex64ResetSystemLib.inf
    CpuExceptionHandlerLib|Emex64Pkg/Library/Emex64ExceptionLib/Emex64ExceptionLib.inf
    PlatformBootManagerLib|Emex64Pkg/Library/PlatformBootManagerLib/PlatformBootManagerLib.inf
    TimeBaseLib|EmbeddedPkg/Library/TimeBaseLib/TimeBaseLib.inf

    # UEFI and DXE
    UefiLib|MdePkg/Library/UefiLib/UefiLib.inf
    UefiBootServicesTableLib|MdePkg/Library/UefiBootServicesTableLib/UefiBootServicesTableLib.inf
    UefiRuntimeServicesTableLib|MdePkg/Library/UefiRuntimeServicesTableLib/UefiRuntimeServicesTableLib.inf
    UefiRuntimeLib|MdePkg/Library/UefiRuntimeLib/UefiRuntimeLib.inf
    UefiDriverEntryPoint|MdePkg/Library/UefiDriverEntryPoint/UefiDriverEntryPoint.inf
    UefiApplicationEntryPoint|MdePkg/Library/UefiApplicationEntryPoint/UefiApplicationEntryPoint.inf
    DxeServicesLib|MdePkg/Library/DxeServicesLib/DxeServicesLib.inf
    DxeServicesTableLib|MdePkg/Library/DxeServicesTableLib/DxeServicesTableLib.inf

    # Other Stuff
    DevicePathLib|MdePkg/Library/UefiDevicePathLib/UefiDevicePathLib.inf
    HobLib|MdePkg/Library/DxeHobLib/DxeHobLib.inf
    PcdLib|MdePkg/Library/DxePcdLib/DxePcdLib.inf
    MemoryAllocationLib|MdePkg/Library/UefiMemoryAllocationLib/UefiMemoryAllocationLib.inf
    ExtractGuidedSectionLib|MdePkg/Library/DxeExtractGuidedSectionLib/DxeExtractGuidedSectionLib.inf
    HiiLib|MdeModulePkg/Library/UefiHiiLib/UefiHiiLib.inf
    UefiHiiServicesLib|MdeModulePkg/Library/UefiHiiServicesLib/UefiHiiServicesLib.inf
    SortLib|MdeModulePkg/Library/UefiSortLib/UefiSortLib.inf
    SecurityManagementLib|MdeModulePkg/Library/DxeSecurityManagementLib/DxeSecurityManagementLib.inf
    CapsuleLib|MdeModulePkg/Library/DxeCapsuleLibNull/DxeCapsuleLibNull.inf
    UefiBootManagerLib|MdeModulePkg/Library/UefiBootManagerLib/UefiBootManagerLib.inf
    BootLogoLib|MdeModulePkg/Library/BootLogoLib/BootLogoLib.inf
    BmpSupportLib|MdeModulePkg/Library/BaseBmpSupportLib/BaseBmpSupportLib.inf
    FrameBufferBltLib|MdeModulePkg/Library/FrameBufferBltLib/FrameBufferBltLib.inf
    CustomizedDisplayLib|MdeModulePkg/Library/CustomizedDisplayLib/CustomizedDisplayLib.inf
    FileExplorerLib|MdeModulePkg/Library/FileExplorerLib/FileExplorerLib.inf
    GptLib|MdeModulePkg/Library/GptLib/GptLib.inf
    ImagePropertiesRecordLib|MdeModulePkg/Library/ImagePropertiesRecordLib/ImagePropertiesRecordLib.inf
    OemHookStatusCodeLib|MdeModulePkg/Library/OemHookStatusCodeLibNull/OemHookStatusCodeLibNull.inf

    # PcdEmuVariableNvModeEnable
    AuthVariableLib|MdeModulePkg/Library/AuthVariableLibNull/AuthVariableLibNull.inf
    VarCheckLib|MdeModulePkg/Library/VarCheckLib/VarCheckLib.inf
    VariableFlashInfoLib|MdeModulePkg/Library/BaseVariableFlashInfoLib/BaseVariableFlashInfoLib.inf
    VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLib.inf
    VariablePolicyHelperLib|MdeModulePkg/Library/VariablePolicyHelperLib/VariablePolicyHelperLib.inf
    TpmMeasurementLib|MdeModulePkg/Library/TpmMeasurementLibNull/TpmMeasurementLibNull.inf

[LibraryClasses.common.SEC]
    PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    HobLib|EmbeddedPkg/Library/PrePiHobLib/PrePiHobLib.inf
    PrePiLib|EmbeddedPkg/Library/PrePiLib/PrePiLib.inf
    PrePiHobListPointerLib|Emex64Pkg/Library/PrePiHobListPointerLib/PrePiHobListPointerLib.inf
    MemoryAllocationLib|EmbeddedPkg/Library/PrePiMemoryAllocationLib/PrePiMemoryAllocationLib.inf
    ExtractGuidedSectionLib|EmbeddedPkg/Library/PrePiExtractGuidedSectionLib/PrePiExtractGuidedSectionLib.inf

[LibraryClasses.common.DXE_CORE]
    HobLib|MdePkg/Library/DxeCoreHobLib/DxeCoreHobLib.inf
    MemoryAllocationLib|MdeModulePkg/Library/DxeCoreMemoryAllocationLib/DxeCoreMemoryAllocationLib.inf
    DxeCoreEntryPoint|MdePkg/Library/DxeCoreEntryPoint/DxeCoreEntryPoint.inf

[LibraryClasses.common.DXE_RUNTIME_DRIVER]
    DebugLib|MdePkg/Library/DxeRuntimeDebugLibSerialPort/DxeRuntimeDebugLibSerialPort.inf
    VariablePolicyLib|MdeModulePkg/Library/VariablePolicyLib/VariablePolicyLibRuntimeDxe.inf

[PcdsFeatureFlag]
    gEfiMdeModulePkgTokenSpaceGuid.PcdHiiOsRuntimeSupport|FALSE
    gEfiMdeModulePkgTokenSpaceGuid.PcdConOutGopSupport|TRUE

[PcdsFixedAtBuild]
    # DEBUG_ASSERT_ENABLED | DEBUG_PRINT_ENABLED | DEBUG_CODE_ENABLED | ASSERT_DEADLOOP_ENABLED
    gEfiMdePkgTokenSpaceGuid.PcdDebugPropertyMask|0x27
    # DEBUG_ERROR
    gEfiMdePkgTokenSpaceGuid.PcdDebugPrintErrorLevel|0x80000000
    gEfiMdePkgTokenSpaceGuid.PcdMaximumUnicodeStringLength|1000000
    gEfiMdePkgTokenSpaceGuid.PcdMaximumAsciiStringLength|1000000
    gEfiMdePkgTokenSpaceGuid.PcdMaximumLinkedListLength|0
    gEfiMdePkgTokenSpaceGuid.PcdUefiLibMaxPrintBufferSize|320

    gEfiMdePkgTokenSpaceGuid.PcdUartDefaultBaudRate|115200
    gEfiMdePkgTokenSpaceGuid.PcdUartDefaultDataBits|8
    gEfiMdePkgTokenSpaceGuid.PcdUartDefaultParity|1
    gEfiMdePkgTokenSpaceGuid.PcdUartDefaultStopBits|1

    gEfiMdeModulePkgTokenSpaceGuid.PcdEmuVariableNvModeEnable|TRUE
    gEfiMdeModulePkgTokenSpaceGuid.PcdMaxVariableSize|0x2000
    gEfiMdeModulePkgTokenSpaceGuid.PcdVariableStoreSize|0x10000

    gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVendor|L"emexLabs"
    gEfiMdeModulePkgTokenSpaceGuid.PcdFirmwareVersionString|L"TianoCore for emex64 1.0"
    gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosVersion|0x0307
    gEfiMdeModulePkgTokenSpaceGuid.PcdSmbiosDocRev|0x0

    # UiApp
    gEfiMdeModulePkgTokenSpaceGuid.PcdBootManagerMenuFile|{ 0x21, 0xaa, 0x2c, 0x46, 0x14, 0x76, 0x03, 0x45, 0x83, 0x6e, 0x8a, 0xb6, 0xf4, 0x66, 0x23, 0x31 }
    gEfiMdeModulePkgTokenSpaceGuid.PcdImageProtectionPolicy|0x0
    gEfiMdeModulePkgTokenSpaceGuid.PcdDxeNxMemoryProtectionPolicy|0x0

[PcdsDynamicDefault]
    gEfiMdePkgTokenSpaceGuid.PcdPlatformBootTimeOut|3
    gEfiMdeModulePkgTokenSpaceGuid.PcdVideoHorizontalResolution|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdVideoVerticalResolution|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdConOutColumn|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdConOutRow|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoHorizontalResolution|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdSetupVideoVerticalResolution|0
    gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutColumn|80
    gEfiMdeModulePkgTokenSpaceGuid.PcdSetupConOutRow|25

[Components]
    Emex64Pkg/Sec/Sec.inf

    MdeModulePkg/Core/Dxe/DxeMain.inf {
        <LibraryClasses>
            NULL|MdeModulePkg/Library/DxeCrc32GuidedSectionExtractLib/DxeCrc32GuidedSectionExtractLib.inf
    }
    MdeModulePkg/Universal/PCD/Dxe/Pcd.inf {
        <LibraryClasses>
            PcdLib|MdePkg/Library/BasePcdLibNull/BasePcdLibNull.inf
    }
    Emex64Pkg/CpuDxe/CpuDxe.inf
    Emex64Pkg/TimerDxe/TimerDxe.inf
    MdeModulePkg/Core/RuntimeDxe/RuntimeDxe.inf
    MdeModulePkg/Universal/SecurityStubDxe/SecurityStubDxe.inf
    MdeModulePkg/Universal/CapsuleRuntimeDxe/CapsuleRuntimeDxe.inf
    MdeModulePkg/Universal/MonotonicCounterRuntimeDxe/MonotonicCounterRuntimeDxe.inf
    MdeModulePkg/Universal/ResetSystemRuntimeDxe/ResetSystemRuntimeDxe.inf
    MdeModulePkg/Universal/WatchdogTimerDxe/WatchdogTimer.inf
    EmbeddedPkg/RealTimeClockRuntimeDxe/RealTimeClockRuntimeDxe.inf
    EmbeddedPkg/MetronomeDxe/MetronomeDxe.inf
    MdeModulePkg/Universal/Variable/RuntimeDxe/VariableRuntimeDxe.inf {
        <LibraryClasses>
            NULL|MdeModulePkg/Library/VarCheckUefiLib/VarCheckUefiLib.inf
    }

    # devices
    Emex64Pkg/GopDxe/GopDxe.inf
    MdeModulePkg/Universal/SmbiosDxe/SmbiosDxe.inf
    Emex64Pkg/SmbiosPlatformDxe/SmbiosPlatformDxe.inf
    Emex64Pkg/KeyboardDxe/KeyboardDxe.inf
    Emex64Pkg/SataDxe/SataDxe.inf
    Emex64Pkg/UsbHcDxe/UsbHcDxe.inf
    MdeModulePkg/Bus/Usb/UsbBusDxe/UsbBusDxe.inf
    MdeModulePkg/Bus/Usb/UsbMassStorageDxe/UsbMassStorageDxe.inf

    MdeModulePkg/Universal/Console/ConPlatformDxe/ConPlatformDxe.inf
    MdeModulePkg/Universal/Console/ConSplitterDxe/ConSplitterDxe.inf
    MdeModulePkg/Universal/Console/GraphicsConsoleDxe/GraphicsConsoleDxe.inf
    MdeModulePkg/Universal/Disk/UnicodeCollation/EnglishDxe/EnglishDxe.inf

    # Partitions and FAT, for \EFI\BOOT\BOOTEMEX64.EFI
    MdeModulePkg/Universal/Disk/DiskIoDxe/DiskIoDxe.inf
    MdeModulePkg/Universal/Disk/PartitionDxe/PartitionDxe.inf
    FatPkg/EnhancedFatDxe/Fat.inf

    # HII, BDS, the setup utility and the logo
    MdeModulePkg/Universal/DevicePathDxe/DevicePathDxe.inf
    MdeModulePkg/Universal/HiiDatabaseDxe/HiiDatabaseDxe.inf
    MdeModulePkg/Universal/SetupBrowserDxe/SetupBrowserDxe.inf
    MdeModulePkg/Universal/DisplayEngineDxe/DisplayEngineDxe.inf
    MdeModulePkg/Universal/DriverHealthManagerDxe/DriverHealthManagerDxe.inf
    MdeModulePkg/Universal/BdsDxe/BdsDxe.inf
    MdeModulePkg/Logo/LogoDxe.inf
    MdeModulePkg/Application/UiApp/UiApp.inf {
        <LibraryClasses>
            NULL|MdeModulePkg/Library/DeviceManagerUiLib/DeviceManagerUiLib.inf
            NULL|MdeModulePkg/Library/BootManagerUiLib/BootManagerUiLib.inf
            NULL|MdeModulePkg/Library/BootMaintenanceManagerUiLib/BootMaintenanceManagerUiLib.inf
    }
