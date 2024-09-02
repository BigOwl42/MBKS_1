#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <wchar.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")
#pragma warning(disable:4055)
#pragma warning(disable:4616)
//---------------------------------------------------------------------------
//      Global variables
//---------------------------------------------------------------------------

#define NULL_FILTER_FILTER_NAME     L"NullFilter"

#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

#define MAX_NAME_LENGTH 256
#define MAX_PROCESSES 8
#define MAX_FILES 8
#define BUFFER_SIZE 1024

#define ACCESS_ALLOWED 88
#define ACCESS_DENIED 44

#define WCONFIG_FILE_OBJECT L"\\DosDevices\\C:\\DriverConfig\\Config.txt"

typedef NTSTATUS(*QUERY_INFO_PROCESS) (
	__in HANDLE ProcessHandle,
	__in PROCESSINFOCLASS ProcessInformationClass,
	__out_bcount(ProcessInformationLength) PVOID ProcessInformation,
	__in ULONG ProcessInformationLength,
	__out_opt PULONG ReturnLength
	);

typedef struct ProcessPermision
{
	CHAR procname[MAX_NAME_LENGTH];
	CHAR read_permission;
	CHAR write_permission;
}ProcPerm;

typedef struct DatabaseElem
{
	CHAR filename[MAX_NAME_LENGTH];
	ProcPerm proc_list[MAX_PROCESSES];
}DbEl;

DbEl gDatabase[MAX_FILES];
DbEl gDatabase_for_change[MAX_FILES];
int gFilesAmount = 0; // Actual
int gFilesAmount_change = 0;
int CountFiles = 0;
int CountFiles_change = 0;
QUERY_INFO_PROCESS ZwQueryInformationProcess;

ULONG gTraceFlags = 0;

typedef struct _NULL_FILTER_DATA {

	//
	//  The filter handle that results from a call to
	//  FltRegisterFilter.
	//

	PFLT_FILTER FilterHandle;				//unique identifier for mini-filter

} NULL_FILTER_DATA, * PNULL_FILTER_DATA;


/*************************************************************************
	Prototypes for the startup and unload routines used for
	this Filter.

	Implementation in nullFilter.c
*************************************************************************/

DRIVER_INITIALIZE DriverEntry;
NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
);

NTSTATUS
NullUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
NullQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

FLT_PREOP_CALLBACK_STATUS
FsFilterXPreOperationRead(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

FLT_PREOP_CALLBACK_STATUS
FsFilterXPreOperationWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
);

VOID
FsFilterXOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
);

FLT_POSTOP_CALLBACK_STATUS
FsFilterXPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
);

BOOLEAN
FsFilterXDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
);

//
//  Structure that contains all the global data structures
//  used throughout NullFilter.
//

FLT_OPERATION_REGISTRATION Callbacks[] = {
	{ IRP_MJ_READ,
	0,
	FsFilterXPreOperationRead,
	FsFilterXPostOperation },

	{ IRP_MJ_WRITE,
	0,
	FsFilterXPreOperationWrite,
	FsFilterXPostOperation },

	/*{ IRP_MJ_CREATE,
	0,
	FsFilterXPreOperationWrite,
	FsFilterXPostOperation },*/

	{ IRP_MJ_OPERATION_END }
};

NULL_FILTER_DATA NullFilterData;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, NullUnload)
#pragma alloc_text(PAGE, NullQueryTeardown)
#pragma alloc_text(PAGE, FsFilterXPreOperationRead)
#pragma alloc_text(PAGE, FsFilterXPreOperationWrite)
#pragma alloc_text(PAGE, FsFilterXOperationStatusCallback)
#pragma alloc_text(PAGE, FsFilterXPostOperation)
#pragma alloc_text(PAGE, FsFilterXDoRequestOperationStatus)
#endif

CONST FLT_REGISTRATION FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	NULL,                               //  Context
	Callbacks,                               //  Operation callbacks

	NullUnload,                         //  FilterUnload

	NULL,                               //  InstanceSetup
	NullQueryTeardown,                  //  InstanceQueryTeardown
	NULL,                               //  InstanceTeardownStart
	NULL,                               //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL,                                //  NormalizeNameComponent
	NULL								// KTMNotificationCallback
};

ULONG_PTR OperationStatusCtx = 1;

