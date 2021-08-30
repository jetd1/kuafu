//
// Created by jet on 8/30/21.
//

#pragma once
#include<string>
#include<vulkan/vulkan.hpp>

namespace kuafu::utils {

bool iequals(const std::string& a, const std::string& b)
{
  return std::equal(a.begin(), a.end(),
                    b.begin(), b.end(),
                    [](char a, char b) {
                      return tolower(a) == tolower(b);
                    });
}
}

