#include <ntifs.h>
#include "log.hpp"
#include "structs.hpp"
#include "helpers.hpp"
#include "driver.hpp"

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
    PKPRCB_THREADS kprcb_threads = reinterpret_cast<PKPRCB_THREADS>(reinterpret_cast<PUCHAR>(kpcr) + 0x188);

    ULONG processor_index = KeGetCurrentProcessorNumberEx(0);
    machine_frame->Rip = nmi_core_infos[processor_index].prev_rip;
    machine_frame->Rsp = nmi_core_infos[processor_index].prev_rsp;
    kprcb_threads->CurrentThread = nmi_core_infos[processor_index].prev_current_thread;
    kprcb_threads->NextThread = nmi_core_infos[processor_index].prev_next_thread;
    *reinterpret_cast<PBOOLEAN>(reinterpret_cast<PUCHAR>(kprcb_threads->IdleThread) + 0x71) = nmi_core_infos[processor_index].prev_active;

    LOG_DEBUG("Swapped back thread");
    if (callback_parent) {
        callback_parent->Next = nullptr;
    }

    return handled;
}

VOID HalPreprocessNmiHook(ULONG arg1) {
    KeGetCurrentThread();
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
    PKPRCB_THREADS kprcb_threads = reinterpret_cast<PKPRCB_THREADS>(reinterpret_cast<PUCHAR>(kpcr) + 0x188);

    ULONG processor_index = KeGetCurrentProcessorNumberEx(0);
    nmi_core_infos[processor_index].prev_rip = machine_frame->Rip;
    nmi_core_infos[processor_index].prev_rsp = machine_frame->Rsp;
    nmi_core_infos[processor_index].prev_current_thread = kprcb_threads->CurrentThread;
    nmi_core_infos[processor_index].prev_next_thread = kprcb_threads->NextThread;
    nmi_core_infos[processor_index].prev_active = *reinterpret_cast<PBOOLEAN>(reinterpret_cast<PUCHAR>(kprcb_threads->IdleThread) + 0x71);

    /*
        We will spoof as the current core's idle system thread
        Through investigation with WinDbg: a valid RSP should be around 0x38 under the initial RSP
        which should work well if we pretend RIP was nt!PoIdle
    */
    
    machine_frame->Rip = PoIdle;
    machine_frame->Rsp = *reinterpret_cast<PULONGLONG>(reinterpret_cast<PUCHAR>(kprcb_threads->IdleThread) + 0x28) - 0x38;
    kprcb_threads->CurrentThread = kprcb_threads->IdleThread;
    kprcb_threads->NextThread = nullptr;
    *reinterpret_cast<PBOOLEAN>(reinterpret_cast<PUCHAR>(kprcb_threads->IdleThread) + 0x71) = TRUE;
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

NTSTATUS SigscanPoIdle() {
    uintptr_t ntos_base_address = helpers::get_ntos_base_address();

    // sig found in KiProcessNmi
    // 48 8B 3D ? ? ? ? 41 8A F4
    char NmiSignature[] = "\x40\x55\x53\x41\x56";
    char NmiSignatureMask[] = "xxxxx";
    PoIdle = helpers::find_pattern(ntos_base_address,
        NmiSignature,
        NmiSignatureMask);

    return PoIdle ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

extern "C"
NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path) {
    UNREFERENCED_PARAMETER(registry_path);

    driver_object->DriverUnload = Unload;

    auto status = STATUS_SUCCESS;
    do {
        nmi_list_head = SigscanKiNmiCallbackListHead();
        status = SigscanPoIdle();
        if (!NT_SUCCESS(status)) {
            LOG_ERROR("Failed to find PoIdle");
            break;
        }

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
        return status;
    }

    LOG_DEBUG("Successfully initialised");

    return status;
}