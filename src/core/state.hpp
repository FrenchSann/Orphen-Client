#pragma once
#include <vector>
#include <Windows.h>

class Module;

// État partagé entre WndProc, SwapBuffers et le callback runTick.
// Ils tournent sur le thread principal du jeu (le callback filtre le thread installateur),
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
