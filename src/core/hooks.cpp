#include "core/hooks.hpp"
#include "core/tick_hook.hpp"
#include <MinHook.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_opengl2.h>
#include <GL/gl.h>
#include "core/log.hpp"
#include "core/state.hpp"
#include "gui/menu.hpp"
#include "modules/module.hpp"
#include "modules/registry.hpp"
#include "modules/autoclicker.hpp"
#include "modules/reach.hpp"
#include "sdk/game.hpp"
#include <string>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND, UINT, WPARAM, LPARAM);

namespace {
    using SwapFn = BOOL(WINAPI*)(HDC);
    using BindBufferFn = void(APIENTRY*)(GLenum, GLuint);
    using ActiveTextureFn = void(APIENTRY*)(GLenum);
    using UseProgramFn = void(APIENTRY*)(GLuint);

    constexpr GLenum GL_ARRAY_BUFFER_ = 0x8892, GL_ELEMENT_ARRAY_BUFFER_ = 0x8893;
    constexpr GLenum GL_ARRAY_BUFFER_BINDING_ = 0x8894, GL_ELEMENT_ARRAY_BUFFER_BINDING_ = 0x8895;
    constexpr GLenum GL_TEXTURE0_ = 0x84C0, GL_ACTIVE_TEXTURE_ = 0x84E0, GL_CURRENT_PROGRAM_ = 0x8B8D;

    JavaVM* g_vm = nullptr;
    HANDLE g_unloadDone = nullptr;
    SwapFn g_origSwap = nullptr;
    void* g_swapTarget = nullptr;
    WNDPROC g_origWndProc = nullptr;

    bool g_imgui = false;
    bool g_sdkTried = false, g_sdkReady = false;
    bool g_finished = false;

    BindBufferFn glBindBuffer_ = nullptr;
    ActiveTextureFn glActiveTexture_ = nullptr;
    UseProgramFn glUseProgram_ = nullptr;

    bool isInputMessage(UINT msg) {
        return (msg >= WM_MOUSEFIRST && msg <= WM_MOUSELAST) || (msg >= WM_KEYFIRST && msg <= WM_KEYLAST) ||
               msg == WM_INPUT || msg == WM_MOUSEHOVER || msg == WM_MOUSELEAVE;
    }

    LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
        if ((msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) && !(lp & (1 << 30))) {
            const int vk = static_cast<int>(wp);
            if (state::menuOpen && state::bindingTarget) {
                state::bindingTarget->key = vk == VK_ESCAPE ? 0 : vk;
                state::bindingTarget = nullptr;
                return 0;
            }
            if (vk == state::MENU_KEY) {
                state::toggleMenuRequested = true;
                return 0;
            }
            if (state::menuOpen && vk == VK_END) {
                state::unloadRequested = true;
                return 0;
            }
            if (!state::menuOpen) state::pendingKeys.push_back(vk);
        }

