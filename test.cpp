#include <windows.h>
#include <iostream>
#include <tlhelp32.h>
#include <string>
#include <locale>
#include <codecvt>

// Функция для получения базового адреса модуля
DWORD GetModuleBaseAddress(DWORD processId, const wchar_t* moduleName) {
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, processId);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }

    MODULEENTRY32W moduleEntry;
    moduleEntry.dwSize = sizeof(MODULEENTRY32W);

    if (Module32FirstW(hSnapshot, &moduleEntry)) {
        do {
            if (wcscmp(moduleEntry.szModule, moduleName) == 0) {
                CloseHandle(hSnapshot);
                return (DWORD)moduleEntry.modBaseAddr;
            }
        } while (Module32NextW(hSnapshot, &moduleEntry));
    }

    CloseHandle(hSnapshot);
    return 0;
}

// Функция для конвертации wstring в string
std::string wstringToString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

int main() {
    // Ищем процесс 
    DWORD processId = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // Тут мы типа просто все процессы получили активные в данный момент
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe)) { // Тут мы просто перебираем все активные процессы
        do {
            std::wstring exeName = pe.szExeFile;
            if (exeName == L"hl.exe") {
                processId = pe.th32ProcessID;
                std::cout << "Найден процесс hl.exe с ID: " << processId << std::endl;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    CloseHandle(hSnapshot);

    if (processId == 0) {
        std::cout << "Процесс hl.exe не найден" << std::endl;
        std::cin.get();
        return 1;
    }

    // Открываем процесс
    HANDLE process = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
    if (process == NULL) {
        std::cout << "Ошибка открытия процесса. Возможно запустился не от админа" << std::endl;
        std::cout << "Код ошибки: " << GetLastError() << std::endl;
        std::cin.get();
        return 1;
    }

    // Получаем базовый адрес client.dll
    DWORD clientBase = GetModuleBaseAddress(processId, L"client.dll"); // имя client.dll также через чит энджин найдено, адрес всегда меняется, поэтому ищем
    if (clientBase == 0) {
        std::cout << "Не удалось найти client.dll" << std::endl;
        CloseHandle(process);
        std::cin.get();
        return 1;
    }

    std::cout << "Базовый адрес client.dll: 0x" << std::hex << clientBase << std::dec << std::endl;

    // Вычисляем адрес здоровья
    DWORD healthOffset = 0x7338C; // Это из чит энджина смещение, оно постоянное 
    //client.dll+B3AE4 client.dll+B4338 - предполагаемые ячейки с которых можно читать координаты по x игрока, остальные координаты где-то рядом
    DWORD healthAddress = clientBase + healthOffset;

    std::cout << "Адрес здоровья: 0x" << std::hex << healthAddress << std::dec << std::endl;
    std::cout << "Отслеживание здоровья..." << std::endl;

    // Читаем здоровье в цикле
    while (true) {
        int health = 0;
        if (ReadProcessMemory(process, (LPCVOID)healthAddress, &health, sizeof(health), NULL)) {
            std::cout << "Здоровье: " << health << "\r";
            std::cout.flush();
        } else {
            std::cout << "Ошибка чтения памяти!" << std::endl;
            break;
        }

        Sleep(100);
    }

    CloseHandle(process);
    return 0;
}