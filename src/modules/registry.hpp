#pragma once
#include <vector>

class Module;
class Reach;
class AutoClicker;

namespace modules {
    std::vector<Module*>& all();
    Reach& reach();
    AutoClicker& autoClicker();
}
