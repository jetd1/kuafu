//
// By Jet <i@jetd.me> 2021.
//
#pragma once

#include<string>
#include<vulkan/vulkan.hpp>

namespace kuafu::utils {

bool iequals(const std::string &a, const std::string &b) {
    return std::equal(a.begin(), a.end(),
                      b.begin(), b.end(),
                      [](char a, char b) {
                          return tolower(a) == tolower(b);
                      });
}
}

