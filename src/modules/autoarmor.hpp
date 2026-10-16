#pragma once
#include <chrono>
#include "modules/module.hpp"

class AutoArmor : public Module {
public:
    AutoArmor() : Module("AutoArmor", "Équipe automatiquement la meilleure armure de l'inventaire.") {}

    void onUpdate(Ctx& c) override;
    void drawSettings() override;

private:
    float score(JNIEnv* env, jobject stack, int wantedType); // -1 si pas une armure du bon type

    using clock = std::chrono::steady_clock;
    int delayMs_ = 150;
    clock::time_point last_{};
};
