//
// Created by MWAC-dev on 8/9/2025.
//

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace ims {

    struct Material {
        std::string id;
        std::string name;
        std::vector<std::string> tags;
        nlohmann::json properties;
    };

    struct Catalogs {
        std::vector<Material> materials;
    };

    struct Bundle {
        std::string config_version{"1.0.0"};
        Catalogs catalogs;
    };
}
