#include "log.h"
#include <Windows.h>
#include <cstdio>
#include <cstring>
#include <mutex>

static std::mutex g_logMutex;

void Log(const char* msg) {
    std::lock_guard<std::mutex> lock(g_logMutex);

    // Определяем путь к папке, где лежит наша DLL (один раз)
    static char logPath[MAX_PATH] = { 0 };
    if (logPath[0] == 0) {
        HMODULE hSelf = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&Log,
            &hSelf
        );
        GetModuleFileNameA(hSelf, logPath, MAX_PATH);
        char* lastSlash = strrchr(logPath, '\\');
        if (lastSlash) *(lastSlash + 1) = 0;
        strcat_s(logPath, "jetseat_log.txt");
    }

    FILE* f = nullptr;
    fopen_s(&f, logPath, "a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}