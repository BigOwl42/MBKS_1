

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>


PFLT_FILTER FilterHandle = NULL;
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

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
 KdPrint(("Driver unloaded \r\n"));
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
 KdPrint(("Post create is running \r\n"));
 return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    FltObjects = FltObjects;
    CompletionContext = CompletionContext;
 PFLT_FILE_NAME_INFORMATION FileNameInfo;
 NTSTATUS status;
 WCHAR Name[200] = { 0 };

 status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

 if (NT_SUCCESS(status))
 {
  status = FltParseFileNameInformation(FileNameInfo);

  if (NT_SUCCESS(status))
  {

   if (FileNameInfo->Name.MaximumLength < 260)
   {
    RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
    KdPrint(("Create file: %ws \r\n", Name));
   }
  }

  FltReleaseFileNameInformation(FileNameInfo);
 }

 return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
    FltObjects = FltObjects;
    CompletionContext = CompletionContext;

 PFLT_FILE_NAME_INFORMATION FileNameInfo;
 NTSTATUS status;
 WCHAR Name[200] = { 0 };

 status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

 if (NT_SUCCESS(status))
 {
  status = FltParseFileNameInformation(FileNameInfo);

  if (NT_SUCCESS(status))
  {

   if (FileNameInfo->Name.MaximumLength < 260)
   {
    RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
    _wcsupr(Name);
    if (wcsstr(Name, L"OPENME.TXT") != NULL)
    {
     KdPrint(("Write file: %ws blocked \r\n", Name));
     Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
     Data->IoStatus.Information = 0;
     FltReleaseFileNameInformation(FileNameInfo);
     return FLT_PREOP_COMPLETE;
    }
    KdPrint(("Create file: %ws \r\n", Name));

   }
  }

  FltReleaseFileNameInformation(FileNameInfo);
 }

 return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    RegistryPath = RegistryPath;
 NTSTATUS status;

 status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

 if (NT_SUCCESS(status))
 {

  status = FltStartFiltering(FilterHandle);

  if (!NT_SUCCESS(status))
  {
   FltUnregisterFilter(FilterHandle);
  }
 }

 return status;
}