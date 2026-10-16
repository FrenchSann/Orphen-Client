#pragma once
#include <chrono>
#include <vector>
#include "modules/module.hpp"

class StorageESP : public Module {
public:
    StorageESP() : Module("StorageESP", "Boîtes et tracers : coffres, coffres piégés, droppers, dispensers, hoppers.") {}

    void onUpdate(Ctx& c) override;
    void onRender(Ctx& c, ImDrawList* dl) override;
    void drawSettings() override;

private:
    enum Kind { Chest, TrappedChest, Dropper, Dispenser, Hopper, KindCount };

    struct Entry {
        int x, y, z;
        Kind kind;
    };

    struct KindCfg {
        const char* label;
        bool show;
        float color[4];
    };

    using clock = std::chrono::steady_clock;
    std::vector<Entry> cache_;
    clock::time_point lastScan_{};

    bool boxes_ = true;
    bool tracers_ = true;
    bool labels_ = false;
    float maxDist_ = 96.0f;
    KindCfg cfg_[KindCount] = {
        { "Coffre",        true, { 1.00f, 0.75f, 0.10f, 1.0f } },
        { "Coffre piégé",  true, { 1.00f, 0.20f, 0.20f, 1.0f } },
        { "Dropper",       true, { 0.40f, 0.85f, 1.00f, 1.0f } },
        { "Dispenser",     true, { 0.30f, 1.00f, 0.45f, 1.0f } },
        { "Hopper",        true, { 0.75f, 0.45f, 1.00f, 1.0f } },
    };
};
