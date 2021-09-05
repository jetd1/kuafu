//
// By Jet <i@jetd.me> 2021.
//
#pragma once

#include<string>
#include<filesystem>
#include<vulkan/vulkan.hpp>

namespace kuafu::utils {

inline bool iequals(std::string_view a, std::string_view b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) { return tolower(a) == tolower(b); });
}

inline glm::vec3 getPerpendicular(glm::vec3 v) {
    glm::vec3 a = glm::abs(v);
    glm::uint xm = ((a.x - a.y) < 0 && (a.x - a.z) <0) ? 1 : 0;
    glm::uint ym = (a.y - a.z) < 0 ? (1u ^ xm) : 0;
    glm::uint zm = 1u ^ (xm | ym);
    return glm::cross(v, glm::vec3{xm, ym, zm});
}

// @pram ext: file extension, should contain dot '.'
inline bool hasExtension(std::string_view filename, std::string_view ext) {
    auto fileExt = std::filesystem::path(filename).extension().string();
    return kuafu::utils::iequals(fileExt, ext);
}

// @pram exts: list of file extensions, should contain dot '.'
inline bool hasExtension(std::string_view filename, const std::vector<std::string_view>& exts) {
    auto fileExt = std::filesystem::path(filename).extension().string();

//    for (auto ext: exts)
//        if (iequals(fileExt, ext))
//            return true;
//    return false;
    return std::any_of(exts.begin(), exts.end(),
                   [fileExt](auto a) { return iequals(fileExt, a); });
}

}

