#include "modules/registry.hpp"
#include "modules/module.hpp"
namespace modules {
    namespace {

    }

    std::vector<Module*>& all() {
        static std::vector<Module*> list{  };
        return list;
    }
}
