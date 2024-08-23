/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include <wdm.h>
#include <ntddk.h>


NTKERNELAPI PCHAR PsGetProcessImageFileName(PEPROCESS Process);
NTKERNELAPI NTSTATUS PsLookupProcessByProcessId(HANDLE ProcessId, PEPROCESS* Process);
BOOLEAN notifyCreateProcess();
void myitoa(int x, char* rez);
NTSTATUS DispatchPassThru(PDEVICE_OBJECT Device, PIRP Irp);
NTSTATUS DispatchDevCTL(PDEVICE_OBJECT Device_Object, PIRP Irp);
int createDevice(PDRIVER_OBJECT Driver);
HANDLE logHandle;
BOOLEAN isCreateNotify = 0;
BOOLEAN isDeleteNotify = 0;
UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\mydeviveio");
PDEVICE_OBJECT device = NULL;
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\mydevicelinkio");
#define CREATE_NOTIFY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)


void myitoa(int x, char* rez) {//Перевод числа в строку
    IO_STATUS_BLOCK logWriteStatus;
    NTSTATUS writeStatus;
    char result_reverce[40] = { 0 };
    char result[40] = {0};
    char num[2] = { 0 };
    num[1] = '\0';
    int len = 0;
    while (x >= 1) {
        num[0] = (x%10) + 48;
        strcat(result_reverce, num);
        x /= 10;  
    }
    len = strlen(result_reverce);
    for (int i = 0; i < len; i++) {
        result[i] = result_reverce[len - 1 - i];
    }
    char ex[50] = { 0 };
    strcpy(rez, result);
}

PCHAR GetProcessNameByProcessId(HANDLE ProcessId)//Получение имени процесса по PID
{
    NTSTATUS st = STATUS_UNSUCCESSFUL;
    PEPROCESS ProcessObj = NULL;
    PCHAR string = NULL;
    st = PsLookupProcessByProcessId(ProcessId, &ProcessObj);
    if (NT_SUCCESS(st))
    {
        string = PsGetProcessImageFileName(ProcessObj);
        ObfDereferenceObject(ProcessObj);
    }
    return string;
}

VOID MyCreateProcessNotifyEx(PEPROCESS Process, HANDLE pid, PPS_CREATE_NOTIFY_INFO CreateInfo)//Нотификатор на создание процессов
{
    IO_STATUS_BLOCK logWriteStatus;
    NTSTATUS writeStatus;
    char ProcName[100] = { 0 };
    char resultStr[100] = {0};
    char ID[100] = { 0 };
    LARGE_INTEGER time; 
    LARGE_INTEGER local_time;
    TIME_FIELDS real_time;
    HANDLE procID;
    intptr_t example = 500;
    char  char_hour[4] = { 0 };
    char  char_min[4] = { 0 };
    char  char_sec[4] = { 0 };
    PPS_CREATE_NOTIFY_INFO info = CreateInfo;
    DbgPrint("Hello! We found process!?!");
    KeQuerySystemTime(&time);
    ExSystemTimeToLocalTime(&time, &local_time);
    time.QuadPart /= 10000000;
    RtlTimeToTimeFields(&local_time, &real_time);
    myitoa(real_time.Hour, char_hour);
    strcpy(ProcName, char_hour);
    strcat(ProcName, ":");
    myitoa(real_time.Minute, char_min);
    strcat(ProcName, char_min);
    strcat(ProcName, ":");
    myitoa(real_time.Second, char_sec);
    strcat(ProcName, char_sec);
    if (info != NULL)
    {
        strcat(ProcName, "     Process is created. Name: ");
        strcat(ProcName, PsGetProcessImageFileName(Process));
        strcat(ProcName, " PID: ");
        procID = PsGetProcessId(Process);
        DbgPrint("Process ID is %d\n", procID);
        example =(intptr_t) procID;
        myitoa((int)example, ID);
        strcat(ProcName, ID);
        strcat(ProcName, "\n");
        DbgPrint("Process: %s", ProcName);
        writeStatus = ZwWriteFile(logHandle,
            NULL,
            NULL,
            NULL,
            &logWriteStatus,
            &ProcName,
            strlen(ProcName),
            NULL,
            NULL);
        DbgPrint("Result of write: %X", writeStatus);
    }
}

VOID MyDeleteProcessNotifyEx(PEPROCESS Process, HANDLE pid, PPS_CREATE_NOTIFY_INFO CreateInfo)//Нотификатор на удаление процессов
{
    IO_STATUS_BLOCK logWriteStatus;
    NTSTATUS writeStatus;
    char ProcName[100] = { 0 };
    char resultStr[100] = { 0 };
    char ID[100] = { 0 };
    LARGE_INTEGER time;
    LARGE_INTEGER local_time;
    TIME_FIELDS real_time;
    HANDLE procID;
    intptr_t example = 500;
    char  char_hour[4] = { 0 };
    char  char_min[4] = { 0 };
    char  char_sec[4] = { 0 };
    PPS_CREATE_NOTIFY_INFO info = CreateInfo;
    DbgPrint("Hello! We found process!?!");
    KeQuerySystemTime(&time);
    ExSystemTimeToLocalTime(&time, &local_time);
    time.QuadPart /= 10000000;
    RtlTimeToTimeFields(&local_time, &real_time);
    myitoa(real_time.Hour, char_hour);
    strcpy(ProcName, char_hour);
    strcat(ProcName, ":");
    myitoa(real_time.Minute, char_min);
    strcat(ProcName, char_min);
    strcat(ProcName, ":");
    myitoa(real_time.Second, char_sec);
    strcat(ProcName, char_sec);
    if (info == NULL)
    {
        strcat(ProcName, "     Process is exit. Name: ");
        strcat(ProcName, PsGetProcessImageFileName(Process));
        strcat(ProcName, " PID: ");
        procID = PsGetProcessId(Process);
        DbgPrint("Process ID is %d\n", procID);
        example = (intptr_t)procID;
        myitoa((int)example, ID);
        strcat(ProcName, ID);
        strcat(ProcName, "\n");
        DbgPrint("Process: %s", ProcName);
        writeStatus = ZwWriteFile(logHandle,
            NULL,
            NULL,
            NULL,
            &logWriteStatus,
            &ProcName,
            strlen(ProcName),
            NULL,
            NULL);
        DbgPrint("Result of write: %X", writeStatus);
    }
}

