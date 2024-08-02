// dllmain.cpp : Определяет точку входа для приложения DLL.
#include <windows.h>
#include <string>
#include <curl/curl.h>
#include <MinHook.h>
#include "ntapi.h"

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Crypt32.lib")
#pragma comment(lib, "Cryptnet.lib")
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Wldap32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma warning(disable : 4996)


typedef CURLcode(*curl_easy_setopt_t)(CURL* handle, CURLoption option, void* args);
curl_easy_setopt_t original_curl_easy_setopt = NULL;

CURLcode __stdcall curl_easy_setopt_hook(CURL* handle, CURLoption option, void* args) {

    if (option == CURLOPT_URL) {

        const char* url = (const char*)args; 
        const char* found = strstr(url, "/anticheat");//Получаем URL который содержит в пути anticheat

        if (found) {
            args = (void*)"none"; // заменяем url на none -> приводит к тому что EAC на сервере не может получить состояние из API EOS (Да да, разработчики передают состояние через API)
        }

    }

    CURLcode result = original_curl_easy_setopt(handle, option, args);
    return result;
}

void CALLBACK LdrDllNotification(ULONG Reason, PCLDR_DLL_NOTIFICATION_DATA NotificationData, PVOID Context) {
    if (Reason == 1)
    {
        if (wcscmp(NotificationData->Loaded.BaseDllName->Buffer, L"EOSSDK-Win64-Shipping.dll") == 0)
        {
            MH_Initialize();
            uintptr_t eosModule = (uintptr_t)GetModuleHandleA("EOSSDK-Win64-Shipping.dll");

            original_curl_easy_setopt = (curl_easy_setopt_t)(eosModule + 0x0); //Заменить 0x0 на действующее смещение на функцию curl_easy_setopt в библиотеке EOSSDK-Win64-Shipping.dll
            MH_CreateHook(original_curl_easy_setopt, curl_easy_setopt_hook, (LPVOID*)&original_curl_easy_setopt);
            MH_EnableHook(0);
        }
    }
}

void Main() {
    PVOID DllNotificationCookie;
    HMODULE ntdll = GetModuleHandleA("ntdll.dll");

    LdrRegisterDllNotification_t LdrRegisterDllNotification = (LdrRegisterDllNotification_t)GetProcAddress(ntdll, "LdrRegisterDllNotification");
    LdrRegisterDllNotification(0, &LdrDllNotification, 0, &DllNotificationCookie);
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        Main();
    }
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
        MH_DisableHook(0);
    }
    return TRUE;
}

