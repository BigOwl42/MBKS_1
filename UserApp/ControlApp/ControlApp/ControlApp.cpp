// ControlApp.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.


//Пользовательское приложение для взаимодействия с драйвером

#include <iostream>
#include <Windows.h>
#include <fileapi.h>
using namespace std;
#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)
int closeDevice();
int sendToDevice();
int readFromDevice();
HANDLE devicehandle = NULL;
//Хорошо бы сделать ее не глобальной...

int main()
{
    int command;

    devicehandle = CreateFile(L"\\\\.\\mydevicelinkio", GENERIC_ALL, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM, 0);
    if (devicehandle == INVALID_HANDLE_VALUE) {
        cout << "Error of open device "<<GetLastError();
        cin >> command;
        return 0;
    }
    cout << "Sucess open device. Enter any to test send function" << endl;
    cin >> command;
    if (sendToDevice() != 0) {
        cout << "Error of send to device " << GetLastError();
        cin >> command;
        return 0;
    }
    cout << "Sucess send to device. Enter any to test recive function" << endl;
    cin >> command;
    if (readFromDevice() != 0) {
        cout << "Error of recive from device " << GetLastError();
        cin >> command;
        return 0;
    }
    cout << "Sucess send to device. Enter any to close device" << endl;
    cin >> command;
    closeDevice();
}

int closeDevice() {
    if (devicehandle != INVALID_HANDLE_VALUE) {
        CloseHandle(devicehandle);
    }
    return 1;
}

int sendToDevice() {
    const WCHAR* message = L"send from user app";
    ULONG returnLenght = 0;
    if (devicehandle != INVALID_HANDLE_VALUE && devicehandle != NULL) {
        if (!DeviceIoControl(devicehandle, DEVICE_SEND, (LPVOID)message, (wcslen(message) + 1) * 2, NULL, 0, &returnLenght, 0)) {
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
      
        if (!DeviceIoControl(devicehandle, DEVICE_REC, NULL, 0, message, 1024*sizeof(WCHAR), &returnLenght, 0)) {
            cout << "Device control error" << endl;
            return 1;
        }
        else
            cout << "Success read from device. Message is: " << message << endl;
        return 0;
    }
    return 1;
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
