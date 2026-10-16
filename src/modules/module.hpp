#pragma once
#include <imgui.h>
#include "sdk/game.hpp"

// Contrat unique de tous les modules :
//  - onUpdate : appelé à chaque frame sur le thread du jeu, en partie (solo ou multijoueur).
//  - onRender : dessin 2D (ImGui background draw list), mêmes conditions.
//  - onTick : à l'entrée du tick Minecraft, jamais depuis le rendu.
//  - drawSettings : réglages affichés dans le menu.
class Module {
public:
    Module(const char* name, const char* description) : name_(name), description_(description) {}
    virtual ~Module() = default;

    const char* name() const { return name_; }
    const char* description() const { return description_; }

    bool enabled() const { return enabled_; }
    void setEnabled(bool v) { enabled_ = v; }
    void toggle() { enabled_ = !enabled_; }

    int key = 0; // touche virtuelle Windows (0 = aucune)

    virtual void onUpdate(Ctx&) {}
    virtual void onTick(Ctx&) {}
    virtual void onRender(Ctx&, ImDrawList*) {}
    virtual void drawSettings() {}

private:
    const char* name_;
    const char* description_;
    bool enabled_ = false;
};
