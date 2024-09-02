#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>



PFLT_FILTER FilterHandle = NULL;
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

//VOID GetProcName(WCHAR* process_name, _Inout_ PFLT_CALLBACK_DATA Data);//Чужое

const FLT_OPERATION_REGISTRATION Callbacks[] = {
 {IRP_MJ_CREATE, 0, MiniPreCreate, MiniPostCreate},
 {IRP_MJ_WRITE, 0, MiniPreWrite, NULL},
 {IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {
 sizeof(FLT_REGISTRATION),
 FLT_REGISTRATION_VERSION,
 0,
 NULL,
 Callbacks,
 MiniUnload,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL,
 NULL
};

NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
    DbgPrint("Driver unloaded \n");
    FltUnregisterFilter(FilterHandle);
    Flags = Flags;
    return STATUS_SUCCESS;
}

FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
    Flags = Flags;

    FltObjects = FltObjects;
    CompletionContext = CompletionContext;
    Data = Data;
    DbgPrint("Post create is running \n");
    return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
   // DbgBreakPoint();
    //DbgPrint("Hello from pre create\n");
    FltObjects = FltObjects;
    CompletionContext = CompletionContext;
    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
    WCHAR Name[200] = { 0 };

    

    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);

        if (NT_SUCCESS(status))
        {

            //PEPROCESS process = FltGetRequestorProcess(Data);

            if (FileNameInfo->Name.MaximumLength < 260)
            {
                RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
              //  DbgPrint("Create file: %ws \n", Name);
            }
        }

        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
   /// DbgBreakPoint();
    DbgPrint("Hello from pre write\n");
    FltObjects = FltObjects;
    CompletionContext = CompletionContext;

    PFLT_FILE_NAME_INFORMATION FileNameInfo;
    NTSTATUS status;
    WCHAR Name[200] = { 0 };
    WCHAR formatProcName[256] = { 0 };
    PUNICODE_STRING procName;
    //GetProcName(procName, Data);
    //DbgPrint("Process %ws create file\n", procName);
    status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);
    
    if (NT_SUCCESS(status))
    {
        status = FltParseFileNameInformation(FileNameInfo);

        if (NT_SUCCESS(status))
        {

            if (FileNameInfo->Name.MaximumLength < 260)
            {
                RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
                _wcsupr(Name);//Получили имя файла и перевели в верхний регистр
                PEPROCESS process = FltGetRequestorProcess(Data);//Получаем процесс
               // PsLookupProcessByProcessId(ppid, &process);
                SeLocateProcessImageName(process, &procName);
                RtlCopyMemory(formatProcName, procName->Buffer, procName->MaximumLength);//Переводим в wchar
                DbgPrint("Filter Message: %ws want write to file!!\n", formatProcName);

                if (wcsstr(Name, L"OPENME.TXT") != NULL)
                {
                    DbgPrint("Write file: %ws blocked \n", Name);
                    Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    Data->IoStatus.Information = 0;
                    FltReleaseFileNameInformation(FileNameInfo);
                    return FLT_PREOP_COMPLETE;
                }
               // DbgPrint("Create file: %ws\n", Name);

            }
        }

        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    DbgBreakPoint();
    DbgPrint("Hellow from my filter!!!!!\n");
    RegistryPath = RegistryPath;
    NTSTATUS status;
    DbgBreakPoint();
    status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

    if (NT_SUCCESS(status))
    {

        status = FltStartFiltering(FilterHandle);

        if (!NT_SUCCESS(status))
        {
            FltUnregisterFilter(FilterHandle);
            DbgPrint("Error of start filter\n");
        }
        else  DbgPrint("Success\n");
        DbgBreakPoint();
    }
    else  DbgPrint("Error of register filter\n");

    return status;
}


///Чужое
/*
VOID GetProcName(WCHAR* process_name, _Inout_ PFLT_CALLBACK_DATA Data)
{
    PUNICODE_STRING name = NULL;
    NTSTATUS status;
    // IoThreadToProcess returns a pointer to the process for the specified thread
    status = GetProcessImageName(IoThreadToProcess(Data->Thread), &name);
    swprintf_s(process_name, 256 * sizeof(WCHAR), L"%ws", name->Buffer);
    //DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GetProcName(): %ws \r\n", process_name);
}
*/