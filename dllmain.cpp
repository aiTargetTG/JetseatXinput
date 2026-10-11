#include <Windows.h>
#include "xinput_proxy.h"
#include "jetseat_bridge.h"
#include "log.h"

BOOL WINAPI DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        Log("[DllMain] PROCESS_ATTACH");
        StartInitThread(hModule);
        break;
    case DLL_PROCESS_DETACH:
        Log("[DllMain] PROCESS_DETACH - skipping JetSeat shutdown");
        // НЕ вызываем ShutdownJetSeat() и StopInitThread().
        // Просто даём процессу умереть. Windows сам выгрузит всё.
        // Это единственный безопасный вариант при работе с COM.
        break;
    }
    return TRUE;
}