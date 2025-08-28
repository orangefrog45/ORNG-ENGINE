#pragma once

#include <yaml-cpp/node/node.h>

#include "Asset.h"

namespace ORNG {
    class SceneAsset : public Asset {
    public:
        explicit SceneAsset(const std::string& _filepath) : Asset(_filepath) {}
        explicit SceneAsset(const std::string& _filepath, uint64_t _uuid) : Asset(_filepath, _uuid) {}

        // Scene contents
        YAML::Node node;
    };
}

