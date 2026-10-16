// Injecteur minimal : LoadLibraryW via CreateRemoteThread dans la fenêtre Minecraft 1.8.9 (classe LWJGL).
// Usage : OrphenClient.exe [chemin\vers\orphen.dll]   (par défaut : orphen.dll à côté de l'exe)
#include <Windows.h>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <string>

namespace {
    struct Found {
        DWORD pid = 0;
        bool lunar = false;
    };

    BOOL CALLBACK enumProc(HWND hwnd, LPARAM lp) {
        auto* f = reinterpret_cast<Found*>(lp);
        wchar_t cls[64]{}, title[128]{};
        GetClassNameW(hwnd, cls, 64);
        GetWindowTextW(hwnd, title, 128);
        if (wcsstr(title, L"Lunar Client")) f->lunar = true;
        if (wcscmp(cls, L"LWJGL") == 0 && wcsstr(title, L"Minecraft 1.8.9")) {
            GetWindowThreadProcessId(hwnd, &f->pid);
            return FALSE;
        }
        return TRUE;
    }

    // Lancé par double-clic : garde la console ouverte pour lire le message.
    int finish(int code) {
        DWORD procs[2];
        if (GetConsoleProcessList(procs, 2) == 1) {
            wprintf(L"\nAppuie sur Entrée pour fermer...");
            getwchar();
        }
        return code;
    }

    int run(int argc, wchar_t** argv);
}

int wmain(int argc, wchar_t** argv) {
    if (_isatty(_fileno(stdout))) _setmode(_fileno(stdout), _O_U16TEXT); // accents corrects dans la console
    wprintf(L"Orphen Client — injecteur (Minecraft 1.8.9 vanilla)\n\n");
    return finish(run(argc, argv));
}

namespace {
int run(int argc, wchar_t** argv) {
    std::wstring dll;
    if (argc > 1) {
        dll = argv[1];
    } else {
        wchar_t self[MAX_PATH];
        GetModuleFileNameW(nullptr, self, MAX_PATH);
        dll = self;
        dll = dll.substr(0, dll.find_last_of(L"\\/") + 1) + L"orphen.dll";
    }
    wchar_t full[MAX_PATH];
    GetFullPathNameW(dll.c_str(), MAX_PATH, full, nullptr);
    if (GetFileAttributesW(full) == INVALID_FILE_ATTRIBUTES) {
        wprintf(L"DLL introuvable : %s\n", full);
        return 1;
    }

    Found f;
    EnumWindows(enumProc, reinterpret_cast<LPARAM>(&f));
    if (!f.pid) {
        if (f.lunar)
            wprintf(L"Lunar Client détecté : non supporté (ses classes ne sont pas celles de la 1.8.9 vanilla).\n"
                    L"Lance la 1.8.9 depuis le launcher Minecraft officiel.\n");
        else
            wprintf(L"Fenêtre \"Minecraft 1.8.9\" introuvable. Lance la 1.8.9 vanilla d'abord.\n");
        return 1;
    }

    HANDLE proc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION |
                              PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, f.pid);
    if (!proc) {
        wprintf(L"OpenProcess a échoué (%lu)\n", GetLastError());
        return 1;
    }

    const SIZE_T bytes = (wcslen(full) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote || !WriteProcessMemory(proc, remote, full, bytes, nullptr)) {
        wprintf(L"écriture mémoire impossible (%lu)\n", GetLastError());
        CloseHandle(proc);
        return 1;
    }

    auto loadLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(GetModuleHandleW(L"kernel32.dll"), "LoadLibraryW"));
    HANDLE th = CreateRemoteThread(proc, nullptr, 0, loadLib, remote, 0, nullptr);
    if (!th) {
        wprintf(L"CreateRemoteThread a échoué (%lu)\n", GetLastError());
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        CloseHandle(proc);
        return 1;
    }
    WaitForSingleObject(th, 10000);
    DWORD code = 0;
    GetExitCodeThread(th, &code);
    CloseHandle(th);
    VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
    CloseHandle(proc);

    if (!code) {
        wprintf(L"LoadLibrary a échoué dans le processus cible.\n");
        return 1;
    }
    wprintf(L"Injecté dans le PID %lu. Appuie sur Inser en jeu.\n", f.pid);
    return 0;
}
} // namespace
