#include "gui/menu.hpp"
#include <cstdio>
#include <imgui.h>
#include <Windows.h>
#include "core/state.hpp"
#include "core/tick_hook.hpp"
#include "modules/module.hpp"
#include "modules/registry.hpp"

namespace {
    const char* keyName(int vk) {
        static char buf[32];
        if (vk == 0) return "aucune";
        UINT sc = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        LONG lp = static_cast<LONG>(sc) << 16;
        switch (vk) { // touches étendues : sinon GetKeyNameText renvoie le pavé numérique
            case VK_INSERT: case VK_DELETE: case VK_HOME: case VK_END: case VK_PRIOR: case VK_NEXT:
            case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_RCONTROL: case VK_RMENU:
                lp |= 1 << 24;
        }
        if (GetKeyNameTextA(lp, buf, sizeof(buf)) > 0) return buf;
        snprintf(buf, sizeof(buf), "0x%02X", vk);
        return buf;
    }
}

namespace menu {

void applyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImGui::StyleColorsDark();
    s.WindowRounding = 6.0f;
    s.FrameRounding = 4.0f;
    s.GrabRounding = 4.0f;
    s.WindowBorderSize = 0.0f;
    s.ItemSpacing = { 8.0f, 6.0f };
    ImVec4* col = s.Colors;
    col[ImGuiCol_WindowBg]       = { 0.08f, 0.08f, 0.10f, 0.96f };
    col[ImGuiCol_Header]         = { 0.20f, 0.22f, 0.30f, 1.00f };
    col[ImGuiCol_HeaderHovered]  = { 0.26f, 0.29f, 0.40f, 1.00f };
    col[ImGuiCol_CheckMark]      = { 0.45f, 0.70f, 1.00f, 1.00f };
    col[ImGuiCol_SliderGrab]     = { 0.45f, 0.70f, 1.00f, 1.00f };
    col[ImGuiCol_FrameBg]        = { 0.15f, 0.16f, 0.20f, 1.00f };
    col[ImGuiCol_TitleBgActive]  = { 0.12f, 0.13f, 0.18f, 1.00f };
}

void draw(const Ctx& c, bool sdkReady) {
    ImGui::SetNextWindowSize({ 380.0f, 0.0f }, ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos({ 40.0f, 40.0f }, ImGuiCond_FirstUseEver);
    ImGui::Begin("Orphen Client", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

    if (!sdkReady) {
        ImGui::TextColored({ 1.0f, 0.4f, 0.4f, 1.0f }, "Mappings invalides : voir %%TEMP%%\\orphen.log");
    } else if (!c.inWorld()) {
        ImGui::TextDisabled("Pas en partie.");
    } else if (!c.singleplayer) {
        ImGui::TextColored({ 0.5f, 1.0f, 0.5f, 1.0f }, "Multijoueur : modules disponibles.");
    } else {
        ImGui::TextColored({ 0.5f, 1.0f, 0.5f, 1.0f }, "Solo : modules actifs.");
    }
    ImGui::Separator();

    if (sdkReady && !tick_hook::ready()) {
        ImGui::TextColored({ 1.0f, 0.7f, 0.3f, 1.0f }, "AutoClicker indisponible : hook de tick absent.");
        ImGui::TextDisabled("Verifier orphen-bridge.jar et %%TEMP%%\\orphen.log");
    }

    for (Module* m : modules::all()) {
        ImGui::PushID(m);
        bool on = m->enabled();
        if (ImGui::Checkbox("##on", &on)) m->setEnabled(on);
        ImGui::SameLine();
        const bool open = ImGui::CollapsingHeader(m->name());
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", m->description());

        if (open) {
            ImGui::Indent();
            const bool waiting = state::bindingTarget == m;
            char label[64];
            snprintf(label, sizeof(label), waiting ? "Appuie sur une touche... (Échap = aucune)" : "Touche : %s", keyName(m->key));
            if (ImGui::Button(label)) state::bindingTarget = waiting ? nullptr : m;
            m->drawSettings();
            ImGui::Unindent();
            ImGui::Spacing();
        }
        ImGui::PopID();
    }

    ImGui::Separator();
    ImGui::TextDisabled("Inser : menu   |   Fin (menu ouvert) : décharger");
    if (ImGui::Button("Décharger")) state::unloadRequested = true;

    ImGui::End();
}

} // namespace menu
