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
    PKPRCB_THREADS kprcb_threads = reinterpret_cast<PKPRCB_THREADS>(kpcr + 0x188);

    ULONG processor_index = KeGetCurrentProcessorNumberEx(0);
    machine_frame->Rip = nmi_core_infos[processor_index].prev_rip;
    machine_frame->Rsp = nmi_core_infos[processor_index].prev_rsp;
    kprcb_threads->CurrentThread = nmi_core_infos[processor_index].prev_current_thread;
    kprcb_threads->NextThread = nmi_core_infos[processor_index].prev_next_thread;
    kprcb_threads->IdleThread = nmi_core_infos[processor_index].prev_idle_thread;

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
    PKPRCB_THREADS kprcb_threads = reinterpret_cast<PKPRCB_THREADS>(kpcr + 0x188);

    ULONG processor_index = KeGetCurrentProcessorNumberEx(0);
    nmi_core_infos[processor_index].prev_rip = machine_frame->Rip;
    nmi_core_infos[processor_index].prev_rsp = machine_frame->Rsp;
    nmi_core_infos[processor_index].prev_current_thread = kprcb_threads->CurrentThread;
    nmi_core_infos[processor_index].prev_next_thread = kprcb_threads->NextThread;
    nmi_core_infos[processor_index].prev_idle_thread = kprcb_threads->IdleThread;

    /*
        We will spoof as the current core's idle system thread
        Through investigation with WinDbg: a valid RSP should be around 0x70 under the initial RSP
        which should work well if we pretend RIP was nt!PoIdle
    */
    
    machine_frame->Rip = PoIdle;
    machine_frame->Rsp = reinterpret_cast<ULONGLONG>(idle_thread_list[processor_index].initial_stack) - 0x70;
    kprcb_threads->CurrentThread = idle_thread_list[processor_index].thread;
    kprcb_threads->NextThread = nullptr;
    kprcb_threads->IdleThread = idle_thread_list[processor_index].thread;
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
    ExFreePoolWithTag(idle_thread_list, NMI_THREAD_LIST_TAG);
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

NTSTATUS GetIdleThreads() {
    if (!PsIdleProcess) {
        LOG_ERROR("PsIdleProcess not found");
        return STATUS_UNSUCCESSFUL;
    }
    // just using offsets because EPROCESS and KTHREAD contain a lot of structs I would need to specify
    // offset to ThreadListLock
    PEX_PUSH_LOCK thread_lock = PEX_PUSH_LOCK(reinterpret_cast<ULONG_PTR>(PsIdleProcess) + 0x860);
    ExAcquirePushLockExclusive(thread_lock);
    // offset to ThreadListHead
    PLIST_ENTRY list_head = reinterpret_cast<PLIST_ENTRY>(PsIdleProcess) + 0x5e0;

    PLIST_ENTRY list_entry = list_head;
    for (ULONG i = 0;
        i < KeQueryActiveProcessorCount(0) && list_entry && list_entry->Flink != list_head;
        list_entry = list_entry->Flink
        ) {
        // offset from ThreadListEntry back to start of KTHREAD
        PKTHREAD thread = reinterpret_cast<PKTHREAD>(list_entry - 0x2f8);
        idle_thread_list[i].thread = thread;
        // offset to InitialStack
        idle_thread_list[i].initial_stack = reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(thread) + 0x28);
        LOG_DEBUG("Stored thread %d", i);
    }

    return STATUS_SUCCESS;
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
        
        idle_thread_list = static_cast<PTHREAD_INFO>(ExAllocatePool2(POOL_FLAG_NON_PAGED,
            num_processors * sizeof(THREAD_INFO),
            NMI_THREAD_LIST_TAG));
        if (!idle_thread_list) {
            LOG_ERROR("Failed to allocate idle thread array");
            status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        status = GetIdleThreads();

        if (!NT_SUCCESS(status)) {
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