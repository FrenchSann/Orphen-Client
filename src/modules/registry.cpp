#include "modules/registry.hpp"
#include "modules/autoclicker.hpp"
#include "modules/reach.hpp"
#include "modules/autoarmor.hpp"

namespace modules {
    namespace {
        AutoClicker g_autoClicker;
        Reach g_reach;
        AutoArmor g_autoArmor;
    }

    std::vector<Module*>& all() {
        static std::vector<Module*> list{ &g_autoClicker, &g_reach, &g_autoArmor };
        return list;
    }

    Reach& reach() { return g_reach; }
    AutoClicker& autoClicker() { return g_autoClicker; }
}
