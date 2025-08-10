//
// Created by MWAC-dev on 8/9/2025.
//
#pragma once

#include "catalogs.hpp"

#include <nlohmann/json.hpp>

namespace ims {

    inline void to_json(nlohmann::json& j, const Material& m) {
        j = nlohmann::json{
            {"id", m.id},
            {"name", m.name},
            {"tags", m.tags},
            {"properties", m.properties}
        };
    }

    inline void from_json(const nlohmann::json& j, Material& m) {
        j.at("id").get_to(m.id);
        j.at("name").get_to(m.name);
        if (j.contains("tags")) {
            j.at("tags").get_to(m.tags);
        }
        if (j.contains("properties")) {
            j.at("properties").get_to(m.properties);
        }
    }
}
