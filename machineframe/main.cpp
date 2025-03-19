#include <ntifs.h>
#include "log.hpp"
#include "structs.hpp"
#include "helpers.hpp"

constexpr ULONG IA32_GS_BASE = 0xC0000101;
constexpr ULONG NMI_CORE_INFO_TAG = 'imn';

typedef VOID(*PHAL_PREPROCESS_NMI)(ULONG arg1);
PHAL_PREPROCESS_NMI HalPreprocessNmiOriginal = nullptr;
BOOLEAN RestoreFrameCallback(PVOID context, BOOLEAN handled);

typedef struct _NMI_CORE_INFO {
    ULONG64 prev_rip;
    ULONG64 prev_rsp;
} NMI_CORE_INFO, *PNMI_CORE_INFO;

// each processor needs its own store for hook-related info
PNMI_CORE_INFO nmi_core_infos;

PKNMI_HANDLER_CALLBACK nmi_list_head = nullptr;
PKNMI_HANDLER_CALLBACK callback_parent = nullptr;
KNMI_HANDLER_CALLBACK restore_callback {nullptr, reinterpret_cast<void(*)()>(RestoreFrameCallback), nullptr, nullptr};

BOOLEAN RestoreFrameCallback(PVOID context, BOOLEAN handled) {
    UNREFERENCED_PARAMETER(context);
    LOG_DEBUG("Restore frame callback called");
    /*
        steps to find machine frame:

        1. get KPCR, which is stored in the GS register
        2. get TSS from the KPCR
        3. get the address of the NMI interrupt stack table from the TSS
        4. subtract the size of the machine frame structure
    */
    PKPCR kpcr = KeGetPcr();
    PKTSS64 tss = (PKTSS64)kpcr->TssBase;
    PMACHINE_FRAME machine_frame = (PMACHINE_FRAME)(tss->Ist[3] - sizeof(MACHINE_FRAME));

    ULONG processor_index = KeGetCurrentProcessorNumberEx(nullptr);
    machine_frame->Rip = nmi_core_infos[processor_index].prev_rip;
    machine_frame->Rsp = nmi_core_infos[processor_index].prev_rsp;

    LOG_DEBUG("Swapped back machine frame");

    if (callback_parent) {
        callback_parent->Next = nullptr;
    }

    return handled;
}

VOID HalPreprocessNmiHook(ULONG arg1) {
    HalPreprocessNmiOriginal(arg1);
    LOG_DEBUG("HalPreprocessNmi hook called");
    
    if (arg1 == 1) return;

    if (!nmi_list_head) {
        LOG_ERROR("Failed to find: Nmi list head");
        return;
    }

    // add our restore function to the back of the callback list
    callback_parent = nullptr;
    PKNMI_HANDLER_CALLBACK current_callback = nmi_list_head;
    while (current_callback) {
        callback_parent = current_callback;
        current_callback = current_callback->Next;
    }
    callback_parent->Next = &restore_callback;

    PKPCR kpcr = KeGetPcr();
    PKTSS64 tss = (PKTSS64)kpcr->TssBase;
    PMACHINE_FRAME machine_frame = (PMACHINE_FRAME)(tss->Ist[3] - sizeof(MACHINE_FRAME));

    ULONG processor_index = KeGetCurrentProcessorNumberEx(nullptr);
    nmi_core_infos[processor_index].prev_rip = machine_frame->Rip;
    nmi_core_infos[processor_index].prev_rsp = machine_frame->Rsp;

    machine_frame->Rip = 0x123;
    machine_frame->Rsp = 0x456;
}

