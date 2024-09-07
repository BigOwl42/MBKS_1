// ControlApp.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//Пользовательское приложение для взаимодействия с драйвером
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <Windows.h>
#include <fileapi.h>
using namespace std;

#define CREATE_NOTIFY CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)
int closeDevice();
int sendToDevice(int command);
int readFromDevice();
void writeRight(char* name, int type, int level);
HANDLE devicehandle = NULL;
//Хорошо бы сделать ее не глобальной...

int main()
{
    int command;
    char name[256];
    int type;
    int level;
    devicehandle = CreateFile(L"\\\\.\\mydevicelinkio", GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
    if (devicehandle == INVALID_HANDLE_VALUE) {
        cout << "Error of open device. Enter eny key to exit"<<GetLastError();
        cin >> command;
        return 0;
    }
    while (true) {
        cout << "Sucess open device. Сhose the command:" << endl
            << "1 - Create notificator for create process" << endl
            << "2 - Create notificator for exit process" << endl
            << "3 - Delete notificator for create process" << endl
            << "4 - Delete notificator for exit process" << endl
            << "5 - Create new right" << endl
            << "0 - Exit";
        cin >> command;
        if (command == 0) {
            cout << "Good bye!" << endl;
            closeDevice();
            return 0;
        }
        if (command == 5) {
            cout << endl << "For what type of ... you want create right" << endl
                << "0 - Object" << endl
                << "1 - Subject" << endl;
            cin >> type;
            cout << endl << "Input the name: " << endl;
            //ввод имени
            cin >> name;
            for (int i=0; i<strlen(name); i++)
                name[i] = toupper(name[i]);
            //запись в переменную в верхнем регистре
            cout << endl << "Input integrity level (from 0 to 7) " << endl;
            cin >> level;
            if (level < 0) level = 0;
            if (level > 7) level = 7;
            writeRight(name, type, level);
        }
        if (sendToDevice(command) != 0) {
            cout << "Error of send to device " << GetLastError();
            wcin >> command;
            return 0;
        }
        cout << "Sucess send to device" << endl;
    }
    closeDevice();
    return 0;
}

int closeDevice() {
    if (devicehandle != INVALID_HANDLE_VALUE) {
        CloseHandle(devicehandle);
    }
    return 1;
}

int sendToDevice(int command) {
    WCHAR message[2];
    _itow_s(command, message, 2, 10);
    ULONG returnLenght = 0;
    if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {
        if (!DeviceIoControl(devicehandle, CREATE_NOTIFY, (LPVOID)message, (wcslen(message) + 1) * 2, NULL, 0, &returnLenght, 0)) {
            cout << "Device control error" << endl;
            return 1;
        }
        else
            cout << "Success write to device"  << endl;
        return 0;
    }
}

int readFromDevice() {
    WCHAR message[1024] = { 0 };
    ULONG returnLenght = 0;
    if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {
      
        if (!DeviceIoControl(devicehandle, DEVICE_REC, NULL, 0, message, 1024 * sizeof(WCHAR), &returnLenght, 0)) {
            cout << "Device control error" << endl;
            return 1;
        }
        else
            cout << "Success read from device. Message is: ";
            printf("%ws\n", message);
        return 0;
    }
    return 1;
}

void writeRight(char* name, int type, int level) {
    char cType[10] = { 0 };
    int status;
    if (type == 0)
        strcpy(cType, "Object\0");
    else
        strcpy(cType, "Subject\0");
    FILE* rightsFile = fopen("C:\\Users\\Owl\\Desktop\\Rights.json", "a");
    fprintf(rightsFile, "{\n");
    fprintf(rightsFile, "\"Type\" : \"%s\" ,\n", cType);
    fprintf(rightsFile, "\"Name\" : \"%s\" ,\n", name);
    fprintf(rightsFile, "\"Level\" : \"%d\"\n", level);
    status = fprintf(rightsFile, "}");
    fclose(rightsFile);
}