        if (state::menuOpen) {
            ImGui_ImplWin32_WndProcHandler(hwnd, msg, wp, lp);
            if (isInputMessage(msg)) return 0; // le jeu ne voit rien tant que le menu est ouvert
        }
        return CallWindowProcW(g_origWndProc, hwnd, msg, wp, lp);
    }

    void attachWindow(HWND hwnd) {
        if (state::hwnd == hwnd) return;
        if (state::hwnd && g_imgui) ImGui_ImplWin32_Shutdown(); // fenêtre recréée (F11)
        state::hwnd = hwnd;
        g_origWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(wndProc)));
        if (g_imgui) ImGui_ImplWin32_Init(hwnd);
        LOG("fenêtre attachée : %p", hwnd);
    }

    void initImGui(HDC hdc) {
        glBindBuffer_ = reinterpret_cast<BindBufferFn>(wglGetProcAddress("glBindBuffer"));
        glActiveTexture_ = reinterpret_cast<ActiveTextureFn>(wglGetProcAddress("glActiveTexture"));
        glUseProgram_ = reinterpret_cast<UseProgramFn>(wglGetProcAddress("glUseProgram"));

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.IniFilename = nullptr;
        io.LogFilename = nullptr;
        menu::applyStyle();

        attachWindow(WindowFromDC(hdc));
        ImGui_ImplWin32_Init(state::hwnd);
        ImGui_ImplOpenGL2_Init();
        g_imgui = true;
        LOG("ImGui initialisé");
    }

    // Rend ImGui sans perturber l'état GL que GlStateManager de Minecraft garde en cache.
    void renderImGui() {
        GLint arrayBuf = 0, elemBuf = 0, program = 0, activeTex = GL_TEXTURE0_;
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING_, &arrayBuf);
        glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING_, &elemBuf);
        glGetIntegerv(GL_CURRENT_PROGRAM_, &program);
        glGetIntegerv(GL_ACTIVE_TEXTURE_, &activeTex);
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glPushClientAttrib(GL_CLIENT_ALL_ATTRIB_BITS);

        if (glBindBuffer_) { glBindBuffer_(GL_ARRAY_BUFFER_, 0); glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_, 0); }
        if (glUseProgram_) glUseProgram_(0);
        if (glActiveTexture_) glActiveTexture_(GL_TEXTURE0_);

        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

        glPopClientAttrib();
        glPopAttrib();
        if (glActiveTexture_) glActiveTexture_(static_cast<GLenum>(activeTex));
        if (glUseProgram_) glUseProgram_(static_cast<GLuint>(program));
        if (glBindBuffer_) {
            glBindBuffer_(GL_ARRAY_BUFFER_, static_cast<GLuint>(arrayBuf));
            glBindBuffer_(GL_ELEMENT_ARRAY_BUFFER_, static_cast<GLuint>(elemBuf));
        }
    }

    Ctx buildCtx(JNIEnv* env) {
        using namespace sdk;
        Ctx c;
        c.env = env;
        c.mc = env->CallStaticObjectMethod(C.Minecraft, M.mc_getMinecraft);
        if (clearEx(env) || !c.mc) return c;
        c.player = env->GetObjectField(c.mc, F.mc_thePlayer);
        c.world = env->GetObjectField(c.mc, F.mc_theWorld);
        c.controller = env->GetObjectField(c.mc, F.mc_playerController);
        c.screen = env->GetObjectField(c.mc, F.mc_currentScreen);
        c.singleplayer = env->CallBooleanMethod(c.mc, M.mc_isSingleplayer);
        if (clearEx(env)) c.singleplayer = false;
        return c;
    }

    // Native bridge called inside Minecraft.runTick(), on the installing game thread.
    // Build fresh JNI local references: never reuse the render frame's context.
    void gameTick(JNIEnv* env) {
        AutoClicker& clicker = modules::autoClicker();
        Reach& reach = modules::reach();
        auto resetCombat = [&] { clicker.reset(); reach.reset(); };
        if (!g_sdkReady || g_finished || state::unloadRequested || state::menuOpen ||
            state::toggleMenuRequested || !state::gameFocused() || (!clicker.enabled() && !reach.enabled())) {
            resetCombat();
            return;
        }
        if (env->PushLocalFrame(256) != JNI_OK) { sdk::clearEx(env); resetCombat(); return; }
        Ctx c = buildCtx(env);
        bool active = !sdk::clearEx(env) && c.inWorld() && !c.screen;
        if (active) {
            const bool paused = env->CallBooleanMethod(c.mc, sdk::M.mc_isGamePaused);
            active = !sdk::clearEx(env) && !paused;
        }
        if (active) {
            if (clicker.enabled()) clicker.onTick(c);
            else clicker.reset();
            sdk::clearEx(env);
            if (reach.enabled()) reach.onTick(c);
            else reach.reset();
        } else resetCombat();
        sdk::clearEx(env);
        env->PopLocalFrame(nullptr);
    }

    void installTickHook(JNIEnv* env) {
        HMODULE module = nullptr;
        wchar_t path[32768]{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&installTickHook), &module)) return;
        DWORD size = GetModuleFileNameW(module, path, static_cast<DWORD>(std::size(path)));
        if (!size || size >= std::size(path)) return;
        std::wstring jar(path, size);
        jar = jar.substr(0, jar.find_last_of(L"\\/") + 1) + L"orphen-bridge.jar";
        if (!tick_hook::install(g_vm, env, sdk::C.Minecraft, jar.c_str(), gameTick))
            LOG("AutoClicker/Reach indisponibles : callback de tick absent (aucun repli sur le rendu)");
    }

    // Ouvre/ferme le menu : libère la souris (setIngameNotInFocus) puis la recapture.
    void handleMenuToggle(Ctx& c) {
        if (!state::toggleMenuRequested) return;
        state::toggleMenuRequested = false;
        state::menuOpen = !state::menuOpen;
        state::bindingTarget = nullptr;
        if (!c.mc || c.screen) return; // un GUI vanilla gère déjà la souris

        JNIEnv* env = c.env;
        if (state::menuOpen) {
            env->CallVoidMethod(c.mc, sdk::M.mc_setIngameNotInFocus);
        } else if (c.player) {
            env->CallVoidMethod(c.mc, sdk::M.mc_setIngameFocus);
            env->SetIntField(c.mc, sdk::F.mc_leftClickCounter, 0); // setIngameFocus le met à 10000
        }
        sdk::clearEx(env);
    }

    void runModules(Ctx& c, bool active) {
        // Keybinds : seulement sans écran ouvert (pas en tapant dans le chat).
        if (!c.screen) {
            for (int vk : state::pendingKeys)
                for (Module* m : modules::all())
                    if (m->key == vk) m->toggle();
        }
        state::pendingKeys.clear();

        if (!active) return;
        for (Module* m : modules::all()) {
            if (!m->enabled()) continue;
            m->onUpdate(c);
            sdk::clearEx(c.env);
        }
    }

    void shutdownAll(Ctx& c) {
        if (state::menuOpen && c.mc && !c.screen && c.player) {
            c.env->CallVoidMethod(c.mc, sdk::M.mc_setIngameFocus);
            sdk::clearEx(c.env);
        }
        state::menuOpen = false;
        if (state::hwnd && g_origWndProc)
            SetWindowLongPtrW(state::hwnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(g_origWndProc));
        if (g_imgui) {
            ImGui_ImplOpenGL2_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            g_imgui = false;
        }
        LOG("déchargement");
    }

    void frame(HDC hdc) {
        JNIEnv* env = nullptr;
        if (g_vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK || !env) return;

        if (!g_imgui) initImGui(hdc);
        attachWindow(WindowFromDC(hdc));

        if (!g_sdkTried) {
            g_sdkTried = true;
            g_sdkReady = sdk::init(env);
            if (g_sdkReady) installTickHook(env);
        }

        if (env->PushLocalFrame(256) != JNI_OK) { sdk::clearEx(env); return; }

        Ctx c;
        c.env = env;
        if (g_sdkReady) c = buildCtx(env);

        if (state::unloadRequested) {
            if (!tick_hook::remove(env)) {
                // Never unload a DLL which Java might still call.
                LOG("Dechargement annule : restauration du tick impossible ; DLL conservee");
                state::unloadRequested = false;
                env->PopLocalFrame(nullptr);
                return;
            }
            shutdownAll(c);
            if (g_sdkReady) sdk::shutdown(env);
            env->PopLocalFrame(nullptr);
            g_finished = true;
            SetEvent(g_unloadDone);
            return;
        }

        if (g_sdkReady) handleMenuToggle(c);
        else if (state::toggleMenuRequested) { state::toggleMenuRequested = false; state::menuOpen = !state::menuOpen; }

        const bool active = g_sdkReady && c.inWorld();
        if (g_sdkReady) runModules(c, active);

        ImGuiIO& io = ImGui::GetIO();
        if (state::menuOpen) io.ConfigFlags &= ~(ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange);
        else io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange;
        io.MouseDrawCursor = false;

        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        if (active) {
            ImDrawList* dl = ImGui::GetBackgroundDrawList();
            for (Module* m : modules::all()) {
                if (!m->enabled()) continue;
                m->onRender(c, dl);
                sdk::clearEx(env);
            }
        }
        if (state::menuOpen) menu::draw(c, g_sdkReady);

        ImGui::Render();
        renderImGui();

        env->PopLocalFrame(nullptr);
    }

    BOOL WINAPI hkSwapBuffers(HDC hdc) {
        if (!g_finished) frame(hdc);
        return g_origSwap(hdc);
    }
}

namespace hooks {

bool install(JavaVM* vm, HANDLE unloadDoneEvent) {
    g_vm = vm;
    g_unloadDone = unloadDoneEvent;
    if (MH_Initialize() != MH_OK) return false;
    if (MH_CreateHookApiEx(L"opengl32.dll", "wglSwapBuffers", reinterpret_cast<void*>(&hkSwapBuffers),
                           reinterpret_cast<void**>(&g_origSwap), &g_swapTarget) != MH_OK) {
        LOG("MH_CreateHookApiEx(wglSwapBuffers) a échoué");
        return false;
    }
    if (MH_EnableHook(g_swapTarget) != MH_OK) return false;
    LOG("hook wglSwapBuffers installé");
    return true;
}

void remove() {
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

} // namespace hooks