/*************************************************************************
	Filter initialization and unload routines.
*************************************************************************/

int CheckPermissionWrite(WCHAR* file_path, WCHAR* process_name)
{
	WCHAR name_tmp[MAX_NAME_LENGTH] = { 0 };
	for (int i = 0; i < gFilesAmount; i++)
	{
		swprintf_s(name_tmp, MAX_NAME_LENGTH, L"%hs", gDatabase[i].filename);

		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d: Tmp str1: %ws \r\n", i, name_tmp);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "file_path str2: %ws \r\n", file_path);

		if (wcscmp(name_tmp, file_path) == 0)
		{
			for (int j = 0; j < MAX_PROCESSES; j++)
			{
				swprintf_s(name_tmp, MAX_NAME_LENGTH, L"%hs", gDatabase[i].proc_list[j].procname);
				if (wcscmp(name_tmp, process_name) == 0)
				{
					//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d, %d: MARK perm_write: %c \r\n", i, j, gDatabase[i].proc_list[j].write_permission);
					if (gDatabase[i].proc_list[j].write_permission == '1') {
						return ACCESS_ALLOWED;
					}
					else {
						//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d: Tmp str1: %ws \r\n", i, name_tmp);
						//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "file_path str2: %ws \r\n", file_path);
						DbgPrintEx(0, 0, "ACCESS_DENIED write: %ws \r\n", file_path);
						return ACCESS_DENIED;
					}
				}
			}
		}
	}
	return ACCESS_ALLOWED;
}

int CheckPermissionRead(WCHAR* file_path, WCHAR* process_name)
{
	WCHAR name_tmp[MAX_NAME_LENGTH] = { 0 };
	for (int i = 0; i < gFilesAmount; i++)
	{
		swprintf_s(name_tmp, MAX_NAME_LENGTH, L"%hs", gDatabase[i].filename);

		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d: Tmp str1: %ws \r\n", i, name_tmp);
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "file_path str2: %ws \r\n", file_path);

		if (wcscmp(name_tmp, file_path) == 0)
		{
			for (int j = 0; j < MAX_PROCESSES; j++)
			{
				swprintf_s(name_tmp, MAX_NAME_LENGTH, L"%hs", gDatabase[i].proc_list[j].procname);
				if (wcscmp(name_tmp, process_name) == 0)
				{
					//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d, %d: MARK perm_read: %c \r\n", i, j, gDatabase[i].proc_list[j].read_permission);
					if (gDatabase[i].proc_list[j].read_permission == '1') {
						return ACCESS_ALLOWED;
					}
					else {
						DbgPrintEx(0, 0, "ACCESS_DENIED read: %ws \r\n", file_path);
						return ACCESS_DENIED;
					}
				}
			}
		}
	}
	return ACCESS_ALLOWED;
}

VOID GetFilePath(WCHAR* file_name, WCHAR* file_path)
{
	int users_idx = 0; // \Users
	for (int i = 0; i < MAX_NAME_LENGTH; i++)
	{
		if (file_name[i] == L'V' && file_name[i + 1] == L'o' && file_name[i + 2] == L'l' &&
			file_name[i + 3] == L'u' && file_name[i + 4] == L'm' && file_name[i + 5] == L'e'
			&& file_name[i + 6] == L'3')
		{
			users_idx = i + 7;
			break;
		}
	}
	file_path[0] = L'C';
	file_path[1] = L':';
	for (int i = 0; i < MAX_NAME_LENGTH - users_idx; i++)
	{
		file_path[i + 2] = file_name[i + users_idx];
		if (file_name[i + users_idx] == L'\0')
			break;
	}
}

VOID GetActualProcName(WCHAR* name)
{
	int zero_idx = 0, name_idx = 0;
	for (int i = MAX_NAME_LENGTH - 1; i > 0; i--)
	{
		if (name[i] == L'\0' && name[i - 1] != L'\0')
			zero_idx = i;
		if (name[i] == L'\\')
		{
			name_idx = i + 1;
			break;
		}
	}
	for (int i = 0; i <= zero_idx - name_idx + 1; i++)
	{
		name[i] = name[i + name_idx];
	}
}

