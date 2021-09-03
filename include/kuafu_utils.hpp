//
// By Jet <i@jetd.me> 2021.
//
#pragma once

#include<string>
#include<vulkan/vulkan.hpp>

namespace kuafu::utils {

inline bool iequals(const std::string &a, const std::string &b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}

inline glm::vec3 getPerpendicular(glm::vec3 v) {
    glm::vec3 a = glm::abs(v);
    glm::uint xm = ((a.x - a.y) < 0 && (a.x - a.z) <0) ? 1 : 0;
    glm::uint ym = (a.y - a.z) < 0 ? (1u ^ xm) : 0;
    glm::uint zm = 1u ^ (xm | ym);
    return glm::cross(v, glm::vec3{xm, ym, zm});
}

}

