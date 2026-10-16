#pragma once
#include <chrono>
#include "modules/module.hpp"

class AutoClicker : public Module {
public:
    AutoClicker() : Module("AutoClicker", "Clique en boucle tant que le clic gauche est maintenu. Laisse casser les blocs.") {}

    void onUpdate(Ctx& c) override;
    void drawSettings() override;

private:
    void click(Ctx& c);

    using clock = std::chrono::steady_clock;
    float cps_ = 12.0f;
    bool breakBlocks_ = true;      // ne clique pas quand on vise un bloc -> minage vanilla
    bool pauseWhileUsing_ = true;  // pas de clic pendant manger / bloquer à l'épée / arc
    clock::time_point next_{};
};
