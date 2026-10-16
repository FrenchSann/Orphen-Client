#pragma once
#include <vector>
#include <Windows.h>

class Module;

// État partagé entre la WndProc et le hook de SwapBuffers.
// Les deux tournent sur le thread principal du jeu (LWJGL2 pompe les messages dans Display.update),
// donc pas besoin de synchronisation.
namespace state {
    inline HWND hwnd = nullptr;
    inline bool menuOpen = false;
    inline bool toggleMenuRequested = false;
    inline bool unloadRequested = false;
    inline Module* bindingTarget = nullptr;   // module en attente d'une touche
    inline std::vector<int> pendingKeys;      // touches pressées menu fermé (keybinds)

    constexpr int MENU_KEY = VK_INSERT;

    inline bool gameFocused() { return hwnd && GetForegroundWindow() == hwnd; }

    // Clic gauche physique maintenu, fenêtre active, menu fermé.
    inline bool leftHeld() {
        return gameFocused() && !menuOpen && (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
    }
}
