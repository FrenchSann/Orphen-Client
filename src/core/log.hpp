#pragma once
#include <cstdio>
#include <cstdarg>
#include <Windows.h>

// Log dans %TEMP%\orphen.log + OutputDebugString (visible avec DebugView).
namespace logx {
    inline void write(const char* fmt, ...) {
        char buf[1024];
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf), fmt, ap);
        va_end(ap);

        OutputDebugStringA("[Orphen] ");
        OutputDebugStringA(buf);
        OutputDebugStringA("\n");

        static FILE* f = [] {
            char path[MAX_PATH];
            GetTempPathA(MAX_PATH, path);
            strcat_s(path, "orphen.log");
            return fopen(path, "w");
        }();
        if (f) {
            fprintf(f, "%s\n", buf);
            fflush(f);
        }
    }
}
#define LOG(...) logx::write(__VA_ARGS__)
