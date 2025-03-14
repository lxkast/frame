#include "log.hpp"
#include "structs.hpp"
#include <ntifs.h>

typedef VOID(*PHAL_PREPROCESS_NMI)(ULONG arg1);
PHAL_PREPROCESS_NMI HalPreprocessNmiOriginal = nullptr;

VOID HalPreprocessNmiHook(ULONG arg1) {
	HalPreprocessNmiOriginal(arg1);
	LOG_DEBUG("HalPreprocessNmi hook called");
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
	Unhook();

	UNICODE_STRING symbolic_link = RTL_CONSTANT_STRING(L"\\??\\frame");
	IoDeleteSymbolicLink(&symbolic_link);

	if (driver_object->DeviceObject) {
		LOG_INFO("Deleting device object");
		IoDeleteDevice(driver_object->DeviceObject);
	}
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