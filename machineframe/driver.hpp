constexpr ULONG NMI_CORE_INFO_TAG = 'imn';

typedef VOID(*PHAL_PREPROCESS_NMI)(ULONG arg1);
PHAL_PREPROCESS_NMI HalPreprocessNmiOriginal = nullptr;
BOOLEAN RestoreFrameCallback(PVOID context, BOOLEAN handled);

typedef struct _NMI_CORE_INFO {
    ULONG64 prev_rip;
    ULONG64 prev_rsp;
    PKTHREAD prev_current_thread;
    PKTHREAD prev_next_thread;
    BOOLEAN prev_active;
} NMI_CORE_INFO, * PNMI_CORE_INFO;

typedef struct _KPRCB_THREADS {
    PKTHREAD CurrentThread;
    PKTHREAD NextThread;
    PKTHREAD IdleThread;
} KPRCB_THREADS, *PKPRCB_THREADS;

PNMI_CORE_INFO nmi_core_infos;
PKNMI_HANDLER_CALLBACK nmi_list_head = nullptr;
PKNMI_HANDLER_CALLBACK callback_parent = nullptr;
KNMI_HANDLER_CALLBACK restore_callback{ nullptr, reinterpret_cast<void(*)()>(RestoreFrameCallback), nullptr, nullptr };

ULONGLONG PoIdle;