VOID ParseBuffer(CHAR* buffer)
{
	int count_files = 0;
	int i = 0, j = 0, db_index = 0, proc_idx = 0;
	while (buffer[i] != '\0')
	{
		if (i == 0 || (i > 3 && buffer[i - 3] == '\n' && buffer[i - 2] == '\r' && buffer[i - 1] == '\n')) // Path to file
		{
			j = i;
			while (buffer[j] != '\r') j++;
			memcpy(gDatabase[db_index].filename, &buffer[i], j - i);
			DbgPrint("%d: file: %s\n", db_index, gDatabase[db_index].filename);
			i = j + 2; // Start of the next line | 2 = \r, \n
			count_files++;
		}
		else if (buffer[i] == '\r' && buffer[i - 1] == '\n' && buffer[i + 1] == '\n')
		{
			db_index++;
			proc_idx = 0;
			i += 2; // \r \n(here) \r \n
		}
		else if (buffer[i - 1] == '\n')
		{
			j = i;
			while (buffer[j] != '|') j++;
			memcpy(gDatabase[db_index].proc_list[proc_idx].procname, &buffer[i], j - i);
			DbgPrint("proc: %s  ", gDatabase[db_index].proc_list[proc_idx].procname);
			gDatabase[db_index].proc_list[proc_idx].read_permission = buffer[j + 1];
			gDatabase[db_index].proc_list[proc_idx].write_permission = buffer[j + 3];
			DbgPrint("perms: %c %c\n", gDatabase[db_index].proc_list[proc_idx].read_permission, gDatabase[db_index].proc_list[proc_idx].write_permission = buffer[j + 3]);
			proc_idx++;
			// j + 5 == '\n'; j + 4 == '\r'; j + 3 == write_permision
			i = j + 6;
		}
	}
	gFilesAmount = db_index + 1;
	if (count_files != 0) {
		CountFiles = count_files;
	}

	if (CountFiles_change < CountFiles && CountFiles_change != 0 && CountFiles != 0 && (CountFiles - CountFiles_change)>0) {
		int count = CountFiles - CountFiles_change;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d files was added \r\n", count);
	}
	if (CountFiles_change > CountFiles && CountFiles_change != 0 && CountFiles != 0 && (CountFiles_change - CountFiles) > 0) {
		int count = CountFiles_change - CountFiles;
		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%d files was deleted \r\n", count);
	}
	CountFiles_change = CountFiles;
}

VOID FillDatabase()
{
	HANDLE handle;
	NTSTATUS ntstatus;
	IO_STATUS_BLOCK ioStatusBlock;
	LARGE_INTEGER      byteOffset;
	UNICODE_STRING     uniName;
	OBJECT_ATTRIBUTES  objAttr;

	CHAR buffer[BUFFER_SIZE] = { 0 };

	RtlInitUnicodeString(&uniName, WCONFIG_FILE_OBJECT);  // or L"\\SystemRoot\\example.txt"
	InitializeObjectAttributes(&objAttr, &uniName,
		OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
		NULL, NULL);

	ntstatus = ZwCreateFile(&handle,
		GENERIC_READ,
		&objAttr, &ioStatusBlock,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_OPEN,
		FILE_SYNCHRONOUS_IO_NONALERT,
		NULL, 0);

	if (NT_SUCCESS(ntstatus))
	{
		byteOffset.LowPart = byteOffset.HighPart = 0;
		ntstatus = ZwReadFile(handle, NULL, NULL, NULL, &ioStatusBlock,
			(PVOID)buffer, BUFFER_SIZE, &byteOffset, NULL);
		if (NT_SUCCESS(ntstatus)) {
			buffer[BUFFER_SIZE - 1] = '\0';
			//DbgPrint("Buffer infos: %s\n", buffer);
		}
		ZwClose(handle);
	}

	ParseBuffer(buffer);
}

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
/*++
Arguments:

	DriverObject - Pointer to driver object created by the system to
		represent this driver.
	RegistryPath - Unicode string identifying where the parameters for this
		driver are located in the registry.
--*/
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(RegistryPath);
	//  Register with FltMgr
	status = FltRegisterFilter(DriverObject, &FilterRegistration, &NullFilterData.FilterHandle);
	DbgPrintEx(0, 0, "Filter start\n");
	FLT_ASSERT(NT_SUCCESS(status));
	if (NT_SUCCESS(status))
	{
		memset(gDatabase, 0x00, sizeof(DbEl) * MAX_FILES); // Dbase that contains info from the config file
		FillDatabase();
		//DbgPrint("Yey, working?!!\n");
		//
		//  Start filtering i/o
		//
		status = FltStartFiltering(NullFilterData.FilterHandle);
		if (!NT_SUCCESS(status))
		{
			FltUnregisterFilter(NullFilterData.FilterHandle);
		}
	}
	return status;
}

