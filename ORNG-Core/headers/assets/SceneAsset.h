#pragma once

#include <yaml-cpp/node/node.h>

#include "Asset.h"

namespace ORNG {
    class SceneAsset : public Asset {
    public:
        explicit SceneAsset(const std::string& filepath) : Asset(filepath) {};
        explicit SceneAsset(const std::string& filepath, uint64_t uuid) : Asset(filepath, uuid) {};

        // Scene contents
        YAML::Node node;
    };
}