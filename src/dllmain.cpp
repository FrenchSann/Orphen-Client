#include <Windows.h>
#include <jni.h>
#include "core/hooks.hpp"
#include "core/log.hpp"

namespace {
    HMODULE g_module = nullptr;

    using GetCreatedJavaVMsFn = jint(JNICALL*)(JavaVM**, jsize, jsize*);

    JavaVM* findJvm() {
        HMODULE jvm = nullptr;
        for (int i = 0; i < 100 && !(jvm = GetModuleHandleW(L"jvm.dll")); ++i) Sleep(100);
        if (!jvm) return nullptr;
        auto getVms = reinterpret_cast<GetCreatedJavaVMsFn>(GetProcAddress(jvm, "JNI_GetCreatedJavaVMs"));
        if (!getVms) return nullptr;
        JavaVM* vm = nullptr;
        jsize count = 0;
        if (getVms(&vm, 1, &count) != JNI_OK || count == 0) return nullptr;
        return vm;
    }

    DWORD WINAPI mainThread(LPVOID) {
        LOG("injecté");
        JavaVM* vm = findJvm();
        if (!vm) {
            LOG("JVM introuvable : est-ce bien javaw.exe de Minecraft ?");
            FreeLibraryAndExitThread(g_module, 0);
        }
        for (int i = 0; i < 100 && !GetModuleHandleW(L"opengl32.dll"); ++i) Sleep(100);

        HANDLE unloadDone = CreateEventW(nullptr, TRUE, FALSE, nullptr);
        if (!hooks::install(vm, unloadDone)) {
            hooks::remove();
            CloseHandle(unloadDone);
            FreeLibraryAndExitThread(g_module, 0);
        }

        // Le nettoyage (ImGui, WndProc, global refs) se fait sur le thread du jeu dans le hook,
        // ici on retire juste les hooks puis on libère la DLL.
        WaitForSingleObject(unloadDone, INFINITE);
        Sleep(100);
        hooks::remove();
        Sleep(200); // laisse sortir un éventuel appel en cours dans le hook
        CloseHandle(unloadDone);
        LOG("DLL libérée");
        FreeLibraryAndExitThread(g_module, 0);
    }
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        if (HANDLE t = CreateThread(nullptr, 0, mainThread, nullptr, 0, nullptr)) CloseHandle(t);
    }
    return TRUE;
}
