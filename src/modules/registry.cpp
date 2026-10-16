#include "modules/registry.hpp"
#include "modules/autoclicker.hpp"
#include "modules/reach.hpp"

namespace modules {
    namespace {
        AutoClicker g_autoClicker;
        Reach g_reach;
    }

    std::vector<Module*>& all() {
        static std::vector<Module*> list{ &g_autoClicker, &g_reach };
        return list;
    }

    Reach& reach() { return g_reach; }
    AutoClicker& autoClicker() { return g_autoClicker; }
}