VOID UnDriver(PDRIVER_OBJECT driver)//Выгрузка драйвера и удаление всех объектов
{
    IoDeleteSymbolicLink(&SymLinkName);
    IoDeleteDevice(device);    
    ZwClose(logHandle);
    DbgPrint("Good Bye from kernel driver!");
    PDRIVER_OBJECT copy = driver;
    if (isCreateNotify) {
        PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, TRUE);
        isCreateNotify = 0;
    }
    if (isDeleteNotify) {
        PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyDeleteProcessNotifyEx, TRUE);
        isDeleteNotify = 0;
    }
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT Driver, PUNICODE_STRING RegistryPath)//ТЬочка входа в драйвер
{
    DbgPrint("Hello from Windows kernel mode!!?\n");
    if (createDevice(Driver)) {//Создаём девайс для взаимодействия с польтзовательским приложением
        DbgPrint("Error of create device\n. Driver will be unload");
        //unload driver
    }
    notifyCreateProcess();//Сохздаём файл для логирования
    Driver->DriverUnload = UnDriver;
    return STATUS_SUCCESS;
}

int createDevice(PDRIVER_OBJECT Driver) {//Создание девайса
    NTSTATUS status = IoCreateDevice(Driver, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &device);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Error of create device!\n");
        return 1;
    }
    //Создаём ссылку на девайс
    status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Error of create simbolic link!\n");
        IoDeleteDevice(device);
        return 1;
    }
    for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        Driver->MajorFunction[i] = DispatchPassThru;
    }
    Driver->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevCTL;
    DbgPrint("Success load\n");
    return 0;
}

BOOLEAN notifyCreateProcess() {//Создание файла для логов
    NTSTATUS createFileStatus;
    OBJECT_ATTRIBUTES  fileAttr;
    UNICODE_STRING     fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    RtlInitUnicodeString(&fileName, L"\\??\\C:\\log2.txt");
    InitializeObjectAttributes(&fileAttr, &fileName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);
    createFileStatus = ZwCreateFile(&logHandle,
        GENERIC_WRITE,
        &fileAttr,
        &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    DbgPrint("Result of open file is %X\n", createFileStatus);
    return createFileStatus;
}

NTSTATUS DispatchPassThru(PDEVICE_OBJECT Device, PIRP Irp) {//Отслеживание операций с девайсом
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    switch (irpsp->MajorFunction) {
    case IRP_MJ_CREATE:
        DbgPrint("Create request\n");
        break;
    case IRP_MJ_CLOSE:
        DbgPrint("Close request\n");
        break;
    case IRP_MJ_READ:
        DbgPrint("Read request\n");
        break;
    default:
        break;
    }
    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

NTSTATUS DispatchDevCTL(PDEVICE_OBJECT Device_Object, PIRP Irp) {//Обработка запросов на "контроль" девайса
    PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
    NTSTATUS status = STATUS_SUCCESS;
    ULONG returnLength = 0;
    PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
    ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
    WCHAR* demo = L"sample returned from driver\n";
    switch (irpsp->Parameters.DeviceIoControl.IoControlCode) {
    case CREATE_NOTIFY:
        //Установка разных нотификаторов по передаваемому номеру
        DbgPrint("send data is %ws \r\n", buffer);
        if (!wcscmp(buffer, L"1")) {//команда 1 - установка нотификатора на создание процессов
            DbgPrint("It is 1\n");
            if (!isCreateNotify) {
                status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, FALSE);
                isCreateNotify = 1;
            }
            DbgPrint("We created ProcessRoutine\n");
            DbgPrint("Result of create process: %X", status);
        }
        if (!wcscmp(buffer, L"2")) {//команда 2 - установка нотификатора на закрытие процессов
            DbgPrint("It is 2\n");
            if (!isDeleteNotify) {
                status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyDeleteProcessNotifyEx, FALSE);
                isDeleteNotify = 1;
            }
            DbgPrint("We created Delete ProcessRoutine\n");
            DbgPrint("Result of create process: %X", status);
        }
        if (!wcscmp(buffer, L"3")) {//команда 3 - снятие нотификатора на создание процессов
            if (isCreateNotify) {
                PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, TRUE);
                isCreateNotify = 0;
            }
        }
        if (!wcscmp(buffer, L"4")) {//команда 4 - снятие нотификатора на закрытие процессов
            DbgPrint("It is 4\n");
            if (isDeleteNotify) {
                PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyDeleteProcessNotifyEx, TRUE);
                isDeleteNotify = 0;
            }
        }
        returnLength = (wcsnlen(buffer, 511) + 1) * 2;
        DbgPrint("Len of buffer= %d\n", returnLength);
        break;
    case DEVICE_REC://Отправка данных в пользовательское приложение, мб пригодится
        wcsncpy(buffer, demo, 511);
        returnLength = (wcsnlen(buffer, 511) + 1) * 2;
        DbgPrint("We send string: %ws\n", buffer);
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