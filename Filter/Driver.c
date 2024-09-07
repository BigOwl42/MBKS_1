#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include <wdm.h>
#include <ntddk.h>


PFLT_FILTER FilterHandle = NULL;
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

//for notificator
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
    char result[40] = { 0 };
    char num[2] = { 0 };
    num[1] = '\0';
    int len = 0;
    while (x >= 1) {
        num[0] = (x % 10) + 48;
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
    if (info != NULL)
    {
        strcat(ProcName, "     Process is created. Name: ");
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








//VOID GetProcName(WCHAR* process_name, _Inout_ PFLT_CALLBACK_DATA Data);//Чужое
struct FltObject {
    WCHAR objName[256];
    int objLevel;
};

struct FltSubject {
    WCHAR subjName[256];
    int subjLevel;
};


struct FltObject objOne = { L"OPENME", 4 };   //  it is for test
struct FltSubject subjOne = { L"NOTEPAD", 2 };//  

struct FltObject objects[1000] = { 0 };
struct FltSubject subjects[1000] = { 0 };

int obj_num = 0;
int sub_num = 0;

void printSubObj();
//Функция для парсинга из Json

//Для тестирования работы с массивами
void fillArray() {
    // DbgPrint("DebugMsg1\n");
    int obOrSubFlag = 0;
    OBJECT_ATTRIBUTES objAttr;
    UNICODE_STRING uniName;
    WCHAR wName[256] = { 0 };
    int iLevel;
    char name[256] = { 0 };
    char mean[256] = { 0 };
    char buffer[2048] = { 0 }; 
    //считываем в буфер содержимое файла
    RtlInitUnicodeString(&uniName, L"\\??\\C:\\Users\\Owl\\Desktop\\Rights.json");
    InitializeObjectAttributes(&objAttr, &uniName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL, NULL);
    HANDLE handle;
    NTSTATUS ntstatus;
    IO_STATUS_BLOCK ioStatusBlock;

    LARGE_INTEGER byteOffset;
    //  DbgPrint("DebugMsg2\n");
    ntstatus = ZwCreateFile(&handle,
        GENERIC_READ,
        &objAttr, &ioStatusBlock,
        NULL,
        FILE_ATTRIBUTE_NORMAL,
        0,
        FILE_OPEN,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    DbgPrintEx(0, 0, "opened file\n");
    //  DbgPrint("DebugMsg3\n");
    if (NT_SUCCESS(ntstatus))
    {
        DbgPrintEx(0, 0, "file opened normal\n");
        byteOffset.LowPart = byteOffset.HighPart = 0;
        ntstatus = ZwReadFile(handle, NULL, NULL, NULL, &ioStatusBlock,
            buffer, 2048, &byteOffset, NULL);
        ZwClose(handle);
    }
    else
        DbgPrint("Error of open file: %X\n", ntstatus);
    //разбираем файл на ообъекты
    //DbgPrint("DebugMsg4\n");
    int t = 0;
    int k;
    DbgPrint("DebugMsg4\n");
    while (buffer[t] != '\0') {
        while (buffer[t] != '{' && buffer[t] != '\0') {
            t++;
            DbgPrint("buffer[%d] is %c\n", t, buffer[t]);
        }
        if (buffer[t] == '\0') break;
            //разбиваем объект на строки
        DbgPrint("DebugMsg5\n");
        while (1) {
            while (buffer[t] != '"' && buffer[t] != '}') t++;
            if (buffer[t] == '}') break;
            t++;
            //считать название
            k = 0;
            for (int i = 0; i < 256; i++) name[i] = '\0';
            while (buffer[t] != '"') {
                name[k] = buffer[t];
                k++;
                t++;
            }
            t++;
            DbgPrint("Field is %s\n", name);

            DbgPrint("DebugMsg6\n");
            while (buffer[t] != '"') t++;
            t++;
            //считать значение
            k = 0;
            for (int i = 0; i < 256; i++) mean[i] = '\0';
            while (buffer[t] != '"') {
                mean[k] = buffer[t];
                k++;
                t++;
            }
            DbgPrint("Mean is %s\n", mean);
            t++;
            //знаем поле и значение. Нужно определить, что и куда
            //Если поле - "тип", то присваиваем переменной-флагу занчение 1(объект) или 2 (субъект)
            if (!strcmp(name, "Type\0")) {
                DbgPrint("!!!!We found Type");
                if (!strcmp(mean, "Object\0"))
                    obOrSubFlag = 1;
                else
                    obOrSubFlag = 2;
            }
            //Если поле - "имя", переводим в имя и записываем в переменную WCHAR*
            
            if (!strcmp(name, "Name")) {
                DbgPrint("We found name!\n");
                //перевод
                RtlMultiByteToUnicodeN(
                    wName,               // Destination buffer
                    sizeof(wName) * sizeof(wchar_t), // Size of the destination buffer
                    NULL,                   // Pointer to the number of bytes written
                    mean,                // Source buffer
                    strlen(mean));         // Length of the source string

                //wcscpy(wName, (const WCHAR*)mean);
                wName[strlen(mean)] = L'\0';
                DbgPrint("Name is: %ws\n", wName);
            }

            //Если поле - "уровень", то переводим в число и записываем в перпемнную число
            if (!strcmp(name, "Level"))
                iLevel = mean[0] - '0';
           
        }
        //По итогу считывания объекта заполняем структуру в одном из массивов
        if (obOrSubFlag == 1) {
            for (int i=0; i<wcslen(wName); i++)
                objects[obj_num].objName[i] = wName[i];
            objects[obj_num].objLevel = iLevel;
            obj_num++;
        }
        if (obOrSubFlag == 2) {
            for (int i = 0; i < wcslen(wName); i++)
                subjects[sub_num].subjName[i] = wName[i];
           // wcscmp(subjects[sub_num].subjName, wName);
            subjects[sub_num].subjLevel = iLevel;
            sub_num++;
        }

    }
    

}


    void printSubObj() {
        for (int i = 0; i < obj_num; i++) {
            DbgPrint("Object %d: %ws level %d\n", i, objects[i].objName, objects[i].objLevel);
        }
        for (int i = 0; i < sub_num; i++) {
            DbgPrint("Subject %d: %ws level %d\n", i, subjects[i].subjName, subjects[i].subjLevel);
        }
    }




int levelCheck(WCHAR* procName, WCHAR* fileName) {
    int subjLevel=0;
    int objLevel=0;
    //Перевести в верхний регистр
    _wcsupr(procName);
    _wcsupr(fileName);
    //Пройтись по масссивам, найти совпадения
    for (int i = 0; i < 1000 && wcslen(subjects[i].subjName) != 0; i++)//пу-пу. Узнаем, как он заполняет массив
        if (wcsstr(procName, subjects[i].subjName) != NULL) {
            subjLevel = subjects[i].subjLevel;
            break;
        }
    for (int i = 0; i < 1000 && wcslen(objects[i].objName) != 0; i++) {//пу-пу. Узнаем, как он заполняет массив
        //DbgPrint("ObjName mumber %d is %ws\n", i, objects[i].objName);
        if (wcsstr(fileName, objects[i].objName) != NULL) {
            objLevel = objects[i].objLevel;
            break;
        }
    }
    //Если уровень процесса ниже уровня файла - запретить
    if (objLevel > subjLevel)
        return 0;
    else
        return 1;
}



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
    IoDeleteSymbolicLink(&SymLinkName);
    IoDeleteDevice(device);
    ZwClose(logHandle);
    if (isCreateNotify) {
        PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyCreateProcessNotifyEx, TRUE);
        isCreateNotify = 0;
    }
    if (isDeleteNotify) {
        PsSetCreateProcessNotifyRoutineEx((PCREATE_PROCESS_NOTIFY_ROUTINE_EX)MyDeleteProcessNotifyEx, TRUE);
        isDeleteNotify = 0;
    }
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
//    DbgPrint("Post create is running \n");
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
  //  DbgPrint("Hello from pre write\n");
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
               //DbgPrint("Filter Message: %ws want write to file!!\n", formatProcName);
                _wcsupr(formatProcName);
                if (wcsstr(formatProcName, subjOne.subjName))
                    DbgPrint("Subject Found!!!!\n");
                //Сделать проверку блокнота
                if (levelCheck(formatProcName, Name)==1)
                    DbgPrint("Check pass! %ws %ws\n", Name, formatProcName);
                else {//Ну вроде работает...
                    DbgPrint("Error! Access Denied!\n");
                    Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    Data->IoStatus.Information = 0;
                    FltReleaseFileNameInformation(FileNameInfo);
                    return FLT_PREOP_COMPLETE;
                }


                /*
                if (wcsstr(Name, L"OPENME.TXT") != NULL)
                {
                    DbgPrint("Write file: %ws blocked \n", Name);
                    Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
                    Data->IoStatus.Information = 0;
                    FltReleaseFileNameInformation(FileNameInfo);
                    return FLT_PREOP_COMPLETE;
                }
               */
               // DbgPrint("Create file: %ws\n", Name);

            }
        }

        FltReleaseFileNameInformation(FileNameInfo);
    }

    return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    //DbgBreakPoint();
    DbgPrint("Hellow from my filter!!!!!\n");
    RegistryPath = RegistryPath;
    NTSTATUS status;
    //DbgBreakPoint();

    fillArray();
    printSubObj();
    DbgPrint("Len of first elem: %d\n", wcslen(objects[1].objName));


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
       
        //DbgBreakPoint();
    }
    else  DbgPrint("Error of register filter\n");

    if (createDevice(DriverObject)) {//Создаём девайс для взаимодействия с польтзовательским приложением
        DbgPrint("Error of create device\n. Driver will be unload");
        //unload driver
    }
    notifyCreateProcess();//Сохздаём файл для логирования
    return status;
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
        if (!wcscmp(buffer, L"5")) {//команда 4 - снятие нотификатора на закрытие процессов
            DbgPrint("It is 5\n");
            fillArray();
            printSubObj();
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
        FILE_SHARE_WRITE || FILE_SHARE_READ,
        FILE_OPEN_IF,
        FILE_SYNCHRONOUS_IO_NONALERT,
        NULL, 0);
    DbgPrint("Result of open file is %X\n", createFileStatus);
    return createFileStatus;
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