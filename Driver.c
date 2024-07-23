﻿/*++

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
void myitoa(int x, char* rez);
HANDLE logHandle;
BOOLEAN isNotifyCreate = 1;


void myitoa(int x, char* rez) {
    IO_STATUS_BLOCK logWriteStatus;
    NTSTATUS writeStatus;
    char result_reverce[40] = { 0 };
    char result[40] = {0};
    char num[2] = { 0 };
    num[1] = '\0';
    int len = 0;

   // strcat(result_reverce, "Hello, world");
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
   // strcpy(ex, r);
    strcpy(rez, result);
}






PCHAR GetProcessNameByProcessId(HANDLE ProcessId)
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

VOID MyCreateProcessNotifyEx(PEPROCESS Process, PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    IO_STATUS_BLOCK logWriteStatus;
    NTSTATUS writeStatus;
    //char* resultString = "Pcocess created. Name: "
    char ProcName[100] = { 0 };
    char resultStr[100] = {0};
    char ID[100] = { 0 };
    HANDLE procID;
    intptr_t example = 500;
    DbgPrint("Hello! We found process!?!");
    if (CreateInfo != NULL)
    {
        //strcpy(resultStr, "Process is created. Name: ");
        strcpy(ProcName, "Process is created. Name: ");
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
    else
    {
       //end of proces
        strcpy(ProcName, "Process is deleted. Name: ");
        strcat(ProcName, PsGetProcessImageFileName(Process));
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

VOID UnDriver(PDRIVER_OBJECT driver)
{
    
    ZwClose(logHandle);
    DbgPrint("Good Bye from kernel driver!");
    PDRIVER_OBJECT copy = driver;
    PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, TRUE);
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT Driver, PUNICODE_STRING RegistryPath)
{
    DbgPrint("Hello from Windows kernel mode!!?\n");
    notifyCreateProcess();
    NTSTATUS status = PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, FALSE);
    DbgPrint("We created ProcessRoutine\n");
    DbgPrint("Result of create process: %X", status);
    Driver->DriverUnload = UnDriver;
//    DbgPrint("Hello from Windows kernel mode!!?");
    return STATUS_SUCCESS;
}

BOOLEAN notifyCreateProcess() {
    NTSTATUS createFileStatus;
    OBJECT_ATTRIBUTES  fileAttr;
    UNICODE_STRING     fileName;
    IO_STATUS_BLOCK ioStatusBlock;
    RtlInitUnicodeString(&fileName, L"\\??\\D:\\log2.txt");
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
        FILE_CREATE,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    DbgPrint("Result of open file is %X\n", createFileStatus);
    return createFileStatus;
}