NTSTATUS InitHook() {
    UNICODE_STRING target = RTL_CONSTANT_STRING(L"HalPrivateDispatchTable");
    PHAL_PRIVATE_DISPATCH hal_private_dispatch = reinterpret_cast<PHAL_PRIVATE_DISPATCH>(MmGetSystemRoutineAddress(&target));
    if (!hal_private_dispatch) {
        LOG_ERROR("Failed to find HAL private dispatch table");
        return STATUS_RESOURCE_NAME_NOT_FOUND;
    }

    HalPreprocessNmiOriginal = hal_private_dispatch->HalPreprocessNmi;
    hal_private_dispatch->HalPreprocessNmi = HalPreprocessNmiHook;
    LOG_DEBUG("Hooked HalPreprocessNmi");

    return STATUS_SUCCESS;
}

VOID Unhook() {
    if (!HalPreprocessNmiOriginal) {
        return;
    }

    UNICODE_STRING target = RTL_CONSTANT_STRING(L"HalPrivateDispatchTable");

    PHAL_PRIVATE_DISPATCH hal_private_dispatch = reinterpret_cast<PHAL_PRIVATE_DISPATCH>(MmGetSystemRoutineAddress(&target));
    if (!hal_private_dispatch) {
        LOG_ERROR("Failed to find HAL private dispatch table");
        return;
    }
    hal_private_dispatch->HalPreprocessNmi = HalPreprocessNmiOriginal;
}

VOID Unload(PDRIVER_OBJECT driver_object) {
    
    LOG_DEBUG("Unloading");
    ExFreePoolWithTag(nmi_core_infos, NMI_CORE_INFO_TAG);
    Unhook();

    UNICODE_STRING symbolic_link = RTL_CONSTANT_STRING(L"\\??\\frame");
    IoDeleteSymbolicLink(&symbolic_link);

    if (driver_object->DeviceObject) {
        LOG_INFO("Deleting device object");
        IoDeleteDevice(driver_object->DeviceObject);
    }
}

PKNMI_HANDLER_CALLBACK SigscanKiNmiCallbackListHead() {
    uintptr_t ntos_base_address = helpers::get_ntos_base_address();

    // sig found in KiProcessNmi
    // 48 8B 3D ? ? ? ? 41 8A F4
    char NmiSignature[] = "\x48\x8b\x3d\x00\x00\x00\x00\x41\x8a\xf4";
    char NmiSignatureMask[] = "xxx????xxx";
    uintptr_t nmi_instruction = helpers::find_pattern(ntos_base_address,
        NmiSignature,
        NmiSignatureMask);

    return reinterpret_cast<PKNMI_HANDLER_CALLBACK>(helpers::resolve_address(nmi_instruction, 0x3, 0x7));
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    driver_object->DriverUnload = Unload;

    PDEVICE_OBJECT device_object;
    UNICODE_STRING device_name = RTL_CONSTANT_STRING(L"\\Device\\frame");
    UNICODE_STRING symbolic_link = RTL_CONSTANT_STRING(L"\\??\\frame");

    auto status = STATUS_SUCCESS;
    do {
        status = IoCreateDevice(driver_object, 0, &device_name, FILE_DEVICE_UNKNOWN, 0, FALSE, &device_object);
        if (!NT_SUCCESS(status)) {
            LOG_ERROR("Failed to create device");
            break;
        }

        status = IoCreateSymbolicLink(&symbolic_link, &device_name);
        if (!NT_SUCCESS(status)) {
            LOG_ERROR("Failed to create symbolic link");
            break;
        }

        nmi_list_head = SigscanKiNmiCallbackListHead();

        ULONG num_processors = KeQueryActiveProcessorCount(0);
        nmi_core_infos = static_cast<PNMI_CORE_INFO>(ExAllocatePool2(POOL_FLAG_NON_PAGED,
            num_processors * sizeof(NMI_CORE_INFO),
            NMI_CORE_INFO_TAG));

        if (!nmi_core_infos) {
            LOG_ERROR("Failed to allocate nmi core info array");
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = InitHook();
        
    } while (false);

    if (!NT_SUCCESS(status)) {
        IoDeleteSymbolicLink(&symbolic_link);
        if (device_object) {
            IoDeleteDevice(device_object);
        }
        return status;
    }

    LOG_DEBUG("Successfully initialised");

    return status;
}