NTSTATUS GetProcessImageName(PEPROCESS eProcess, PUNICODE_STRING* ProcessImageName)
{
	NTSTATUS status = STATUS_UNSUCCESSFUL;
	ULONG returnedLength;
	HANDLE hProcess = NULL; // Handle to needed process

	PAGED_CODE(); // this eliminates the possibility of the IDLE Thread/Process 

	if (eProcess == NULL)
		return STATUS_INVALID_PARAMETER_1;

	// function opens an object referenced by a pointer and returns a handle to the object
	status = ObOpenObjectByPointer(eProcess, 0, NULL, 0, 0, KernelMode, &hProcess);
	if (!NT_SUCCESS(status))
	{
		DbgPrint("ObOpenObjectByPointer Failed: %08x\n", status);
		return status;
	}

	if (ZwQueryInformationProcess == NULL)
	{
		UNICODE_STRING routineName;
		RtlInitUnicodeString(&routineName, L"ZwQueryInformationProcess");
		// routine returns a pointer to a function specified by routineName
		ZwQueryInformationProcess = (QUERY_INFO_PROCESS)MmGetSystemRoutineAddress(&routineName);
		if (ZwQueryInformationProcess == NULL)
		{
			DbgPrint("Cannot resolve ZwQueryInformationProcess\n");
			status = STATUS_UNSUCCESSFUL;
			goto cleanUp;
		}
	}

	// Step 1 - get the needed size
	status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		NULL, // buffer 
		0, // buffer size 
		&returnedLength);

	if (STATUS_INFO_LENGTH_MISMATCH != status) {
		DbgPrint("ZwQueryInformationProcess status = %x\n", status);
		goto cleanUp;
	}

	*ProcessImageName = ExAllocatePoolWithTag(NonPagedPoolNx, returnedLength, '2gat');

	if (ProcessImageName == NULL)
	{
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto cleanUp;
	}

	/* Retrieve the process path from the handle to the process */
	status = ZwQueryInformationProcess(hProcess,
		ProcessImageFileName,
		*ProcessImageName,
		returnedLength,
		&returnedLength);

	if (!NT_SUCCESS(status))
		ExFreePoolWithTag(*ProcessImageName, '2gat');
cleanUp:
	ZwClose(hProcess);
	return status;
}

VOID GetProcName(WCHAR* process_name, _Inout_ PFLT_CALLBACK_DATA Data)
{
	PUNICODE_STRING name = NULL;
	NTSTATUS status;
	// IoThreadToProcess returns a pointer to the process for the specified thread
	status = GetProcessImageName(IoThreadToProcess(Data->Thread), &name);
	swprintf_s(process_name, MAX_NAME_LENGTH * sizeof(WCHAR), L"%ws", name->Buffer);
	//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "GetProcName(): %ws \r\n", process_name);
}

NTSTATUS
NullUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
/*++

Routine Description:

	This is the unload routine for this miniFilter driver. This is called
	when the minifilter is about to be unloaded. We can fail this unload
	request if this is not a mandatory unloaded indicated by the Flags
	parameter.

Arguments:

	Flags - Indicating if this is a mandatory unload.

Return Value:

	Returns the final status of this operation.

--*/
{
	DbgPrint("Entered Unload!");
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	FltUnregisterFilter(NullFilterData.FilterHandle);

	return STATUS_SUCCESS;
}

NTSTATUS
NullQueryTeardown(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
)
/*++

Routine Description:

	This is the instance detach routine for this miniFilter driver.
	This is called when an instance is being manually deleted by a
	call to FltDetachVolume or FilterDetach thereby giving us a
	chance to fail that detach request.

Arguments:

	FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
		opaque handles to this filter, instance and its associated volume.

	Flags - Indicating where this detach request came from.

Return Value:

	Returns the status of this operation.

--*/
{
	DbgPrint("Entered Query teardown");
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	return STATUS_SUCCESS;
}

