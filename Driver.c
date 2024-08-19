/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include <stdio.h>
#include <ntddk.h>

NTKERNELAPI PCHAR PsGetProcessImageFileName(PEPROCESS Process);
NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
BOOLEAN notifyCreateProcess();
HANDLE logHandle;
BOOLEAN isNotifyCreate = 1;


UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\mydeviveio");
PDEVICE_OBJECT device = NULL;
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\mydevicelinkio");

NTSTATUS DispatchPassThru(PDEVICE_OBJECT Device, PIRP Irp) {
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    
    NTSTATUS status = STATUS_SUCCESS;
    switch (irpsp->MajorFunction) {
    case IRP_MJ_CREATE:
        DbgPrint("Create request");
        break;
    case IRP_MJ_CLOSE:
        DbgPrint("Close request");
        break;
    case IRP_MJ_READ:
        DbgPrint("Read request");
        break;
    default:
        break;
    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

/*

NTSTATUS DispatchDevCTL(PDEVICE_OBJECT Device_Object, PIRP Irp) {    // 16:58

    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength = 0;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    WCHAR* demo = L"sample returned from driver";

    switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
    case DEVICE_SEND:
        KdPrint(("send data is %ws \r\n", buffer));
        returnLength = (wcsnlen(buffer, 511) + 1) * 2;
        break;
    case DEVICE_REC:
        wcsncpy(buffer, demo, 511);
        returnLength = (wcsnlen(buffer, 511) + 1) * 2;
        break;
    default:
        status = STATUS_INVALID_PARAMETER;
        break;
    }
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = returnLength;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

 */ 

VOID UnDriver(PDRIVER_OBJECT driver)
{

    IoDeleteSymbolicLink(&SymLinkName);
    IoDeleteDevice(device);
    DbgPrint("Good Bye from kernel driver!");
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT Driver, PUNICODE_STRING RegistryPath)
{
    DbgPrint("Hello from Windows kernel mode!!?\n");
    Driver->DriverUnload = UnDriver;
    //Создаём девайс
    NTSTATUS status = IoCreateDevice(Driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device);

    if (!NT_SUCCESS(status)) {
        DbgPrint("Error of create device!\n");
    }
    //Создаём ссылку на девайс
    status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Error of create simbolic link!\n");
        IoDeleteDevice(device);
    }

    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        Driver->MajorFunction[i] = DispatchPassThru;
    }


    DbgPrint("Success load");
    return STATUS_SUCCESS;
}
