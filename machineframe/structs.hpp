#pragma once
#include <ntifs.h>

#pragma pack(push, 1)
typedef struct _KTSS64
{
    ULONG Reserved0;                                                        //0x0
    ULONGLONG Rsp0;                                                         //0x4
    ULONGLONG Rsp1;                                                         //0xc
    ULONGLONG Rsp2;                                                         //0x14
    ULONGLONG Ist[8];                                                       //0x1c
    ULONGLONG Reserved1;                                                    //0x5c
    USHORT Reserved2;                                                       //0x64
    USHORT IoMapBase;                                                       //0x66
} KTSS64, * PKTSS64;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct _MACHINE_FRAME
{
    ULONGLONG Rip;                                                          //0x0
    USHORT SegCs;                                                           //0x8
    USHORT Fill1[3];                                                        //0xa
    ULONG EFlags;                                                           //0x10
    ULONG Fill2;                                                            //0x14
    ULONGLONG Rsp;                                                          //0x18
    USHORT SegSs;                                                           //0x20
    USHORT Fill3[3];                                                        //0x22
} MACHINE_FRAME, * PMACHINE_FRAME;
#pragma pack(pop)

//0x4b0 bytes (sizeof)
typedef struct _HAL_PRIVATE_DISPATCH
{
    ULONG Version;                                                          //0x0
    struct _BUS_HANDLER* (*HalHandlerForBus)(enum _INTERFACE_TYPE arg1, ULONG arg2); //0x8
    struct _BUS_HANDLER* (*HalHandlerForConfigSpace)(enum _BUS_DATA_TYPE arg1, ULONG arg2); //0x10
    VOID(*HalLocateHiberRanges)(VOID* arg1);                               //0x18
    LONG(*HalRegisterBusHandler)(enum _INTERFACE_TYPE arg1, enum _BUS_DATA_TYPE arg2, ULONG arg3, enum _INTERFACE_TYPE arg4, ULONG arg5, ULONG arg6, LONG(*arg7)(struct _BUS_HANDLER* arg1), struct _BUS_HANDLER** arg8); //0x20
    VOID(*HalSetWakeEnable)(UCHAR arg1);                                   //0x28
    LONG(*HalSetWakeAlarm)(ULONGLONG arg1, ULONGLONG arg2);                //0x30
    UCHAR(*HalPciTranslateBusAddress)(enum _INTERFACE_TYPE arg1, ULONG arg2, union _LARGE_INTEGER arg3, ULONG* arg4, union _LARGE_INTEGER* arg5); //0x38
    LONG(*HalPciAssignSlotResources)(struct _UNICODE_STRING* arg1, struct _UNICODE_STRING* arg2, struct _DRIVER_OBJECT* arg3, struct _DEVICE_OBJECT* arg4, enum _INTERFACE_TYPE arg5, ULONG arg6, ULONG arg7, struct _CM_RESOURCE_LIST** arg8); //0x40
    VOID(*HalHaltSystem)();                                                //0x48
    UCHAR(*HalFindBusAddressTranslation)(union _LARGE_INTEGER arg1, ULONG* arg2, union _LARGE_INTEGER* arg3, ULONGLONG* arg4, UCHAR arg5); //0x50
    UCHAR(*HalResetDisplay)();                                             //0x58
    LONG(*HalAllocateMapRegisters)(struct _ADAPTER_OBJECT* arg1, ULONG arg2, ULONG arg3, struct _MAP_REGISTER_ENTRY* arg4); //0x60
    LONG(*KdSetupPciDeviceForDebugging)(VOID* arg1, struct _DEBUG_DEVICE_DESCRIPTOR* arg2); //0x68
    LONG(*KdReleasePciDeviceForDebugging)(struct _DEBUG_DEVICE_DESCRIPTOR* arg1); //0x70
    VOID* (*KdGetAcpiTablePhase0)(struct _LOADER_PARAMETER_BLOCK* arg1, ULONG arg2); //0x78
    VOID(*KdCheckPowerButton)();                                           //0x80
    UCHAR(*HalVectorToIDTEntry)(ULONG arg1);                               //0x88
    VOID* (*KdMapPhysicalMemory64)(union _LARGE_INTEGER arg1, ULONG arg2, UCHAR arg3); //0x90
    VOID(*KdUnmapVirtualAddress)(VOID* arg1, ULONG arg2, UCHAR arg3);      //0x98
    ULONG(*KdGetPciDataByOffset)(ULONG arg1, ULONG arg2, VOID* arg3, ULONG arg4, ULONG arg5); //0xa0
    ULONG(*KdSetPciDataByOffset)(ULONG arg1, ULONG arg2, VOID* arg3, ULONG arg4, ULONG arg5); //0xa8
    ULONG(*HalGetInterruptVectorOverride)(enum _INTERFACE_TYPE arg1, ULONG arg2, ULONG arg3, ULONG arg4, UCHAR* arg5, ULONGLONG* arg6); //0xb0
    LONG(*HalGetVectorInputOverride)(ULONG arg1, struct _GROUP_AFFINITY* arg2, ULONG* arg3, enum _KINTERRUPT_POLARITY* arg4, struct _INTERRUPT_REMAPPING_INFO* arg5); //0xb8
    LONG(*HalLoadMicrocode)(VOID* arg1);                                   //0xc0
    LONG(*HalUnloadMicrocode)();                                           //0xc8
    LONG(*HalPostMicrocodeUpdate)();                                       //0xd0
    LONG(*HalAllocateMessageTargetOverride)(struct _DEVICE_OBJECT* arg1, struct _GROUP_AFFINITY* arg2, ULONG arg3, enum _KINTERRUPT_MODE arg4, UCHAR arg5, ULONG* arg6, UCHAR* arg7, ULONG* arg8); //0xd8
    VOID(*HalFreeMessageTargetOverride)(struct _DEVICE_OBJECT* arg1, ULONG arg2, struct _GROUP_AFFINITY* arg3); //0xe0
    LONG(*HalDpReplaceBegin)(struct _HAL_DP_REPLACE_PARAMETERS* arg1, VOID** arg2); //0xe8
    VOID(*HalDpReplaceTarget)(VOID* arg1);                                 //0xf0
    LONG(*HalDpReplaceControl)(ULONG arg1, VOID* arg2);                    //0xf8
    VOID(*HalDpReplaceEnd)(VOID* arg1);                                    //0x100
    VOID(*HalPrepareForBugcheck)(ULONG arg1);                              //0x108
    UCHAR(*HalQueryWakeTime)(ULONGLONG* arg1, ULONGLONG* arg2);            //0x110
    VOID(*HalReportIdleStateUsage)(UCHAR arg1, struct _KAFFINITY_EX* arg2); //0x118
    VOID(*HalTscSynchronization)(UCHAR arg1, ULONG* arg2);                 //0x120
    LONG(*HalWheaInitProcessorGenericSection)(struct _WHEA_ERROR_RECORD_SECTION_DESCRIPTOR* arg1, struct _WHEA_PROCESSOR_GENERIC_ERROR_SECTION* arg2); //0x128
    VOID(*HalStopLegacyUsbInterrupts)(enum _SYSTEM_POWER_STATE arg1);      //0x130
    LONG(*HalReadWheaPhysicalMemory)(union _LARGE_INTEGER arg1, ULONG arg2, VOID* arg3); //0x138
    LONG(*HalWriteWheaPhysicalMemory)(union _LARGE_INTEGER arg1, ULONG arg2, VOID* arg3); //0x140
    LONG(*HalDpMaskLevelTriggeredInterrupts)();                            //0x148
    LONG(*HalDpUnmaskLevelTriggeredInterrupts)();                          //0x150
    LONG(*HalDpGetInterruptReplayState)(VOID* arg1, VOID** arg2);          //0x158
    LONG(*HalDpReplayInterrupts)(VOID* arg1);                              //0x160
    UCHAR(*HalQueryIoPortAccessSupported)();                               //0x168
    LONG(*KdSetupIntegratedDeviceForDebugging)(VOID* arg1, struct _DEBUG_DEVICE_DESCRIPTOR* arg2); //0x170
    LONG(*KdReleaseIntegratedDeviceForDebugging)(struct _DEBUG_DEVICE_DESCRIPTOR* arg1); //0x178
    VOID(*HalGetEnlightenmentInformation)(struct _HAL_INTEL_ENLIGHTENMENT_INFORMATION* arg1); //0x180
    VOID* (*HalAllocateEarlyPages)(struct _LOADER_PARAMETER_BLOCK* arg1, ULONG arg2, ULONGLONG* arg3, ULONG arg4); //0x188
    VOID* (*HalMapEarlyPages)(ULONGLONG arg1, ULONG arg2, ULONG arg3);      //0x190
    VOID* Dummy1;                                                           //0x198
    VOID* Dummy2;                                                           //0x1a0
    VOID(*HalNotifyProcessorFreeze)(UCHAR arg1, UCHAR arg2);               //0x1a8
    LONG(*HalPrepareProcessorForIdle)(ULONG arg1);                         //0x1b0
    VOID(*HalRegisterLogRoutine)(struct _HAL_LOG_REGISTER_CONTEXT* arg1);  //0x1b8
    VOID(*HalResumeProcessorFromIdle)();                                   //0x1c0
    VOID* Dummy;                                                            //0x1c8
    ULONG(*HalVectorToIDTEntryEx)(ULONG arg1);                             //0x1d0
    LONG(*HalSecondaryInterruptQueryPrimaryInformation)(struct _INTERRUPT_VECTOR_DATA* arg1, ULONG* arg2); //0x1d8
    LONG(*HalMaskInterrupt)(ULONG arg1, ULONG arg2);                       //0x1e0
    LONG(*HalUnmaskInterrupt)(ULONG arg1, ULONG arg2);                     //0x1e8
    UCHAR(*HalIsInterruptTypeSecondary)(ULONG arg1, ULONG arg2);           //0x1f0
    LONG(*HalAllocateGsivForSecondaryInterrupt)(CHAR* arg1, USHORT arg2, ULONG* arg3); //0x1f8
    LONG(*HalAddInterruptRemapping)(ULONG arg1, ULONG arg2, struct _PCI_BUSMASTER_DESCRIPTOR* arg3, UCHAR arg4, struct _INTERRUPT_VECTOR_DATA* arg5, ULONG arg6); //0x200
    VOID(*HalRemoveInterruptRemapping)(ULONG arg1, ULONG arg2, struct _PCI_BUSMASTER_DESCRIPTOR* arg3, UCHAR arg4, struct _INTERRUPT_VECTOR_DATA* arg5, ULONG arg6); //0x208
    VOID(*HalSaveAndDisableHvEnlightenment)();                             //0x210
    VOID(*HalRestoreHvEnlightenment)();                                    //0x218
    VOID(*HalFlushIoBuffersExternalCache)(struct _MDL* arg1, UCHAR arg2);  //0x220
    VOID(*HalFlushExternalCache)(UCHAR arg1);                              //0x228
    LONG(*HalPciEarlyRestore)(enum _SYSTEM_POWER_STATE arg1);              //0x230
    LONG(*HalGetProcessorId)(ULONG arg1, ULONG* arg2, ULONG* arg3);        //0x238
    LONG(*HalAllocatePmcCounterSet)(ULONG arg1, enum _KPROFILE_SOURCE* arg2, ULONG arg3, struct _HAL_PMC_COUNTERS** arg4); //0x240
    VOID(*HalCollectPmcCounters)(struct _HAL_PMC_COUNTERS* arg1, ULONGLONG* arg2); //0x248
    VOID(*HalFreePmcCounterSet)(struct _HAL_PMC_COUNTERS* arg1);           //0x250
    LONG(*HalProcessorHalt)(ULONG arg1, VOID* arg2, LONG(*arg3)(VOID* arg1)); //0x258
    ULONGLONG(*HalTimerQueryCycleCounter)(ULONGLONG* arg1);                //0x260
    VOID* Dummy3;                                                           //0x268
    VOID(*HalPciMarkHiberPhase)();                                         //0x270
    LONG(*HalQueryProcessorRestartEntryPoint)(union _LARGE_INTEGER* arg1); //0x278
    LONG(*HalRequestInterrupt)(ULONG arg1);                                //0x280
    LONG(*HalEnumerateUnmaskedInterrupts)(UCHAR(*arg1)(VOID* arg1, struct _HAL_UNMASKED_INTERRUPT_INFORMATION* arg2), VOID* arg2, struct _HAL_UNMASKED_INTERRUPT_INFORMATION* arg3); //0x288
    VOID(*HalFlushAndInvalidatePageExternalCache)(union _LARGE_INTEGER arg1); //0x290
    LONG(*KdEnumerateDebuggingDevices)(VOID* arg1, struct _DEBUG_DEVICE_DESCRIPTOR* arg2, enum KD_CALLBACK_ACTION(*arg3)(struct _DEBUG_DEVICE_DESCRIPTOR* arg1)); //0x298
    VOID(*HalFlushIoRectangleExternalCache)(struct _MDL* arg1, ULONG arg2, ULONG arg3, ULONG arg4, ULONG arg5, UCHAR arg6); //0x2a0
    VOID(*HalPowerEarlyRestore)(ULONG arg1);                               //0x2a8
    LONG(*HalQueryCapsuleCapabilities)(VOID* arg1, ULONG arg2, ULONGLONG* arg3, ULONG* arg4); //0x2b0
    LONG(*HalUpdateCapsule)(VOID* arg1, ULONG arg2, union _LARGE_INTEGER arg3); //0x2b8
    UCHAR(*HalPciMultiStageResumeCapable)();                               //0x2c0
    VOID(*HalDmaFreeCrashDumpRegisters)(ULONG arg1);                       //0x2c8
    UCHAR(*HalAcpiAoacCapable)();                                          //0x2d0
    LONG(*HalInterruptSetDestination)(struct _INTERRUPT_VECTOR_DATA* arg1, struct _GROUP_AFFINITY* arg2, ULONG* arg3); //0x2d8
    VOID(*HalGetClockConfiguration)(struct _HAL_CLOCK_TIMER_CONFIGURATION* arg1); //0x2e0
    VOID(*HalClockTimerActivate)(UCHAR arg1);                              //0x2e8
    VOID(*HalClockTimerInitialize)();                                      //0x2f0
    VOID(*HalClockTimerStop)();                                            //0x2f8
    LONG(*HalClockTimerArm)(enum _HAL_CLOCK_TIMER_MODE arg1, ULONGLONG arg2, ULONGLONG* arg3); //0x300
    UCHAR(*HalTimerOnlyClockInterruptPending)();                           //0x308
    VOID* (*HalAcpiGetMultiNode)();                                         //0x310
    VOID(*(*HalPowerSetRebootHandler)(void (*)(unsigned int, volatile int*)))(unsigned int, volatile int*); // 0x318
    VOID(*HalIommuRegisterDispatchTable)(struct _HAL_IOMMU_DISPATCH* arg1); //0x320
    VOID(*HalTimerWatchdogStart)();                                        //0x328
    VOID(*HalTimerWatchdogResetCountdown)();                               //0x330
    VOID(*HalTimerWatchdogStop)();                                         //0x338
    UCHAR(*HalTimerWatchdogGeneratedLastReset)();                          //0x340
    LONG(*HalTimerWatchdogTriggerSystemReset)(UCHAR arg1);                 //0x348
    LONG(*HalInterruptVectorDataToGsiv)(struct _INTERRUPT_VECTOR_DATA* arg1, ULONG* arg2); //0x350
    LONG(*HalInterruptGetHighestPriorityInterrupt)(ULONG* arg1, UCHAR* arg2); //0x358
    LONG(*HalProcessorOn)(ULONG arg1);                                     //0x360
    LONG(*HalProcessorOff)();                                              //0x368
    LONG(*HalProcessorFreeze)();                                           //0x370
    LONG(*HalDmaLinkDeviceObjectByToken)(ULONGLONG arg1, struct _DEVICE_OBJECT* arg2); //0x378
    LONG(*HalDmaCheckAdapterToken)(ULONGLONG arg1);                        //0x380
    VOID* Dummy4;                                                           //0x388
    LONG(*HalTimerConvertPerformanceCounterToAuxiliaryCounter)(ULONGLONG arg1, ULONGLONG* arg2, ULONGLONG* arg3); //0x390
    LONG(*HalTimerConvertAuxiliaryCounterToPerformanceCounter)(ULONGLONG arg1, ULONGLONG* arg2, ULONGLONG* arg3); //0x398
    LONG(*HalTimerQueryAuxiliaryCounterFrequency)(ULONGLONG* arg1);        //0x3a0
    LONG(*HalConnectThermalInterrupt)(UCHAR(*arg1)(struct _KINTERRUPT* arg1, VOID* arg2)); //0x3a8
    UCHAR(*HalIsEFIRuntimeActive)();                                       //0x3b0
    UCHAR(*HalTimerQueryAndResetRtcErrors)(UCHAR arg1);                    //0x3b8
    VOID(*HalAcpiLateRestore)();                                           //0x3c0
    LONG(*KdWatchdogDelayExpiration)(ULONGLONG* arg1);                     //0x3c8
    LONG(*HalGetProcessorStats)(enum _HAL_PROCESSOR_STAT_TYPE arg1, ULONG arg2, ULONG arg3, ULONGLONG* arg4); //0x3d0
    ULONGLONG(*HalTimerWatchdogQueryDueTime)(UCHAR arg1);                  //0x3d8
    LONG(*HalConnectSyntheticInterrupt)(UCHAR(*arg1)(struct _KINTERRUPT* arg1, VOID* arg2)); //0x3e0
    VOID(*HalPreprocessNmi)(ULONG arg1);                                   //0x3e8
    LONG(*HalEnumerateEnvironmentVariablesWithFilter)(ULONG arg1, UCHAR(*arg2)(struct _GUID* arg1, WCHAR* arg2), VOID* arg3, ULONG* arg4); //0x3f0
    LONG(*HalCaptureLastBranchRecordStack)(ULONG arg1, struct _HAL_LBR_ENTRY* arg2, ULONG* arg3); //0x3f8
    UCHAR(*HalClearLastBranchRecordStack)();                               //0x400
    LONG(*HalConfigureLastBranchRecord)(ULONG arg1, ULONG arg2);           //0x408
    UCHAR(*HalGetLastBranchInformation)(ULONG* arg1, ULONG* arg2);         //0x410
    VOID(*HalResumeLastBranchRecord)(UCHAR arg1);                          //0x418
    LONG(*HalStartLastBranchRecord)(ULONG arg1, ULONG* arg2);              //0x420
    LONG(*HalStopLastBranchRecord)(ULONG arg1);                            //0x428
    LONG(*HalIommuBlockDevice)(VOID* arg1);                                //0x430
    LONG(*HalIommuUnblockDevice)(struct _EXT_IOMMU_DEVICE_ID* arg1, VOID** arg2); //0x438
    LONG(*HalGetIommuInterface)(ULONG arg1, struct _DMA_IOMMU_INTERFACE* arg2); //0x440
    LONG(*HalRequestGenericErrorRecovery)(VOID* arg1, ULONG* arg2);        //0x448
    LONG(*HalTimerQueryHostPerformanceCounter)(ULONGLONG* arg1);           //0x450
    LONG(*HalTopologyQueryProcessorRelationships)(ULONG arg1, ULONG arg2, UCHAR* arg3, UCHAR* arg4, UCHAR* arg5, ULONG* arg6, ULONG* arg7); //0x458
    VOID(*HalInitPlatformDebugTriggers)();                                 //0x460
    VOID(*HalRunPlatformDebugTriggers)(UCHAR arg1);                        //0x468
    VOID* (*HalTimerGetReferencePage)();                                    //0x470
    LONG(*HalGetHiddenProcessorPowerInterface)(struct _HIDDEN_PROCESSOR_POWER_INTERFACE* arg1); //0x478
    ULONG(*HalGetHiddenProcessorPackageId)(ULONG arg1);                    //0x480
    ULONG(*HalGetHiddenPackageProcessorCount)(ULONG arg1);                 //0x488
    LONG(*HalGetHiddenProcessorApicIdByIndex)(ULONG arg1, ULONG* arg2);    //0x490
    LONG(*HalRegisterHiddenProcessorIdleState)(ULONG arg1, ULONGLONG arg2); //0x498
    VOID(*HalIommuReportIommuFault)(ULONGLONG arg1, struct _FAULT_INFORMATION* arg2); //0x4a0
    UCHAR(*HalIommuDmaRemappingCapable)(struct _EXT_IOMMU_DEVICE_ID* arg1, ULONG* arg2); //0x4a8
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;