FLT_PREOP_CALLBACK_STATUS
FsFilterXPreOperationRead(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
/*++

This is non-pageable because it could be called on the paging path
Arguments:
Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The context for the completion routine for this
operation.
--*/
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	//DbgPrint("FsFilterX!ssOperation: READ Entered\n");

	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION FileNameInfo;

	WCHAR file_name[MAX_NAME_LENGTH] = { 0 };
	WCHAR file_path[MAX_NAME_LENGTH] = { 0 };
	WCHAR process_name[MAX_NAME_LENGTH] = { 0 };
	// Retrives the info into PFLT_FILE_NAME_INFORMATION structure
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

	if (NT_SUCCESS(status))
	{
		status = FltParseFileNameInformation(FileNameInfo); // Parses the contents of a PFLT_FILE_NAME_INFORMATION structure
		if (NT_SUCCESS(status))
		{
			if (FileNameInfo->Name.MaximumLength < MAX_NAME_LENGTH)
			{
				RtlCopyMemory(file_name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength); // <=> memcpy
				// Example: \Device\HarddiskVolume3\Users\Virtual\Desktop\Commands.txt
				GetProcName(process_name, Data);
				GetActualProcName(process_name); // Example: Notepad.exe
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Actual Proc Name: %ws \r\n", process_name);
				GetFilePath(file_name, file_path);
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Actual Path: %ws \r\n", file_path);
				///DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Path with Volume: %ws \r\n", file_name);
				if (CheckPermissionRead(file_path, process_name) == ACCESS_DENIED)
				{
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					Data->IoStatus.Information = 0;
					FltReleaseFileNameInformation(FileNameInfo);
					return FLT_PREOP_COMPLETE;
				}
			}
		}
		FltReleaseFileNameInformation(FileNameInfo);
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

int cmp(char* first, char* second)
{
	while (*first && *second)
	{
		if (*first != *second) return 0;
		++first;
		++second;
	}
	return 1;
}

FLT_PREOP_CALLBACK_STATUS
FsFilterXPreOperationWrite(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_Flt_CompletionContext_Outptr_ PVOID* CompletionContext
)
{
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	//DbgPrint("FsFilterX!FsFilterXPreOperation: READ Entered\n");

	NTSTATUS status;
	PFLT_FILE_NAME_INFORMATION FileNameInfo;

	WCHAR file_name[MAX_NAME_LENGTH] = { 0 };
	WCHAR file_path[MAX_NAME_LENGTH] = { 0 };
	WCHAR process_name[MAX_NAME_LENGTH] = { 0 };


	// Retrives the info into PFLT_FILE_NAME_INFORMATION structure
	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

	if (NT_SUCCESS(status))
	{
		status = FltParseFileNameInformation(FileNameInfo); // Parses the contents of a PFLT_FILE_NAME_INFORMATION structure
		if (NT_SUCCESS(status))
		{
			if (FileNameInfo->Name.MaximumLength < MAX_NAME_LENGTH)
			{
				RtlCopyMemory(file_name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength); // <=> memcpy
																									   // Example: \Device\HarddiskVolume3\Users\Virtual\Desktop\Commands.txt
				GetProcName(process_name, Data);
				GetActualProcName(process_name); // Example: Notepad.exe
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Actual Proc Name: %ws \r\n", process_name);
				GetFilePath(file_name, file_path);
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Actual Path: %ws \r\n", file_path);
				//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "Path with Volume: %ws \r\n", file_name);

				/*(process_name[0] == 'n') && (process_name[1] == 'o') && (process_name[2] == 't') && (process_name[3] == 'e') && (process_name[4] == 'p') && (process_name[5] == 'a') && (process_name[6] == 'd')
					&& (file_name[24] == 'D') && (file_name[25] == 'r')
					&& (file_name[26] == 'i') && (file_name[27] == 'v') && (file_name[28] == 'e')
					&& (file_name[29] == 'r') && (file_name[30] == 'C') && (file_name[31] == 'o') && (file_name[32] == 'n') && (file_name[33] == 'f') && (file_name[34] == 'i')
					&& (file_name[35] == 'g') && (file_name[36] == '\\') && (file_name[37] == 'C') && (file_name[38] == 'o') && (file_name[39] == 'n')
					&& (file_name[40] == 'f') && (file_name[41] == 'i') && (file_name[42] == 'g') && (file_name[43] == '.') && (file_name[44] == 't') && (file_name[45] == 'x') && (file_name[46] == 't')*/
					//&& cmp(file_path, "\\Device\\HarddiskVolume4\\DriverConfig\\Config.txt")
				memset(gDatabase, 0x00, sizeof(DbEl) * MAX_FILES);
				FillDatabase();
				if (cmp("notepad.exe", process_name))

				{
					DbgPrintEx(0, 0, "Config was edit\n");



				}

				if (CheckPermissionWrite(file_path, process_name) == ACCESS_DENIED)
				{
					Data->IoStatus.Status = STATUS_ACCESS_DENIED;
					Data->IoStatus.Information = 0;
					FltReleaseFileNameInformation(FileNameInfo);
					return FLT_PREOP_COMPLETE;
				}

			}
		}
		FltReleaseFileNameInformation(FileNameInfo);
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}



FLT_POSTOP_CALLBACK_STATUS
FsFilterXPostOperation(
	_Inout_ PFLT_CALLBACK_DATA Data,
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_opt_ PVOID CompletionContext,
	_In_ FLT_POST_OPERATION_FLAGS Flags
)
/*++

Routine Description:

This routine is the post-operation completion routine for this
miniFilter.

This is non-pageable because it may be called at DPC level.

Arguments:

Data - Pointer to the filter callbackData that is passed to us.

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

CompletionContext - The completion context set in the pre-operation routine.

Flags - Denotes whether the completion is successful or is being drained.

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(Data);
	UNREFERENCED_PARAMETER(FltObjects);
	UNREFERENCED_PARAMETER(CompletionContext);
	UNREFERENCED_PARAMETER(Flags);







	return FLT_POSTOP_FINISHED_PROCESSING;
}

VOID
FsFilterXOperationStatusCallback(
	_In_ PCFLT_RELATED_OBJECTS FltObjects,
	_In_ PFLT_IO_PARAMETER_BLOCK ParameterSnapshot,
	_In_ NTSTATUS OperationStatus,
	_In_ PVOID RequesterContext
)
/*++

Routine Description:

This routine is called when the given operation returns from the call
to IoCallDriver.  This is useful for operations where STATUS_PENDING
means the operation was successfully queued.  This is useful for OpLocks
and directory change notification operations.

This callback is called in the context of the originating thread and will
never be called at DPC level.  The file object has been correctly
referenced so that you can access it.  It will be automatically
dereferenced upon return.

This is non-pageable because it could be called on the paging path

Arguments:

FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
opaque handles to this filter, instance, its associated volume and
file object.

RequesterContext - The context for the completion routine for this
operation.

OperationStatus -

Return Value:

The return value is the status of the operation.

--*/
{
	UNREFERENCED_PARAMETER(FltObjects);

	DbgPrint("FsFilterX!FsFilterXOperationStatusCallback: Status=%08x ctx=%p IrpMj=%02x.%02x \"%s\"\n",
		OperationStatus,
		RequesterContext,
		ParameterSnapshot->MajorFunction,
		ParameterSnapshot->MinorFunction,
		FltGetIrpName(ParameterSnapshot->MajorFunction));
}

BOOLEAN
FsFilterXDoRequestOperationStatus(
	_In_ PFLT_CALLBACK_DATA Data
)
/*++

Routine Description:

This identifies those operations we want the operation status for.  These
are typically operations that return STATUS_PENDING as a normal completion
status.

Arguments:

Return Value:

TRUE - If we want the operation status
FALSE - If we don't

--*/
{
	PFLT_IO_PARAMETER_BLOCK iopb = Data->Iopb;

	//
	//  return boolean state based on which operations we are interested in
	//

	return (BOOLEAN)

		//
		//  Check for oplock operations
		//

		(((iopb->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
			((iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_FILTER_OPLOCK) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_BATCH_OPLOCK) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_1) ||
				(iopb->Parameters.FileSystemControl.Common.FsControlCode == FSCTL_REQUEST_OPLOCK_LEVEL_2)))

			||

			//
			//    Check for directy change notification
			//

			((iopb->MajorFunction == IRP_MJ_DIRECTORY_CONTROL) &&
				(iopb->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY))
			);
}